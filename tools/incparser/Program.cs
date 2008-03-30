using System;
using System.Collections.Generic;
using System.Text;

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
            if (args.Length < 1)
            {
                Console.WriteLine("Usage: incparser <infile>");
                return 1;
            }

            IncParser inc = null;
            try
            {
                inc = new IncParser(args[0]);
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
    }
}
