using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace profviewer
{
    public partial class Main : Form
    {
        private ProfileReport m_Report;

        public Main()
        {
            InitializeComponent();
        }

        private void menu_file_open_Click(object sender, EventArgs e)
        {
            DialogResult res;
            
            res = dialog_open.ShowDialog(this);

            if (res != DialogResult.OK)
            {
                return;
            }

            m_Report = null;

            try
            {
                m_Report = new ProfileReport(dialog_open.FileName);
            }
            catch (System.Exception ex)
            {
                MessageBox.Show("Error opening or parsing file: " + ex.Message);
            }

            UpdateListView();
        }

        private void UpdateListView()
        {
            ProfileItem atom;
            ListViewItem item;

            report_list.Items.Clear();
            report_info_duration.Text = "";
            report_info_starttime.Text = "";

            report_info_duration.Text = m_Report.Duration.ToString() + " seconds";
            report_info_starttime.Text = m_Report.StartTime.ToString();

            for (int i = 0; i < m_Report.Count; i++)
            {
                atom = m_Report.GetItem(i);
                item = new ListViewItem(ProfileReport.TypeStrings[(int)atom.type]);

                item.SubItems.Add(atom.name);
                item.SubItems.Add(atom.num_calls.ToString());
                item.SubItems.Add(atom.AverageTime.ToString("F6"));
                item.SubItems.Add(atom.min_time.ToString("F6"));
                item.SubItems.Add(atom.max_time.ToString("F6"));
                item.SubItems.Add(atom.total_time.ToString("F6"));

                report_list.Items.Add(item);
            }
        }

        private void menu_file_exit_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        private void report_list_ColumnClick(object sender, ColumnClickEventArgs e)
        {
            if (e.Column == 1)
            {
                report_list.ListViewItemSorter = new LIStringComparator(1);
            }
            else if (e.Column == 2)
            {
                report_list.ListViewItemSorter = new LIIntComparator(2);
            }
            else if (e.Column > 2)
            {
                report_list.ListViewItemSorter = new LIDoubleComparator(e.Column);
            }
        }
    }
}