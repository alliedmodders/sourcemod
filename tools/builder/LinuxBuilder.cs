using System;
using System.IO;
using System.Diagnostics;

namespace builder
{
	public class LinuxBuilder : ABuilder
	{
		public LinuxBuilder(Config _cfg)
		{
			cfg = _cfg;
		}

		public override bool BuildLibrary(Package pkg, Library lib, ref string _binName, ref string _binPath)
		{
			ProcessStartInfo info = new ProcessStartInfo();

			string path = Config.PathFormat("{0}/{1}", 
				cfg.SourceBase,
				lib.LocalPath);

			/* PlatformExt ignored for us */
			string binName = lib.Name;
			
			if (!lib.IsExecutable)
			{
				if (lib.PlatformExt)
				{
					binName += "_i486.so";
				} 
				else 
				{
					binName += ".so";
				}
			}

			string binpath = Config.PathFormat("{0}/{1}/{2}",
				path,
				lib.ReleaseBuild,
				binName);
			
			if (File.Exists(binpath))
			{
				File.Delete(binpath);
			}

			/* Clean the project first */
			info.WorkingDirectory = path;
			info.FileName = cfg.BuilderPath;
			info.Arguments = "clean";
			info.UseShellExecute = false;

			Process p = Process.Start(info);
			p.WaitForExit();
			p.Close();

			/* Now build it */
			info.WorkingDirectory = path;
			info.FileName = cfg.BuilderPath;
			info.Arguments = "Release";
			info.UseShellExecute = false;
			p = Process.Start(info);
			p.WaitForExit();
			p.Close();

			if (!File.Exists(binpath))
			{
				return false;
			}
			
			/* Now verify the binary */
			info.WorkingDirectory = "/bin";			/* :TODO: Fix this */
			info.FileName = "dlsym";
			info.Arguments = binpath;
			info.UseShellExecute = false;
			info.RedirectStandardOutput = true;
			p = Process.Start(info);
			string output = p.StandardOutput.ReadToEnd();
			p.WaitForExit();
			p.Close();

			if (output.IndexOf("Handle:") == -1)
			{
				return false;
			}

			_binName = binName;
			_binPath = binpath;

			return true;
		}
	}
}
