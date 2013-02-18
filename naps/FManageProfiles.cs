/*
    NAPS2 (Not Another PDF Scanner 2)
    http://sourceforge.net/projects/naps2/
    
    Copyright (C) 2009        Pavel Sorejs
    Copyright (C) 2012, 2013  Ben Olden-Cooligan

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace NAPS2
{
    public partial class FManageProfiles : Form
    {
        private List<CScanSettings> profiles;

        public FManageProfiles()
        {
            InitializeComponent();
        }

        private void btnOK_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        private void loadList()
        {
            lvProfiles.Items.Clear();
            foreach (CScanSettings profile in profiles)
            {
                lvProfiles.Items.Add(profile.DisplayName, profile.IconID);
            }

        }

        private void btnAdd_Click(object sender, EventArgs e)
        {
            FEditScanSettings fedit = new FEditScanSettings();
            fedit.ScanSettings = new CScanSettings();
            fedit.ShowDialog();
            if (fedit.Result)
            {
                this.profiles.Add(fedit.ScanSettings);
                loadList();
                CSettings.SaveProfiles(profiles);
            }
        }

        private void btnEdit_Click(object sender, EventArgs e)
        {
            if (lvProfiles.SelectedItems.Count > 0)
            {
                int profileIndex = lvProfiles.SelectedItems[0].Index;
                FEditScanSettings fedit = new FEditScanSettings();
                fedit.ScanSettings = profiles[profileIndex];
                fedit.ShowDialog();
                if (fedit.Result)
                {
                    loadList();
                    lvProfiles.SelectedIndices.Add(profileIndex);
                    CSettings.SaveProfiles(profiles);
                }
            }
        }

        private void lvProfiles_SelectedIndexChanged(object sender, EventArgs e)
        {
            btnEdit.Enabled = lvProfiles.SelectedItems.Count > 0;
            btnDelete.Enabled = lvProfiles.SelectedItems.Count > 0;
        }

        private void FManageProfiles_Load(object sender, EventArgs e)
        {
            lvProfiles.LargeImageList = ilProfileIcons.IconsList;
            btnEdit.Enabled = false;
            btnDelete.Enabled = false;
            this.profiles = CSettings.LoadProfiles();
            loadList();
        }

        private void btnDelete_Click(object sender, EventArgs e)
        {
            if (lvProfiles.SelectedItems.Count > 0)
            {
                if (MessageBox.Show("Do you really want to delete " + profiles[lvProfiles.SelectedItems[0].Index].DisplayName + "?", "Delete", MessageBoxButtons.YesNo, MessageBoxIcon.Exclamation) == DialogResult.Yes)
                {
                    profiles.RemoveAt(lvProfiles.SelectedItems[0].Index);
                    loadList();
                    CSettings.SaveProfiles(profiles);
                    lvProfiles_SelectedIndexChanged(null, null);
                }
            }
        }

        private void lvProfiles_ItemActivate(object sender, EventArgs e)
        {
            btnEdit_Click(null, null);
        }

        private void lvProfiles_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyData == Keys.Delete && btnDelete.Enabled)
            {
                btnDelete_Click(null, null);
            }
        }

    }
}
