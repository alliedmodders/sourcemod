using System;
using System.Collections.Generic;
using System.Text;

namespace incparser
{
    class ParseWriter
    {
        private int level;
        private StringBuilder data;

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
