using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace incparser
{
    class Program
    {
        static void Main(string[] args)
        {
            Environment.Exit(SubMain(args));
        }

        static int SubMain(string[] args)
        {
            string directory = ".";
            string template = "template.txt";
            string outputfile = "output.txt";
            string file = null;

            if (args.Length == 0 || (args.Length == 1 && args[0] == "-h"))
            {
                PrintHelp();
                return 0;
            }

            for (int i=0; i<args.Length-1; i++)
            {
                if (args[i] == "-d")
                {
                    directory = args[i + 1];
                }

                if (args[i] == "-t")
                {
                    template = args[i + 1];
                }

                if (args[i] == "-o")
                {
                    outputfile = args[i + 1];
                }

                if (args[i] == "-f")
                {
                    file = args[i + 1];
                }

                if (args[i] == "-h")
                {
                    if (args[i + 1] == "template")
                    {
                        PrintTemplateHelp();
                        return 0;
                    }
                    PrintHelp();
                    return 0;
                }
            }

            IncParser inc = null;

            if (file == null)
            {
                DirectoryInfo di = new DirectoryInfo(directory);
                FileInfo[] rgFiles = di.GetFiles("*.inc");

                ParseWriter pwr = new ParseWriter();

                foreach (FileInfo fi in rgFiles)
                {
                    pwr.Reset();

                    Console.Write("Parsing file: " + fi.ToString() + "... ");
                  
                    try
                    {
                      inc = new IncParser(fi.FullName);
                    }
                    catch (ParseException e)
                    {
                        Console.WriteLine("Initial browsing failed: " + e.Message);
                        continue;
                    }
                    catch (System.Exception e)
                    {
                        Console.WriteLine("Failed to read file: " + e.Message);
                        continue;
                    }

                    try
                    {
                        inc.Parse(pwr);
                    }
                    catch (System.Exception e)
                    {
                        Console.WriteLine("Error parsing file (line " + inc.GetLineNumber() + "): " + e.Message);
                        continue;
                    }

                    if (pwr.Level != 0)
                    {
                        Console.WriteLine("Fatal parse error detected; unable to complete output.");
                        continue;
                    }

                    Console.WriteLine("Complete!");
                 }

                 pwr.WriteFiles(template, outputfile);

                 Console.WriteLine("Parsing Complete!");

                 return 0;
            }
      
            try
            {
                inc = new IncParser(file);
            }
            catch (ParseException e)
            {
                Console.WriteLine("Initial browsing failed: " + e.Message);
                return 1;
            }
            catch (System.Exception e)
            {
                Console.WriteLine("Failed to read file: " + e.Message);
                return 1;
            }

            ParseWriter pw = new ParseWriter();

            try
            {
                inc.Parse(pw);
            }
            catch (System.Exception e)
            {
                Console.WriteLine("Error parsing file (line " + inc.GetLineNumber() + "): " + e.Message);
                return 1;
            }

            if (pw.Level != 0)
            {
                Console.WriteLine("Fatal parse error detected; unable to complete output.");
                return 1;
            }

            Console.Write(pw.Contents);
            Console.Write("\n");

            return 0;
        }

        static void PrintHelp()
        {
            Console.WriteLine("SourcePawn include file parser by BAILOPAN (edited by pRED*)");
            Console.Write("\n");
            Console.WriteLine("This can parse a single file into SMC configuration format or an entire directory into a template file if -f is not specified (current directory is used if -d is not specified)");
            Console.Write("\n");
            Console.WriteLine("Parameters:");
            Console.Write("\n");
            Console.WriteLine("-f <filename> - Specify an input file to be used");
            Console.WriteLine("-d <path> - Specify a directory to parse (only *.inc files are used)");
            Console.WriteLine("-t <filename> - Specify a template file to be used");
            Console.WriteLine("-o <filename> - Specify an output file to be used");
            Console.WriteLine("-h - Display this help");
            Console.WriteLine("-h template - Displays help about templates");
        }

        static void PrintTemplateHelp()
        {
            Console.WriteLine("Template File Help:");
            Console.WriteLine("The inc parser can read a template file and replace variables with the outputs of it's parse and write into the output file");
            Console.Write("\n");
            Console.WriteLine("Variables:");
            Console.Write("\n");
            Console.WriteLine("$defines $enums $enumtypes $forwards $natives $stocks $funcenums $functags $structs");
        }
    }
}
