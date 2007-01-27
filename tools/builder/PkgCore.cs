using System;

namespace builder
{
	public class PkgCore : Package
	{
		public PkgCore()
		{
		}

		public override string GetBaseFolder()
		{
			return "base/addons/sourcemod";
		}

		/** 
		 * Must return the list of folders to create.
		 */
		public override string [] GetFolders()
		{
			string [] folders = new string[7];

			folders[0] = "bin";
			folders[1] = "plugins/disabled";
			folders[2] = "configs";
			folders[3] = "translations";
			folders[4] = "logs";
			folders[5] = "extensions";
			folders[6] = "scripting/include";

			return folders;
		}

		/**
		 * Called when file to file copies must be performed
		 */
		public override void OnCopyFiles()
		{
		}

		/**
		 * Called when dir to dir copies must be performed
		 */
		public override void OnCopyFolders(ABuilder builder)
		{
			builder.CopyFolder(this, "configs", "configs", null);
			
			string [] plugin_omits = new string[1];
			plugin_omits[0] = "spcomp.exe";

			builder.CopyFolder(this, "plugins", "scripting", plugin_omits);
			builder.CopyFolder(this, "plugins/include", "scripting/include", null);
			builder.CopyFolder(this, "translations", "translations", null);
		}

		/**
		 * Called to build libraries
		 */
		public override Library [] GetLibraries()
		{
			Library [] libs = new Library[5];

			for (int i=0; i<libs.Length; i++)
			{
				libs[i] = new Library();
			}

			libs[0].Destination = "bin";
			libs[0].LocalPath = "core";
			libs[0].Name = "sourcemod_mm";
			libs[0].PlatformExt = true;

			libs[1].Destination = "bin";
			libs[1].LocalPath = "sourcepawn/jit/x86";
			libs[1].Name = "sourcepawn.jit.x86";
			libs[1].ProjectFile = "jit-x86";

			libs[2].Destination = "scripting";
			libs[2].LocalPath = "sourcepawn/compiler";
			libs[2].Name = "spcomp";
			libs[2].IsExecutable = true;

			libs[3].Destination = "extensions";
			libs[3].LocalPath = "extensions/geoip";
			libs[3].Name = "geoip.ext";
			libs[3].ProjectFile = "geoip";

			libs[4].Destination = "extensions";
			libs[4].LocalPath = "extensions/threader";
			libs[4].Name = "threader.ext";
			libs[4].ProjectFile = "threader";

			return libs;
		}
	}
}
