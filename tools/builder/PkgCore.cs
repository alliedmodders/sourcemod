using System;
using System.Collections;

namespace builder
{
	public class PkgCore : Package
	{
		private ArrayList libraries;
		private ArrayList plugins;
		private ArrayList folders;

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
			if (folders != null)
			{
				return (string [])folders.ToArray(typeof(string));
			}

			folders = new ArrayList();

			folders.Add("addons/sourcemod/bin");
			folders.Add("addons/sourcemod/plugins/disabled");
			folders.Add("addons/sourcemod/gamedata");
			folders.Add("addons/sourcemod/gamedata/core.games");
			folders.Add("addons/sourcemod/gamedata/sdktools.games");
			folders.Add("addons/sourcemod/configs/geoip");
			folders.Add("addons/sourcemod/translations");
			folders.Add("addons/sourcemod/logs");
			folders.Add("addons/sourcemod/extensions");
			folders.Add("addons/sourcemod/data");
			folders.Add("addons/sourcemod/scripting/include");
			folders.Add("addons/sourcemod/scripting/admin-flatfile");
			folders.Add("addons/sourcemod/scripting/adminmenu");
			folders.Add("addons/sourcemod/scripting/testsuite");
			folders.Add("cfg/sourcemod");
			folders.Add("addons/sourcemod/configs/sql-init-scripts");
			folders.Add("addons/sourcemod/configs/sql-init-scripts/mysql");
			folders.Add("addons/sourcemod/configs/sql-init-scripts/sqlite");
			folders.Add("addons/sourcemod/scripting/basecommands");
			folders.Add("addons/sourcemod/scripting/basecomm");
			folders.Add("addons/sourcemod/scripting/funvotes");
			folders.Add("addons/sourcemod/scripting/basevotes");
			folders.Add("addons/sourcemod/scripting/basebans");
			folders.Add("addons/sourcemod/scripting/funcommands");
			folders.Add("addons/sourcemod/extensions/auto.1.ep1");
			folders.Add("addons/sourcemod/extensions/auto.2.darkm");
			//folders.Add("addons/sourcemod/extensions/auto.2.ep1");
			folders.Add("addons/sourcemod/extensions/auto.2.ep2");
			folders.Add("addons/sourcemod/extensions/auto.2.l4d");
			folders.Add("addons/sourcemod/scripting/playercommands");
			folders.Add("addons/metamod");
			
			return (string [])folders.ToArray(typeof(string));
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
			builder.CopyFolder(this, "configs/metamod", "addons/metamod", null);
			builder.CopyFolder(this,
				"configs/sql-init-scripts", 
				"addons/sourcemod/configs/sql-init-scripts", 
				null);
			builder.CopyFolder(this,
				"configs/sql-init-scripts/mysql",
				"addons/sourcemod/configs/sql-init-scripts/mysql",
				null);
			builder.CopyFolder(this,
				"configs/sql-init-scripts/sqlite",
				"addons/sourcemod/configs/sql-init-scripts/sqlite",
				null);
			
			string [] plugin_omits = new string[1];
			plugin_omits[0] = "spcomp.exe";

			string [] include_omits = new string[1];
			include_omits[0] = "version.tpl";

			builder.CopyFolder(this, "gamedata", "addons/sourcemod/gamedata", null);
			builder.CopyFolder(this, "gamedata/sdktools.games", "addons/sourcemod/gamedata/sdktools.games", null);
			builder.CopyFolder(this, "gamedata/core.games", "addons/sourcemod/gamedata/core.games", null);
			builder.CopyFolder(this, "plugins", "addons/sourcemod/scripting", plugin_omits);
			builder.CopyFolder(this, "plugins/include", "addons/sourcemod/scripting/include", include_omits);
			builder.CopyFolder(this, "translations", "addons/sourcemod/translations", null);
			builder.CopyFolder(this, "public/licenses", "addons/sourcemod", null);
			builder.CopyFolder(this, "plugins/admin-flatfile", "addons/sourcemod/scripting/admin-flatfile", null);
			builder.CopyFolder(this, "plugins/adminmenu", "addons/sourcemod/scripting/adminmenu", null);
			builder.CopyFolder(this, "plugins/testsuite", "addons/sourcemod/scripting/testsuite", null);
			builder.CopyFolder(this, "plugins/basecommands", "addons/sourcemod/scripting/basecommands", null);
			builder.CopyFolder(this, "plugins/basecomm", "addons/sourcemod/scripting/basecomm", null);
			builder.CopyFolder(this, "plugins/funvotes", "addons/sourcemod/scripting/funvotes", null);
			builder.CopyFolder(this, "plugins/basevotes", "addons/sourcemod/scripting/basevotes", null);
			builder.CopyFolder(this, "plugins/basebans", "addons/sourcemod/scripting/basebans", null);
			builder.CopyFolder(this, "plugins/funcommands", "addons/sourcemod/scripting/funcommands", null);
			builder.CopyFolder(this, "plugins/playercommands", "addons/sourcemod/scripting/playercommands", null);
		}

		/**
		 * Called to build libraries
		 */
		public override Library [] GetLibraries()
		{
			if (libraries != null)
			{
				return (Library [])libraries.ToArray(typeof(Library));
			}

			libraries = new ArrayList();

			Library lib = new Library();
			lib.package_path = "addons/sourcemod/bin";
			lib.source_path = "loader";
			lib.binary_name = "sourcemod_mm";
			lib.vcproj_name = "loader";
			lib.build_mode = BuildMode.BuildMode_Simple;
			lib.has_platform_ext = true;
			libraries.Add(lib);

			lib = new Library();
			lib.package_path = "addons/sourcemod/bin";
			lib.source_path = "core";
			lib.binary_name = "sourcemod.1.ep1";
			lib.vcproj_name = "sourcemod_mm";
			lib.build_mode = BuildMode.BuildMode_OldMetamod;
			libraries.Add(lib);

			lib = new Library();
			lib.package_path = "addons/sourcemod/bin";
			lib.source_path = "core";
			lib.binary_name = "sourcemod.2.darkm";
			lib.vcproj_name = "sourcemod_mm";
			lib.build_mode = BuildMode.BuildMode_DarkMessiah;
			lib.platform = BasePlatform.Platform_Windows;
			libraries.Add(lib);

			lib = new Library();
			lib.package_path = "addons/sourcemod/bin";
			lib.source_path = "core";
			lib.binary_name = "sourcemod.2.ep2";
			lib.vcproj_name = "sourcemod_mm";
			lib.build_mode = BuildMode.BuildMode_Episode2;
			libraries.Add(lib);

			lib = new Library();
			lib.package_path = "addons/sourcemod/bin";
			lib.source_path = "core";
			lib.binary_name = "sourcemod.2.l4d";
			lib.vcproj_name = "sourcemod_mm";
			lib.build_mode = BuildMode.BuildMode_Left4Dead;
			libraries.Add(lib);

			lib = new Library();
			lib.package_path = "addons/sourcemod/bin";
			lib.source_path = "sourcepawn/jit";
			lib.binary_name = "sourcepawn.jit.x86";
			lib.vcproj_name = "jit-x86";
			libraries.Add(lib);

			lib = new Library();
			lib.package_path = "addons/sourcemod/scripting";
			lib.source_path = "sourcepawn/compiler";
			lib.binary_name = "spcomp";
			lib.is_executable = true;
			libraries.Add(lib);

			lib = new Library();
			lib.package_path = "addons/sourcemod/extensions";
			lib.source_path = "extensions/geoip";
			lib.binary_name = "geoip.ext";
			lib.vcproj_name = "geoip";
			libraries.Add(lib);

			lib = new Library();
			lib.package_path = "addons/sourcemod/extensions/auto.1.ep1";
			lib.source_path = "extensions/bintools";
			lib.binary_name = "bintools.ext";
			lib.vcproj_name = "bintools";
			lib.build_mode = BuildMode.BuildMode_OldMetamod;
			libraries.Add(lib);

            lib = new Library();
            lib.package_path = "addons/sourcemod/extensions/auto.2.darkm";
            lib.source_path = "extensions/bintools";
            lib.binary_name = "bintools.ext";
            lib.vcproj_name = "bintools";
            lib.build_mode = BuildMode.BuildMode_DarkMessiah;
			lib.platform = BasePlatform.Platform_Windows;
            libraries.Add(lib);

			lib = new Library();
			lib.package_path = "addons/sourcemod/extensions/auto.2.ep2";
			lib.source_path = "extensions/bintools";
			lib.binary_name = "bintools.ext";
			lib.vcproj_name = "bintools";
			lib.build_mode = BuildMode.BuildMode_Episode2;
			libraries.Add(lib);

			lib = new Library();
			lib.package_path = "addons/sourcemod/extensions/auto.2.l4d";
			lib.source_path = "extensions/bintools";
			lib.binary_name = "bintools.ext";
			lib.vcproj_name = "bintools";
			lib.build_mode = BuildMode.BuildMode_Left4Dead;
			libraries.Add(lib);

			lib = new Library();
			lib.package_path = "addons/sourcemod/extensions";
			lib.source_path = "extensions/mysql";
			lib.binary_name = "dbi.mysql.ext";
			lib.vcproj_name = "sm_mysql";
			libraries.Add(lib);

			lib = new Library();
			lib.package_path = "addons/sourcemod/extensions/auto.1.ep1";
			lib.source_path = "extensions/sdktools";
			lib.binary_name = "sdktools.ext";
			lib.vcproj_name = "sdktools";
			lib.build_mode = BuildMode.BuildMode_OldMetamod;
			libraries.Add(lib);

            lib = new Library();
            lib.package_path = "addons/sourcemod/extensions/auto.2.darkm";
            lib.source_path = "extensions/sdktools";
            lib.binary_name = "sdktools.ext";
            lib.vcproj_name = "sdktools";
            lib.build_mode = BuildMode.BuildMode_DarkMessiah;
			lib.platform = BasePlatform.Platform_Windows;
            libraries.Add(lib);

			lib = new Library();
			lib.package_path = "addons/sourcemod/extensions/auto.2.ep2";
			lib.source_path = "extensions/sdktools";
			lib.binary_name = "sdktools.ext";
			lib.vcproj_name = "sdktools";
			lib.build_mode = BuildMode.BuildMode_Episode2;
			libraries.Add(lib);

			lib = new Library();
			lib.package_path = "addons/sourcemod/extensions/auto.2.l4d";
			lib.source_path = "extensions/sdktools";
			lib.binary_name = "sdktools.ext";
			lib.vcproj_name = "sdktools";
			lib.build_mode = BuildMode.BuildMode_Left4Dead;
			libraries.Add(lib);

			lib = new Library();
			lib.package_path = "addons/sourcemod/extensions";
			lib.source_path = "extensions/sqlite";
			lib.binary_name = "dbi.sqlite.ext";
			lib.vcproj_name = "sm_sqlite";
			libraries.Add(lib);

			lib = new Library();
			lib.package_path = "addons/sourcemod/extensions/auto.1.ep1";
			lib.source_path = "extensions/cstrike";
			lib.binary_name = "game.cstrike.ext";
			lib.vcproj_name = "cstrike";
			lib.build_mode = BuildMode.BuildMode_OldMetamod;
			libraries.Add(lib);

			lib = new Library();
			lib.package_path = "addons/sourcemod/extensions";
			lib.source_path = "extensions/topmenus";
			lib.binary_name = "topmenus.ext";
			lib.vcproj_name = "topmenus";
			libraries.Add(lib);

			lib = new Library();
			lib.package_path = "addons/sourcemod/extensions/auto.2.ep2";
			lib.source_path = "extensions/tf2";
			lib.binary_name = "game.tf2.ext";
			lib.vcproj_name = "tf2";
			lib.build_mode = BuildMode.BuildMode_Episode2;
			libraries.Add(lib);

			lib = new Library();
			lib.package_path = "addons/sourcemod/extensions";
			lib.source_path = "extensions/regex";
			lib.binary_name = "regex.ext";
			lib.vcproj_name = "regex";
			libraries.Add(lib);

			lib = new Library();
			lib.package_path = "addons/sourcemod/extensions";
			lib.source_path = "extensions/clientprefs";
			lib.binary_name = "clientprefs.ext";
			lib.vcproj_name = "clientprefs";
			libraries.Add(lib);

			lib = new Library();
			lib.package_path = "addons/sourcemod/extensions";
			lib.source_path = "extensions/curl";
			lib.binary_name = "webternet.ext";
			lib.vcproj_name = "curl";
			libraries.Add(lib);

			return (Library [])libraries.ToArray(typeof(Library));
		}

		/**
		 * Called to build plugins
		 */
		public override Plugin [] GetPlugins()
		{
			if (plugins != null)
			{
				return (Plugin [])plugins.ToArray(typeof(Plugin));
			}

			plugins = new ArrayList();

			plugins.Add(new Plugin("admin-flatfile", "admin-flatfile"));
			plugins.Add(new Plugin("adminhelp"));
			plugins.Add(new Plugin("antiflood"));
			plugins.Add(new Plugin("basecommands"));
			plugins.Add(new Plugin("reservedslots"));
			plugins.Add(new Plugin("basetriggers"));
			plugins.Add(new Plugin("nextmap"));
			plugins.Add(new Plugin("basechat"));
			plugins.Add(new Plugin("funcommands"));
			plugins.Add(new Plugin("basevotes"));
			plugins.Add(new Plugin("funvotes"));
			plugins.Add(new Plugin("admin-sql-prefetch", true));
			plugins.Add(new Plugin("admin-sql-threaded", true));
			plugins.Add(new Plugin("sql-admin-manager", true));
			plugins.Add(new Plugin("basebans"));
			plugins.Add(new Plugin("mapchooser", true));
			plugins.Add(new Plugin("basecomm"));
			plugins.Add(new Plugin("randomcycle", true));
			plugins.Add(new Plugin("rockthevote", true));
			plugins.Add(new Plugin("adminmenu"));
			plugins.Add(new Plugin("playercommands"));
			plugins.Add(new Plugin("clientprefs"));
			plugins.Add(new Plugin("nominations", true));
			plugins.Add(new Plugin("sounds"));

			return (Plugin [])plugins.ToArray(typeof(Plugin));
		}
	}
}

