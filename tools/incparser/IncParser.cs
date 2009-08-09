using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace incparser
{
    enum LexToken
    {
        TOKEN_NONE,
        TOKEN_INCLUDE,
        TOKEN_DEFINE,
        TOKEN_CONST,
        TOKEN_STRUCT,
        TOKEN_FUNCTAG,
        TOKEN_FUNCENUM,
        TOKEN_ENUM,
        TOKEN_NATIVE,
        TOKEN_STOCK,
        TOKEN_PUBLIC,
        TOKEN_FORWARD,
        TOKEN_QUOTCHAR,
        TOKEN_OPERATOR,
        TOKEN_CHARACTER,
        /*TOKEN_STRING,*/
        TOKEN_LABEL,
        TOKEN_SYMBOL,
        TOKEN_DOCBLOCK,
        TOKEN_EOF,
    }

    class ParseException : System.Exception
    {
        public ParseException(string message) : base(message)
        {
        }
    };
         
    class KeywordToken
    {
        public KeywordToken(string k, LexToken t)
        {
            keyword = k;
            token = t;
        }
        public string keyword;
        public LexToken token;
    };

    class Tokenizer
    {
        private static bool s_initialized = false;
        private static int s_position = 0;
        private static KeywordToken [] s_tokens = null;
        public static KeywordToken[] Tokens = null;

        public static void Initialize()
        {
            if (s_initialized)
            {
                return;
            }
            s_tokens = new KeywordToken[12];
            s_initialized = true;
            s_position = 0;

            AddToken("#include", LexToken.TOKEN_INCLUDE);
            AddToken("#define", LexToken.TOKEN_DEFINE);
            AddToken("native", LexToken.TOKEN_NATIVE);
            AddToken("functag", LexToken.TOKEN_FUNCTAG);
            AddToken("funcenum", LexToken.TOKEN_FUNCENUM);
            AddToken("enum", LexToken.TOKEN_ENUM);
            AddToken("stock", LexToken.TOKEN_STOCK);
            AddToken("public", LexToken.TOKEN_PUBLIC);
            AddToken("forward", LexToken.TOKEN_FORWARD);
            AddToken("const", LexToken.TOKEN_CONST);
            AddToken("struct", LexToken.TOKEN_STRUCT);
            AddToken("operator", LexToken.TOKEN_OPERATOR);

            Tokens = s_tokens;
        }

        private static void AddToken(string lex, LexToken tok)
        {
            s_tokens[s_position] = new KeywordToken(lex, tok);
            s_position++;
        }
    };

    class IncParser
    {
        private string FileName;
        private StringBuilder Contents;
        private bool LexPushed;
        private LexToken _LastToken;
        private string _LexString;
        private char _LexChar;
        private uint LineNo;

        public string LexString
        {
            get
            {
                return _LexString;
            }
        }

        public char LexChar
        {
            get
            {
                return _LexChar;
            }
        }

        public uint GetLineNumber()
        {
            return LineNo;
        }

        public IncParser(string file)
        {
            FileName = file;

            /* Clear out lexer stuff */
            LexPushed = false;
            _LastToken = LexToken.TOKEN_NONE;
            _LexString = null;
            _LexChar = '\0';
            LineNo = 1;

            /* Initialize; this can throw an exception */
            Initialize();
        }

         public void Parse(ParseWriter w)
        {
            w.BeginSection(System.IO.Path.GetFileName(FileName));
            LexToken tok = LexToken.TOKEN_NONE;
            while ((tok = lex()) != LexToken.TOKEN_EOF)
            {
                switch (tok)
                {
                    case LexToken.TOKEN_DOCBLOCK:
                        {
                            w.WritePair("doc", LexString);
                            break;
                        }
                    case LexToken.TOKEN_DEFINE:
                        {
                            PARSE_Define(w);
                            break;
                        }
                    case LexToken.TOKEN_ENUM:
                        {
                            PARSE_Enum(w);
                            break;
                        }
                    case LexToken.TOKEN_FORWARD:
                        {
                            PARSE_Function(tok, w);
                            break;
                        }
                    case LexToken.TOKEN_NATIVE:
                        {
                            PARSE_Function(tok, w);
                            break;
                        }
                    case LexToken.TOKEN_STOCK:
                        {
                            PARSE_Function(tok, w);
                            break;
                        }
                    case LexToken.TOKEN_FUNCTAG:
                        {
                            PARSE_FuncTag(w);
                            break;
                        }
                    case LexToken.TOKEN_FUNCENUM:
                        {
                            PARSE_FuncEnum(w);
                            break;
                        }
                    case LexToken.TOKEN_INCLUDE:
                        {
                            PARSE_Include(w);
                            break;
                        }
                    case LexToken.TOKEN_STRUCT:
                        {
                            PARSE_Struct(w);
                            break;
                        }
                    case LexToken.TOKEN_PUBLIC:
                        {
                            PARSE_Public(w);
                            break;
                        }
                    default:
                        {
                            if (_LexString == "static" || _LexString == "new")
                            {
                                PARSE_Public(w);
                                break;
                            }
                            throw new ParseException("Unrecognized token: " + tok + " " + _LexString);
                        }
                }
            }
            w.EndSection();
        }

        private void PARSE_Public(ParseWriter w)
        {
            MatchToken(LexToken.TOKEN_CONST);

            /* Eat up an optional tag */
            MatchToken(LexToken.TOKEN_LABEL);

            /* Eat up a name */
            NeedToken(LexToken.TOKEN_SYMBOL);

            while (MatchChar('['))
            {
                DiscardUntilChar(']');
                NeedChar(']');
            }

            if (MatchChar('='))
            {
                ignore_block();
            }
            else if (MatchChar('(') && MatchChar(')'))
            {
                ignore_block();
                return;
            }

            NeedChar(';');
        }

        private void PARSE_Struct(ParseWriter w)
        {
            /* For now, we completely ignore these (they're not written to the output) */
            NeedToken(LexToken.TOKEN_SYMBOL);
            w.structList.Add(LexString);
            NeedChar('{');

            bool need_closebrace = true;
            bool more_entries = true;
            do
            {
                if (MatchChar('}'))
                {
                    need_closebrace = false;
                    break;
                }
                MatchToken(LexToken.TOKEN_CONST);
                MatchToken(LexToken.TOKEN_LABEL);
                NeedToken(LexToken.TOKEN_SYMBOL);
                while (MatchChar('['))
                {
                    DiscardUntilChar(']');
                    NeedChar(']');
                }
                more_entries = MatchChar(',');
                MatchToken(LexToken.TOKEN_DOCBLOCK);
                if (!more_entries)
                {
                    break;
                }
            } while (true);
            if (need_closebrace)
            {
                NeedChar('}');
            }
            NeedChar(';');
        }

        private void PARSE_Include(ParseWriter w)
        {
            /* read until the end of the line */
            int index = Contents.ToString().IndexOf('\n');
            string value;
            if (index == -1)
            {
                value = Contents.ToString();
                Contents.Remove(0, value.Length);
            }
            else if (index == 0)
            {
                value = "";
            }
            else
            {
                value = Contents.ToString().Substring(0, index);
                Contents.Remove(0, index);
            }

            /* Strip whitespace */
            value = value.Trim('\r', '\t', ' ');

            /* Write */
            w.BeginSection("include");
            w.WritePair("name", value);
            w.EndSection();
        }

        private void PARSE_FuncEnum(ParseWriter w)
        {
            w.BeginSection("funcenum");

            /* Get the functag name */
            NeedToken(LexToken.TOKEN_SYMBOL);
            w.WritePair("name", LexString);
            w.funcenumList.Add(LexString);

            NeedChar('{');

            bool need_closebrace = true;
            do
            {
                /* Shortcut out? */
                if (MatchChar('}'))
                {
                    need_closebrace = false;
                    break;
                }

                /* Start function section */
                w.BeginSection("function");

                if (MatchToken(LexToken.TOKEN_DOCBLOCK))
                {
                    w.WritePair("doc", LexString);
                }
                
                /* Get the return tag */
                if (MatchToken(LexToken.TOKEN_LABEL))
                {
                    w.WritePair("return", LexString);
                }

                /* Get function type */
                if (MatchToken(LexToken.TOKEN_PUBLIC))
                {
                    w.WritePair("type", "public");
                }
                else if (MatchToken(LexToken.TOKEN_STOCK))
                {
                    w.WritePair("type", "stock");
                }
                
                /* Parse the parameters */
                ParseParameters(w);

                /* End the section */
                w.EndSection();

                if (!MatchChar(','))
                {
                    break;
                }
            } while (true);

            if (need_closebrace)
            {
                NeedChar('}');
            }
            NeedChar(';');

            w.EndSection();
        }

        private void PARSE_FuncTag(ParseWriter w)
        {
            w.BeginSection("functag");

            if (MatchToken(LexToken.TOKEN_PUBLIC))
            {
                w.WritePair("type", "public");
            }
            else if (MatchToken(LexToken.TOKEN_STOCK))
            {
                w.WritePair("type", "stock");
            }

            if (MatchToken(LexToken.TOKEN_LABEL))
            {
                w.WritePair("return", LexString);
            }

            /* Get the functag name */
            NeedToken(LexToken.TOKEN_SYMBOL);
            w.WritePair("name", LexString);
            w.functagList.Add(LexString);

            ParseParameters(w);
            NeedChar(';');
            
            w.EndSection();
        }

        private void PARSE_Function(LexToken tok, ParseWriter w)
        {
            string tag="", name;

            /* Get the return value */
            if (MatchToken(LexToken.TOKEN_LABEL))
            {
                tag = LexString;
            }

            if (MatchToken(LexToken.TOKEN_OPERATOR))
            {
                /* Check if we're some sort of invalid function */
                DiscardUntilCharOrComment('(', false);
                ParseParameters(null);
                if (tok == LexToken.TOKEN_STOCK)
                {
                    ignore_block();
                }
                else if (tok == LexToken.TOKEN_NATIVE || tok == LexToken.TOKEN_FORWARD)
                {
                    if (MatchChar('='))
                    {
                        DiscardUntilCharOrComment(';', false);
                    }
                    NeedChar(';');
                }
                return;
            }

            /* Get the name */
            NeedToken(LexToken.TOKEN_SYMBOL);
            name = LexString;

            PARSE_Function(tok, w, tag, name);
        }

        private void PARSE_Function(LexToken tok, ParseWriter w, string tag, string name)
        {
            if (tok == LexToken.TOKEN_FORWARD)
            {
                w.BeginSection("forward");
                w.forwardList.Add(name);
            }
            else if (tok == LexToken.TOKEN_NATIVE)
            {
                w.BeginSection("native");
                w.nativeList.Add(name);
            }
            else if (tok == LexToken.TOKEN_STOCK)
            {
                w.BeginSection("stock");
                w.stockList.Add(name);
            }

            w.WritePair("name", name);

            if (tag.Length > 0)
            {
                w.WritePair("return", tag);
            }

            ParseParameters(w);

            if (tok == LexToken.TOKEN_STOCK)
            {
                ignore_block();
            }
            else
            {
                /* Make sure there's a semicolon */
                if (MatchChar('='))
                {
                    DiscardUntilCharOrComment(';', false);
                }
                NeedChar(';');
            }

            w.EndSection();
        }

        private void ParseParameters(ParseWriter w)
        {
            NeedChar('(');
            string extra;
            do
            {
                if (MatchChar(')'))
                {
                    return;
                }

                /* Start the parameter */
                if (w != null)
                {
                    w.BeginSection("parameter");
                }

                /* Check for a 'const' token */
                if (MatchToken(LexToken.TOKEN_CONST) && w != null)
                {
                    w.WritePair("const", "true");
                }

                /* Check if it's by reference */
                if (MatchChar('&') && w != null)
                {
                    w.WritePair("byref", "true");
                }

                /* Check for a tag */
                if (MatchToken(LexToken.TOKEN_LABEL) && w != null)
                {
                    w.WritePair("tag", LexString);
                }

                /* Get the symbol and write it */
                if (MatchString(0, "...") && w != null)
                {
                    w.WritePair("name", "...");
                }
                else
                {
                    NeedToken(LexToken.TOKEN_SYMBOL);
                    if (w != null)
                    {
                        w.WritePair("name", LexString);
                    }
                }

                /* Check if we have a default value yet */
                bool default_value = false;
                if (MatchChar('='))
                {
                    default_value = true;
                }
                else if (MatchChar('['))
                {
                    extra = "";
                    string temp;
                    do
                    {
                        extra += "[";
                        temp = ConsumeUntilChar(']');
                        if (temp != null)
                        {
                            extra += temp;
                        }
                        extra += "]";
                        NeedChar(']');
                    } while (MatchChar('['));
                    w.WritePair("decoration", extra);
                }

                /* If we have a default value, get it */
                if (default_value || MatchChar('='))
                {
                    if ((extra = ConsumeParamDefValue()) == null)
                    {
                        throw new ParseException("Expected a default value; found none");
                    }
                    if (w != null)
                    {
                        w.WritePair("defval", extra);
                    }
                }

                /* End the parameter */
                if (w != null)
                {
                    w.EndSection();
                }

                if (!MatchChar(','))
                {
                    break;
                }
            } while (true);

            NeedChar(')');
        }

        private void PARSE_Enum(ParseWriter w)
        {
            string name = "";
            if (MatchToken(LexToken.TOKEN_SYMBOL))
            {
                name = LexString;
            }

            w.BeginSection("enum");
            w.WritePair("name", name);
            w.BeginSection("properties");

            w.enumList.Add(name);

            NeedChar('{');
            bool more_entries = true;
            bool need_closebrace = true;

            if (MatchToken(LexToken.TOKEN_DOCBLOCK))
            {
              w.WritePair("doc", LexString);
            }

            do
            {
                if (MatchChar('}'))
                {
                    need_closebrace = false;
                    break;
                }
                NeedToken(LexToken.TOKEN_SYMBOL);
                name = LexString;
 
                if (MatchChar('='))
                {
                    DiscardUntilCharOrComment(",\n}", true);
                }
                more_entries = MatchChar(',');
                w.WritePair("name", name);
                w.enumTypeList.Add(name);
                if (MatchToken(LexToken.TOKEN_DOCBLOCK))
                {
                    w.WritePair("doc", LexString);
                }
            } while (more_entries);
            if (need_closebrace)
            {
                NeedChar('}');
            }
            NeedChar(';');
            w.EndSection();
            w.EndSection();
        }

        private void PARSE_Define(ParseWriter w)
        {
            /* first, get the symbol name */
            NeedToken(LexToken.TOKEN_SYMBOL);
            string name = LexString;
            string value;

            /* read until we hit the end of a line OR a comment */
            int endpoint = -1;
            for (int i = 0; i < Contents.Length; i++)
            {
                if (Contents[i] == '\n')
                {
                    endpoint = i;
                    break;
                }
                else if (CheckString(i, "/**"))
                {
                    endpoint = i;
                    break;
                }
            }

            if (endpoint < 1)
            {
                value = "";
            }
            else
            {
                value = Contents.ToString().Substring(0, endpoint);
                Contents.Remove(0, endpoint);
            }

            /* Write */
            w.defineList.Add(name);

            w.BeginSection("define");
            w.WritePair("name", name);
            w.WritePair("linetext", value);
            w.EndSection();
        }

        /* Reads up to a character and discards all text before it.
         */
        private void DiscardUntilChar(char c)
        {
            int eol = Contents.ToString().IndexOf(c);

            if (eol < 1)
            {
                return;
            }

            Contents.Remove(0, eol);
        }

        /* Same as DiscardUntilChar, except the discarded
         * text is returned first.
         */
        private string ConsumeUntilChar(char c)
        {
            int eol = Contents.ToString().IndexOf(c);

            if (eol < 1)
            {
                return null;
            }

            string rest = Contents.ToString().Substring(0, eol);
            Contents.Remove(0, eol);
            return rest;
        }

        /**
         * Returns a parameter's default value.
         */
        private string ConsumeParamDefValue()
        {
            int eol = FindEndOfExprLine(0);
            if (eol < 1)
            {
                return null;
            }

            string rest = Contents.ToString().Substring(0, eol);
            Contents.Remove(0, eol);
            return rest;
        }

        /**
         * Discards all data up to a certain character, ignoring
         * the contents, unless specified to stop at a doc block comment.
         */
        private void DiscardUntilCharOrComment(char c, bool stop_at_comment)
        {
            for (int i = 0; i < Contents.Length; i++)
            {
                if (Contents[i] == '\n')
                {
                    LineNo++;
                }
                else if (Contents[i] == c)
                {
                    if (i > 0)
                    {
                        Contents.Remove(0, i);
                    }
                    return;
                }
                else if (stop_at_comment && CheckString(i, "/**"))
                {
                    if (i > 0)
                    {
                        Contents.Remove(0, i);
                    }
                    return;
                }
            }
        }

       /**
        * Discards all data up to a certain character, ignoring
        * the contents, unless specified to stop at a doc block comment.
        */
        private void DiscardUntilCharOrComment(string s, bool stop_at_comment)
        {
          for (int i = 0; i < Contents.Length; i++)
          {
            if (Contents[i] == '\n')
            {
              LineNo++;
            }
            else if (s.Contains(Contents[i].ToString()))
            {
              if (i > 0)
              {
                Contents.Remove(0, i);
              }
              return;
            }
            else if (stop_at_comment && CheckString(i, "/**"))
            {
              if (i > 0)
              {
                Contents.Remove(0, i);
              }
              return;
            }
          }
        }

        /**
         * Finds the end of an expression
         */
        private int FindEndOfExprLine(int start)
        {
            for (int i = start; i < Contents.Length; i++)
            {
                if (Contents[i] == '{')
                {
                    if ((i = FindEndOfConsumptionLine('}', start+1)) == -1)
                    {
                        throw new ParseException("Expected token: } (found none)");
                    }
                }
                else if (Contents[i] == ')' || Contents[i] == ',')
                {
                    return i;
                }
            }

            return -1;
        }

        private int FindEndOfConsumptionLine(char c, int start)
        {
            for (int i = start; i < Contents.Length; i++)
            {
                if (Contents[i] == '{')
                {
                    if ((i = FindEndOfConsumptionLine('}', i + 1)) == -1)
                    {
                        throw new ParseException("Expected token: } (found none)");
                    }
                }
                else if (Contents[i] == c)
                {
                    return i;
                }
            }

            return -1;
        }

        private void Initialize()
        {
            /* Open file, read it, and close it */
            StreamReader sr = File.OpenText(FileName);
            Contents = new StringBuilder(sr.ReadToEnd());
            sr.Close();

            /* Strip comments (may throw an exception) */
            preprocess();

            /* Lastly, initialize the tokenizer */
            Tokenizer.Initialize();
        }

        /* Consumes a token iff it matches */
        private bool MatchToken(LexToken token)
        {
            LexToken tok = lex();
            if (tok != token)
            {
                lexpush();
                return false;
            }
            return true;
        }

        /* Consumes a token and aborts the parser if there is no match */
        private void NeedToken(LexToken token)
        {
            if (!MatchToken(token))
            {
                throw new ParseException("Expected token: " + token);
            }
        }

        /* Consumes a character iff it matches */
        private bool MatchChar(char c)
        {
            if (lex() != LexToken.TOKEN_CHARACTER)
            {
                lexpush();
                return false;
            }
            if (_LexChar != c)
            {
                lexpush();
                return false;
            }
            return true;
        }

        /* Consumes a character or aborts the parser if there is no match */
        private void NeedChar(char c)
        {
            if (!MatchChar(c))
            {
                throw new ParseException("Expected character: " + c);
            }
        }
        
        /* Saves the lexer state so we don't reparse unnecessarily */
        private void lexpush()
        {
            if (LexPushed)
            {
                throw new ParseException("Lexer cannot be pushed twice");
            }
            LexPushed = true;
        }

        /* Attempts to match a string against the next stream tokens.
         * On success, the token stream is advanced.
         */
        private bool MatchString(int startPos, string str)
        {
            if (str.Length > Contents.Length - startPos)
            {
                return false;
            }
            for (int i = 0; i < str.Length; i++)
            {
                if (str[i] != Contents[startPos + i])
                {
                    return false;
                }
            }
            Contents.Remove(startPos, str.Length);
            return true;
        }

        /* Attempts to match a string against the next stream tokens */
        private bool CheckString(int startPos, string str)
        {
            if (str.Length > Contents.Length - startPos)
            {
                return false;
            }
            for (int i = 0; i < str.Length; i++)
            {
                if (str[i] != Contents[startPos + i])
                {
                    return false;
                }
            }
            return true;
        }

        /* Ignores a block starting and ending with { and } respectively */
        private void ignore_block()
        {
            /* Hrm, this is tricky.  First, expect an open brace. */
            NeedChar('{');
            /* Consume the entire function by keeping track of braces */
            int numBraces = 1;
            bool in_string = false;
            bool found_end = false;
            for (int i = 0; i < Contents.Length; i++)
            {
                if (Contents[i] == '\n')
                {
                    LineNo++;
                }
                if (!in_string)
                {
                    if (Contents[i] == '"')
                    {
                        in_string = true;
                    }
                    else if (Contents[i] == '{')
                    {
                        numBraces++;
                    }
                    else if (Contents[i] == '}')
                    {
                        if (numBraces == 0)
                        {
                            throw new ParseException("Unable to find a matching open brace");
                        }
                        if (--numBraces == 0)
                        {
                            Contents.Remove(0, i + 1);
                            found_end = true;
                            break;
                        }
                    }
                }
                else
                {
                    /* It is possible to be in a string and at the first character */
                    if (Contents[i] == '"' && Contents[i - 1] != '\\')
                    {
                        in_string = false;
                    }
                }
            }
            if (!found_end)
            {
                throw new ParseException("Unable to find a matching close brace");
            }
        }

        /* Strips all non-doc C and C++ style comments. as well as unused pragmas */
        private void preprocess()
        {
            if (Contents.Length == 1)
            {
                return;
            }

            bool in_comment = false;
            bool is_ml = false;
            bool is_doc = false;
            bool is_newline = true;

            for (int i=0; i<Contents.Length; i++)
            {
                if (Contents[i] == '\n')
                {
                    is_newline = true;
                }
                if (!in_comment)
                {
                    if (Contents[i] == '/')
                    {
                        if (Contents[i+1] == '/')
                        {
                            in_comment = true;
                            is_ml = false;
                            is_doc = false;
                            is_newline = false;
                        } else if (Contents[i+1] == '*') {
                            /* Detect whether this is a doc comment or not */
                            if (Contents.Length - i >= 3
                                && Contents[i+2] == '*')
                            {
                                is_doc = true;
                            } else {
                                is_doc = false;
                            }
                            in_comment = true;
                            is_ml = true;
                        }
                        if (in_comment && !is_doc)
                        {
                            Contents[i] = ' ';
                            Contents[i+1] = ' ';        /* Just for safety */
                        }
                    }
                    else if (is_newline)
                    {
                        if (Contents[i] == '#')
                        {
                            if (!CheckString(i, "#define")
                                && !CheckString(i, "#include"))
                            {
                                /* Ignore! */
                                Contents[i] = ' ';
                                in_comment = true;
                                is_doc = false;
                                is_ml = false;
                            }
                            is_newline = false;
                        }
                        else if (!Char.IsWhiteSpace(Contents[i]))
                        {
                            is_newline = false;
                        }
                    }
                } else {
                    if (is_ml)
                    {
                        if (Contents[i] == '*' && Contents[i+1] == '/')
                        {
                            if (!is_doc)
                            {
                                Contents[i] = ' ';
                                Contents[i+1] = ' ';
                            }
                            in_comment = false;
                            i++;    /* Don't look at the next character */
                        }
                        else if (!is_doc && Contents[i] != '\n')
                        {
                            /* Erase comment */
                            Contents[i] = ' ';
                        }
                    } else {
                        if (is_newline)
                        {
                            in_comment = false;
                        }
                        else
                        {
                            /* Erase comment */
                            Contents[i] = ' ';
                        }
                    }
                }
            }

            if (in_comment)
            {
                throw new ParseException("Multi-line comment has no terminator");
            }
        }

        /* Strips whitespace */
        private void trim_whitespace_pre()
        {
            int spaces = 0;
            while (spaces < Contents.Length
                   && Contents[spaces] != '\0'
                   && Char.IsWhiteSpace(Contents[spaces]))
            {
                if (Contents[spaces] == '\n')
                {
                    LineNo++;
                }
                spaces++;
            }
            if (spaces > 0)
            {
                Contents.Remove(0, spaces);
            }
        }

        private void preprocess_line()
        {
            trim_whitespace_pre();
        }

        private bool IsSymbolChar(char c, bool first)
        {
            bool valid = (first ? Char.IsLetter(c) : Char.IsLetterOrDigit(c));
            return (valid || (c == '_'));
        }

        private bool IsEscapedChar(char c)
        {
            return (c == 'n'
                    || c == 'r'
                    || c == 't'
                    || c == 'v'
                    || c == '\\'
                    || c == '"');
        }

        private bool IsNotKeywordChar(char c)
        {
            return (Char.IsWhiteSpace(c)
                    || (c == '-' || c == '+')
                    || (c == '/' || c == '*')
                    || (c == '(' || c == ')')
                    || (c == '=' || c == '%')
                    || (c == '>' || c == '<')
                    || (c == '!' || c == '~')
                    );
        }

        /* Consumes one lexical token and gathers information about it */
        private LexToken lex()
        {
            if (LexPushed)
            {
                LexPushed = false;
                return _LastToken;
            }

            /* Number of chars we will be deleting from the input */
            int stripchars = 0;

            /* Clear our state */
            _LastToken = LexToken.TOKEN_NONE;

            /* Remove stuff we don't want from the line */
            preprocess_line();

            if (Contents.Length < 1)
            {
                _LastToken = LexToken.TOKEN_EOF;
                return _LastToken;
            }
            
            /* Get the token list */
            KeywordToken[] tokens = Tokenizer.Tokens;
            for (int i = 0; i < tokens.Length; i++)
            {
                if (!CheckString(0, tokens[i].keyword))
                {
                    continue;
                }
                int len = tokens[i].keyword.Length;
                /* Now check to see what the next token is */
                if (Contents.Length == len
                    || IsNotKeywordChar(Contents[len]))
                {
                    /* We have a token match! */
                    _LastToken = tokens[i].token;
                    _LexString = null;
                    _LexChar = '\0';
                    stripchars = len;
                    break;
                }
            }

            if (_LastToken == LexToken.TOKEN_NONE)
            {
                /* See if we can try to read a symbol */
                if (IsSymbolChar(Contents[0], true))
                {
                    int characters = 1;
                    while (Contents[characters] != '\0'
                           && IsSymbolChar(Contents[characters], false))
                    {
                        characters++;
                    }
                    stripchars = characters;
                    /* We're done! See what's next.. */
                    if (Contents[characters] == ':')
                    {
                        _LastToken = LexToken.TOKEN_LABEL;
                        stripchars++;
                    }
                    else
                    {
                        _LastToken = LexToken.TOKEN_SYMBOL;
                    }
                    _LexString = Contents.ToString().Substring(0, characters);
                    _LexChar = _LexString[0];
                }/*
                else if (Contents[0] == '"')
                {
                    bool reached_end = false;
                    StringBuilder builder = new StringBuilder();
                    for (int i = 1; i < Contents.Length; i++)
                    {
                        if (i < Contents.Length - 1
                            && Contents[i] == '\\'
                            && IsEscapedChar(Contents[i+1]))
                        {
                            i++;
                        }
                        else if (Contents[i] == '"')
                        {
                            reached_end = true;
                            break;
                        }
                        builder.Append(Contents[i]);
                    }
                    if (!reached_end)
                    {
                        throw new ParseException("Expected end of string; none found");
                    }
                    _LexString = builder.ToString();
                    _LexChar = _LexString.Length > 0 ? _LexString[0] : '\0';
                }*/
                else if (Contents[0] == '\'')
                {
                    char c = '\0';
                    if (Contents.Length < 3)
                    {
                        throw new ParseException("Expected end of character; none found");
                    }
                    else if (Contents[1] == '\\')
                    {
                        if (Contents.Length < 4 || !IsEscapedChar(Contents[2]))
                        {
                            throw new ParseException("Expected end of character; none found");
                        }
                        if (Contents[2] == 'r')
                        {
                            c = '\r';
                        }
                        else if (Contents[2] == 'n')
                        {
                            c = '\n';
                        }
                        else if (Contents[2] == 't')
                        {
                            c = '\t';
                        }
                        else if (Contents[2] == 'v')
                        {
                            c = '\v';
                        }
                        else if (Contents[2] == '\\')
                        {
                            c = '\\';
                        }
                        else if (Contents[2] == '"')
                        {
                            c = '"';
                        }
                        stripchars = 4;
                    }
                    else
                    {
                        c = Contents[1];
                        stripchars = 3;
                    }
                    _LexString = c.ToString();
                    _LexChar = c;
                    _LastToken = LexToken.TOKEN_QUOTCHAR;
                }
                else if (CheckString(0, "/**"))
                {
                    int endpoint = 0;
                    for (int i=3; i<Contents.Length-1; i++)
                    {
                        if (Contents[i] == '\n')
                        {
                            LineNo++;
                        }
                        else if (Contents[i] == '*' && Contents[i+1] == '/')
                        {
                            endpoint = i+1;
                            break;
                        }
                    }
                    if (endpoint == 0)
                    {
                        throw new ParseException("Expected end of multi-line comment; found end of file");
                    }
                    /* We have to factor one more character in because we're zero based */
                    endpoint++;
                    _LastToken = LexToken.TOKEN_DOCBLOCK;
                    _LexString = Contents.ToString().Substring(0, endpoint);
                    _LexChar = _LexString[0];
                    stripchars = endpoint;
                }
                else
                {
                    _LastToken = LexToken.TOKEN_CHARACTER;
                    _LexString = Contents[0].ToString();
                    _LexChar = Contents[0];
                    stripchars = 1;
                }
            }

            /* Strip N chars */
            if (stripchars > 0)
            {
                Contents.Remove(0, stripchars);
            }

            return _LastToken;
        }
    }
}
