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

		public override bool BuildLibrary(Package pkg, Library lib, ref string _binName, ref string _binPath)
		{
			ProcessStartInfo info = new ProcessStartInfo();

			string path = Config.PathFormat("{0}/{1}/msvc8", 
				cfg.SourceBase,
				lib.LocalPath);

			/* PlatformExt ignored for us */
			string binName = lib.Name + (lib.IsExecutable ? ".exe" : ".dll");

			string binpath = Config.PathFormat("{0}/{1}/{2}",
				path,
				lib.ReleaseBuild,
				binName);
			
			if (File.Exists(binpath))
			{
				File.Delete(binpath);
			}

			string projectFile = null;
			if (lib.ProjectFile != null)
			{
				projectFile = lib.ProjectFile + ".vcproj";
			} 
			else 
			{
				projectFile = lib.Name + ".vcproj";
			}

			info.WorkingDirectory = path;
			info.FileName = cfg.BuilderPath;
			info.Arguments = "/rebuild " + lib.ReleaseBuild + " " + projectFile;
			info.UseShellExecute = false;

			Process p = Process.Start(info);
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
