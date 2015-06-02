namespace OSD
{
    partial class PlayuavOSD
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
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(PlayuavOSD));
            this.Params = new BrightIdeasSoftware.DataTreeListView();
            this.olvColumn1 = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
            this.olvColumn2 = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
            this.olvColumn3 = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
            this.olvColumn4 = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
            this.olvColumn5 = ((BrightIdeasSoftware.OLVColumn)(new BrightIdeasSoftware.OLVColumn()));
            this.CMB_ComPort = new System.Windows.Forms.ComboBox();
            this.btn_up_fw = new System.Windows.Forms.Button();
            this.progress = new System.Windows.Forms.ProgressBar();
            this.lbl_status = new System.Windows.Forms.Label();
            this.Load_from_OSD = new System.Windows.Forms.Button();
            this.Save_To_OSD = new System.Windows.Forms.Button();
            this.Sav_To_EEPROM = new System.Windows.Forms.Button();
            this.menuStrip1 = new System.Windows.Forms.MenuStrip();
            this.fileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.saveOSDFileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.openOSDFileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this.loadDefaultsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            this.exitToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.optionsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.languageToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.englishToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.chineseToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.helpToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.gettingStartedToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.checkUpdatesToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.aboutToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.glControl1 = new OpenTK.GLControl();
            this.labPanle = new System.Windows.Forms.Label();
            this.lab_inc = new System.Windows.Forms.Label();
            this.lab_dec = new System.Windows.Forms.Label();
            this.lbl_port = new System.Windows.Forms.Label();
            this.lbl_fc = new System.Windows.Forms.Label();
            this.cbx_fc = new System.Windows.Forms.ComboBox();
            this.timer1 = new System.Windows.Forms.Timer(this.components);
            this.loadCustomFirmwareToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            ((System.ComponentModel.ISupportInitialize)(this.Params)).BeginInit();
            this.menuStrip1.SuspendLayout();
            this.SuspendLayout();
            // 
            // Params
            // 
            this.Params.AllColumns.Add(this.olvColumn1);
            this.Params.AllColumns.Add(this.olvColumn2);
            this.Params.AllColumns.Add(this.olvColumn3);
            this.Params.AllColumns.Add(this.olvColumn4);
            this.Params.AllColumns.Add(this.olvColumn5);
            this.Params.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.Params.BackColor = System.Drawing.SystemColors.Desktop;
            this.Params.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.olvColumn1,
            this.olvColumn2,
            this.olvColumn3,
            this.olvColumn4,
            this.olvColumn5});
            this.Params.DataSource = null;
            this.Params.ForeColor = System.Drawing.SystemColors.ControlText;
            this.Params.Location = new System.Drawing.Point(3, 27);
            this.Params.Name = "Params";
            this.Params.OwnerDraw = true;
            this.Params.RootKeyValueString = "";
            this.Params.RowHeight = 26;
            this.Params.ShowGroups = false;
            this.Params.Size = new System.Drawing.Size(500, 426);
            this.Params.TabIndex = 80;
            this.Params.UseAlternatingBackColors = true;
            this.Params.UseCompatibleStateImageBehavior = false;
            this.Params.View = System.Windows.Forms.View.Details;
            this.Params.VirtualMode = true;
            this.Params.CellEditFinishing += new BrightIdeasSoftware.CellEditEventHandler(this.Params_CellEditFinishing);
            this.Params.FormatRow += new System.EventHandler<BrightIdeasSoftware.FormatRowEventArgs>(this.Params_FormatRow);
            // 
            // olvColumn1
            // 
            this.olvColumn1.AspectName = "paramname";
            this.olvColumn1.CellPadding = null;
            this.olvColumn1.IsEditable = false;
            this.olvColumn1.Text = "参数名";
            this.olvColumn1.Width = 160;
            // 
            // olvColumn2
            // 
            this.olvColumn2.AspectName = "Value";
            this.olvColumn2.AutoCompleteEditor = false;
            this.olvColumn2.AutoCompleteEditorMode = System.Windows.Forms.AutoCompleteMode.None;
            this.olvColumn2.CellPadding = null;
            this.olvColumn2.Text = "值";
            this.olvColumn2.Width = 80;
            // 
            // olvColumn3
            // 
            this.olvColumn3.AspectName = "unit";
            this.olvColumn3.CellPadding = null;
            this.olvColumn3.IsEditable = false;
            this.olvColumn3.Text = "单位";
            // 
            // olvColumn4
            // 
            this.olvColumn4.AspectName = "range";
            this.olvColumn4.CellPadding = null;
            this.olvColumn4.IsEditable = false;
            this.olvColumn4.Text = "范围";
            this.olvColumn4.Width = 100;
            this.olvColumn4.WordWrap = true;
            // 
            // olvColumn5
            // 
            this.olvColumn5.AspectName = "desc";
            this.olvColumn5.CellPadding = null;
            this.olvColumn5.IsEditable = false;
            this.olvColumn5.Text = "描述";
            this.olvColumn5.Width = 210;
            this.olvColumn5.WordWrap = true;
            // 
            // CMB_ComPort
            // 
            this.CMB_ComPort.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.CMB_ComPort.FormattingEnabled = true;
            this.CMB_ComPort.Location = new System.Drawing.Point(623, 374);
            this.CMB_ComPort.Name = "CMB_ComPort";
            this.CMB_ComPort.Size = new System.Drawing.Size(91, 20);
            this.CMB_ComPort.TabIndex = 81;
            this.CMB_ComPort.SelectedIndexChanged += new System.EventHandler(this.CMB_ComPort_SelectedIndexChanged);
            this.CMB_ComPort.Click += new System.EventHandler(this.CMB_ComPort_Click);
            // 
            // btn_up_fw
            // 
            this.btn_up_fw.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btn_up_fw.Location = new System.Drawing.Point(753, 491);
            this.btn_up_fw.Name = "btn_up_fw";
            this.btn_up_fw.Size = new System.Drawing.Size(133, 22);
            this.btn_up_fw.TabIndex = 82;
            this.btn_up_fw.Text = "固件更新";
            this.btn_up_fw.UseVisualStyleBackColor = true;
            this.btn_up_fw.Click += new System.EventHandler(this.btn_up_fw_Click);
            // 
            // progress
            // 
            this.progress.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.progress.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.progress.Location = new System.Drawing.Point(3, 492);
            this.progress.Name = "progress";
            this.progress.Size = new System.Drawing.Size(744, 21);
            this.progress.Step = 1;
            this.progress.TabIndex = 83;
            // 
            // lbl_status
            // 
            this.lbl_status.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.lbl_status.AutoSize = true;
            this.lbl_status.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.lbl_status.Location = new System.Drawing.Point(1, 516);
            this.lbl_status.Name = "lbl_status";
            this.lbl_status.Size = new System.Drawing.Size(41, 12);
            this.lbl_status.TabIndex = 84;
            this.lbl_status.Text = "Status";
            // 
            // Load_from_OSD
            // 
            this.Load_from_OSD.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.Load_from_OSD.Location = new System.Drawing.Point(753, 374);
            this.Load_from_OSD.Name = "Load_from_OSD";
            this.Load_from_OSD.Size = new System.Drawing.Size(133, 22);
            this.Load_from_OSD.TabIndex = 85;
            this.Load_from_OSD.Text = "读取参数";
            this.Load_from_OSD.UseVisualStyleBackColor = true;
            this.Load_from_OSD.Click += new System.EventHandler(this.Load_from_OSD_Click);
            // 
            // Save_To_OSD
            // 
            this.Save_To_OSD.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.Save_To_OSD.BackColor = System.Drawing.SystemColors.Control;
            this.Save_To_OSD.Location = new System.Drawing.Point(753, 402);
            this.Save_To_OSD.Name = "Save_To_OSD";
            this.Save_To_OSD.Size = new System.Drawing.Size(133, 22);
            this.Save_To_OSD.TabIndex = 86;
            this.Save_To_OSD.Text = "保存到内存";
            this.Save_To_OSD.UseVisualStyleBackColor = false;
            this.Save_To_OSD.Click += new System.EventHandler(this.Save_To_OSD_Click);
            this.Save_To_OSD.MouseEnter += new System.EventHandler(this.Save_To_OSD_MouseEnter);
            // 
            // Sav_To_EEPROM
            // 
            this.Sav_To_EEPROM.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.Sav_To_EEPROM.Location = new System.Drawing.Point(753, 431);
            this.Sav_To_EEPROM.Name = "Sav_To_EEPROM";
            this.Sav_To_EEPROM.Size = new System.Drawing.Size(133, 22);
            this.Sav_To_EEPROM.TabIndex = 87;
            this.Sav_To_EEPROM.Text = "保存到FLASH";
            this.Sav_To_EEPROM.UseVisualStyleBackColor = true;
            this.Sav_To_EEPROM.Click += new System.EventHandler(this.Sav_To_EEPROM_Click);
            this.Sav_To_EEPROM.MouseEnter += new System.EventHandler(this.Sav_To_EEPROM_MouseEnter);
            // 
            // menuStrip1
            // 
            this.menuStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.fileToolStripMenuItem,
            this.optionsToolStripMenuItem,
            this.helpToolStripMenuItem});
            this.menuStrip1.Location = new System.Drawing.Point(0, 0);
            this.menuStrip1.Name = "menuStrip1";
            this.menuStrip1.Size = new System.Drawing.Size(898, 25);
            this.menuStrip1.TabIndex = 91;
            this.menuStrip1.Text = "menuStrip1";
            // 
            // fileToolStripMenuItem
            // 
            this.fileToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.openOSDFileToolStripMenuItem,
            this.saveOSDFileToolStripMenuItem,
            this.toolStripSeparator1,
            this.loadDefaultsToolStripMenuItem,
            this.loadCustomFirmwareToolStripMenuItem,
            this.toolStripSeparator2,
            this.exitToolStripMenuItem});
            this.fileToolStripMenuItem.Name = "fileToolStripMenuItem";
            this.fileToolStripMenuItem.Size = new System.Drawing.Size(39, 21);
            this.fileToolStripMenuItem.Text = "File";
            // 
            // saveOSDFileToolStripMenuItem
            // 
            this.saveOSDFileToolStripMenuItem.Name = "saveOSDFileToolStripMenuItem";
            this.saveOSDFileToolStripMenuItem.Size = new System.Drawing.Size(215, 22);
            this.saveOSDFileToolStripMenuItem.Text = "Save OSD file...";
            this.saveOSDFileToolStripMenuItem.Click += new System.EventHandler(this.saveOSDFileToolStripMenuItem_Click);
            // 
            // openOSDFileToolStripMenuItem
            // 
            this.openOSDFileToolStripMenuItem.Name = "openOSDFileToolStripMenuItem";
            this.openOSDFileToolStripMenuItem.Size = new System.Drawing.Size(215, 22);
            this.openOSDFileToolStripMenuItem.Text = "Open OSD file...";
            this.openOSDFileToolStripMenuItem.Click += new System.EventHandler(this.openOSDFileToolStripMenuItem_Click);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(212, 6);
            // 
            // loadDefaultsToolStripMenuItem
            // 
            this.loadDefaultsToolStripMenuItem.Name = "loadDefaultsToolStripMenuItem";
            this.loadDefaultsToolStripMenuItem.Size = new System.Drawing.Size(215, 22);
            this.loadDefaultsToolStripMenuItem.Text = "Load Defaults";
            this.loadDefaultsToolStripMenuItem.Click += new System.EventHandler(this.loadDefaultsToolStripMenuItem_Click);
            // 
            // toolStripSeparator2
            // 
            this.toolStripSeparator2.Name = "toolStripSeparator2";
            this.toolStripSeparator2.Size = new System.Drawing.Size(212, 6);
            // 
            // exitToolStripMenuItem
            // 
            this.exitToolStripMenuItem.Name = "exitToolStripMenuItem";
            this.exitToolStripMenuItem.Size = new System.Drawing.Size(215, 22);
            this.exitToolStripMenuItem.Text = "Exit";
            this.exitToolStripMenuItem.Click += new System.EventHandler(this.exitToolStripMenuItem_Click);
            // 
            // optionsToolStripMenuItem
            // 
            this.optionsToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.languageToolStripMenuItem});
            this.optionsToolStripMenuItem.Name = "optionsToolStripMenuItem";
            this.optionsToolStripMenuItem.Size = new System.Drawing.Size(66, 21);
            this.optionsToolStripMenuItem.Text = "Options";
            // 
            // languageToolStripMenuItem
            // 
            this.languageToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.englishToolStripMenuItem,
            this.chineseToolStripMenuItem});
            this.languageToolStripMenuItem.Name = "languageToolStripMenuItem";
            this.languageToolStripMenuItem.Size = new System.Drawing.Size(152, 22);
            this.languageToolStripMenuItem.Text = "Language";
            // 
            // englishToolStripMenuItem
            // 
            this.englishToolStripMenuItem.Checked = true;
            this.englishToolStripMenuItem.CheckState = System.Windows.Forms.CheckState.Checked;
            this.englishToolStripMenuItem.Name = "englishToolStripMenuItem";
            this.englishToolStripMenuItem.Size = new System.Drawing.Size(121, 22);
            this.englishToolStripMenuItem.Text = "English";
            this.englishToolStripMenuItem.Click += new System.EventHandler(this.englishToolStripMenuItem_Click);
            // 
            // chineseToolStripMenuItem
            // 
            this.chineseToolStripMenuItem.Name = "chineseToolStripMenuItem";
            this.chineseToolStripMenuItem.Size = new System.Drawing.Size(121, 22);
            this.chineseToolStripMenuItem.Text = "Chinese";
            this.chineseToolStripMenuItem.Click += new System.EventHandler(this.chineseToolStripMenuItem_Click);
            // 
            // helpToolStripMenuItem
            // 
            this.helpToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.gettingStartedToolStripMenuItem,
            this.checkUpdatesToolStripMenuItem,
            this.aboutToolStripMenuItem});
            this.helpToolStripMenuItem.Name = "helpToolStripMenuItem";
            this.helpToolStripMenuItem.Size = new System.Drawing.Size(47, 21);
            this.helpToolStripMenuItem.Text = "Help";
            // 
            // gettingStartedToolStripMenuItem
            // 
            this.gettingStartedToolStripMenuItem.Name = "gettingStartedToolStripMenuItem";
            this.gettingStartedToolStripMenuItem.Size = new System.Drawing.Size(164, 22);
            this.gettingStartedToolStripMenuItem.Text = "Getting started";
            this.gettingStartedToolStripMenuItem.Click += new System.EventHandler(this.gettingStartedToolStripMenuItem_Click);
            // 
            // checkUpdatesToolStripMenuItem
            // 
            this.checkUpdatesToolStripMenuItem.Name = "checkUpdatesToolStripMenuItem";
            this.checkUpdatesToolStripMenuItem.Size = new System.Drawing.Size(164, 22);
            this.checkUpdatesToolStripMenuItem.Text = "Check Updates";
            this.checkUpdatesToolStripMenuItem.Click += new System.EventHandler(this.checkUpdatesToolStripMenuItem_Click);
            // 
            // aboutToolStripMenuItem
            // 
            this.aboutToolStripMenuItem.Name = "aboutToolStripMenuItem";
            this.aboutToolStripMenuItem.Size = new System.Drawing.Size(164, 22);
            this.aboutToolStripMenuItem.Text = "About";
            this.aboutToolStripMenuItem.Click += new System.EventHandler(this.aboutToolStripMenuItem_Click);
            // 
            // glControl1
            // 
            this.glControl1.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.glControl1.AutoSize = true;
            this.glControl1.BackColor = System.Drawing.Color.Black;
            this.glControl1.Location = new System.Drawing.Point(510, 28);
            this.glControl1.Name = "glControl1";
            this.glControl1.Size = new System.Drawing.Size(376, 288);
            this.glControl1.TabIndex = 93;
            this.glControl1.VSync = false;
            this.glControl1.Load += new System.EventHandler(this.glControl1_Load);
            this.glControl1.Paint += new System.Windows.Forms.PaintEventHandler(this.glControl1_Paint);
            this.glControl1.Resize += new System.EventHandler(this.glControl1_Resize);
            // 
            // labPanle
            // 
            this.labPanle.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.labPanle.AutoSize = true;
            this.labPanle.Font = new System.Drawing.Font("Microsoft Sans Serif", 14.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.labPanle.Location = new System.Drawing.Point(690, 330);
            this.labPanle.Name = "labPanle";
            this.labPanle.Size = new System.Drawing.Size(21, 24);
            this.labPanle.TabIndex = 94;
            this.labPanle.Text = "1";
            this.labPanle.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            // 
            // lab_inc
            // 
            this.lab_inc.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.lab_inc.AutoSize = true;
            this.lab_inc.Font = new System.Drawing.Font("Microsoft Sans Serif", 14.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lab_inc.Location = new System.Drawing.Point(713, 330);
            this.lab_inc.Name = "lab_inc";
            this.lab_inc.Size = new System.Drawing.Size(34, 24);
            this.lab_inc.TabIndex = 95;
            this.lab_inc.Text = ">>";
            this.lab_inc.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this.lab_inc.Click += new System.EventHandler(this.lab_inc_Click);
            // 
            // lab_dec
            // 
            this.lab_dec.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.lab_dec.AutoSize = true;
            this.lab_dec.Font = new System.Drawing.Font("Microsoft Sans Serif", 14.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lab_dec.Location = new System.Drawing.Point(652, 331);
            this.lab_dec.Name = "lab_dec";
            this.lab_dec.Size = new System.Drawing.Size(34, 24);
            this.lab_dec.TabIndex = 96;
            this.lab_dec.Text = "<<";
            this.lab_dec.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this.lab_dec.Click += new System.EventHandler(this.lab_dec_Click);
            // 
            // lbl_port
            // 
            this.lbl_port.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.lbl_port.AutoSize = true;
            this.lbl_port.Location = new System.Drawing.Point(552, 378);
            this.lbl_port.Name = "lbl_port";
            this.lbl_port.Size = new System.Drawing.Size(29, 12);
            this.lbl_port.TabIndex = 97;
            this.lbl_port.Text = "Port";
            this.lbl_port.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // lbl_fc
            // 
            this.lbl_fc.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.lbl_fc.AutoSize = true;
            this.lbl_fc.Location = new System.Drawing.Point(552, 406);
            this.lbl_fc.Name = "lbl_fc";
            this.lbl_fc.Size = new System.Drawing.Size(83, 12);
            this.lbl_fc.TabIndex = 98;
            this.lbl_fc.Text = "FlightControl";
            this.lbl_fc.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // cbx_fc
            // 
            this.cbx_fc.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.cbx_fc.FormattingEnabled = true;
            this.cbx_fc.Items.AddRange(new object[] {
            "APM/Pixhawk",
            "CC3D/Revo"});
            this.cbx_fc.Location = new System.Drawing.Point(623, 404);
            this.cbx_fc.Name = "cbx_fc";
            this.cbx_fc.Size = new System.Drawing.Size(91, 20);
            this.cbx_fc.TabIndex = 99;
            this.cbx_fc.SelectedIndexChanged += new System.EventHandler(this.cbx_fc_SelectedIndexChanged);
            // 
            // timer1
            // 
            this.timer1.Interval = 5000;
            this.timer1.Tick += new System.EventHandler(this.timer1_Tick);
            // 
            // loadCustomFirmwareToolStripMenuItem
            // 
            this.loadCustomFirmwareToolStripMenuItem.Name = "loadCustomFirmwareToolStripMenuItem";
            this.loadCustomFirmwareToolStripMenuItem.Size = new System.Drawing.Size(215, 22);
            this.loadCustomFirmwareToolStripMenuItem.Text = "Load custom firmware...";
            this.loadCustomFirmwareToolStripMenuItem.Click += new System.EventHandler(this.loadCustomFirmwareToolStripMenuItem_Click);
            // 
            // PlayuavOSD
            // 
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.BackColor = System.Drawing.SystemColors.Control;
            this.ClientSize = new System.Drawing.Size(898, 543);
            this.Controls.Add(this.cbx_fc);
            this.Controls.Add(this.lbl_fc);
            this.Controls.Add(this.lbl_port);
            this.Controls.Add(this.lab_dec);
            this.Controls.Add(this.lab_inc);
            this.Controls.Add(this.labPanle);
            this.Controls.Add(this.glControl1);
            this.Controls.Add(this.Sav_To_EEPROM);
            this.Controls.Add(this.Save_To_OSD);
            this.Controls.Add(this.Load_from_OSD);
            this.Controls.Add(this.lbl_status);
            this.Controls.Add(this.progress);
            this.Controls.Add(this.btn_up_fw);
            this.Controls.Add(this.CMB_ComPort);
            this.Controls.Add(this.Params);
            this.Controls.Add(this.menuStrip1);
            this.ForeColor = System.Drawing.Color.Black;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MainMenuStrip = this.menuStrip1;
            this.Name = "PlayuavOSD";
            this.Text = "PlayuavOSD";
            this.WindowState = System.Windows.Forms.FormWindowState.Maximized;
            this.Load += new System.EventHandler(this.PlayuavOSD_Load);
            this.Resize += new System.EventHandler(this.PlayuavOSD_Resize);
            ((System.ComponentModel.ISupportInitialize)(this.Params)).EndInit();
            this.menuStrip1.ResumeLayout(false);
            this.menuStrip1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private BrightIdeasSoftware.DataTreeListView Params;
        private BrightIdeasSoftware.OLVColumn olvColumn1;
        private BrightIdeasSoftware.OLVColumn olvColumn2;
        private BrightIdeasSoftware.OLVColumn olvColumn3;
        private BrightIdeasSoftware.OLVColumn olvColumn4;
        private BrightIdeasSoftware.OLVColumn olvColumn5;
        private System.Windows.Forms.ComboBox CMB_ComPort;
        private System.Windows.Forms.Button btn_up_fw;
        private System.Windows.Forms.ProgressBar progress;
        private System.Windows.Forms.Label lbl_status;
        private System.Windows.Forms.Button Load_from_OSD;
        private System.Windows.Forms.Button Save_To_OSD;
        private System.Windows.Forms.Button Sav_To_EEPROM;
        private System.Windows.Forms.MenuStrip menuStrip1;
        private System.Windows.Forms.ToolStripMenuItem fileToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem saveOSDFileToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem openOSDFileToolStripMenuItem;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripMenuItem loadDefaultsToolStripMenuItem;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
        private System.Windows.Forms.ToolStripMenuItem exitToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem optionsToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem languageToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem englishToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem chineseToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem helpToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem gettingStartedToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem aboutToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem checkUpdatesToolStripMenuItem;
        private OpenTK.GLControl glControl1;
        private System.Windows.Forms.Label labPanle;
        private System.Windows.Forms.Label lab_inc;
        private System.Windows.Forms.Label lab_dec;
        private System.Windows.Forms.Label lbl_port;
        private System.Windows.Forms.Label lbl_fc;
        private System.Windows.Forms.ComboBox cbx_fc;
        private System.Windows.Forms.Timer timer1;
        private System.Windows.Forms.ToolStripMenuItem loadCustomFirmwareToolStripMenuItem;
    }
}