using System;
using System.IO;
using System.Diagnostics;

namespace builder
{
	public abstract class ABuilder
	{
		public Config cfg;

		public ABuilder()
		{
		}

		public abstract string CompressPackage(Package pkg);

		public abstract bool BuildLibrary(Package pkg, Library lib, ref string _binName, ref string _binPath);

		public abstract string GetPawnCompilerName();

		public bool CompilePlugin(Package pkg, Plugin pl)
		{
			string local_dir = Config.PathFormat("{0}/{1}/addons/sourcemod/scripting", cfg.OutputBase, pkg.GetBaseFolder());

			string filepath = null;
			if (pl.Folder != null)
			{
				filepath = Config.PathFormat("{0}/{1}", pl.Folder, pl.Source);
			} 
			else 
			{
				filepath = pl.Source;
			}

			ProcessStartInfo info = new ProcessStartInfo();
			info.WorkingDirectory = local_dir;
			info.FileName = Config.PathFormat("{0}/{1}", local_dir, GetPawnCompilerName());
			info.Arguments = filepath + ".sp";
			info.UseShellExecute = false;
			info.RedirectStandardOutput = true;
			info.RedirectStandardError = true;

			Process p = Process.Start(info);
			string output = p.StandardOutput.ReadToEnd() + "\n";
			output += p.StandardError.ReadToEnd();
			p.WaitForExit();
			p.Close();

			Console.WriteLine(output);

			string binary = Config.PathFormat("{0}/{1}/addons/sourcemod/scripting/{2}.smx", cfg.OutputBase, pkg.GetBaseFolder(), pl.Source);
			if (!File.Exists(binary))
			{
				return false;
			}
			string new_loc = Config.PathFormat("{0}/{1}/addons/sourcemod/plugins/{2}.smx", cfg.OutputBase, pkg.GetBaseFolder(), pl.Source);

			try
			{
				if (File.Exists(new_loc))
				{
					File.Delete(new_loc);
				}
				File.Move(binary, new_loc);
			}
			catch (System.Exception e)
			{
				Console.WriteLine(e.Message);
				return false;
			}

			return true;
		}

		public string GetRevsionOfPath(string path)
		{
			ProcessStartInfo info = new ProcessStartInfo();

			info.WorkingDirectory = path;
			info.FileName = cfg.SVNVersion;
			info.Arguments = "--committed \"" + path + "\"";
			info.UseShellExecute = false;
			info.RedirectStandardOutput = true;

			Process p = Process.Start(info);
			string output = p.StandardOutput.ReadToEnd();
			p.WaitForExit();
			p.Close();

			string [] revs = output.Split(":".ToCharArray(), 2);
			if (revs.Length < 1)
			{
				return null;
			}

			string rev = null;
			if (revs.Length == 1)
			{
				rev = revs[0];
			} 
			else 
			{
				rev = revs[1];
			}

			rev = rev.Trim();
			rev = rev.Replace("M", "");
			rev = rev.Replace("S", "");

			return rev;
		}

		public bool CopyFile(Package pkg, string source, string dest)
		{
			string from = Config.PathFormat("{0}/{1}", 
				cfg.SourceBase,
				source);
			string to = Config.PathFormat("{0}/{1}/{2}",
				cfg.OutputBase,
				pkg.GetBaseFolder(),
				dest);

			File.Copy(from, to, true);

			return true;
		}

		/** dest can be null to mean root base folder */
		public void CopyFolder(Package pkg, string source, string dest, string [] omits)
		{
			string from_base = Config.PathFormat("{0}/{1}", cfg.SourceBase, source);
			string to_base = null;
			
			if (dest == null)
			{
				to_base = Config.PathFormat("{0}/{1}", 
					cfg.OutputBase, 
					pkg.GetBaseFolder());
			} 
			else 
			{
				to_base = Config.PathFormat("{0}/{1}/{2}", 
					cfg.OutputBase, 
					pkg.GetBaseFolder(), 
					dest);
			}

			string [] files = Directory.GetFiles(from_base);
			string file;

			for (int i=0; i<files.Length; i++)
			{
				file = Path.GetFileName(files[i]);

				if (omits != null)
				{
					bool skip = false;
					for (int j=0; j<omits.Length; j++)
					{
						if (file.CompareTo(omits[j]) == 0)
						{
							skip = true;
							break;
						}
					}
					if (skip)
					{
						continue;
					}
				}
				dest = Config.PathFormat("{0}/{1}", to_base, file);
				File.Copy(files[i], dest, true);
			}
		}

		public string PackageBuildName(Package pkg)
		{
			return pkg.GetPackageName() 
				+ "-r"
				+ GetRevsionOfPath(cfg.SourceBase)
				+ "-"
				+ DateTime.Now.Year
				+ DateTime.Now.Month.ToString("00")
				+ DateTime.Now.Day.ToString("00");
		}

		public void BuildPackage(Package pkg)
		{
			string path = Config.PathFormat("{0}/{1}", cfg.OutputBase, pkg.GetBaseFolder());

			if (!Directory.Exists(path))
			{
				Directory.CreateDirectory(path);
			}

			/* Create all dirs */
			string [] paths = pkg.GetFolders();
			for (int i=0; i<paths.GetLength(0); i++)
			{
				path = Config.PathFormat("{0}/{1}/{2}", cfg.OutputBase, pkg.GetBaseFolder(), paths[i]);
				if (!Directory.Exists(path))
				{
					Directory.CreateDirectory(path);
				}
			}

			/* Do primitive copies */
			pkg.OnCopyFolders(this);
			pkg.OnCopyFiles(this);

			/* Do libraries */
			Library [] libs = pkg.GetLibraries();
			string bin = null, binpath = null;
			for (int i=0; i<libs.Length; i++)
			{
				if (BuildLibrary(pkg, libs[i], ref bin, ref binpath))
				{
					path = Config.PathFormat("{0}/{1}/{2}/{3}",
						cfg.OutputBase,
						pkg.GetBaseFolder(),
						libs[i].Destination,
						bin);
					File.Copy(binpath, path, true);
				}
				else 
				{
					throw new System.Exception("Failed to compile library: " + libs[i].Name);
				}
			}

			/* Do plugins */
			Plugin [] plugins = pkg.GetPlugins();
			if (plugins != null)
			{
				for (int i=0; i<plugins.Length; i++)
				{
					if (!CompilePlugin(pkg, plugins[i]))
					{
						throw new System.Exception("Failed to compile plugin: " + plugins[i].Source);
					}
				}
			}

			string pkg_file = null;
			if (cfg.Compressor != null)
			{
				if ((pkg_file=CompressPackage(pkg)) == null)
				{
					throw new System.Exception("Failed to compress package: " + pkg.GetPackageName());
				}
	
				string lpath = null, ltarget = null;
				pkg.GetCompressBases(ref lpath, ref ltarget);
				lpath = Config.PathFormat("{0}/{1}/{2}",
					cfg.OutputBase,
					lpath,
					pkg_file);
				ltarget = Config.PathFormat("{0}/{1}", cfg.OutputBase, pkg_file);
	
				if (File.Exists(ltarget))
				{
					File.Delete(ltarget);
				}

				File.Move(lpath, ltarget);
			}
		}
	}
}
