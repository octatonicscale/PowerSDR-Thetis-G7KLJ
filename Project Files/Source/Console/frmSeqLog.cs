﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Diagnostics;

namespace Thetis
{
    public partial class frmSeqLog : Form
    {
        private int m_nTabMainTop;

        public delegate void ClearButtonHandler();
        private event ClearButtonHandler clearButtonEvents;

        public frmSeqLog()
        {
            InitializeComponent();

            m_nTabMainTop = tabMain.Top;

            btnClear.Text = "Close";
            btnCopyImageToClipboard.Enabled = false;
            btnCopyToClipboard.Enabled = false;

            tabMain.SelectedIndex = 0;
        }

        public void InitAndShow()
        {
            tabMain.SelectedIndex = 0;

            this.Show();
        }

        public void SetWireSharkPath(string sPath)
        {
            setupControlsDumpCap(sPath);
        }

        private void btnClear_Click(object sender, EventArgs e)
        {
            btnClear.Text = "Close";
            btnCopyImageToClipboard.Enabled = false;
            btnCopyToClipboard.Enabled = false;
            txtLog.Text = "";
            log.Clear();
            clearButtonEvents();
            this.Close();
        }

        private StringBuilder log = new StringBuilder();
        public void LogString(string s)
        {
            log.Insert(0, s + System.Environment.NewLine);
            if (log.Length > 16000) log.Remove(16000, log.Length - 16000);

            txtLog.Text = log.ToString();

            btnClear.Text = "Clear Log + Close";
            btnCopyImageToClipboard.Enabled = true;
            btnCopyToClipboard.Enabled = true;
        }

        private void frmSeqLog_FormClosing(object sender, FormClosingEventArgs e)
        {
            this.Hide();
            e.Cancel = true;
        }

        public event ClearButtonHandler ClearButtonEvent
        {
            add
            {
                clearButtonEvents += value;
            }
            remove
            {
                clearButtonEvents -= value;
            }
        }

        private void btnCopyToClipboard_Click(object sender, EventArgs e)
        {
            Clipboard.SetText(txtLog.Text);
        }

        private void btnCopyImageToClipboard_Click(object sender, EventArgs e)
        {
            using (Bitmap bmp = new Bitmap(txtLog.Width, txtLog.Height))
            {
                txtLog.DrawToBitmap(bmp, new Rectangle(new Point(0, 0), txtLog.Size));
                Clipboard.SetImage(bmp);
            }
        }

        private void btnSetWireSharkFolder_Click(object sender, EventArgs e)
        {
            using (FolderBrowserDialog fbd = new FolderBrowserDialog())
            {
                DialogResult result = fbd.ShowDialog();

                if (result == DialogResult.OK && !string.IsNullOrWhiteSpace(fbd.SelectedPath))
                {
                    setupControlsDumpCap(fbd.SelectedPath);
                }
            }
        }

        private void setupControlsDumpCap(string sPath)
        {
            txtWireSharkFolder.Text = sPath;
            DumpCap.WireSharkPath = sPath;

            bool bExists = DumpCap.DumpCapExists();

            groupDumpCap.Enabled = bExists;
            if (bExists) groupDumpCap.Text = "DumpCap [FOUND]";
            else groupDumpCap.Text = "DumpCap [NOT FOUND]";

            // default always off
            chkDumpCapEnabled.Checked = false;
            DumpCap.Enabled = false;

            udInterface.Value = DumpCap.Interface;
            chkKillOnNegativeOnly.Checked = DumpCap.KillOnNegativeSeqOnly;
            chkClearRingBufferFolderOnRestart.Checked = DumpCap.ClearFolderOnRestart;
        }

        private void udInterface_ValueChanged(object sender, EventArgs e)
        {
            DumpCap.Interface = (int)udInterface.Value;
        }

        private void chkKillOnNegativeOnly_CheckedChanged(object sender, EventArgs e)
        {
            DumpCap.KillOnNegativeSeqOnly = chkKillOnNegativeOnly.Checked;
        }

        private void chkDumpCapEnabled_CheckedChanged(object sender, EventArgs e)
        {
            DumpCap.Enabled = chkDumpCapEnabled.Checked;
        }

        private void chkClearRingBufferFolderOnRestart_CheckedChanged(object sender, EventArgs e)
        {
            DumpCap.ClearFolderOnRestart = chkClearRingBufferFolderOnRestart.Checked;
        }

        private void btnShowDumpCapFolder_Click(object sender, EventArgs e)
        {
            DumpCap.ShowAppPathFolder();
        }

        private void chkStatusBarWarningNegativeOnly_CheckedChanged(object sender, EventArgs e)
        {

        }

        public bool StatusBarWarningOnNegativeOnly
        {
            get { return chkStatusBarWarningNegativeOnly.Checked; }
            set { chkStatusBarWarningNegativeOnly.Checked = value; }
        }
    }
}
