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

		public override void GetCompressBases(ref string path, ref string folder)
		{
			path = "base";
			folder = "addons";
		}

		public override string GetPackageName()
		{
			return "sourcemod-core";
		}

		/** 
		 * Must return the list of folders to create.
		 */
		public override string [] GetFolders()
		{
			string [] folders = new string[11];

			folders[0] = "bin";
			folders[1] = "plugins/disabled";
			folders[2] = "gamedata";
			folders[3] = "configs/geoip";
			folders[4] = "translations";
			folders[5] = "logs";
			folders[6] = "extensions";
			folders[7] = "data";
			folders[8] = "scripting/include";
			folders[9] = "scripting/admin-flatfile";
			folders[10] = "scripting/testsuite";

			return folders;
		}

		/**
		 * Called when file to file copies must be performed
		 */
		public override void OnCopyFiles(ABuilder builder)
		{
		}

		/**
		 * Called when dir to dir copies must be performed
		 */
		public override void OnCopyFolders(ABuilder builder)
		{
			builder.CopyFolder(this, "configs", "configs", null);
			builder.CopyFolder(this, "configs/geoip", "configs/geoip", null);
			
			string [] plugin_omits = new string[1];
			plugin_omits[0] = "spcomp.exe";

			string [] include_omits = new string[1];
			include_omits[0] = "version.tpl";

			builder.CopyFolder(this, "gamedata", "gamedata", null);
			builder.CopyFolder(this, "plugins", "scripting", plugin_omits);
			builder.CopyFolder(this, "plugins/include", "scripting/include", include_omits);
			builder.CopyFolder(this, "translations", "translations", null);
			builder.CopyFolder(this, "public/licenses", null, null);
			builder.CopyFolder(this, "plugins/admin-flatfile", "scripting/admin-flatfile", null);
			builder.CopyFolder(this, "plugins/testsuite", "scripting/testsuite", null);
		}

		/**
		 * Called to build libraries
		 */
		public override Library [] GetLibraries()
		{
			Library [] libs = new Library[6];

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
			libs[4].LocalPath = "extensions/bintools";
			libs[4].Name = "bintools.ext";
			libs[4].ProjectFile = "bintools";

			libs[5].Destination = "extensions";
			libs[5].LocalPath = "extensions/mysql";
			libs[5].Name = "dbi.mysql.ext";
			libs[5].ProjectFile = "sm_mysql";

			return libs;
		}

		/**
		 * Called to build plugins
		 */
		public override Plugin [] GetPlugins()
		{
			Plugin [] plugins = new Plugin[3];

			plugins[0] = new Plugin("admin-flatfile", "admin-flatfile");
			plugins[1] = new Plugin("basecommands");
			plugins[2] = new Plugin("antiflood");

			return plugins;
		}
	}
}

