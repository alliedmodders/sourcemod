using System;

namespace builder
{
	public class Library
	{
		public Library()
		{
			PlatformExt = false;
			ProjectFile = null;
			IsExecutable = false;
			ReleaseBuild = "Release";
		}
		public string Name;			/* Name of binary */
		public string LocalPath;	/* Local path to library build scripts */
		public string ReleaseBuild;	/* Release build name */
		public string Destination;	/* Final relative path */
		public bool PlatformExt;	/* Extra platform extension */
		public string ProjectFile;	/* Project file, NULL for standard */
		public bool IsExecutable;	/* If this is an EXE instead of a DLL */
		//string DebugBuild;		/* Debug build name */
	};

	public abstract class Package
	{
		/**
		 * Must return the base package folder.
		 */
		public abstract string GetBaseFolder();

		/** 
		 * Must return the list of folders to create.
		 */
		public abstract string [] GetFolders();

		/**
		 * Called when file to file copies must be performed
		 */
		public abstract void OnCopyFiles();

		/**
		 * Called when dir to dir copies must be performed
		 */
		public abstract void OnCopyFolders(ABuilder builder);

		/**
		 * Called to build libraries
		 */
		public abstract Library [] GetLibraries();
	}
}
