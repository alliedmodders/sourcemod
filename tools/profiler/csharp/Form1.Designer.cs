namespace profviewer
{
    partial class Main
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.report_list = new System.Windows.Forms.ListView();
            this.pr_type = new System.Windows.Forms.ColumnHeader();
            this.pr_name = new System.Windows.Forms.ColumnHeader();
            this.pr_calls = new System.Windows.Forms.ColumnHeader();
            this.pr_avg_time = new System.Windows.Forms.ColumnHeader();
            this.pr_min_time = new System.Windows.Forms.ColumnHeader();
            this.pr_max_time = new System.Windows.Forms.ColumnHeader();
            this.pr_total_time = new System.Windows.Forms.ColumnHeader();
            this.menuStrip1 = new System.Windows.Forms.MenuStrip();
            this.fileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.menu_file_open = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripMenuItem1 = new System.Windows.Forms.ToolStripSeparator();
            this.menu_file_exit = new System.Windows.Forms.ToolStripMenuItem();
            this.label1 = new System.Windows.Forms.Label();
            this.report_info_starttime = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.report_info_duration = new System.Windows.Forms.Label();
            this.dialog_open = new System.Windows.Forms.OpenFileDialog();
            this.panel1 = new System.Windows.Forms.Panel();
            this.menuStrip1.SuspendLayout();
            this.panel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // report_list
            // 
            this.report_list.AllowColumnReorder = true;
            this.report_list.AutoArrange = false;
            this.report_list.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.pr_type,
            this.pr_name,
            this.pr_calls,
            this.pr_avg_time,
            this.pr_min_time,
            this.pr_max_time,
            this.pr_total_time});
            this.report_list.Dock = System.Windows.Forms.DockStyle.Fill;
            this.report_list.Location = new System.Drawing.Point(0, 24);
            this.report_list.MultiSelect = false;
            this.report_list.Name = "report_list";
            this.report_list.Size = new System.Drawing.Size(759, 300);
            this.report_list.TabIndex = 0;
            this.report_list.UseCompatibleStateImageBehavior = false;
            this.report_list.View = System.Windows.Forms.View.Details;
            this.report_list.ColumnClick += new System.Windows.Forms.ColumnClickEventHandler(this.report_list_ColumnClick);
            // 
            // pr_type
            // 
            this.pr_type.Text = "Type";
            this.pr_type.Width = 71;
            // 
            // pr_name
            // 
            this.pr_name.Text = "Name";
            this.pr_name.Width = 270;
            // 
            // pr_calls
            // 
            this.pr_calls.Text = "Calls";
            this.pr_calls.Width = 61;
            // 
            // pr_avg_time
            // 
            this.pr_avg_time.Text = "Avg Time";
            this.pr_avg_time.Width = 74;
            // 
            // pr_min_time
            // 
            this.pr_min_time.Text = "Min Time";
            this.pr_min_time.Width = 78;
            // 
            // pr_max_time
            // 
            this.pr_max_time.Text = "Max Time";
            this.pr_max_time.Width = 77;
            // 
            // pr_total_time
            // 
            this.pr_total_time.Text = "Total Time";
            this.pr_total_time.Width = 84;
            // 
            // menuStrip1
            // 
            this.menuStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.fileToolStripMenuItem});
            this.menuStrip1.Location = new System.Drawing.Point(0, 0);
            this.menuStrip1.Name = "menuStrip1";
            this.menuStrip1.Size = new System.Drawing.Size(759, 24);
            this.menuStrip1.TabIndex = 1;
            this.menuStrip1.Text = "menuStrip1";
            // 
            // fileToolStripMenuItem
            // 
            this.fileToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.menu_file_open,
            this.toolStripMenuItem1,
            this.menu_file_exit});
            this.fileToolStripMenuItem.Name = "fileToolStripMenuItem";
            this.fileToolStripMenuItem.Size = new System.Drawing.Size(35, 20);
            this.fileToolStripMenuItem.Text = "&File";
            // 
            // menu_file_open
            // 
            this.menu_file_open.Name = "menu_file_open";
            this.menu_file_open.Size = new System.Drawing.Size(100, 22);
            this.menu_file_open.Text = "&Open";
            this.menu_file_open.Click += new System.EventHandler(this.menu_file_open_Click);
            // 
            // toolStripMenuItem1
            // 
            this.toolStripMenuItem1.Name = "toolStripMenuItem1";
            this.toolStripMenuItem1.Size = new System.Drawing.Size(97, 6);
            // 
            // menu_file_exit
            // 
            this.menu_file_exit.Name = "menu_file_exit";
            this.menu_file_exit.Size = new System.Drawing.Size(100, 22);
            this.menu_file_exit.Text = "E&xit";
            this.menu_file_exit.Click += new System.EventHandler(this.menu_file_exit_Click);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(3, 13);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(76, 13);
            this.label1.TabIndex = 2;
            this.label1.Text = "Profile Started:";
            // 
            // report_info_starttime
            // 
            this.report_info_starttime.AutoSize = true;
            this.report_info_starttime.Location = new System.Drawing.Point(79, 13);
            this.report_info_starttime.Name = "report_info_starttime";
            this.report_info_starttime.Size = new System.Drawing.Size(0, 13);
            this.report_info_starttime.TabIndex = 3;
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(264, 13);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(82, 13);
            this.label2.TabIndex = 4;
            this.label2.Text = "Profile Duration:";
            // 
            // report_info_duration
            // 
            this.report_info_duration.AutoSize = true;
            this.report_info_duration.Location = new System.Drawing.Point(346, 13);
            this.report_info_duration.Name = "report_info_duration";
            this.report_info_duration.Size = new System.Drawing.Size(0, 13);
            this.report_info_duration.TabIndex = 5;
            // 
            // dialog_open
            // 
            this.dialog_open.Filter = "Profiler files|*.xml|All files|*.*";
            // 
            // panel1
            // 
            this.panel1.Controls.Add(this.label1);
            this.panel1.Controls.Add(this.report_info_duration);
            this.panel1.Controls.Add(this.report_info_starttime);
            this.panel1.Controls.Add(this.label2);
            this.panel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.panel1.Location = new System.Drawing.Point(0, 324);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(759, 33);
            this.panel1.TabIndex = 6;
            // 
            // Main
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(759, 357);
            this.Controls.Add(this.report_list);
            this.Controls.Add(this.menuStrip1);
            this.Controls.Add(this.panel1);
            this.MainMenuStrip = this.menuStrip1;
            this.Name = "Main";
            this.Text = "SourceMod Profiler Report Viewer";
            this.menuStrip1.ResumeLayout(false);
            this.menuStrip1.PerformLayout();
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.ListView report_list;
        private System.Windows.Forms.ColumnHeader pr_type;
        private System.Windows.Forms.ColumnHeader pr_name;
        private System.Windows.Forms.ColumnHeader pr_calls;
        private System.Windows.Forms.ColumnHeader pr_avg_time;
        private System.Windows.Forms.ColumnHeader pr_min_time;
        private System.Windows.Forms.ColumnHeader pr_max_time;
        private System.Windows.Forms.ColumnHeader pr_total_time;
        private System.Windows.Forms.MenuStrip menuStrip1;
        private System.Windows.Forms.ToolStripMenuItem fileToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem menu_file_open;
        private System.Windows.Forms.ToolStripSeparator toolStripMenuItem1;
        private System.Windows.Forms.ToolStripMenuItem menu_file_exit;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label report_info_starttime;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label report_info_duration;
        private System.Windows.Forms.OpenFileDialog dialog_open;
        private System.Windows.Forms.Panel panel1;
    }
}

