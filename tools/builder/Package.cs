using System;

namespace builder
{
	public enum ReleaseMode : int
	{
		ReleaseMode_Release,
		ReleaseMode_Debug,
	};

	public enum BuildMode : int
	{
		BuildMode_Simple,
		BuildMode_OldMetamod,
		BuildMode_Episode1,
		BuildMode_Episode2
	};

	public class Library
	{
		public Library()
		{
			has_platform_ext = false;
			is_executable = false;
			release_mode = ReleaseMode.ReleaseMode_Release;
			build_mode = BuildMode.BuildMode_Simple;
		}
		public string binary_name;		/* Name of binary */
		public string source_path;		/* Local path to library build scripts */
		public ReleaseMode release_mode; /* Release mode */
		public BuildMode build_mode;	/* Build mode */
		public string package_path;		/* Final relative path */
		public bool has_platform_ext;	/* Add extra platform extension? */
		public string vcproj_name;		/* Project file, NULL for standard */
		public bool is_executable;		/* If this is an EXE instead of a DLL */
	};

	public class Plugin
	{
		public Plugin(string file)
		{
			Source = file;
			disabled = false;
		}
		public Plugin (string file, string folder)
		{
			Source = file;
			Folder = folder;
			disabled = false;
		}
		public Plugin (string file, bool is_disabled)
		{
			Source = file;
			disabled = is_disabled;
		}
		public string Folder;		/* Source folder relative to scripting (null for default) */
		public string Source;		/* Source file name */
		public bool disabled;		/* Is the plugin disabled? */
	};

	public abstract class Package
	{
		/**
		 * Must return the root compression point.
		 */
		public abstract void GetCompressBases(ref string path, ref string folder);

		/**
		 * Must return the base package output folder.
		 */
		public abstract string GetBaseFolder();

		/** 
		 * Must return the list of folders to create.
		 */
		public abstract string [] GetFolders();

		/**
		 * Called when file to file copies must be performed
		 */
		public abstract void OnCopyFiles(ABuilder builder);

		/**
		 * Called when dir to dir copies must be performed
		 */
		public abstract void OnCopyFolders(ABuilder builder);

		/**
		 * Called to build libraries
		 */
		public abstract Library [] GetLibraries();

		/**
		 * Called to get package name
		 */
		public abstract string GetPackageName();

		/**
		 * Called to get a plugin list
		 */
		public abstract Plugin [] GetPlugins();
	}
}
