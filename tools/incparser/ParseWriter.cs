using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace incparser
{
    class ParseWriter
    {
        private int level;
        private StringBuilder data;

        public ArrayList enumList = new ArrayList();
        public ArrayList defineList = new ArrayList();
        public ArrayList enumTypeList = new ArrayList();
        public ArrayList forwardList = new ArrayList();
        public ArrayList nativeList = new ArrayList();
        public ArrayList stockList = new ArrayList();
        public ArrayList funcenumList = new ArrayList();
        public ArrayList functagList = new ArrayList();
        public ArrayList structList = new ArrayList();

        public int Level
        {
            get
            {
                return level;
            }
        }

        public string Contents
        {
            get
            {
                return data.ToString();
            }
        }

        public void Reset()
        {
            level = 0;
            data = new StringBuilder();
        }

        public ParseWriter()
        {
            level = 0;
            data = new StringBuilder();
        }

        public void BeginSection(string name)
        {
            WriteLine("\"" + PrepString(name) + "\"");
            WriteLine("{");
            level++;
        }

        public void WritePair(string key, string value)
        {
            WriteLine("\"" + PrepString(key) + "\"\t\t\"" + PrepString(value) + "\"");
        }

        public void EndSection()
        {
            if (--level < 0)
            {
                throw new System.Exception("Writer nesting level went out of bounds");
            }
            WriteLine("}");
        }

        public void WriteFiles(string template, string outputfile)
        {
            StreamReader sr = null;

            try
            {
                sr = File.OpenText(template);
            }
            catch (Exception e)
            {
                Console.WriteLine("Failed to open template file: " + e.Message);
                return;
            }

            string contents = sr.ReadToEnd();

            string replace = ToOutputString(defineList);
            contents = contents.Replace("$defines", replace);

            replace = ToOutputString(enumList);
            contents = contents.Replace("$enums", replace);

            replace = ToOutputString(enumTypeList);
            contents = contents.Replace("$enumtypes", replace);

            replace = ToOutputString(forwardList);
            contents = contents.Replace("$forwards", replace);

            replace = ToOutputString(nativeList);
            contents = contents.Replace("$natives", replace);

            replace = ToOutputString(stockList);
            contents = contents.Replace("$stocks", replace);

            replace = ToOutputString(funcenumList);
            contents = contents.Replace("$funcenums", replace);

            replace = ToOutputString(functagList);
            contents = contents.Replace("$functags", replace);

            replace = ToOutputString(structList);
            contents = contents.Replace("$structs", replace);

            StreamWriter sw;
            sw = File.CreateText(outputfile);

            sw.Write(contents);

            sr.Close();
            sw.Close();
        }

        private string ToOutputString(ArrayList a)
        {
            string defines = "";
            int count = 0;

            foreach (object o in a)
            {
              defines += o;
              defines += " ";
              count += o.ToString().Length;

              if (count > 180)
              {
                defines += "\r\n";
                count = 0;
              }
            }

            return defines;
        }

        private void WriteLine(string line)
        {
            Tabinate();
            data.Append(line + "\n");
        }

        private void Tabinate()
        {
            for (int i = 0; i < level; i++)
            {
                data.Append("\t");
            }
        }

        private string PrepString(string text)
        {
            /* Escape all escaped newlines (so they can be unescaped later) */
            text = text.Replace("\\n", "\\\\n");
            /* Escape all literal newlines */
            text = text.Replace("\n", "\\n");
            text = text.Replace("\r", "");
            /* Remove escaped quotations */
            text = text.Replace("\\\"", "\"");
            /* Replace all quotations with escaped ones now */
            text = text.Replace("\"", "\\\"");
            return text;
        }
    }
}
