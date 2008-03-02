using System;
using System.Collections;
using System.Text;
using System.Windows.Forms;

namespace profviewer
{
    class LIStringComparator : IComparer
    {
        private int m_col;

        public LIStringComparator(int col)
        {
            m_col = col;
        }

        public int Compare(object x, object y)
        {
            ListViewItem a = (ListViewItem)x;
            ListViewItem b = (ListViewItem)y;

            return String.Compare(a.SubItems[m_col].Text, b.SubItems[m_col].Text);
        }
    }

    class LIIntComparator : IComparer
    {
        private int m_col;

        public LIIntComparator(int col)
        {
            m_col = col;
        }

        public int Compare(object x, object y)
        {
            ListViewItem a = (ListViewItem)x;
            ListViewItem b = (ListViewItem)y;

            int num1 = Int32.Parse(a.SubItems[m_col].Text);
            int num2 = Int32.Parse(b.SubItems[m_col].Text);

            if (num1 > num2)
            {
                return -1;
            }
            else if (num1 < num2)
            {
                return 1;
            }

            return 0;
        }
    }

    class LIDoubleComparator : IComparer
    {
        private int m_col;

        public LIDoubleComparator(int col)
        {
            m_col = col;
        }

        public int Compare(object x, object y)
        {
            ListViewItem a = (ListViewItem)x;
            ListViewItem b = (ListViewItem)y;

            double num1 = Double.Parse(a.SubItems[m_col].Text);
            double num2 = Double.Parse(b.SubItems[m_col].Text);

            if (num1 > num2)
            {
                return -1;
            }
            else if (num1 < num2)
            {
                return 1;
            }

            return 0;
        }
    }
}
