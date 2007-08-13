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
			return "base";
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
			string [] folders = new string[12];

			folders[0] = "addons/sourcemod/bin";
			folders[1] = "addons/sourcemod/plugins/disabled";
			folders[2] = "addons/sourcemod/gamedata";
			folders[3] = "addons/sourcemod/configs/geoip";
			folders[4] = "addons/sourcemod/translations";
			folders[5] = "addons/sourcemod/logs";
			folders[6] = "addons/sourcemod/extensions";
			folders[7] = "addons/sourcemod/data";
			folders[8] = "addons/sourcemod/scripting/include";
			folders[9] = "addons/sourcemod/scripting/admin-flatfile";
			folders[10] = "addons/sourcemod/scripting/testsuite";
			folders[11] = "cfg/sourcemod";

			return folders;
		}

		/**
		 * Called when file to file copies must be performed
		 */
		public override void OnCopyFiles(ABuilder builder)
		{
			builder.CopyFile(this, "sourcepawn/batchtool/compile.exe", "addons/sourcemod/scripting/compile.exe");
		}

		/**
		 * Called when dir to dir copies must be performed
		 */
		public override void OnCopyFolders(ABuilder builder)
		{
			builder.CopyFolder(this, "configs", "addons/sourcemod/configs", null);
			builder.CopyFolder(this, "configs/geoip", "addons/sourcemod/configs/geoip", null);
			builder.CopyFolder(this, "configs/cfg", "cfg/sourcemod", null);
			
			string [] plugin_omits = new string[1];
			plugin_omits[0] = "spcomp.exe";

			string [] include_omits = new string[1];
			include_omits[0] = "version.tpl";

			builder.CopyFolder(this, "gamedata", "addons/sourcemod/gamedata", null);
			builder.CopyFolder(this, "plugins", "addons/sourcemod/scripting", plugin_omits);
			builder.CopyFolder(this, "plugins/include", "addons/sourcemod/scripting/include", include_omits);
			builder.CopyFolder(this, "translations", "addons/sourcemod/translations", null);
			builder.CopyFolder(this, "public/licenses", "addons/sourcemod", null);
			builder.CopyFolder(this, "plugins/admin-flatfile", "addons/sourcemod/scripting/admin-flatfile", null);
			builder.CopyFolder(this, "plugins/testsuite", "addons/sourcemod/scripting/testsuite", null);
		}

		/**
		 * Called to build libraries
		 */
		public override Library [] GetLibraries()
		{
			Library [] libs = new Library[8];

			for (int i=0; i<libs.Length; i++)
			{
				libs[i] = new Library();
			}

			libs[0].Destination = "addons/sourcemod/bin";
			libs[0].LocalPath = "core";
			libs[0].Name = "sourcemod_mm";
			libs[0].PlatformExt = true;

			libs[1].Destination = "addons/sourcemod/bin";
			libs[1].LocalPath = "sourcepawn/jit/x86";
			libs[1].Name = "sourcepawn.jit.x86";
			libs[1].ProjectFile = "jit-x86";

			libs[2].Destination = "addons/sourcemod/scripting";
			libs[2].LocalPath = "sourcepawn/compiler";
			libs[2].Name = "spcomp";
			libs[2].IsExecutable = true;

			libs[3].Destination = "addons/sourcemod/extensions";
			libs[3].LocalPath = "extensions/geoip";
			libs[3].Name = "geoip.ext";
			libs[3].ProjectFile = "geoip";

			libs[4].Destination = "addons/sourcemod/extensions";
			libs[4].LocalPath = "extensions/bintools";
			libs[4].Name = "bintools.ext";
			libs[4].ProjectFile = "bintools";

			libs[5].Destination = "addons/sourcemod/extensions";
			libs[5].LocalPath = "extensions/mysql";
			libs[5].Name = "dbi.mysql.ext";
			libs[5].ProjectFile = "sm_mysql";

			libs[6].Destination = "addons/sourcemod/extensions";
			libs[6].LocalPath = "extensions/sdktools";
			libs[6].Name = "sdktools.ext";
			libs[6].ProjectFile = "sdktools";

			libs[7].Destination = "addons/sourcemod/extensions";
			libs[7].LocalPath = "extensions/sqlite";
			libs[7].Name = "dbi.sqlite.ext";
			libs[7].ProjectFile = "sm_sqlite";

			return libs;
		}

		/**
		 * Called to build plugins
		 */
		public override Plugin [] GetPlugins()
		{
			Plugin [] plugins = new Plugin[11];

			plugins[0] = new Plugin("admin-flatfile", "admin-flatfile");
			plugins[1] = new Plugin("adminhelp");
			plugins[2] = new Plugin("antiflood");
			plugins[3] = new Plugin("basecommands");
			plugins[4] = new Plugin("reservedslots");
			plugins[5] = new Plugin("basetriggers");
			plugins[6] = new Plugin("nextmap");
			plugins[7] = new Plugin("basechat");
			plugins[8] = new Plugin("basefuncommands");
			plugins[9] = new Plugin("basevotes");
			plugins[10] = new Plugin("basefunvotes");

			return plugins;
		}
	}
}

