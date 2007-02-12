using System;
using System.IO;
using System.Text;

namespace builder
{
	public enum BasePlatform
	{
		Platform_Windows,
		Platform_Linux
	};

	public class Config
	{
		public string SourceBase;
		public string OutputBase;
		public string BuilderPath;
		public string CompressOptions;
		public string SVNVersion;
		public string ProductVersion;
		public string Compressor;
		public string BuildOptions;
		public builder.BasePlatform Platform;

		public Config()
		{
			if ((int)System.Environment.OSVersion.Platform == 128)
			{
				Platform = BasePlatform.Platform_Linux;
			} 
			else 
			{
				Platform = BasePlatform.Platform_Windows;
			}
		}

		public static string PathFormat(string format, params string [] args)
		{
			string temp = string.Format(format, args);
			return temp.Replace(Path.AltDirectorySeparatorChar, Path.DirectorySeparatorChar);
		}

		public bool ReadFromFile(string file)
		{
			bool read = true;
			StreamReader sr = null;
			try
			{
				sr = new StreamReader(file);

				string line;
				string delim = "\t \n\r\v";
				string split = "=";
				
				while ( (line = sr.ReadLine()) != null )
				{
					line = line.Trim(delim.ToCharArray());
					if (line.Length < 1 || line[0] == ';')
					{
						continue;
					}
					string [] s = line.Split(split.ToCharArray());
					string key, val = "";
					if (s.GetLength(0) >= 1)
					{
						key = s[0];
						if (s.GetLength(0) >= 2)
						{
							for (int i=1; i<s.GetLength(0); i++)
							{
								val += s[i];
							}
						}
						key = key.Trim(delim.ToCharArray());
						val = val.Trim(delim.ToCharArray());
						if (key.CompareTo("SourceBase") == 0)
						{
							SourceBase = val;
						} 
						else if (key.CompareTo("OutputBase") == 0) 
						{
							OutputBase = val;
						} 
						else if (key.CompareTo("BuilderPath") == 0) 
						{
							BuilderPath = val;
						} 
						else if (key.CompareTo("CompressOptions") == 0) 
						{
							CompressOptions = val;
						} 
						else if (key.CompareTo("SVNVersion") == 0) 
						{
							SVNVersion = val;
						} 
						else if (key.CompareTo("ProductVersion") == 0)
						{
							ProductVersion = val;
						} 
						else if (key.CompareTo("Compressor") == 0) 
						{
							Compressor = val;
						} 
						else if (key.CompareTo("BuildOptions") == 0) 
						{
							BuildOptions = val;
						}
					}
				}
			} 
			catch (System.Exception e)
			{
				Console.WriteLine("Unable to read {0:s}: {1:s}", file, e.Message);
				read = false;
			}

			if (sr != null)
			{
				sr.Close();
			}

			return read;
		}
	}
}
