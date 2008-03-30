using System;
using System.IO;
using System.Diagnostics;

namespace builder
{
	class Program
	{
		static void Main(string[] args)
		{
			if (args.GetLength(0) < 1)
			{
				System.Console.WriteLine("Usage: <config file>");
				return;
			}

			Config cfg = new Config();
			if (!cfg.ReadFromFile(args[0]))
			{
				return;
			}

			/* :TODO: Add path validation */

			ABuilder bld = null;
			
			if (cfg.Platform == BasePlatform.Platform_Linux)
			{
				bld = new LinuxBuilder(cfg);
			} 
			else if (cfg.Platform == BasePlatform.Platform_Windows)
			{
				bld = new Win32Builder(cfg);
				if (cfg.pdb_log_file != null && File.Exists(cfg.pdb_log_file))
				{
					File.Delete(cfg.pdb_log_file);
				}
			}

			try
			{
				bld.BuildPackage(new PkgCore());
			} 
			catch (System.Exception e) 
			{
				Console.WriteLine("Build failed: " + e.Message);
			}
		}
	}
}
