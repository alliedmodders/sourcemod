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

		public override string GetPawnCompilerName()
		{
			return "spcomp";
		}

		public override bool BuildLibrary(Package pkg, Library lib)
		{
			ProcessStartInfo info = new ProcessStartInfo();

			string path = Config.PathFormat("{0}/{1}", 
				cfg.source_path,
				lib.source_path);

			/* PlatformExt ignored for us */
			string binName = lib.binary_name;
			
			if (!lib.is_executable)
			{
				if (lib.has_platform_ext)
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
				(lib.release_mode == ReleaseMode.ReleaseMode_Release) ? "Release" : "Debug",
				binName);
			
			if (File.Exists(binpath))
			{
				File.Delete(binpath);
			}

			string makefile_name = "Makefile";

			if (lib.build_mode == BuildMode.BuildMode_Episode1)
			{
				makefile_name = "Makefile.ep1";
			}
			else if (lib.build_mode == BuildMode.BuildMode_Episode2)
			{
				makefile_name = "Makefile.ep2";
			}
			else if (lib.build_mode == BuildMode.BuildMode_OldMetamod)
			{
				makefile_name = "Makefile.orig";
			}

			/* Clean the project first */
			info.WorkingDirectory = path;
			info.FileName = cfg.builder_path;
			info.Arguments = "-f " + makefile_name + " clean";
			info.UseShellExecute = false;

			Process p = Process.Start(info);
			p.WaitForExit();
			p.Close();

			/* Now build it */
			info.WorkingDirectory = path;
			info.FileName = cfg.builder_path;
			info.Arguments = "-f " + makefile_name;
			info.UseShellExecute = false;

			if (cfg.build_options != null)
			{
				info.Arguments += " " + cfg.build_options;
			}

			p = Process.Start(info);
			p.WaitForExit();
			p.Close();

			if (!File.Exists(binpath))
			{
				return false;
			}
			
			path = Config.PathFormat("{0}/{1}/{2}/{3}",
				cfg.pkg_path,
				pkg.GetBaseFolder(),
				lib.package_path,
				binName);
			File.Copy(binpath, path, true);

			return true;
		}
	}
}
