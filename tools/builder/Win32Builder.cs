using System;
using System.IO;
using System.Diagnostics;

namespace builder
{
	public class Win32Builder : ABuilder
	{
		public Win32Builder(Config _cfg)
		{
			cfg = _cfg;
		}

		public override string GetPawnCompilerName()
		{
			return "spcomp.exe";
		}

		public override bool BuildLibrary(Package pkg, Library lib, ref string _binName, ref string _binPath)
		{
			ProcessStartInfo info = new ProcessStartInfo();

			string path = Config.PathFormat("{0}/{1}/msvc8", 
				cfg.source_path,
				lib.source_path);

			/* PlatformExt ignored for us */
			string binName = lib.binary_name + (lib.is_executable ? ".exe" : ".dll");

			string config_name = "Unknown";

			if (lib.release_mode == ReleaseMode.ReleaseMode_Release)
			{
				config_name = "Release";
			}
			else if (lib.release_mode == ReleaseMode.ReleaseMode_Debug)
			{
				config_name = "Debug";
			}

			if (lib.build_mode == BuildMode.BuildMode_Episode1)
			{
				config_name = config_name + " - Episode 1";
			}
			else if (lib.build_mode == BuildMode.BuildMode_Episode2)
			{
				config_name = config_name + " - Orange Box";
			}
			else if (lib.build_mode == BuildMode.BuildMode_OldMetamod)
			{
				config_name = config_name + " - Old Metamod";
			}

			string binpath = Config.PathFormat("{0}/{1}/{2}",
				path,
				config_name,
				binName);
			
			if (File.Exists(binpath))
			{
				File.Delete(binpath);
			}

			string project_file = null;
			if (lib.vcproj_name != null)
			{
				project_file = lib.vcproj_name + ".vcproj";
			} 
			else 
			{
				project_file = lib.binary_name + ".vcproj";
			}

			info.WorkingDirectory = path;
			info.FileName = cfg.builder_path;
			info.UseShellExecute = false;
			info.RedirectStandardOutput = true;
			info.RedirectStandardError = true;

			if (cfg.build_options != null)
			{
				info.Arguments = cfg.build_options + " ";
			}

			info.Arguments += "/rebuild \"" + config_name + "\" " + project_file;

			Process p = Process.Start(info);
			Console.WriteLine(p.StandardOutput.ReadToEnd());
			p.WaitForExit();
			p.Close();

			if (!File.Exists(binpath))
			{
				return false;
			}

			_binName = binName;
			_binPath = binpath;

			return true;
		}
	}
}
