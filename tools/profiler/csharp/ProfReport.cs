using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;

namespace profviewer
{
    enum ProfileType : int
    {
        ProfType_Unknown = 0,
        ProfType_Native,
        ProfType_Callback,
        ProfType_Function
    }

    class ProfileItem
    {
        public string name;
        public double total_time;
        public uint num_calls;
        public double min_time;
        public double max_time;
        public ProfileType type;

        public double AverageTime
        {
            get
            {
                return total_time / num_calls;
            }
        }
    }

    class ProfileReport
    {
        public static string[] TypeStrings;
        private DateTime m_start_time;
        private double m_duration;
        private List<ProfileItem> m_Items;

        public int Count
        {
            get
            {
                return m_Items.Count;
            }
        }

        static ProfileReport()
        {
            TypeStrings = new string[4];

            TypeStrings[0] = "unknown";
            TypeStrings[1] = "native";
            TypeStrings[2] = "callback";
            TypeStrings[3] = "function";
        }

        public ProfileReport(string file)
        {
            bool in_profile;
            ProfileType type;
            string cur_report;
            XmlTextReader xml;

            xml = new XmlTextReader(file);
            xml.WhitespaceHandling = WhitespaceHandling.None;

            m_Items = new List<ProfileItem>();

            type = ProfileType.ProfType_Unknown;
            in_profile = false;
            cur_report = null;
            while (xml.Read())
            {
                if (xml.NodeType == XmlNodeType.Element)
                {
                    if (xml.Name.CompareTo("profile") == 0)
                    {
                        int t;
                        in_profile = true;
                        m_duration = Double.Parse(xml.GetAttribute("uptime"));
                        m_start_time = new DateTime(1970, 1, 1, 0, 0, 0);
                        t = Int32.Parse(xml.GetAttribute("time"));
                        m_start_time = m_start_time.AddSeconds(t);
                    }
                    else if (in_profile)
                    {
                        if (xml.Name.CompareTo("report") == 0)
                        {
                            cur_report = xml.GetAttribute("name");
                            if (cur_report.CompareTo("natives") == 0)
                            {
                                type = ProfileType.ProfType_Native;
                            }
                            else if (cur_report.CompareTo("callbacks") == 0)
                            {
                                type = ProfileType.ProfType_Callback;
                            }
                            else if (cur_report.CompareTo("functions") == 0)
                            {
                                type = ProfileType.ProfType_Function;
                            }
                            else
                            {
                                type = ProfileType.ProfType_Unknown;
                            }
                        }
                        else if (xml.Name.CompareTo("item") == 0 && cur_report != null)
                        {
                            ProfileItem item;

                            item = new ProfileItem();
                            item.name = xml.GetAttribute("name");
                            item.max_time = Double.Parse(xml.GetAttribute("maxtime"));
                            item.min_time = Double.Parse(xml.GetAttribute("mintime"));
                            item.num_calls = UInt32.Parse(xml.GetAttribute("numcalls"));
                            item.total_time = Double.Parse(xml.GetAttribute("totaltime"));
                            item.type = type;
                            m_Items.Add(item);
                        }
                    }
                }
                else if (xml.NodeType == XmlNodeType.EndElement)
                {
                    if (xml.Name.CompareTo("profile") == 0)
                    {
                        break;
                    }
                    else if (xml.Name.CompareTo("report") == 0)
                    {
                        cur_report = null;
                    }
                }
            }
            xml.Close();
        }

        public double Duration
        {
            get
            {
                return m_duration;
            }
        }

        public DateTime StartTime
        {
            get
            {
                return m_start_time;
            }
        }

        public ProfileItem GetItem(int i)
        {
            return m_Items[i];
        }
    }
}
