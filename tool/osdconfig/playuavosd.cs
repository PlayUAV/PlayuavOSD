using System;
using System.Collections;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.IO.Ports;
using System.IO;
using System.Net;
using MissionPlanner.Comms;
using MissionPlanner.Utilities;
using System.Collections.Generic;
using MissionPlanner.Controls;
using MissionPlanner.Arduino;
using MissionPlanner;
using System.Diagnostics;
using System.Drawing.Drawing2D;
using OpenTK.Graphics;
using OpenTK.Graphics.OpenGL;
using log4net;
namespace OSD
{
    public partial class PlayuavOSD : Form
    {
        ProgressReporterDialogue pdr = new ProgressReporterDialogue();
        MissionPlanner.Utilities.Firmware fw = new MissionPlanner.Utilities.Firmware();
        public static string comPortName = "";
        static internal ICommsSerial comPort;
        
        byte[] eeprom = new byte[1024];
        byte[] paramdefault = new byte[1024];

        PlayuavOSD self;
        string currentVersion = "1.0.0.3";

        // Changes made to the params between writing to the copter
        readonly Hashtable _changes = new Hashtable();

        readonly Hashtable _paramsAddr = new Hashtable();

        public enum LanguageEnum : int
        {
            LANG_EN = 0,
            LANG_ZH = 1,
        }
        public static LanguageEnum langid;
        private Language lang = new Language();

        public int osd_rev;

        public enum Code : byte
        {
            // response codes
            NOP = 0x00,
            OK = 0x10,
            FAILED = 0x11,
            INSYNC = 0x12,
            INVALID = 0x13,

            // protocol commands
            EOC = 0x20,
            GET_SYNC = 0x21,
            GET_DEVICE = 0x22,
            CHIP_ERASE = 0x23,
            START_TRANSFER = 0x24,      //tell the osd we will start send params
            SET_PARAMS = 0x25,           //actually send params
            GET_PARAMS = 0x26,          //recv params from osd
            INFO_OSD_REV = 0x27,        //get the firmware revision
            END_TRANSFER = 0x28,
            SAVE_TO_EEPROM = 0x29,

            PROG_MULTI_MAX = 60,        //# protocol max is 255, must be multiple of 4
            READ_MULTI_MAX = 60,        //# protocol max is 255, something overflows with >= 64

        }

        /* *********************************************** */
        // Version number, incrementing this will erase/upload factory settings.
        // Only devs should increment this
        const int VER = 1;

        public class data
        {
            public string root;
            public string paramname;
            public string Value;
            public string unit;
            public string range;
            public string desc;
            public List<data> children = new List<PlayuavOSD.data>();
        }

        bool loaded = false;

        public Color hudcolor { get { return whitePen.Color; } set { _hudcolor = value; whitePen = new Pen(value, 2); } }
        Color _hudcolor = Color.White;
        Pen whitePen = new Pen(Color.White, 1);
        Pen blackPen = new Pen(Color.Black, 1);
        Pen greenPen = new Pen(Color.Green, 1);
        Pen redPen = new Pen(Color.Red, 1);
        Font font = new Font("Arial", 10);
        List<int> SIZE_TO_FONT = new List<int>();

        oglUtility ogl = new oglUtility();
        Stopwatch sw = new Stopwatch(); // available to all event handlers
        Graphics graphicsObjectGDIP;
        public Bitmap objBitmap = new Bitmap(1024, 1024, System.Drawing.Imaging.PixelFormat.Format32bppArgb);
        public PlayuavOSD()
        {
            self = this;
            InitializeComponent();
            SIZE_TO_FONT.Add(5);
            SIZE_TO_FONT.Add(8);
            SIZE_TO_FONT.Add(11);
            langid = LanguageEnum.LANG_EN;
            switch (System.Globalization.CultureInfo.CurrentUICulture.Name)
            {
                case "zh-CN":
                case "zh-Hans":
                    langid = LanguageEnum.LANG_ZH;
                    break;
                default:
                    break;
            }

            graphicsObjectGDIP = Graphics.FromImage(objBitmap);
        }

        private void PlayuavOSD_Load(object sender, EventArgs e)
        {
            comPort = new MissionPlanner.Comms.SerialPort();

            setDefaultParams();

            paramdefault.CopyTo(eeprom, 0);
            processToScreen();

            this.Text = this.Text + "-V" + currentVersion;
            //CheckNewVersion();
            timer1.Start();
        }

        private void CheckNewVersion()
        {
            if (Updater.NewVersionExists(currentVersion))
            {
                UpdateAvailable frmUpdateAvailable = new UpdateAvailable();
                System.Windows.Forms.DialogResult result = frmUpdateAvailable.ShowDialog();

                if (result == System.Windows.Forms.DialogResult.Yes)
                {
                    Process.Start(AppDomain.CurrentDomain.BaseDirectory + @"\CTToolUpdater.exe");
                    this.Close();
                }
            }          
        }

        public void __send(byte c)
        {
            comPort.Write(new byte[] { c }, 0, 1);
        }

        public void __send(byte[] c)
        {
            comPort.Write(c, 0, c.Length);
        }

        public byte[] __recv(int count = 1)
        {
            // this will auto timeout
            // Console.WriteLine("recv "+count);
            byte[] c = new byte[count];
            int pos = 0;
            while (pos < count)
                pos += comPort.Read(c, pos, count - pos);

            return c;
        }

        public int __recv_int()
        {
            byte[] raw = __recv(4);
            //raw.Reverse();
            int val = BitConverter.ToInt32(raw, 0);
            return val;
        }

        public void __getSync()
        {
            comPort.BaseStream.Flush();
            byte c = __recv()[0];
            if (c != (byte)Code.INSYNC)
                throw new Exception(string.Format("unexpected {0:X} instead of INSYNC", (byte)c));
            c = __recv()[0];
            if (c == (byte)Code.INVALID)
                throw new Exception(string.Format("playuavosd reports INVALID OPERATION", (byte)c));
            if (c == (byte)Code.FAILED)
                throw new Exception(string.Format("playuavosd reports OPERATION FAILED", (byte)c));
            if (c != (byte)Code.OK)
                throw new Exception(string.Format("unexpected {0:X} instead of OK", (byte)c));
        }

        public void __sync()
        {
            comPort.BaseStream.Flush();
            __send(new byte[] { (byte)Code.GET_SYNC, (byte)Code.EOC });
            __getSync();
        }

        public bool __trySync()
        {
            comPort.BaseStream.Flush();
            byte c = __recv()[0];
            if (c != (byte)Code.INSYNC)
                return false;
            if (c != (byte)Code.OK)
                return false;

            return true;
        }

        public int __getInfo(Code param)
        {
            __send(new byte[] { (byte)Code.GET_DEVICE, (byte)param, (byte)Code.EOC });
            byte c = __recv()[0];
            int info = c;
            __getSync();
            //Array.Reverse(raw);
            return info;
        }

        public List<Byte[]> __split_len(byte[] seq, int length)
        {
            List<Byte[]> answer = new List<byte[]>();
            int size = length;
            for (int a = 0; a < seq.Length; )
            {
                byte[] ba = new byte[size];
                // Console.WriteLine(a + " " + seq.Length +" " + size);
                Array.Copy(seq, a, ba, 0, size);
                answer.Add(ba);
                a += size;
                if ((seq.Length - a) < size)
                    size = seq.Length - a;
            }
            return answer;
        }

        public void __set_parameters(byte[] data)
        {
            __send(new byte[] { (byte)Code.SET_PARAMS, (byte)data.Length });
            __send(data);
            __send((byte)Code.EOC);
            __getSync();
        }

        public void u16toEPPROM(byte[] buf, int addr, short val)
        {
            buf[addr] = (byte)(val & 0xFF);
            buf[addr + 1] = (byte)((val >> 8) & 0xFF);
        }

        internal string getU16ParamString(byte[] buf, int paramAddr)
        {
            string strRet = "";
            try
            {
                short stmp = Convert.ToInt16(buf[paramAddr]);
                short stmp1 = Convert.ToInt16(buf[paramAddr + 1]);
                strRet = Convert.ToString(stmp + (stmp1 << 8));
            }
            catch
            {
            }
            
            return strRet;
        }

        internal short getU16Param(byte[] buf, int paramAddr)
        {
            short ret = 0;
            try
            {
                short stmp = Convert.ToInt16(buf[paramAddr]);
                short stmp1 = Convert.ToInt16(buf[paramAddr + 1]);
                ret = Convert.ToInt16(stmp + (stmp1 << 8));
            }
            catch
            {
            }
            
            return ret;
        }

        string panelValF2Str(int val)
        {
            if (val == 0)
                return "0";

            if (val == 1)
                return "1";

            string strRet = "";
            try
            {
                List<int> valArr = new List<int>();
                for (int i = 1; i < 10; i++)
                {
                    int b = val & (int)(System.Math.Pow(2, i - 1));
                    if (b != 0)
                    {
                        valArr.Add(i);
                    }
                }

                valArr.Sort();
                foreach (int i in valArr)
                {
                    strRet += Convert.ToString(i) + ",";
                }
                strRet = strRet.Remove(strRet.Length - 1);
            }
            catch
            {
            }
            
            return strRet;
        }

        float panelValStr2F(string str)
        {
            float newvalue = 0;
            try
            {
                string[] strnewarr = str.Split(',');
                double tmpvalue = 0;
                foreach (string bytes in strnewarr)
                {
                    tmpvalue = double.Parse(bytes);
                    newvalue += (float)(System.Math.Pow(2, tmpvalue - 1));
                }

            }
            catch { CustomMessageBox.Show("Bad number"); return 0; }

            return newvalue;
        }

        internal string getU16PanelString(byte[] buf, int paramAddr)
        {
            int a = 0;
            try
            {
                short stmp = Convert.ToInt16(buf[paramAddr]);
                short stmp1 = Convert.ToInt16(buf[paramAddr + 1]);
                a = Convert.ToInt32(stmp + (stmp1 << 8));
            }
            catch
            {
                return "";
            }
            
            return panelValF2Str(a);
        }

        private string[] GetPortNames()
        {
            string[] devs = new string[0];

            if (Directory.Exists("/dev/"))
                devs = Directory.GetFiles("/dev/", "*ACM*");

            string[] ports = System.IO.Ports.SerialPort.GetPortNames();

            string[] all = new string[devs.Length + ports.Length];

            devs.CopyTo(all, 0);
            ports.CopyTo(all, devs.Length);

            return all;
        }

        private void CMB_ComPort_Click(object sender, EventArgs e)
        {
            CMB_ComPort.Items.Clear();
            CMB_ComPort.Items.AddRange(GetPortNames());
        }

        private void btn_up_fw_Click(object sender, EventArgs e)
        {
            string baseurl = "http://www.playuav.com/download/Playuavosd/playuavosd.hex";


            try
            {
                // Create a request using a URL that can receive a post. 
                WebRequest request = WebRequest.Create(baseurl);
                request.Timeout = 10000;
                // Set the Method property of the request to POST.
                request.Method = "GET";
                // Get the request stream.
                Stream dataStream; //= request.GetRequestStream();
                // Get the response.
                WebResponse response = request.GetResponse();

                // Get the stream containing content returned by the server.
                dataStream = response.GetResponseStream();

                long bytes = response.ContentLength;
                long contlen = bytes;

                byte[] buf1 = new byte[1024];

                FileStream fs = new FileStream(Path.GetDirectoryName(Application.ExecutablePath) + Path.DirectorySeparatorChar + @"playuavosd.hex", FileMode.Create);

                lbl_status.Text = "Downloading from Internet";

                this.Refresh();
                Application.DoEvents();

                dataStream.ReadTimeout = 30000;

                while (dataStream.CanRead)
                {
                    try
                    {
                        progress.Value = 50;// (int)(((float)(response.ContentLength - bytes) / (float)response.ContentLength) * 100);
                        this.progress.Refresh();
                    }
                    catch { }
                    int len = dataStream.Read(buf1, 0, 1024);
                    if (len == 0)
                        break;
                    bytes -= len;
                    fs.Write(buf1, 0, len);
                }

                fs.Close();
                dataStream.Close();
                response.Close();

                lbl_status.Text = "Done";
                Application.DoEvents();

                //upload firmware
                fw.Progress -= fw_Progress;
                fw.Progress += fw_Progress1;

                BoardDetect.boards boardtype = BoardDetect.boards.none;
                try
                {
                    boardtype = BoardDetect.DetectBoard(comPortName);
                }
                catch
                {
                     CustomMessageBox.Show("Can not connect to com port and detect board type", Strings.ERROR);
                    return;
                }

                fw.UploadFlash(comPortName, fs.Name, boardtype);
            }
            catch { 
                 CustomMessageBox.Show("Error receiving firmware", Strings.ERROR); 
                return; 
            }
        }

        /// <summary>
        /// for updating fw list
        /// </summary>
        /// <param name="progress"></param>
        /// <param name="status"></param>
        void fw_Progress(int progress, string status)
        {
            pdr.UpdateProgressAndStatus(progress, status);
        }

        /// <summary>
        /// for when updating fw to hardware
        /// </summary>
        /// <param name="progress"></param>
        /// <param name="status"></param>
        void fw_Progress1(int progress, string status)
        {
            bool change = false;

            if (progress != -1)
            {
                if (this.progress.Value != progress)
                {
                    this.progress.Value = progress;
                    change = true;
                }

            }
            if (lbl_status.Text != status)
            {
                lbl_status.Text = status;
                change = true;
            }

            if (change)
                this.Refresh();
        }

        private void CMB_ComPort_SelectedIndexChanged(object sender, EventArgs e)
        {
            comPortName = CMB_ComPort.Text;
        }

        private void setDefaultParams()
        {
            int address = 0;

            _paramsAddr["ArmState_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["ArmState_Enable"], 1);
            _paramsAddr["ArmState_Panel"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["ArmState_Panel"], 1);
            _paramsAddr["ArmState_H_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["ArmState_H_Position"], 350);
            _paramsAddr["ArmState_V_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["ArmState_V_Position"], 34);
            _paramsAddr["ArmState_Font_Size"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["ArmState_Font_Size"], 0);
            _paramsAddr["ArmState_H_Alignment"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["ArmState_H_Alignment"], 2);

            _paramsAddr["BatteryVoltage_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["BatteryVoltage_Enable"], 1);
            _paramsAddr["BatteryVoltage_Panel"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["BatteryVoltage_Panel"], 1);
            _paramsAddr["BatteryVoltage_H_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["BatteryVoltage_H_Position"], 350);
            _paramsAddr["BatteryVoltage_V_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["BatteryVoltage_V_Position"], 4);
            _paramsAddr["BatteryVoltage_Font_Size"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["BatteryVoltage_Font_Size"], 0);
            _paramsAddr["BatteryVoltage_H_Alignment"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["BatteryVoltage_H_Alignment"], 2);

            _paramsAddr["BatteryCurrent_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["BatteryCurrent_Enable"], 1);
            _paramsAddr["BatteryCurrent_Panel"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["BatteryCurrent_Panel"], 1);
            _paramsAddr["BatteryCurrent_H_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["BatteryCurrent_H_Position"], 350);
            _paramsAddr["BatteryCurrent_V_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["BatteryCurrent_V_Position"], 14);
            _paramsAddr["BatteryCurrent_Font_Size"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["BatteryCurrent_Font_Size"], 0);
            _paramsAddr["BatteryCurrent_H_Alignment"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["BatteryCurrent_H_Alignment"], 2);

            _paramsAddr["BatteryConsumed_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["BatteryConsumed_Enable"], 1);
            _paramsAddr["BatteryConsumed_Panel"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["BatteryConsumed_Panel"], 1);
            _paramsAddr["BatteryConsumed_H_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["BatteryConsumed_H_Position"], 350);
            _paramsAddr["BatteryConsumed_V_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["BatteryConsumed_V_Position"], 24);
            _paramsAddr["BatteryConsumed_Font_Size"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["BatteryConsumed_Font_Size"], 0);
            _paramsAddr["BatteryConsumed_H_Alignment"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["BatteryConsumed_H_Alignment"], 2);

            _paramsAddr["FlightMode_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["FlightMode_Enable"], 1);
            _paramsAddr["FlightMode_Panel"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["FlightMode_Panel"], 1);
            _paramsAddr["FlightMode_H_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["FlightMode_H_Position"], 350);
            _paramsAddr["FlightMode_V_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["FlightMode_V_Position"], 44);
            _paramsAddr["FlightMode_Font_Size"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["FlightMode_Font_Size"], 1);
            _paramsAddr["FlightMode_H_Alignment"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["FlightMode_H_Alignment"], 2);

            _paramsAddr["GPSStatus_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPSStatus_Enable"], 1);
            _paramsAddr["GPSStatus_Panel"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPSStatus_Panel"], 1);
            _paramsAddr["GPSStatus_H_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPSStatus_H_Position"], 0);
            _paramsAddr["GPSStatus_V_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPSStatus_V_Position"], 230);
            _paramsAddr["GPSStatus_Font_Size"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPSStatus_Font_Size"], 0);
            _paramsAddr["GPSStatus_H_Alignment"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPSStatus_H_Alignment"], 0);

            _paramsAddr["GPSHDOP_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPSHDOP_Enable"], 1);
            _paramsAddr["GPSHDOP_Panel"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPSHDOP_Panel"], 1);
            _paramsAddr["GPSHDOP_H_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPSHDOP_H_Position"], 70);
            _paramsAddr["GPSHDOP_V_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPSHDOP_V_Position"], 230);
            _paramsAddr["GPSHDOP_Font_Size"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPSHDOP_Font_Size"], 0);
            _paramsAddr["GPSHDOP_H_Alignment"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPSHDOP_H_Alignment"], 0);

            _paramsAddr["GPSLatitude_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPSLatitude_Enable"], 1);
            _paramsAddr["GPSLatitude_Panel"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPSLatitude_Panel"], 1);
            _paramsAddr["GPSLatitude_H_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPSLatitude_H_Position"], 200);
            _paramsAddr["GPSLatitude_V_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPSLatitude_V_Position"], 230);
            _paramsAddr["GPSLatitude_Font_Size"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPSLatitude_Font_Size"], 0);
            _paramsAddr["GPSLatitude_H_Alignment"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPSLatitude_H_Alignment"], 0);

            _paramsAddr["GPSLongitude_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPSLongitude_Enable"], 1);
            _paramsAddr["GPSLongitude_Panel"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPSLongitude_Panel"], 1);
            _paramsAddr["GPSLongitude_H_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPSLongitude_H_Position"], 280);
            _paramsAddr["GPSLongitude_V_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPSLongitude_V_Position"], 230);
            _paramsAddr["GPSLongitude_Font_Size"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPSLongitude_Font_Size"], 0);
            _paramsAddr["GPSLongitude_H_Alignment"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPSLongitude_H_Alignment"], 0);

            _paramsAddr["GPS2Status_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPS2Status_Enable"], 1);
            _paramsAddr["GPS2Status_Panel"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPS2Status_Panel"], 2);
            _paramsAddr["GPS2Status_H_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPS2Status_H_Position"], 0);
            _paramsAddr["GPS2Status_V_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPS2Status_V_Position"], 230);
            _paramsAddr["GPS2Status_Font_Size"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPS2Status_Font_Size"], 0);
            _paramsAddr["GPS2Status_H_Alignment"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPS2Status_H_Alignment"], 0);

            _paramsAddr["GPS2HDOP_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPS2HDOP_Enable"], 1);
            _paramsAddr["GPS2HDOP_Panel"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPS2HDOP_Panel"], 2);
            _paramsAddr["GPS2HDOP_H_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPS2HDOP_H_Position"], 70);
            _paramsAddr["GPS2HDOP_V_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPS2HDOP_V_Position"], 230);
            _paramsAddr["GPS2HDOP_Font_Size"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPS2HDOP_Font_Size"], 0);
            _paramsAddr["GPS2HDOP_H_Alignment"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPS2HDOP_H_Alignment"], 0);

            _paramsAddr["GPS2Latitude_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPS2Latitude_Enable"], 1);
            _paramsAddr["GPS2Latitude_Panel"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPS2Latitude_Panel"], 2);
            _paramsAddr["GPS2Latitude_H_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPS2Latitude_H_Position"], 200);
            _paramsAddr["GPS2Latitude_V_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPS2Latitude_V_Position"], 230);
            _paramsAddr["GPS2Latitude_Font_Size"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPS2Latitude_Font_Size"], 0);
            _paramsAddr["GPS2Latitude_H_Alignment"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPS2Latitude_H_Alignment"], 0);

            _paramsAddr["GPS2Longitude_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPS2Longitude_Enable"], 1);
            _paramsAddr["GPS2Longitude_Panel"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPS2Longitude_Panel"], 2);
            _paramsAddr["GPS2Longitude_H_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPS2Longitude_H_Position"], 280);
            _paramsAddr["GPS2Longitude_V_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPS2Longitude_V_Position"], 230);
            _paramsAddr["GPS2Longitude_Font_Size"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPS2Longitude_Font_Size"], 0);
            _paramsAddr["GPS2Longitude_H_Alignment"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["GPS2Longitude_H_Alignment"], 0);

            _paramsAddr["Time_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Time_Enable"], 1);
            _paramsAddr["Time_Panel"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Time_Panel"], 1);
            _paramsAddr["Time_H_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Time_H_Position"], 350);
            _paramsAddr["Time_V_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Time_V_Position"], 220);
            _paramsAddr["Time_Font_Size"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Time_Font_Size"], 0);
            _paramsAddr["Time_H_Alignment"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Time_H_Alignment"], 2);

            _paramsAddr["Altitude_TALT_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Altitude_TALT_Enable"], 1);
            _paramsAddr["Altitude_TALT_Panel"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Altitude_TALT_Panel"], 2);
            _paramsAddr["Altitude_TALT_H_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Altitude_TALT_H_Position"], 5);
            _paramsAddr["Altitude_TALT_V_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Altitude_TALT_V_Position"], 10);
            _paramsAddr["Altitude_TALT_Font_Size"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Altitude_TALT_Font_Size"], 0);
            _paramsAddr["Altitude_TALT_H_Alignment"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Altitude_TALT_H_Alignment"], 0);
            _paramsAddr["Altitude_Scale_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Altitude_Scale_Enable"], 1);
            _paramsAddr["Altitude_Scale_Panel"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Altitude_Scale_Panel"], 1);
            _paramsAddr["Altitude_Scale_H_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Altitude_Scale_H_Position"], 350);
            _paramsAddr["Altitude_Scale_Align"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Altitude_Scale_Align"], 1);
            _paramsAddr["Altitude_Scale_Source"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Altitude_Scale_Source"], 0);

            _paramsAddr["Speed_TSPD_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Speed_TSPD_Enable"], 1);
            _paramsAddr["Speed_TSPD_Panel"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Speed_TSPD_Panel"], 2);
            _paramsAddr["Speed_TSPD_H_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Speed_TSPD_H_Position"], 5);
            _paramsAddr["Speed_TSPD_V_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Speed_TSPD_V_Position"], 25);
            _paramsAddr["Speed_TSPD_Font_Size"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Speed_TSPD_Font_Size"], 0);
            _paramsAddr["Speed_TSPD_H_Alignment"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Speed_TSPD_H_Alignment"], 0);
            _paramsAddr["Speed_Scale_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Speed_Scale_Enable"], 1);
            _paramsAddr["Speed_Scale_Panel"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Speed_Scale_Panel"], 1);
            _paramsAddr["Speed_Scale_H_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Speed_Scale_H_Position"], 10);
            _paramsAddr["Speed_Scale_Align"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Speed_Scale_Align"], 0);
            _paramsAddr["Speed_Scale_Source"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Speed_Scale_Source"], 0);

            _paramsAddr["Throttle_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Throttle_Enable"], 1);
            _paramsAddr["Throttle_Panel"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Throttle_Panel"], 1);
            _paramsAddr["Throttle_Scale_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Throttle_Scale_Enable"], 1);
            _paramsAddr["Throttle_H_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Throttle_H_Position"], 295);
            _paramsAddr["Throttle_V_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Throttle_V_Position"], 202);

            //home distance
            _paramsAddr["HomeDistance_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["HomeDistance_Enable"], 1);
            _paramsAddr["HomeDistance_Panel"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["HomeDistance_Panel"], 1);
            _paramsAddr["HomeDistance_H_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["HomeDistance_H_Position"], 70);
            _paramsAddr["HomeDistance_V_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["HomeDistance_V_Position"], 14);
            _paramsAddr["HomeDistance_Font_Size"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["HomeDistance_Font_Size"], 0);
            _paramsAddr["HomeDistance_H_Alignment"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["HomeDistance_H_Alignment"], 0);

            //way-point distance
            _paramsAddr["WPDistance_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["WPDistance_Enable"], 1);
            _paramsAddr["WPDistance_Panel"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["WPDistance_Panel"], 1);
            _paramsAddr["WPDistance_H_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["WPDistance_H_Position"], 70);
            _paramsAddr["WPDistance_V_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["WPDistance_V_Position"], 24);
            _paramsAddr["WPDistance_Font_Size"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["WPDistance_Font_Size"], 0);
            _paramsAddr["WPDistance_H_Alignment"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["WPDistance_H_Alignment"], 0);

            //heading, home and wp direction
            _paramsAddr["CHWDIR_Tmode_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["CHWDIR_Tmode_Enable"], 1);
            _paramsAddr["CHWDIR_Tmode_Panel"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["CHWDIR_Tmode_Panel"], 2);
            _paramsAddr["CHWDIR_Tmode_V_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["CHWDIR_Tmode_V_Position"], 15);
            _paramsAddr["CHWDIR_Nmode_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["CHWDIR_Nmode_Enable"], 1);
            _paramsAddr["CHWDIR_Nmode_Panel"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["CHWDIR_Nmode_Panel"], 1);
            _paramsAddr["CHWDIR_Nmode_H_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["CHWDIR_Nmode_H_Position"], 30);
            _paramsAddr["CHWDIR_Nmode_V_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["CHWDIR_Nmode_V_Position"], 35);
            _paramsAddr["CHWDIR_Nmode_Radius"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["CHWDIR_Nmode_Radius"], 20);
            _paramsAddr["CHWDIR_Nmode_Home_Radius"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["CHWDIR_Nmode_Home_Radius"], 25);
            _paramsAddr["CHWDIR_Nmode_WP_Radius"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["CHWDIR_Nmode_WP_Radius"], 25);

            //Attitude
            _paramsAddr["Attitude_MP_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Attitude_MP_Enable"], 1);
            _paramsAddr["Attitude_MP_Panel"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Attitude_MP_Panel"], 1);
            _paramsAddr["Attitude_MP_Mode"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Attitude_MP_Mode"], 0);
            _paramsAddr["Attitude_3D_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Attitude_3D_Enable"], 1);
            _paramsAddr["Attitude_3D_Panel"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Attitude_3D_Panel"], 2);

            //misc
            _paramsAddr["Misc_Units_Mode"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Misc_Units_Mode"], 0);
            _paramsAddr["Misc_Max_Panels"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Misc_Max_Panels"], 2);

            //PWM Config
            _paramsAddr["PWM_Video_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["PWM_Video_Enable"], 1);
            _paramsAddr["PWM_Video_Chanel"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["PWM_Video_Chanel"], 6);
            _paramsAddr["PWM_Video_Value"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["PWM_Video_Value"], 1200);
            _paramsAddr["PWM_Panel_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["PWM_Panel_Enable"], 1);
            _paramsAddr["PWM_Panel_Chanel"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["PWM_Panel_Chanel"], 7);
            _paramsAddr["PWM_Panel_Value"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["PWM_Panel_Value"], 1200);

            //should use bit mask? enable/disable maybe more intuition
            _paramsAddr["Alarm_H_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Alarm_H_Position"], 180);
            _paramsAddr["Alarm_V_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Alarm_V_Position"], 25);
            _paramsAddr["Alarm_Font_Size"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Alarm_Font_Size"], 2);
            _paramsAddr["Alarm_H_Alignment"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Alarm_H_Alignment"], 1);
            _paramsAddr["Alarm_GPS_Status_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Alarm_GPS_Status_Enable"], 1);
            _paramsAddr["Alarm_Low_Batt_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Alarm_Low_Batt_Enable"], 1);
            _paramsAddr["Alarm_Low_Batt"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Alarm_Low_Batt"], 20);
            _paramsAddr["Alarm_Under_Speed_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Alarm_Under_Speed_Enable"], 0);
            _paramsAddr["Alarm_Under_Speed"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Alarm_Under_Speed"], 2);
            _paramsAddr["Alarm_Over_Speed_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Alarm_Over_Speed_Enable"], 0);
            _paramsAddr["Alarm_Over_Speed"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Alarm_Over_Speed"], 100);
            _paramsAddr["Alarm_Under_Alt_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Alarm_Under_Alt_Enable"], 0);
            _paramsAddr["Alarm_Under_Alt"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Alarm_Under_Alt"], 10);
            _paramsAddr["Alarm_Over_Alt_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Alarm_Over_Alt_Enable"], 0);
            _paramsAddr["Alarm_Over_Alt"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["Alarm_Over_Alt"], 1000);

            _paramsAddr["ClimbRate_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["ClimbRate_Enable"], 1);
            _paramsAddr["ClimbRate_Panel"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["ClimbRate_Panel"], 1);
            _paramsAddr["ClimbRate_H_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["ClimbRate_H_Position"], 5);
            _paramsAddr["ClimbRate_V_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["ClimbRate_V_Position"], 220);
            _paramsAddr["ClimbRate_Font_Size"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["ClimbRate_Font_Size"], 0);
            //_paramsAddr["ClimbRate_H_Alignment"] = address; address += 2;
            //u16toEPPROM(paramdefault, (int)_paramsAddr["ClimbRate_H_Alignment"], 0);

            _paramsAddr["RSSI_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["RSSI_Enable"], 0);
            _paramsAddr["RSSI_Panel"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["RSSI_Panel"], 1);
            _paramsAddr["RSSI_H_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["RSSI_H_Position"], 70);
            _paramsAddr["RSSI_V_Position"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["RSSI_V_Position"], 220);
            _paramsAddr["RSSI_Font_Size"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["RSSI_Font_Size"], 0);
            _paramsAddr["RSSI_H_Alignment"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["RSSI_H_Alignment"], 0);
            _paramsAddr["RSSI_Min"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["RSSI_Min"], 0);
            _paramsAddr["RSSI_Max"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["RSSI_Max"], 255);
            _paramsAddr["RSSI_Raw_Enable"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["RSSI_Raw_Enable"], 0);

            _paramsAddr["FC_Type"] = address; address += 2;
            u16toEPPROM(paramdefault, (int)_paramsAddr["FC_Type"], 0);
        }

        internal PlayuavOSD.data genChildData(string root, string name, string value, string unit, string range, string desc)
        {
            data data = new PlayuavOSD.data();
            data.root = root;
            data.paramname = name;
            data.Value = value;
            data.unit = unit;
            data.range = range;
            data.desc = desc;
            return data;
        }

        internal void processToScreen()
        {
            set_widget_lang();

            Params.Items.Clear();

            Params.Objects.ForEach(x => { Params.RemoveObject(x); });

            Params.CellEditActivation = BrightIdeasSoftware.ObjectListView.CellEditActivateMode.SingleClick;

            Params.CanExpandGetter = delegate(object x)
            {
                data y = (data)x;
                if (y.children != null && y.children.Count > 0)
                    return true;
                return false;
            };

            this.cbx_fc.SelectedIndex = getU16Param(eeprom, (int)_paramsAddr["FC_Type"]);

            Params.ChildrenGetter = delegate(object x)
            {
                data y = (data)x;
                return new ArrayList(y.children);
            };

            List<data> roots = new List<data>();

            data dataAtti = new PlayuavOSD.data();
            dataAtti.paramname = "Attitude";
            dataAtti.desc = lang.getLangStr("Attitude");
            dataAtti.children.Add(genChildData(dataAtti.paramname, "MP_Enable", getU16ParamString(eeprom, (int)_paramsAddr["Attitude_MP_Enable"]), "", "0, 1", lang.getLangStr("Attitude_MP_Enable")));
            dataAtti.children.Add(genChildData(dataAtti.paramname, "MP_Panel", getU16PanelString(eeprom, (int)_paramsAddr["Attitude_MP_Panel"]), "", "1 - Max_Panels", lang.getLangStr("panel")));
            dataAtti.children.Add(genChildData(dataAtti.paramname, "MP_Mode", getU16ParamString(eeprom, (int)_paramsAddr["Attitude_MP_Mode"]), "", "0, 1", lang.getLangStr("Attitude_MP_Mode")));
            dataAtti.children.Add(genChildData(dataAtti.paramname, "3D_Enable", getU16ParamString(eeprom, (int)_paramsAddr["Attitude_3D_Enable"]), "", "0, 1", lang.getLangStr("Attitude_3D_Enable")));
            dataAtti.children.Add(genChildData(dataAtti.paramname, "3D_Panel", getU16ParamString(eeprom, (int)_paramsAddr["Attitude_3D_Panel"]), "", "1 - Max_Panels", lang.getLangStr("panel")));
            roots.Add(dataAtti);

            data dataMisc = new PlayuavOSD.data();
            dataMisc.paramname = "Misc";
            dataMisc.desc = lang.getLangStr("Misc");
            dataMisc.children.Add(genChildData(dataMisc.paramname, "Units_Mode", getU16ParamString(eeprom, (int)_paramsAddr["Misc_Units_Mode"]), "", "0, 1", lang.getLangStr("Misc_Units_Mode")));
            dataMisc.children.Add(genChildData(dataMisc.paramname, "Max_Panels", getU16ParamString(eeprom, (int)_paramsAddr["Misc_Max_Panels"]), "", ">=1", lang.getLangStr("Misc_Max_Panels")));
            roots.Add(dataMisc);

            data dataPWM = new PlayuavOSD.data();
            dataPWM.paramname = "PWM";
            dataPWM.desc = lang.getLangStr("PWM");
            dataPWM.children.Add(genChildData(dataPWM.paramname, "Video_Enable", getU16ParamString(eeprom, (int)_paramsAddr["PWM_Video_Enable"]), "", "0, 1", lang.getLangStr("enable")));
            dataPWM.children.Add(genChildData(dataPWM.paramname, "Video_Chanel", getU16ParamString(eeprom, (int)_paramsAddr["PWM_Video_Chanel"]), "", "1-8", lang.getLangStr("PWM_Video_Chanel")));
            dataPWM.children.Add(genChildData(dataPWM.paramname, "Video_Value", getU16ParamString(eeprom, (int)_paramsAddr["PWM_Video_Value"]), "", "", lang.getLangStr("PWM_Video_Value")));
            dataPWM.children.Add(genChildData(dataPWM.paramname, "Panel_Enable", getU16ParamString(eeprom, (int)_paramsAddr["PWM_Panel_Enable"]), "", "0, 1", lang.getLangStr("enable")));
            dataPWM.children.Add(genChildData(dataPWM.paramname, "Panel_Chanel", getU16ParamString(eeprom, (int)_paramsAddr["PWM_Panel_Chanel"]), "", "1-8", lang.getLangStr("PWM_Panel_Chanel")));
            dataPWM.children.Add(genChildData(dataPWM.paramname, "Panel_Value", getU16ParamString(eeprom, (int)_paramsAddr["PWM_Panel_Value"]), "", "", lang.getLangStr("PWM_Panel_Value")));
            roots.Add(dataPWM);

            data dataArm = new PlayuavOSD.data();
            dataArm.paramname = "ArmState";
            dataArm.desc = lang.getLangStr("ArmState");
            dataArm.children.Add(genChildData(dataArm.paramname, "Enable", getU16ParamString(eeprom, (int)_paramsAddr["ArmState_Enable"]), "", "0, 1", lang.getLangStr("enable")));
            dataArm.children.Add(genChildData(dataArm.paramname, "Panel", getU16PanelString(eeprom, (int)_paramsAddr["ArmState_Panel"]), "", "1 - Max_Panels", lang.getLangStr("panel")));
            dataArm.children.Add(genChildData(dataArm.paramname, "H_Position", getU16ParamString(eeprom, (int)_paramsAddr["ArmState_H_Position"]), "", "0 - 350", lang.getLangStr("hpos")));
            dataArm.children.Add(genChildData(dataArm.paramname, "V_Position", getU16ParamString(eeprom, (int)_paramsAddr["ArmState_V_Position"]), "", "0 - 230", lang.getLangStr("vpos")));
            dataArm.children.Add(genChildData(dataArm.paramname, "Font_Size", getU16ParamString(eeprom, (int)_paramsAddr["ArmState_Font_Size"]), "", "0, 1, 2", lang.getLangStr("font")));
            dataArm.children.Add(genChildData(dataArm.paramname, "H_Alignment", getU16ParamString(eeprom, (int)_paramsAddr["ArmState_H_Alignment"]), "", "0, 1, 2", lang.getLangStr("halign")));
            roots.Add(dataArm);

            data dataBattVolt = new PlayuavOSD.data();
            dataBattVolt.paramname = "BatteryVoltage";
            dataBattVolt.desc = lang.getLangStr("BatteryVoltage");
            dataBattVolt.children.Add(genChildData(dataBattVolt.paramname, "Enable", getU16ParamString(eeprom, (int)_paramsAddr["BatteryVoltage_Enable"]), "", "0, 1", lang.getLangStr("enable")));
            dataBattVolt.children.Add(genChildData(dataBattVolt.paramname, "Panel", getU16PanelString(eeprom, (int)_paramsAddr["BatteryVoltage_Panel"]), "", "1 - Max_Panels", lang.getLangStr("panel")));
            dataBattVolt.children.Add(genChildData(dataBattVolt.paramname, "H_Position", getU16ParamString(eeprom, (int)_paramsAddr["BatteryVoltage_H_Position"]), "", "0 - 350", lang.getLangStr("hpos")));
            dataBattVolt.children.Add(genChildData(dataBattVolt.paramname, "V_Position", getU16ParamString(eeprom, (int)_paramsAddr["BatteryVoltage_V_Position"]), "", "0 - 230", lang.getLangStr("vpos")));
            dataBattVolt.children.Add(genChildData(dataBattVolt.paramname, "Font_Size", getU16ParamString(eeprom, (int)_paramsAddr["BatteryVoltage_Font_Size"]), "", "0, 1, 2", lang.getLangStr("font")));
            dataBattVolt.children.Add(genChildData(dataBattVolt.paramname, "H_Alignment", getU16ParamString(eeprom, (int)_paramsAddr["BatteryVoltage_H_Alignment"]), "", "0, 1, 2", lang.getLangStr("halign")));
            roots.Add(dataBattVolt);

            data dataBattCurrent = new PlayuavOSD.data();
            dataBattCurrent.paramname = "BatteryCurrent";
            dataBattCurrent.desc = lang.getLangStr("BatteryCurrent");
            dataBattCurrent.children.Add(genChildData(dataBattCurrent.paramname, "Enable", getU16ParamString(eeprom, (int)_paramsAddr["BatteryCurrent_Enable"]), "", "0, 1", lang.getLangStr("enable")));
            dataBattCurrent.children.Add(genChildData(dataBattCurrent.paramname, "Panel", getU16PanelString(eeprom, (int)_paramsAddr["BatteryCurrent_Panel"]), "", "1 - Max_Panels", lang.getLangStr("panel")));
            dataBattCurrent.children.Add(genChildData(dataBattCurrent.paramname, "H_Position", getU16ParamString(eeprom, (int)_paramsAddr["BatteryCurrent_H_Position"]), "", "0 - 350", lang.getLangStr("hpos")));
            dataBattCurrent.children.Add(genChildData(dataBattCurrent.paramname, "V_Position", getU16ParamString(eeprom, (int)_paramsAddr["BatteryCurrent_V_Position"]), "", "0 - 230", lang.getLangStr("vpos")));
            dataBattCurrent.children.Add(genChildData(dataBattCurrent.paramname, "Font_Size", getU16ParamString(eeprom, (int)_paramsAddr["BatteryCurrent_Font_Size"]), "", "0, 1, 2", lang.getLangStr("font")));
            dataBattCurrent.children.Add(genChildData(dataBattCurrent.paramname, "H_Alignment", getU16ParamString(eeprom, (int)_paramsAddr["BatteryCurrent_H_Alignment"]), "", "0, 1, 2", lang.getLangStr("halign")));
            roots.Add(dataBattCurrent);

            data dataBattConsumed = new PlayuavOSD.data();
            dataBattConsumed.paramname = "BatteryConsumed";
            dataBattConsumed.desc = lang.getLangStr("BatteryConsumed");
            dataBattConsumed.children.Add(genChildData(dataBattConsumed.paramname, "Enable", getU16ParamString(eeprom, (int)_paramsAddr["BatteryConsumed_Enable"]), "", "0, 1", lang.getLangStr("enable")));
            dataBattConsumed.children.Add(genChildData(dataBattConsumed.paramname, "Panel", getU16PanelString(eeprom, (int)_paramsAddr["BatteryConsumed_Panel"]), "", "1 - Max_Panels", lang.getLangStr("panel")));
            dataBattConsumed.children.Add(genChildData(dataBattConsumed.paramname, "H_Position", getU16ParamString(eeprom, (int)_paramsAddr["BatteryConsumed_H_Position"]), "", "0 - 350", lang.getLangStr("hpos")));
            dataBattConsumed.children.Add(genChildData(dataBattConsumed.paramname, "V_Position", getU16ParamString(eeprom, (int)_paramsAddr["BatteryConsumed_V_Position"]), "", "0 - 230", lang.getLangStr("vpos")));
            dataBattConsumed.children.Add(genChildData(dataBattConsumed.paramname, "Font_Size", getU16ParamString(eeprom, (int)_paramsAddr["BatteryConsumed_Font_Size"]), "", "0, 1, 2", lang.getLangStr("font")));
            dataBattConsumed.children.Add(genChildData(dataBattConsumed.paramname, "H_Alignment", getU16ParamString(eeprom, (int)_paramsAddr["BatteryConsumed_H_Alignment"]), "", "0, 1, 2", lang.getLangStr("halign")));
            roots.Add(dataBattConsumed);

            data dataFlightMode = new PlayuavOSD.data();
            dataFlightMode.paramname = "FlightMode";
            dataFlightMode.desc = lang.getLangStr("FlightMode");
            dataFlightMode.children.Add(genChildData(dataFlightMode.paramname, "Enable", getU16ParamString(eeprom, (int)_paramsAddr["FlightMode_Enable"]), "", "0, 1", lang.getLangStr("enable")));
            dataFlightMode.children.Add(genChildData(dataFlightMode.paramname, "Panel", getU16PanelString(eeprom, (int)_paramsAddr["FlightMode_Panel"]), "", "1 - Max_Panels", lang.getLangStr("panel")));
            dataFlightMode.children.Add(genChildData(dataFlightMode.paramname, "H_Position", getU16ParamString(eeprom, (int)_paramsAddr["FlightMode_H_Position"]), "", "0 - 350", lang.getLangStr("hpos")));
            dataFlightMode.children.Add(genChildData(dataFlightMode.paramname, "V_Position", getU16ParamString(eeprom, (int)_paramsAddr["FlightMode_V_Position"]), "", "0 - 230", lang.getLangStr("vpos")));
            dataFlightMode.children.Add(genChildData(dataFlightMode.paramname, "Font_Size", getU16ParamString(eeprom, (int)_paramsAddr["FlightMode_Font_Size"]), "", "0, 1, 2", lang.getLangStr("font")));
            dataFlightMode.children.Add(genChildData(dataFlightMode.paramname, "H_Alignment", getU16ParamString(eeprom, (int)_paramsAddr["FlightMode_H_Alignment"]), "", "0, 1, 2", lang.getLangStr("halign")));
            roots.Add(dataFlightMode);

            data dataGPSStatus = new PlayuavOSD.data();
            dataGPSStatus.paramname = "GPSStatus";
            dataGPSStatus.desc = lang.getLangStr("GPSStatus");
            dataGPSStatus.children.Add(genChildData(dataGPSStatus.paramname, "Enable", getU16ParamString(eeprom, (int)_paramsAddr["GPSStatus_Enable"]), "", "0, 1", lang.getLangStr("enable")));
            dataGPSStatus.children.Add(genChildData(dataGPSStatus.paramname, "Panel", getU16PanelString(eeprom, (int)_paramsAddr["GPSStatus_Panel"]), "", "1 - Max_Panels", lang.getLangStr("panel")));
            dataGPSStatus.children.Add(genChildData(dataGPSStatus.paramname, "H_Position", getU16ParamString(eeprom, (int)_paramsAddr["GPSStatus_H_Position"]), "", "0 - 350", lang.getLangStr("hpos")));
            dataGPSStatus.children.Add(genChildData(dataGPSStatus.paramname, "V_Position", getU16ParamString(eeprom, (int)_paramsAddr["GPSStatus_V_Position"]), "", "0 - 230", lang.getLangStr("vpos")));
            dataGPSStatus.children.Add(genChildData(dataGPSStatus.paramname, "Font_Size", getU16ParamString(eeprom, (int)_paramsAddr["GPSStatus_Font_Size"]), "", "0, 1, 2", lang.getLangStr("font")));
            dataGPSStatus.children.Add(genChildData(dataGPSStatus.paramname, "H_Alignment", getU16ParamString(eeprom, (int)_paramsAddr["GPSStatus_H_Alignment"]), "", "0, 1, 2", lang.getLangStr("halign")));
            roots.Add(dataGPSStatus);

            data dataGPSHDOP = new PlayuavOSD.data();
            dataGPSHDOP.paramname = "GPSHDOP";
            dataGPSHDOP.desc = lang.getLangStr("GPSHDOP");
            dataGPSHDOP.children.Add(genChildData(dataGPSHDOP.paramname, "Enable", getU16ParamString(eeprom, (int)_paramsAddr["GPSHDOP_Enable"]), "", "0, 1", lang.getLangStr("enable")));
            dataGPSHDOP.children.Add(genChildData(dataGPSHDOP.paramname, "Panel", getU16PanelString(eeprom, (int)_paramsAddr["GPSHDOP_Panel"]), "", "1 - Max_Panels", lang.getLangStr("panel")));
            dataGPSHDOP.children.Add(genChildData(dataGPSHDOP.paramname, "H_Position", getU16ParamString(eeprom, (int)_paramsAddr["GPSHDOP_H_Position"]), "", "0 - 350", lang.getLangStr("hpos")));
            dataGPSHDOP.children.Add(genChildData(dataGPSHDOP.paramname, "V_Position", getU16ParamString(eeprom, (int)_paramsAddr["GPSHDOP_V_Position"]), "", "0 - 230", lang.getLangStr("vpos")));
            dataGPSHDOP.children.Add(genChildData(dataGPSHDOP.paramname, "Font_Size", getU16ParamString(eeprom, (int)_paramsAddr["GPSHDOP_Font_Size"]), "", "0, 1, 2", lang.getLangStr("font")));
            dataGPSHDOP.children.Add(genChildData(dataGPSHDOP.paramname, "H_Alignment", getU16ParamString(eeprom, (int)_paramsAddr["GPSHDOP_H_Alignment"]), "", "0, 1, 2", lang.getLangStr("halign")));
            roots.Add(dataGPSHDOP);

            data dataGPSLat = new PlayuavOSD.data();
            dataGPSLat.paramname = "GPSLatitude";
            dataGPSLat.desc = lang.getLangStr("GPSLatitude");
            dataGPSLat.children.Add(genChildData(dataGPSLat.paramname, "Enable", getU16ParamString(eeprom, (int)_paramsAddr["GPSLatitude_Enable"]), "", "0, 1", lang.getLangStr("enable")));
            dataGPSLat.children.Add(genChildData(dataGPSLat.paramname, "Panel", getU16PanelString(eeprom, (int)_paramsAddr["GPSLatitude_Panel"]), "", "1 - Max_Panels", lang.getLangStr("panel")));
            dataGPSLat.children.Add(genChildData(dataGPSLat.paramname, "H_Position", getU16ParamString(eeprom, (int)_paramsAddr["GPSLatitude_H_Position"]), "", "0 - 350", lang.getLangStr("hpos")));
            dataGPSLat.children.Add(genChildData(dataGPSLat.paramname, "V_Position", getU16ParamString(eeprom, (int)_paramsAddr["GPSLatitude_V_Position"]), "", "0 - 230", lang.getLangStr("vpos")));
            dataGPSLat.children.Add(genChildData(dataGPSLat.paramname, "Font_Size", getU16ParamString(eeprom, (int)_paramsAddr["GPSLatitude_Font_Size"]), "", "0, 1, 2", lang.getLangStr("font")));
            dataGPSLat.children.Add(genChildData(dataGPSLat.paramname, "H_Alignment", getU16ParamString(eeprom, (int)_paramsAddr["GPSLatitude_H_Alignment"]), "", "0, 1, 2", lang.getLangStr("halign")));
            roots.Add(dataGPSLat);

            data dataGPSLon = new PlayuavOSD.data();
            dataGPSLon.paramname = "GPSLongitude";
            dataGPSLon.desc = lang.getLangStr("GPSLongitude");
            dataGPSLon.children.Add(genChildData(dataGPSLon.paramname, "Enable", getU16ParamString(eeprom, (int)_paramsAddr["GPSLongitude_Enable"]), "", "0, 1", lang.getLangStr("enable")));
            dataGPSLon.children.Add(genChildData(dataGPSLon.paramname, "Panel", getU16PanelString(eeprom, (int)_paramsAddr["GPSLongitude_Panel"]), "", "1 - Max_Panels", lang.getLangStr("panel")));
            dataGPSLon.children.Add(genChildData(dataGPSLon.paramname, "H_Position", getU16ParamString(eeprom, (int)_paramsAddr["GPSLongitude_H_Position"]), "", "0 - 350", lang.getLangStr("hpos")));
            dataGPSLon.children.Add(genChildData(dataGPSLon.paramname, "V_Position", getU16ParamString(eeprom, (int)_paramsAddr["GPSLongitude_V_Position"]), "", "0 - 230", lang.getLangStr("vpos")));
            dataGPSLon.children.Add(genChildData(dataGPSLon.paramname, "Font_Size", getU16ParamString(eeprom, (int)_paramsAddr["GPSLongitude_Font_Size"]), "", "0, 1, 2", lang.getLangStr("font")));
            dataGPSLon.children.Add(genChildData(dataGPSLon.paramname, "H_Alignment", getU16ParamString(eeprom, (int)_paramsAddr["GPSLongitude_H_Alignment"]), "", "0, 1, 2", lang.getLangStr("halign")));
            roots.Add(dataGPSLon);

            data dataGPS2Status = new PlayuavOSD.data();
            dataGPS2Status.paramname = "GPS2Status";
            dataGPS2Status.desc = lang.getLangStr("GPS2Status");
            dataGPS2Status.children.Add(genChildData(dataGPS2Status.paramname, "Enable", getU16ParamString(eeprom, (int)_paramsAddr["GPS2Status_Enable"]), "", "0, 1", lang.getLangStr("enable")));
            dataGPS2Status.children.Add(genChildData(dataGPS2Status.paramname, "Panel", getU16PanelString(eeprom, (int)_paramsAddr["GPS2Status_Panel"]), "", "1 - Max_Panels", lang.getLangStr("panel")));
            dataGPS2Status.children.Add(genChildData(dataGPS2Status.paramname, "H_Position", getU16ParamString(eeprom, (int)_paramsAddr["GPS2Status_H_Position"]), "", "0 - 350", lang.getLangStr("hpos")));
            dataGPS2Status.children.Add(genChildData(dataGPS2Status.paramname, "V_Position", getU16ParamString(eeprom, (int)_paramsAddr["GPS2Status_V_Position"]), "", "0 - 230", lang.getLangStr("vpos")));
            dataGPS2Status.children.Add(genChildData(dataGPS2Status.paramname, "Font_Size", getU16ParamString(eeprom, (int)_paramsAddr["GPS2Status_Font_Size"]), "", "0, 1, 2", lang.getLangStr("font")));
            dataGPS2Status.children.Add(genChildData(dataGPS2Status.paramname, "H_Alignment", getU16ParamString(eeprom, (int)_paramsAddr["GPS2Status_H_Alignment"]), "", "0, 1, 2", lang.getLangStr("halign")));
            roots.Add(dataGPS2Status);

            data dataGPS2HDOP = new PlayuavOSD.data();
            dataGPS2HDOP.paramname = "GPS2HDOP";
            dataGPS2HDOP.desc = lang.getLangStr("GPS2HDOP");
            dataGPS2HDOP.children.Add(genChildData(dataGPS2HDOP.paramname, "Enable", getU16ParamString(eeprom, (int)_paramsAddr["GPS2HDOP_Enable"]), "", "0, 1", lang.getLangStr("enable")));
            dataGPS2HDOP.children.Add(genChildData(dataGPS2HDOP.paramname, "Panel", getU16PanelString(eeprom, (int)_paramsAddr["GPS2HDOP_Panel"]), "", "1 - Max_Panels", lang.getLangStr("panel")));
            dataGPS2HDOP.children.Add(genChildData(dataGPS2HDOP.paramname, "H_Position", getU16ParamString(eeprom, (int)_paramsAddr["GPS2HDOP_H_Position"]), "", "0 - 350", lang.getLangStr("hpos")));
            dataGPS2HDOP.children.Add(genChildData(dataGPS2HDOP.paramname, "V_Position", getU16ParamString(eeprom, (int)_paramsAddr["GPS2HDOP_V_Position"]), "", "0 - 230", lang.getLangStr("vpos")));
            dataGPS2HDOP.children.Add(genChildData(dataGPS2HDOP.paramname, "Font_Size", getU16ParamString(eeprom, (int)_paramsAddr["GPS2HDOP_Font_Size"]), "", "0, 1, 2", lang.getLangStr("font")));
            dataGPS2HDOP.children.Add(genChildData(dataGPS2HDOP.paramname, "H_Alignment", getU16ParamString(eeprom, (int)_paramsAddr["GPS2HDOP_H_Alignment"]), "", "0, 1, 2", lang.getLangStr("halign")));
            roots.Add(dataGPS2HDOP);

            data dataGPS2Lat = new PlayuavOSD.data();
            dataGPS2Lat.paramname = "GPS2Latitude";
            dataGPS2Lat.desc = lang.getLangStr("GPS2Latitude");
            dataGPS2Lat.children.Add(genChildData(dataGPS2Lat.paramname, "Enable", getU16ParamString(eeprom, (int)_paramsAddr["GPS2Latitude_Enable"]), "", "0, 1", lang.getLangStr("enable")));
            dataGPS2Lat.children.Add(genChildData(dataGPS2Lat.paramname, "Panel", getU16PanelString(eeprom, (int)_paramsAddr["GPS2Latitude_Panel"]), "", "1 - Max_Panels", lang.getLangStr("panel")));
            dataGPS2Lat.children.Add(genChildData(dataGPS2Lat.paramname, "H_Position", getU16ParamString(eeprom, (int)_paramsAddr["GPS2Latitude_H_Position"]), "", "0 - 350", lang.getLangStr("hpos")));
            dataGPS2Lat.children.Add(genChildData(dataGPS2Lat.paramname, "V_Position", getU16ParamString(eeprom, (int)_paramsAddr["GPS2Latitude_V_Position"]), "", "0 - 230", lang.getLangStr("vpos")));
            dataGPS2Lat.children.Add(genChildData(dataGPS2Lat.paramname, "Font_Size", getU16ParamString(eeprom, (int)_paramsAddr["GPS2Latitude_Font_Size"]), "", "0, 1, 2", lang.getLangStr("font")));
            dataGPS2Lat.children.Add(genChildData(dataGPS2Lat.paramname, "H_Alignment", getU16ParamString(eeprom, (int)_paramsAddr["GPS2Latitude_H_Alignment"]), "", "0, 1, 2", lang.getLangStr("halign")));
            roots.Add(dataGPS2Lat);

            data dataGPS2Lon = new PlayuavOSD.data();
            dataGPS2Lon.paramname = "GPS2Longitude";
            dataGPS2Lon.desc = lang.getLangStr("GPS2Longitude");
            dataGPS2Lon.children.Add(genChildData(dataGPS2Lon.paramname, "Enable", getU16ParamString(eeprom, (int)_paramsAddr["GPS2Longitude_Enable"]), "", "0, 1", lang.getLangStr("enable")));
            dataGPS2Lon.children.Add(genChildData(dataGPS2Lon.paramname, "Panel", getU16PanelString(eeprom, (int)_paramsAddr["GPS2Longitude_Panel"]), "", "1 - Max_Panels", lang.getLangStr("panel")));
            dataGPS2Lon.children.Add(genChildData(dataGPS2Lon.paramname, "H_Position", getU16ParamString(eeprom, (int)_paramsAddr["GPS2Longitude_H_Position"]), "", "0 - 350", lang.getLangStr("hpos")));
            dataGPS2Lon.children.Add(genChildData(dataGPS2Lon.paramname, "V_Position", getU16ParamString(eeprom, (int)_paramsAddr["GPS2Longitude_V_Position"]), "", "0 - 230", lang.getLangStr("vpos")));
            dataGPS2Lon.children.Add(genChildData(dataGPS2Lon.paramname, "Font_Size", getU16ParamString(eeprom, (int)_paramsAddr["GPS2Longitude_Font_Size"]), "", "0, 1, 2", lang.getLangStr("font")));
            dataGPS2Lon.children.Add(genChildData(dataGPS2Lon.paramname, "H_Alignment", getU16ParamString(eeprom, (int)_paramsAddr["GPS2Longitude_H_Alignment"]), "", "0, 1, 2", lang.getLangStr("halign")));
            roots.Add(dataGPS2Lon);

            data dataTime = new PlayuavOSD.data();
            dataTime.paramname = "Time";
            dataTime.desc = lang.getLangStr("Time");
            dataTime.children.Add(genChildData(dataTime.paramname, "Enable", getU16ParamString(eeprom, (int)_paramsAddr["Time_Enable"]), "", "0, 1", lang.getLangStr("enable")));
            dataTime.children.Add(genChildData(dataTime.paramname, "Panel", getU16PanelString(eeprom, (int)_paramsAddr["Time_Panel"]), "", "1 - Max_Panels", lang.getLangStr("panel")));
            dataTime.children.Add(genChildData(dataTime.paramname, "H_Position", getU16ParamString(eeprom, (int)_paramsAddr["Time_H_Position"]), "", "0 - 350", lang.getLangStr("hpos")));
            dataTime.children.Add(genChildData(dataTime.paramname, "V_Position", getU16ParamString(eeprom, (int)_paramsAddr["Time_V_Position"]), "", "0 - 230", lang.getLangStr("vpos")));
            dataTime.children.Add(genChildData(dataTime.paramname, "Font_Size", getU16ParamString(eeprom, (int)_paramsAddr["Time_Font_Size"]), "", "0, 1, 2", lang.getLangStr("font")));
            dataTime.children.Add(genChildData(dataTime.paramname, "H_Alignment", getU16ParamString(eeprom, (int)_paramsAddr["Time_H_Alignment"]), "", "0, 1, 2", lang.getLangStr("halign")));
            roots.Add(dataTime);

            data dataAlt = new PlayuavOSD.data();
            dataAlt.paramname = "Altitude";
            dataAlt.desc = lang.getLangStr("Altitude");
            dataAlt.children.Add(genChildData(dataAlt.paramname, "TALT_Enable", getU16ParamString(eeprom, (int)_paramsAddr["Altitude_TALT_Enable"]), "", "0, 1", lang.getLangStr("Altitude_TALT_Enable")));
            dataAlt.children.Add(genChildData(dataAlt.paramname, "TALT_Panel", getU16PanelString(eeprom, (int)_paramsAddr["Altitude_TALT_Panel"]), "", "1 - Max_Panels", lang.getLangStr("panel")));
            dataAlt.children.Add(genChildData(dataAlt.paramname, "TALT_H_Position", getU16ParamString(eeprom, (int)_paramsAddr["Altitude_TALT_H_Position"]), "", "0 - 350", lang.getLangStr("hpos")));
            dataAlt.children.Add(genChildData(dataAlt.paramname, "TALT_V_Position", getU16ParamString(eeprom, (int)_paramsAddr["Altitude_TALT_V_Position"]), "", "0 - 230", lang.getLangStr("vpos")));
            dataAlt.children.Add(genChildData(dataAlt.paramname, "TALT_Font_Size", getU16ParamString(eeprom, (int)_paramsAddr["Altitude_TALT_Font_Size"]), "", "0, 1, 2", lang.getLangStr("font")));
            dataAlt.children.Add(genChildData(dataAlt.paramname, "TALT_H_Alignment", getU16ParamString(eeprom, (int)_paramsAddr["Altitude_TALT_H_Alignment"]), "", "0, 1, 2", lang.getLangStr("halign")));
            dataAlt.children.Add(genChildData(dataAlt.paramname, "Scale_Enable", getU16ParamString(eeprom, (int)_paramsAddr["Altitude_Scale_Enable"]), "", "0, 1", lang.getLangStr("Altitude_Scale_Enable")));
            dataAlt.children.Add(genChildData(dataAlt.paramname, "Scale_Panel", getU16PanelString(eeprom, (int)_paramsAddr["Altitude_Scale_Panel"]), "", "1 - Max_Panels", lang.getLangStr("panel")));
            dataAlt.children.Add(genChildData(dataAlt.paramname, "Scale_H_Position", getU16ParamString(eeprom, (int)_paramsAddr["Altitude_Scale_H_Position"]), "", "0 - 350", lang.getLangStr("hpos")));
            dataAlt.children.Add(genChildData(dataAlt.paramname, "Scale_Align", getU16ParamString(eeprom, (int)_paramsAddr["Altitude_Scale_Align"]), "", "0, 1", lang.getLangStr("Scale_Align")));
            roots.Add(dataAlt);

            data dataSpeed = new PlayuavOSD.data();
            dataSpeed.paramname = "Speed";
            dataSpeed.desc = lang.getLangStr("Speed");
            dataSpeed.children.Add(genChildData(dataSpeed.paramname, "TSPD_Enable", getU16ParamString(eeprom, (int)_paramsAddr["Speed_TSPD_Enable"]), "", "0, 1", lang.getLangStr("Speed_TSPD_Enable")));
            dataSpeed.children.Add(genChildData(dataSpeed.paramname, "TSPD_Panel", getU16PanelString(eeprom, (int)_paramsAddr["Speed_TSPD_Panel"]), "", "1 - Max_Panels", lang.getLangStr("panel")));
            dataSpeed.children.Add(genChildData(dataSpeed.paramname, "TSPD_H_Position", getU16ParamString(eeprom, (int)_paramsAddr["Speed_TSPD_H_Position"]), "", "0 - 350", lang.getLangStr("hpos")));
            dataSpeed.children.Add(genChildData(dataSpeed.paramname, "TSPD_V_Position", getU16ParamString(eeprom, (int)_paramsAddr["Speed_TSPD_V_Position"]), "", "0 - 230", lang.getLangStr("vpos")));
            dataSpeed.children.Add(genChildData(dataSpeed.paramname, "TSPD_Font_Size", getU16ParamString(eeprom, (int)_paramsAddr["Speed_TSPD_Font_Size"]), "", "0, 1, 2", lang.getLangStr("font")));
            dataSpeed.children.Add(genChildData(dataSpeed.paramname, "TSPD_H_Alignment", getU16ParamString(eeprom, (int)_paramsAddr["Speed_TSPD_H_Alignment"]), "", "0, 1, 2", lang.getLangStr("halign")));
            dataSpeed.children.Add(genChildData(dataSpeed.paramname, "Scale_Enable", getU16ParamString(eeprom, (int)_paramsAddr["Speed_Scale_Enable"]), "", "0, 1", lang.getLangStr("Speed_Scale_Enable")));
            dataSpeed.children.Add(genChildData(dataSpeed.paramname, "Scale_Panel", getU16PanelString(eeprom, (int)_paramsAddr["Speed_Scale_Panel"]), "", "1 - Max_Panels", lang.getLangStr("panel")));
            dataSpeed.children.Add(genChildData(dataSpeed.paramname, "Scale_H_Position", getU16ParamString(eeprom, (int)_paramsAddr["Speed_Scale_H_Position"]), "", "0 - 350", lang.getLangStr("hpos")));
            dataSpeed.children.Add(genChildData(dataSpeed.paramname, "Scale_Align", getU16ParamString(eeprom, (int)_paramsAddr["Speed_Scale_Align"]), "", "0, 1", lang.getLangStr("Scale_Align")));
            roots.Add(dataSpeed);

            data dataThrottle = new PlayuavOSD.data();
            dataThrottle.paramname = "Throttle";
            dataThrottle.desc = lang.getLangStr("Throttle");
            dataThrottle.children.Add(genChildData(dataThrottle.paramname, "Enable", getU16ParamString(eeprom, (int)_paramsAddr["Throttle_Enable"]), "", "0, 1", lang.getLangStr("enable")));
            dataThrottle.children.Add(genChildData(dataThrottle.paramname, "Panel", getU16PanelString(eeprom, (int)_paramsAddr["Throttle_Panel"]), "", "1 - Max_Panels", lang.getLangStr("panel")));
            dataThrottle.children.Add(genChildData(dataThrottle.paramname, "Scale_Enable", getU16ParamString(eeprom, (int)_paramsAddr["Throttle_Scale_Enable"]), "", "0, 1", lang.getLangStr("Throttle_Scale_Enable")));
            dataThrottle.children.Add(genChildData(dataThrottle.paramname, "H_Position", getU16ParamString(eeprom, (int)_paramsAddr["Throttle_H_Position"]), "", "0 - 350", lang.getLangStr("hpos")));
            dataThrottle.children.Add(genChildData(dataThrottle.paramname, "V_Position", getU16ParamString(eeprom, (int)_paramsAddr["Throttle_V_Position"]), "", "0 - 230", lang.getLangStr("vpos")));
            roots.Add(dataThrottle);

            data dataHomeDist = new PlayuavOSD.data();
            dataHomeDist.paramname = "HomeDistance";
            dataHomeDist.desc = lang.getLangStr("HomeDistance");
            dataHomeDist.children.Add(genChildData(dataHomeDist.paramname, "Enable", getU16ParamString(eeprom, (int)_paramsAddr["HomeDistance_Enable"]), "", "0, 1", lang.getLangStr("enable")));
            dataHomeDist.children.Add(genChildData(dataHomeDist.paramname, "Panel", getU16PanelString(eeprom, (int)_paramsAddr["HomeDistance_Panel"]), "", "1 - Max_Panels", lang.getLangStr("panel")));
            dataHomeDist.children.Add(genChildData(dataHomeDist.paramname, "H_Position", getU16ParamString(eeprom, (int)_paramsAddr["HomeDistance_H_Position"]), "", "0 - 350", lang.getLangStr("hpos")));
            dataHomeDist.children.Add(genChildData(dataHomeDist.paramname, "V_Position", getU16ParamString(eeprom, (int)_paramsAddr["HomeDistance_V_Position"]), "", "0 - 230", lang.getLangStr("vpos")));
            dataHomeDist.children.Add(genChildData(dataHomeDist.paramname, "Font_Size", getU16ParamString(eeprom, (int)_paramsAddr["HomeDistance_Font_Size"]), "", "0, 1, 2", lang.getLangStr("font")));
            dataHomeDist.children.Add(genChildData(dataHomeDist.paramname, "H_Alignment", getU16ParamString(eeprom, (int)_paramsAddr["HomeDistance_H_Alignment"]), "", "0, 1, 2", lang.getLangStr("halign")));
            roots.Add(dataHomeDist);

            data dataWPDist = new PlayuavOSD.data();
            dataWPDist.paramname = "WPDistance";
            dataWPDist.desc = lang.getLangStr("WPDistance");
            dataWPDist.children.Add(genChildData(dataWPDist.paramname, "Enable", getU16ParamString(eeprom, (int)_paramsAddr["WPDistance_Enable"]), "", "0, 1", lang.getLangStr("enable")));
            dataWPDist.children.Add(genChildData(dataWPDist.paramname, "Panel", getU16PanelString(eeprom, (int)_paramsAddr["WPDistance_Panel"]), "", "1 - Max_Panels", lang.getLangStr("panel")));
            dataWPDist.children.Add(genChildData(dataWPDist.paramname, "H_Position", getU16ParamString(eeprom, (int)_paramsAddr["WPDistance_H_Position"]), "", "0 - 350", lang.getLangStr("hpos")));
            dataWPDist.children.Add(genChildData(dataWPDist.paramname, "V_Position", getU16ParamString(eeprom, (int)_paramsAddr["WPDistance_V_Position"]), "", "0 - 230", lang.getLangStr("vpos")));
            dataWPDist.children.Add(genChildData(dataWPDist.paramname, "Font_Size", getU16ParamString(eeprom, (int)_paramsAddr["WPDistance_Font_Size"]), "", "0, 1, 2", lang.getLangStr("font")));
            dataWPDist.children.Add(genChildData(dataWPDist.paramname, "H_Alignment", getU16ParamString(eeprom, (int)_paramsAddr["WPDistance_H_Alignment"]), "", "0, 1, 2", lang.getLangStr("halign")));
            roots.Add(dataWPDist);

            data dataDir = new PlayuavOSD.data();
            dataDir.paramname = "CHWDIR";
            dataDir.desc = lang.getLangStr("CHWDIR");
            dataDir.children.Add(genChildData(dataDir.paramname, "Tmode_Enable", getU16ParamString(eeprom, (int)_paramsAddr["CHWDIR_Tmode_Enable"]), "", "0, 1", lang.getLangStr("CHWDIR_Tmode_Enable")));
            dataDir.children.Add(genChildData(dataDir.paramname, "Tmode_Panel", getU16PanelString(eeprom, (int)_paramsAddr["CHWDIR_Tmode_Panel"]), "", "1 - Max_Panels", lang.getLangStr("panel")));
            dataDir.children.Add(genChildData(dataDir.paramname, "Tmode_V_Position", getU16ParamString(eeprom, (int)_paramsAddr["CHWDIR_Tmode_V_Position"]), "", "0 - 230", lang.getLangStr("vpos")));
            dataDir.children.Add(genChildData(dataDir.paramname, "Nmode_Enable", getU16ParamString(eeprom, (int)_paramsAddr["CHWDIR_Nmode_Enable"]), "", "0, 1", lang.getLangStr("CHWDIR_Nmode_Enable")));
            dataDir.children.Add(genChildData(dataDir.paramname, "Nmode_Panel", getU16PanelString(eeprom, (int)_paramsAddr["CHWDIR_Nmode_Panel"]), "", "1 - Max_Panels", lang.getLangStr("panel")));
            dataDir.children.Add(genChildData(dataDir.paramname, "Nmode_H_Position", getU16ParamString(eeprom, (int)_paramsAddr["CHWDIR_Nmode_H_Position"]), "", "0 - 350", lang.getLangStr("hpos")));
            dataDir.children.Add(genChildData(dataDir.paramname, "Nmode_V_Position", getU16ParamString(eeprom, (int)_paramsAddr["CHWDIR_Nmode_V_Position"]), "", "0 - 230", lang.getLangStr("vpos")));
            dataDir.children.Add(genChildData(dataDir.paramname, "Nmode_Radius", getU16ParamString(eeprom, (int)_paramsAddr["CHWDIR_Nmode_Radius"]), "", "0 - 230", lang.getLangStr("CHWDIR_Nmode_Radius")));
            dataDir.children.Add(genChildData(dataDir.paramname, "Nmode_Home_Radius", getU16ParamString(eeprom, (int)_paramsAddr["CHWDIR_Nmode_Home_Radius"]), "", "0 - 230", lang.getLangStr("CHWDIR_Nmode_Home_Radius")));
            dataDir.children.Add(genChildData(dataDir.paramname, "Nmode_WP_Radius", getU16ParamString(eeprom, (int)_paramsAddr["CHWDIR_Nmode_WP_Radius"]), "", "0 - 230", lang.getLangStr("CHWDIR_Nmode_WP_Radius")));
            roots.Add(dataDir);


            data dataAlarm = new PlayuavOSD.data();
            dataAlarm.paramname = "Alarm";
            dataAlarm.desc = lang.getLangStr("Alarm");
            dataAlarm.children.Add(genChildData(dataAlarm.paramname, "H_Position", getU16ParamString(eeprom, (int)_paramsAddr["Alarm_H_Position"]), "", "0 - 350", lang.getLangStr("hpos")));
            dataAlarm.children.Add(genChildData(dataAlarm.paramname, "V_Position", getU16ParamString(eeprom, (int)_paramsAddr["Alarm_V_Position"]), "", "0 - 230", lang.getLangStr("vpos")));
            dataAlarm.children.Add(genChildData(dataAlarm.paramname, "Font_Size", getU16ParamString(eeprom, (int)_paramsAddr["Alarm_Font_Size"]), "", "0, 1, 2", lang.getLangStr("font")));
            dataAlarm.children.Add(genChildData(dataAlarm.paramname, "H_Alignment", getU16ParamString(eeprom, (int)_paramsAddr["Alarm_H_Alignment"]), "", "0, 1, 2", lang.getLangStr("halign")));
            dataAlarm.children.Add(genChildData(dataAlarm.paramname, "GPS_Status_Enable", getU16ParamString(eeprom, (int)_paramsAddr["Alarm_GPS_Status_Enable"]), "", "0, 1", lang.getLangStr("Alarm_GPS_Status_Enable")));
            dataAlarm.children.Add(genChildData(dataAlarm.paramname, "Low_Batt_Enable", getU16ParamString(eeprom, (int)_paramsAddr["Alarm_Low_Batt_Enable"]), "", "0, 1", lang.getLangStr("Alarm_Low_Batt_Enable")));
            dataAlarm.children.Add(genChildData(dataAlarm.paramname, "Low_Batt", getU16ParamString(eeprom, (int)_paramsAddr["Alarm_Low_Batt"]), "", "0 - 99", lang.getLangStr("alarmval")));
            dataAlarm.children.Add(genChildData(dataAlarm.paramname, "Under_Speed_Enable", getU16ParamString(eeprom, (int)_paramsAddr["Alarm_Under_Speed_Enable"]), "", "0, 1", lang.getLangStr("Alarm_Under_Speed_Enable")));
            dataAlarm.children.Add(genChildData(dataAlarm.paramname, "Under_Speed", getU16ParamString(eeprom, (int)_paramsAddr["Alarm_Under_Speed"]), "", "", lang.getLangStr("alarmval")));
            dataAlarm.children.Add(genChildData(dataAlarm.paramname, "Over_Speed_Enable", getU16ParamString(eeprom, (int)_paramsAddr["Alarm_Over_Speed_Enable"]), "", "0, 1", lang.getLangStr("Alarm_Over_Speed_Enable")));
            dataAlarm.children.Add(genChildData(dataAlarm.paramname, "Over_Speed", getU16ParamString(eeprom, (int)_paramsAddr["Alarm_Over_Speed"]), "", "", lang.getLangStr("alarmval")));
            dataAlarm.children.Add(genChildData(dataAlarm.paramname, "Under_Alt_Enable", getU16ParamString(eeprom, (int)_paramsAddr["Alarm_Under_Alt_Enable"]), "", "0, 1", lang.getLangStr("Alarm_Under_Alt_Enable")));
            dataAlarm.children.Add(genChildData(dataAlarm.paramname, "Under_Alt", getU16ParamString(eeprom, (int)_paramsAddr["Alarm_Under_Alt"]), "", "", lang.getLangStr("alarmval")));
            dataAlarm.children.Add(genChildData(dataAlarm.paramname, "Over_Alt_Enable", getU16ParamString(eeprom, (int)_paramsAddr["Alarm_Over_Alt_Enable"]), "", "0, 1", lang.getLangStr("Alarm_Over_Alt_Enable")));
            dataAlarm.children.Add(genChildData(dataAlarm.paramname, "Over_Alt", getU16ParamString(eeprom, (int)_paramsAddr["Alarm_Over_Alt"]), "", "", lang.getLangStr("alarmval")));
            roots.Add(dataAlarm);

            data dataClimb = new PlayuavOSD.data();
            dataClimb.paramname = "ClimbRate";
            dataClimb.desc = lang.getLangStr("ClimbRate");
            dataClimb.children.Add(genChildData(dataClimb.paramname, "Enable", getU16ParamString(eeprom, (int)_paramsAddr["ClimbRate_Enable"]), "", "0, 1", lang.getLangStr("enable")));
            dataClimb.children.Add(genChildData(dataClimb.paramname, "Panel", getU16PanelString(eeprom, (int)_paramsAddr["ClimbRate_Panel"]), "", "1 - Max_Panels", lang.getLangStr("panel")));
            dataClimb.children.Add(genChildData(dataClimb.paramname, "H_Position", getU16ParamString(eeprom, (int)_paramsAddr["ClimbRate_H_Position"]), "", "0 - 350", lang.getLangStr("hpos")));
            dataClimb.children.Add(genChildData(dataClimb.paramname, "V_Position", getU16ParamString(eeprom, (int)_paramsAddr["ClimbRate_V_Position"]), "", "0 - 230", lang.getLangStr("vpos")));
            dataClimb.children.Add(genChildData(dataClimb.paramname, "Font_Size", getU16ParamString(eeprom, (int)_paramsAddr["ClimbRate_Font_Size"]), "", "0, 1, 2", lang.getLangStr("font")));
            //dataClimb.children.Add(genChildData(dataClimb.paramname, "H_Alignment", getU16ParamString(eeprom, (int)_paramsAddr["ClimbRate_H_Alignment"]), "", "0, 1, 2", lang.getLangStr("halign")));
            roots.Add(dataClimb);

            data dataRSSI = new PlayuavOSD.data();
            dataRSSI.paramname = "RSSI";
            dataRSSI.desc = lang.getLangStr("RSSI");
            dataRSSI.children.Add(genChildData(dataRSSI.paramname, "Enable", getU16ParamString(eeprom, (int)_paramsAddr["RSSI_Enable"]), "", "0, 1", lang.getLangStr("enable")));
            dataRSSI.children.Add(genChildData(dataRSSI.paramname, "Panel", getU16PanelString(eeprom, (int)_paramsAddr["RSSI_Panel"]), "", "1 - Max_Panels", lang.getLangStr("panel")));
            dataRSSI.children.Add(genChildData(dataRSSI.paramname, "H_Position", getU16ParamString(eeprom, (int)_paramsAddr["RSSI_H_Position"]), "", "0 - 350", lang.getLangStr("hpos")));
            dataRSSI.children.Add(genChildData(dataRSSI.paramname, "V_Position", getU16ParamString(eeprom, (int)_paramsAddr["RSSI_V_Position"]), "", "0 - 230", lang.getLangStr("Misc")));
            dataRSSI.children.Add(genChildData(dataRSSI.paramname, "Font_Size", getU16ParamString(eeprom, (int)_paramsAddr["RSSI_Font_Size"]), "", "0, 1, 2", lang.getLangStr("font")));
            dataRSSI.children.Add(genChildData(dataRSSI.paramname, "H_Alignment", getU16ParamString(eeprom, (int)_paramsAddr["RSSI_H_Alignment"]), "", "0, 1, 2", lang.getLangStr("halign")));
            dataRSSI.children.Add(genChildData(dataRSSI.paramname, "Min", getU16ParamString(eeprom, (int)_paramsAddr["RSSI_Min"]), "", "0-255", lang.getLangStr("RSSI_Min")));
            dataRSSI.children.Add(genChildData(dataRSSI.paramname, "Max", getU16ParamString(eeprom, (int)_paramsAddr["RSSI_Max"]), "", "RSSI_Min-255", lang.getLangStr("RSSI_Max")));
            dataRSSI.children.Add(genChildData(dataRSSI.paramname, "Raw_Enable", getU16ParamString(eeprom, (int)_paramsAddr["RSSI_Raw_Enable"]), "", "0, 1", lang.getLangStr("RSSI_Raw_Enable")));
            roots.Add(dataRSSI);

            foreach (var item in roots)
            {
                // if the child has no children, we dont need the root.
                if (((List<data>)item.children).Count == 1)
                {
                    Params.AddObject(((List<data>)item.children)[0]);
                    continue;
                }
                else
                {

                }

                Params.AddObject(item);
            }
        }

        private void Params_CellEditFinishing(object sender, BrightIdeasSoftware.CellEditEventArgs e)
        {
            bool bPanelValue = false;
            string paramsfullname = ((data)e.RowObject).root + "_" + ((data)e.RowObject).paramname;

            if (paramsfullname.Contains("_Panel") && !(paramsfullname.Contains("PWM")) && !(paramsfullname.Contains("Max_Panels")))
                bPanelValue = true;

            if (e.NewValue != e.Value && e.Cancel == false)
            {
                Console.WriteLine(e.NewValue + " " + e.NewValue.GetType());

                //double min = 0;
                //double max = 0;
                if (((data)e.RowObject).children.Count > 0)
                {
                    e.Cancel = true;
                    return;
                }

                float newvalue = 0;

                if (bPanelValue)
                {
                    newvalue = panelValStr2F(e.NewValue.ToString());
                    if (newvalue == 0)
                    {
                        e.Cancel = true;
                        return;
                    }
                }
                else
                {
                    try
                    {
                        newvalue = float.Parse(e.NewValue.ToString());
                    }
                    catch { CustomMessageBox.Show("Bad number"); e.Cancel = true; return; }
                }

                //if (ParameterMetaDataRepository.GetParameterRange(((data)e.RowObject).paramname, ref min, ref max, MainV2.comPort.MAV.cs.firmware.ToString()))
                //{
                //    if (newvalue > max || newvalue < min)
                //    {
                //        if (CustomMessageBox.Show(((data)e.RowObject).paramname + " value is out of range. Do you want to continue?", "Out of range", MessageBoxButtons.YesNo) == DialogResult.No)
                //        {
                //            return;
                //        }
                //    }
                //}

                //_changes[((data)e.RowObject).paramname] = newvalue;
                _changes[paramsfullname] = newvalue;

                u16toEPPROM(eeprom, (int)_paramsAddr[paramsfullname], Convert.ToInt16(newvalue));

                ((data)e.RowObject).Value = e.NewValue.ToString();

                var typer = e.RowObject.GetType();

                e.Cancel = true;

                Params.RefreshObject(e.RowObject);

            }
        }

        private void Params_FormatRow(object sender, BrightIdeasSoftware.FormatRowEventArgs e)
        {
            string paramsfullname = "";
            if (e != null && e.ListView != null && e.ListView.Items.Count > 0)
            {
                paramsfullname = ((data)e.Model).root + "_" + ((data)e.Model).paramname;
                //if (_changes.ContainsKey(((data)e.Model).paramname))
                if (_changes.ContainsKey(paramsfullname))
                    e.Item.BackColor = Color.Green;
                else
                    e.Item.BackColor = this.BackColor;
            }
        }

        private void Load_from_OSD_Click(object sender, EventArgs e)
        {
            if (comPort.IsOpen)
                comPort.Close();

            try
            {
                comPort.PortName = comPortName;
                comPort.BaudRate = 115200;
                comPort.ReadBufferSize = 1024 * 1024 * 4;
                comPort.ReadTimeout = 1000;
                comPort.WriteTimeout = 1000;
                comPort.Open();
            }
            catch { MessageBox.Show("Error opening com port", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error); return; }

            System.Threading.Thread.Sleep(500);

            try
            {
                // make sure we are in sync before starting
                self.__sync();

                //first clear the recv buf
                comPort.ReadExisting();

                __send(new byte[] { (byte)Code.GET_PARAMS, (byte)Code.EOC });
                byte[] eepromtmp = __recv(1024);
                __getSync();

                //do CRC?

                //eeprom = eepromtmp;
                eepromtmp.CopyTo(eeprom, 0);

                processToScreen();

                //comPort.BaseStream.Flush();
                System.Threading.Thread.Sleep(500);
                comPort.Close();

                MessageBox.Show("Successful reading parameters", "Message", MessageBoxButtons.OK, MessageBoxIcon.None);
            }
            catch 
            {
                paramdefault.CopyTo(eeprom, 0);
                MessageBox.Show("Error reading parameters", "Message", MessageBoxButtons.OK, MessageBoxIcon.None);
            }
            
        }

        private void Save_To_OSD_Click(object sender, EventArgs e)
        {
            try
            {

                if (comPort.IsOpen)
                    comPort.Close();

                try
                {
                    comPort.PortName = comPortName;
                    comPort.BaudRate = 115200;
                    comPort.ReadBufferSize = 1024 * 1024 * 4;
                    comPort.ReadTimeout = 1000;
                    comPort.WriteTimeout = 1000;
                    comPort.Open();
                }
                catch { MessageBox.Show("Error opening com port", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error); return; }

                System.Threading.Thread.Sleep(500);

                // make sure we are in sync before starting
                self.__sync();

                //get the board version first
                self.osd_rev = self.__getInfo(Code.INFO_OSD_REV);

                //not matched, send the default params to EEPROM
                if (self.osd_rev != VER)
                {
                    //TODO
                }

                //begin to write params
                __send(new byte[] { (byte)Code.START_TRANSFER, (byte)Code.EOC });
                __getSync();
                List<byte[]> groups = self.__split_len(eeprom, (byte)Code.PROG_MULTI_MAX);
                foreach (Byte[] bytes in groups)
                {
                    self.__set_parameters(bytes);
                }
                __send(new byte[] { (byte)Code.END_TRANSFER, (byte)Code.EOC });
                __getSync();

                //TODO - CRC?

                comPort.BaseStream.Flush();
                System.Threading.Thread.Sleep(500);
                comPort.Close();

                MessageBox.Show("Successful writing parameters to memory", "Message", MessageBoxButtons.OK, MessageBoxIcon.None);

            }
            catch { MessageBox.Show("Error writing parameters to memory", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error); return; }
        }

        private void Sav_To_EEPROM_Click(object sender, EventArgs e)
        {
            try
            {

                if (comPort.IsOpen)
                    comPort.Close();

                try
                {
                    comPort.PortName = comPortName;
                    comPort.BaudRate = 115200;
                    comPort.ReadBufferSize = 1024;
                    comPort.ReadTimeout = 3000;
                    comPort.WriteTimeout = 3000;
                    comPort.Open();
                }
                catch { MessageBox.Show("Error opening com port", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error); return; }

                System.Threading.Thread.Sleep(500);

                // make sure we are in sync before starting
                self.__sync();

                //get the board version first
                self.osd_rev = self.__getInfo(Code.INFO_OSD_REV);

                //not matched, send the default params to EEPROM
                if (self.osd_rev != VER)
                {
                    //TODO
                }

                //begin to write params
                __send(new byte[] { (byte)Code.START_TRANSFER, (byte)Code.EOC });
                __getSync();
                List<byte[]> groups = self.__split_len(eeprom, (byte)Code.PROG_MULTI_MAX);
                foreach (Byte[] bytes in groups)
                {
                    self.__set_parameters(bytes);
                }
                __send(new byte[] { (byte)Code.END_TRANSFER, (byte)Code.EOC });
                __getSync();

                //send command
                __send(new byte[] { (byte)Code.SAVE_TO_EEPROM, (byte)Code.EOC });
                __getSync();

                comPort.BaseStream.Flush();
                comPort.Close();

                MessageBox.Show("Successful writing parameters to FLASH", "Message", MessageBoxButtons.OK, MessageBoxIcon.None);

            }
            catch { MessageBox.Show("Error writing parameters to FLASH", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error); return; }
        }

        void loadparamsfromfile(string fn)
        {
            Hashtable param2 = MissionPlanner.Utilities.ParamFile.loadParamFile(fn);

            foreach (string name in param2.Keys)
            {
                string value = param2[name].ToString();

                checkandupdateparam(name, value);
            }
        }

        void checkandupdateparam(string name, string value)
        {
            //if (name == "SYSID_SW_MREV")
            //    return;
            //if (name == "WP_TOTAL")
            //    return;
            //if (name == "CMD_TOTAL")
            //    return;
            //if (name == "FENCE_TOTAL")
            //    return;
            //if (name == "SYS_NUM_RESETS")
            //    return;
            //if (name == "ARSPD_OFFSET")
            //    return;
            //if (name == "GND_ABS_PRESS")
            //    return;
            //if (name == "GND_TEMP")
            //    return;
            //if (name == "CMD_INDEX")
            //    return;
            //if (name == "LOG_LASTFILE")
            //    return;
            //if (name == "FORMAT_VERSION")
            //    return;

            paramCompareForm_dtlvcallback(name, float.Parse(value));
        }

        void paramCompareForm_dtlvcallback(string param, float value)
        {
            string strParent = "";
            string strChild = "";
            bool bPanelValue = false;
            int nPos = param.IndexOf('_');
            if (nPos != 0)
            {
                strParent = param.Substring(0, nPos);
                strChild = param.Substring(nPos + 1);
            }

            foreach (data item in Params.Objects)
            {
                if (item.paramname == strParent)
                {
                    foreach (data item2 in item.children)
                    {
                        if (item2.paramname == strChild)
                        {
                            if (param.Contains("_Panel") && !(param.Contains("PWM")) && !(param.Contains("Max_Panels")))
                                bPanelValue = true;

                            if (bPanelValue)
                            {
                                item2.Value = panelValF2Str((int)value);
                            }
                            else
                            {
                                item2.Value = value.ToString();
                            }

                            _changes[param] = value;
                            Params.RefreshObject(item2);
                            Params.Expand(item2);
                            u16toEPPROM(eeprom, (int)_paramsAddr[param], (short)value);
                            break;
                        }
                    }
                }

            }
        }

        private void PlayuavOSD_Resize(object sender, EventArgs e)
        {
            //SetupViewport();
            //glControl1.Invalidate();

            Params.Columns[0].Width = (int)(Params.Size.Width * 0.1);
            Params.Columns[1].Width = (int)(Params.Size.Width * 0.1);
            Params.Columns[2].Width = (int)(Params.Size.Width * 0.1);
            Params.Columns[3].Width = (int)(Params.Size.Width * 0.25);
            Params.Columns[4].Width = (int)(Params.Size.Width * 0.45);
            
             
        }

        private void saveOSDFileToolStripMenuItem_Click(object sender, EventArgs e)
        {
            var sfd = new SaveFileDialog
            {
                AddExtension = true,
                DefaultExt = ".posd",
                RestoreDirectory = true,
                Filter = "OSD Param List|*.posd"
            };

            var dr = sfd.ShowDialog();
            if (dr == DialogResult.OK)
            {
                Hashtable data = new Hashtable();
                string fullparamname;
                bool bPanelValue = false;
                foreach (data row in Params.Objects)
                {

                    foreach (var item in row.children)
                    {
                        if (item.Value != null)
                        {
                            float value;
                            fullparamname = row.paramname + "_" + item.paramname.ToString();
                            bPanelValue = false;
                            if (fullparamname.Contains("_Panel") && !(fullparamname.Contains("PWM")) && !(fullparamname.Contains("Max_Panels")))
                                bPanelValue = true;

                            if (bPanelValue)
                            {
                                value = panelValStr2F(item.Value.ToString());
                            }
                            else
                            {
                                value = float.Parse(item.Value.ToString());
                            }
                            data[fullparamname] = value;
                        }
                    }

                    if (row.Value != null)
                    {
                        float value = float.Parse(row.Value.ToString());

                        data[row.paramname.ToString()] = value;
                    }
                }

                MissionPlanner.Utilities.ParamFile.SaveParamFile(sfd.FileName, data);

            }
        }

        private void openOSDFileToolStripMenuItem_Click(object sender, EventArgs e)
        {
            var ofd = new OpenFileDialog
            {
                AddExtension = true,
                DefaultExt = ".posd",
                RestoreDirectory = true,
                Filter = "OSD Param List|*.posd"
            };
            var dr = ofd.ShowDialog();

            if (dr == DialogResult.OK)
            {
                loadparamsfromfile(ofd.FileName);
            }
        }

        private void loadDefaultsToolStripMenuItem_Click(object sender, EventArgs e)
        {
            paramdefault.CopyTo(eeprom, 0);
            processToScreen();
        }

        private void englishToolStripMenuItem_Click(object sender, EventArgs e)
        {
            //chineseToolStripMenuItem.Checked = false;
            //englishToolStripMenuItem.CheckState = CheckState.Checked;
            langid = LanguageEnum.LANG_EN;
            processToScreen();
        }

        private void chineseToolStripMenuItem_Click(object sender, EventArgs e)
        {
            //englishToolStripMenuItem.Checked = false;
            //chineseToolStripMenuItem.CheckState = CheckState.Checked;
            langid = LanguageEnum.LANG_ZH;
            processToScreen();
        }

        private void Save_To_OSD_MouseEnter(object sender, EventArgs e)
        {
            ToolTip p = new ToolTip();
            p.ShowAlways = true;
            p.SetToolTip(this.Save_To_OSD, lang.getLangStr("Save_To_OSD_Tip"));
        }

        private void Sav_To_EEPROM_MouseEnter(object sender, EventArgs e)
        {
            ToolTip p = new ToolTip();
            p.ShowAlways = true;
            p.SetToolTip(this.Sav_To_EEPROM, lang.getLangStr("Sav_To_EEPROM_Tip"));
        }

        private void set_widget_lang()
        {
            this.Load_from_OSD.Text = lang.getLangStr("Load_from_OSD");
            this.Save_To_OSD.Text = lang.getLangStr("Save_To_OSD");
            this.Sav_To_EEPROM.Text = lang.getLangStr("Sav_To_EEPROM");
            this.btn_up_fw.Text = lang.getLangStr("btn_up_fw");
            this.fileToolStripMenuItem.Text = lang.getLangStr("menu_file");
            this.saveOSDFileToolStripMenuItem.Text = lang.getLangStr("menu_file_save");
            this.openOSDFileToolStripMenuItem.Text = lang.getLangStr("menu_file_load");
            this.loadDefaultsToolStripMenuItem.Text = lang.getLangStr("menu_file_default");
            this.exitToolStripMenuItem.Text = lang.getLangStr("menu_file_exit");
            this.optionsToolStripMenuItem.Text = lang.getLangStr("menu_opt");
            this.languageToolStripMenuItem.Text = lang.getLangStr("menu_opt_lang");
            this.englishToolStripMenuItem.Text = lang.getLangStr("menu_opt_lang_en");
            this.chineseToolStripMenuItem.Text = lang.getLangStr("menu_opt_lang_zh");
            this.helpToolStripMenuItem.Text = lang.getLangStr("menu_opt_help");
            this.gettingStartedToolStripMenuItem.Text = lang.getLangStr("menu_opt_help_Manual");
            this.checkUpdatesToolStripMenuItem.Text = lang.getLangStr("menu_opt_help_update");
            this.aboutToolStripMenuItem.Text = lang.getLangStr("menu_opt_help_about");
            this.Params.Columns[0].Text = lang.getLangStr("List_col0");
            this.Params.Columns[1].Text = lang.getLangStr("List_col1");
            this.Params.Columns[2].Text = lang.getLangStr("List_col2");
            this.Params.Columns[3].Text = lang.getLangStr("List_col3");
            this.Params.Columns[4].Text = lang.getLangStr("List_col4");
            this.lbl_port.Text = lang.getLangStr("lab_port");
            this.lbl_fc.Text = lang.getLangStr("lab_fc");

            if (langid == LanguageEnum.LANG_EN)
            {
                this.chineseToolStripMenuItem.Checked = false;
                this.englishToolStripMenuItem.CheckState = CheckState.Checked;
            }
            else if (langid == LanguageEnum.LANG_ZH)
            {
                englishToolStripMenuItem.Checked = false;
                chineseToolStripMenuItem.CheckState = CheckState.Checked;
            }
        }

        private void checkUpdatesToolStripMenuItem_Click(object sender, EventArgs e)
        {
            CheckNewVersion();
        }

        private void gettingStartedToolStripMenuItem_Click(object sender, EventArgs e)
        {
            string strURL = "http://www.playuav.com/download/Playuavosd/playuavosd_en.pdf";
            if (PlayuavOSD.langid == PlayuavOSD.LanguageEnum.LANG_ZH)
            {
                strURL = "http://www.playuav.com/download/Playuavosd/playuavosd_zh.pdf";
            }

            try
            {
                System.Diagnostics.Process.Start(strURL);
            }
            catch { CustomMessageBox.Show("Can not open the online manual", Strings.ERROR); }
        }

        private void aboutToolStripMenuItem_Click(object sender, EventArgs e)
        {
            AboutBox1 about = new AboutBox1();
            about.Show();
        }

        private void glControl1_Load(object sender, EventArgs e)
        {
            loaded = true;
            SetupViewport();

            Application.Idle += Application_Idle; // press TAB twice after +=
            sw.Start(); // start at application boot
            try
            {
                GL.PushAttrib(AttribMask.DepthBufferBit);
                GL.Disable(EnableCap.DepthTest);
                //GL.Enable(EnableCap.Texture2D); 
                GL.BlendFunc(BlendingFactorSrc.SrcAlpha, BlendingFactorDest.OneMinusSrcAlpha);
                GL.Enable(EnableCap.Blend);

                GL.Hint(HintTarget.PerspectiveCorrectionHint, HintMode.Nicest);

                GL.Hint(HintTarget.LineSmoothHint, HintMode.Nicest);
                GL.Hint(HintTarget.PolygonSmoothHint, HintMode.Nicest);
                GL.Hint(HintTarget.PointSmoothHint, HintMode.Nicest);

                GL.Hint(HintTarget.TextureCompressionHint, HintMode.Nicest);

                GL.Enable(EnableCap.LineSmooth);
                GL.Enable(EnableCap.PointSmooth);
                GL.Enable(EnableCap.PolygonSmooth);
            }
            catch{}

            ogl.Width = glControl1.Width;
            ogl.Height = glControl1.Height;
            GraphicsContext.CurrentContext.VSync = true;
        }

        void Application_Idle(object sender, EventArgs e)
        {
            sw.Stop();
            double timeslice = sw.Elapsed.TotalMilliseconds;
            sw.Reset();
            sw.Start();
            glControl1.Invalidate();
        }

        private void glControl1_Paint(object sender, PaintEventArgs e)
        {
            if (!loaded)
                return;

            GL.Clear(ClearBufferMask.ColorBufferBit | ClearBufferMask.DepthBufferBit);

            GL.MatrixMode(MatrixMode.Modelview);
            GL.LoadIdentity();

            doPaint(e);

            glControl1.SwapBuffers();
        }

        private void SetupViewport()
        {
            int w = glControl1.Width;
            int h = glControl1.Height;
            GL.MatrixMode(MatrixMode.Projection);
            GL.LoadIdentity();
            GL.Ortho(0, w, h, 0, -1, 1); // Bottom-left corner pixel has coordinate (0, 0)
            GL.Viewport(0, 0, w, h); // Use all of the glControl painting area
        }

        

        bool bShownAtPanle(short itemPanel, short curPanle)
        {
            //issue #1 - fixed
            return ((itemPanel & (1 << (curPanle - 1))) != 0);
        }

        private void glControl1_Resize(object sender, EventArgs e)
        {
            SetupViewport();
            glControl1.Invalidate();
        }

        private void lab_inc_Click(object sender, EventArgs e)
        {
            short curpanel = Convert.ToInt16(labPanle.Text);
            curpanel++;
            if (curpanel > getU16Param(eeprom, (int)_paramsAddr["Misc_Max_Panels"]))
            {
                curpanel = 1;
            }
            labPanle.Text = curpanel.ToString();
        }

        private void lab_dec_Click(object sender, EventArgs e)
        {
            short curpanel = Convert.ToInt16(labPanle.Text);
            curpanel--;
            if (curpanel < 1)
            {
                curpanel = getU16Param(eeprom, (int)_paramsAddr["Misc_Max_Panels"]);
            }
            labPanle.Text = curpanel.ToString();
        }

        
        void doPaint(PaintEventArgs e)
        {
            int width = glControl1.Width;
            int height = glControl1.Height;
            int fontsize = glControl1.Height / 28; // = 10
            int fontoffset = fontsize - 10;
            int smallfont = fontsize - 5;
            int midfont = fontsize;
            int largefont = fontsize + 3;

            float strOffset;

            float every5deg = -glControl1.Height / 65;

            int halfwidth = width / 2;
            int halfheight = height / 2;

            //item attribute
            short iposX, iposY, ipanel, ifont, ifontalign, ien;

            SolidBrush whiteBrush = new SolidBrush(whitePen.Color);

            blackPen = new Pen(Color.Black, 1);
            greenPen = new Pen(Color.Green, 1);
            redPen = new Pen(Color.Red, 1);

            short curPanel = Convert.ToInt16(labPanle.Text);
            GL.ClearColor(Color.Transparent);

            //draw bg
            RectangleF bg = new RectangleF(0, 0, width, halfheight);
            if (bg.Height != 0)
            {
                LinearGradientBrush linearBrush = new LinearGradientBrush(bg, Color.Blue,
                    Color.LightBlue, LinearGradientMode.Vertical);

                ogl.FillRectangle(linearBrush, bg);
            }

            bg = new RectangleF(0, halfheight, width, height);
            if (bg.Height != 0)
            {
                LinearGradientBrush linearBrush = new LinearGradientBrush(bg, Color.FromArgb(0x9b, 0xb8, 0x24),
                    Color.FromArgb(0x41, 0x4f, 0x07), LinearGradientMode.Vertical);

                ogl.FillRectangle(linearBrush, bg);
            }
            //draw 
            //ogl.DrawLine(whitePen, 0, halfheight, width, halfheight);
            //ogl.DrawLine(whitePen, 180, 0, 180, height);

            //draw compass, way-point,home direction with new style
            ien = getU16Param(eeprom, (int)_paramsAddr["CHWDIR_Nmode_Enable"]);
            ipanel = getU16Param(eeprom, (int)_paramsAddr["CHWDIR_Nmode_Panel"]);
            if (bShownAtPanle(ipanel, curPanel) && ien == 1)
            {
                iposX = getU16Param(eeprom, (int)_paramsAddr["CHWDIR_Nmode_H_Position"]);
                iposY = getU16Param(eeprom, (int)_paramsAddr["CHWDIR_Nmode_V_Position"]);
                short radio = getU16Param(eeprom, (int)_paramsAddr["CHWDIR_Nmode_Radius"]);
                short hradio = getU16Param(eeprom, (int)_paramsAddr["CHWDIR_Nmode_Home_Radius"]);
                short wpradio = getU16Param(eeprom, (int)_paramsAddr["CHWDIR_Nmode_WP_Radius"]);
                Rectangle rc = new Rectangle(iposX - radio, iposY - radio, 2 * radio, 2 * radio);
                ogl.DrawEllipse(whitePen, rc);
                ogl.drawstring("H", font, SIZE_TO_FONT[0], whiteBrush, iposX + hradio, iposY);
                ogl.drawstring("W", font, SIZE_TO_FONT[0], whiteBrush, iposX, iposY + wpradio);
                ogl.DrawLine(whitePen, iposX, iposY - 7, iposX - 3, iposY+7);
                ogl.DrawLine(whitePen, iposX, iposY - 7, iposX + 3, iposY+7);
            }

            //home distance
            ien = getU16Param(eeprom, (int)_paramsAddr["HomeDistance_Enable"]);
            ipanel = getU16Param(eeprom, (int)_paramsAddr["HomeDistance_Panel"]);
            if (bShownAtPanle(ipanel, curPanel) && ien == 1)
            {
                iposX = getU16Param(eeprom, (int)_paramsAddr["HomeDistance_H_Position"]);
                iposY = getU16Param(eeprom, (int)_paramsAddr["HomeDistance_V_Position"]);
                ifont = getU16Param(eeprom, (int)_paramsAddr["HomeDistance_Font_Size"]);
                ifontalign = getU16Param(eeprom, (int)_paramsAddr["HomeDistance_H_Alignment"]);
                strOffset = ogl.calstring("H:10", font, SIZE_TO_FONT[ifont], whiteBrush, ifontalign);
                ogl.drawstring("H:10m", font, SIZE_TO_FONT[ifont], whiteBrush, iposX - strOffset, iposY);
            }

            //home distance
            ien = getU16Param(eeprom, (int)_paramsAddr["WPDistance_Enable"]);
            ipanel = getU16Param(eeprom, (int)_paramsAddr["WPDistance_Panel"]);
            if (bShownAtPanle(ipanel, curPanel) && ien == 1)
            {
                iposX = getU16Param(eeprom, (int)_paramsAddr["WPDistance_H_Position"]);
                iposY = getU16Param(eeprom, (int)_paramsAddr["WPDistance_V_Position"]);
                ifont = getU16Param(eeprom, (int)_paramsAddr["WPDistance_Font_Size"]);
                ifontalign = getU16Param(eeprom, (int)_paramsAddr["WPDistance_H_Alignment"]);
                strOffset = ogl.calstring("W:30m", font, SIZE_TO_FONT[ifont], whiteBrush, ifontalign);
                ogl.drawstring("W:30m", font, SIZE_TO_FONT[ifont], whiteBrush, iposX - strOffset, iposY);
            }

            //waring
            iposX = getU16Param(eeprom, (int)_paramsAddr["Alarm_H_Position"]);
            iposY = getU16Param(eeprom, (int)_paramsAddr["Alarm_V_Position"]);
            ifont = getU16Param(eeprom, (int)_paramsAddr["Alarm_Font_Size"]);
            ifontalign = getU16Param(eeprom, (int)_paramsAddr["Alarm_H_Alignment"]);
            
            strOffset = ogl.calstring("NO GPS FIX", font, SIZE_TO_FONT[ifont], whiteBrush, ifontalign);
            ogl.drawstring("NO GPS FIX", font, SIZE_TO_FONT[ifont], whiteBrush, iposX - strOffset, iposY);

            //battery Voltage
            ien = getU16Param(eeprom, (int)_paramsAddr["BatteryVoltage_Enable"]);
            ipanel = getU16Param(eeprom, (int)_paramsAddr["BatteryVoltage_Panel"]);
            if (bShownAtPanle(ipanel, curPanel) && ien == 1)
            {
                iposX = getU16Param(eeprom, (int)_paramsAddr["BatteryVoltage_H_Position"]);
                iposY = getU16Param(eeprom, (int)_paramsAddr["BatteryVoltage_V_Position"]);
                ifont = getU16Param(eeprom, (int)_paramsAddr["BatteryVoltage_Font_Size"]);
                ifontalign = getU16Param(eeprom, (int)_paramsAddr["BatteryVoltage_H_Alignment"]);
                strOffset = ogl.calstring("16.7V", font, SIZE_TO_FONT[ifont], whiteBrush, ifontalign);
                ogl.drawstring("16.7V", font, SIZE_TO_FONT[ifont], whiteBrush, iposX - strOffset, iposY);
            }

            //battery current
            ien = getU16Param(eeprom, (int)_paramsAddr["BatteryCurrent_Enable"]);
            ipanel = getU16Param(eeprom, (int)_paramsAddr["BatteryCurrent_Panel"]);
            if (bShownAtPanle(ipanel, curPanel) && ien == 1)
            {
                iposX = getU16Param(eeprom, (int)_paramsAddr["BatteryCurrent_H_Position"]);
                iposY = getU16Param(eeprom, (int)_paramsAddr["BatteryCurrent_V_Position"]);
                ifont = getU16Param(eeprom, (int)_paramsAddr["BatteryCurrent_Font_Size"]);
                ifontalign = getU16Param(eeprom, (int)_paramsAddr["BatteryCurrent_H_Alignment"]);
                strOffset = ogl.calstring("5.8A", font, SIZE_TO_FONT[ifont], whiteBrush, ifontalign);
                ogl.drawstring("5.8A", font, SIZE_TO_FONT[ifont], whiteBrush, iposX - strOffset, iposY);
            }

            //battery remaining
            ien = getU16Param(eeprom, (int)_paramsAddr["BatteryConsumed_Enable"]);
            ipanel = getU16Param(eeprom, (int)_paramsAddr["BatteryConsumed_Panel"]);
            if (bShownAtPanle(ipanel, curPanel) && ien == 1)
            {
                iposX = getU16Param(eeprom, (int)_paramsAddr["BatteryConsumed_H_Position"]);
                iposY = getU16Param(eeprom, (int)_paramsAddr["BatteryConsumed_V_Position"]);
                ifont = getU16Param(eeprom, (int)_paramsAddr["BatteryConsumed_Font_Size"]);
                ifontalign = getU16Param(eeprom, (int)_paramsAddr["BatteryConsumed_H_Alignment"]);
                strOffset = ogl.calstring("99%", font, SIZE_TO_FONT[ifont], whiteBrush, ifontalign);
                ogl.drawstring("99%", font, SIZE_TO_FONT[ifont], whiteBrush, iposX - strOffset, iposY);
            }

            //Arm state
            ien = getU16Param(eeprom, (int)_paramsAddr["ArmState_Enable"]);
            ipanel = getU16Param(eeprom, (int)_paramsAddr["ArmState_Panel"]);
            if (bShownAtPanle(ipanel, curPanel) && ien == 1)
            {
                iposX = getU16Param(eeprom, (int)_paramsAddr["ArmState_H_Position"]);
                iposY = getU16Param(eeprom, (int)_paramsAddr["ArmState_V_Position"]);
                ifont = getU16Param(eeprom, (int)_paramsAddr["ArmState_Font_Size"]);
                ifontalign = getU16Param(eeprom, (int)_paramsAddr["ArmState_H_Alignment"]);
                strOffset = ogl.calstring("DISARMED", font, SIZE_TO_FONT[ifont], whiteBrush, ifontalign);
                ogl.drawstring("DISARMED", font, SIZE_TO_FONT[ifont], whiteBrush, iposX - strOffset, iposY);
            }

            //flight mode
            ien = getU16Param(eeprom, (int)_paramsAddr["FlightMode_Enable"]);
            ipanel = getU16Param(eeprom, (int)_paramsAddr["FlightMode_Panel"]);
            if (bShownAtPanle(ipanel, curPanel) && ien == 1)
            {
                iposX = getU16Param(eeprom, (int)_paramsAddr["FlightMode_H_Position"]);
                iposY = getU16Param(eeprom, (int)_paramsAddr["FlightMode_V_Position"]);
                ifont = getU16Param(eeprom, (int)_paramsAddr["FlightMode_Font_Size"]);
                ifontalign = getU16Param(eeprom, (int)_paramsAddr["FlightMode_H_Alignment"]);
                strOffset = ogl.calstring("STAB", font, SIZE_TO_FONT[ifont], whiteBrush, ifontalign);
                ogl.drawstring("STAB", font, SIZE_TO_FONT[ifont], whiteBrush, iposX - strOffset, iposY);
            }

            //MissionPlanner attitude
            ien = getU16Param(eeprom, (int)_paramsAddr["Attitude_MP_Enable"]);
            ipanel = getU16Param(eeprom, (int)_paramsAddr["Attitude_MP_Panel"]);
            if (bShownAtPanle(ipanel, curPanel) && ien == 1)
            {
                ogl.drawstring("0", font, SIZE_TO_FONT[1], whiteBrush, 175, 40);
                RectangleF rect = new RectangleF(105, 55, 150, 100);
                ogl.DrawArc(whitePen, rect, -142, 134);
                PointF[] plist = new PointF[3];
                plist[0] = new PointF(180,58);
                plist[1] = new PointF(176, 66);
                plist[2] = new PointF(184, 66);
                ogl.DrawPolygon(whitePen, plist);
                ogl.DrawLine(whitePen, 160, halfheight - 60, 200, halfheight - 60);
                ogl.DrawLine(whitePen, 160, halfheight - 30, 200, halfheight - 30);
                ogl.DrawLine(whitePen, 140, halfheight, 220, halfheight);
                ogl.DrawLine(whitePen, 160, halfheight + 60, 200, halfheight + 60);
                ogl.DrawLine(whitePen, 160, halfheight + 30, 200, halfheight + 30);
                ogl.DrawLine(whitePen, 180, halfheight, 170, halfheight + 5);
                ogl.DrawLine(whitePen, 180, halfheight, 190, halfheight + 5);
                ogl.drawstring("0", font, SIZE_TO_FONT[1], whiteBrush, 175, halfheight);
            }

            //altitude scale
            ien = getU16Param(eeprom, (int)_paramsAddr["Altitude_Scale_Enable"]);
            ipanel = getU16Param(eeprom, (int)_paramsAddr["Altitude_Scale_Panel"]);
            if (bShownAtPanle(ipanel, curPanel) && ien == 1)
            {
                iposX = getU16Param(eeprom, (int)_paramsAddr["Altitude_Scale_H_Position"]);
                short scaleAlign = getU16Param(eeprom, (int)_paramsAddr["Altitude_Scale_Align"]);
                ogl.DrawVScale(whitePen, scaleAlign, iposX, halfheight, 0, 0, 0);
            }

            //speed scale
            ien = getU16Param(eeprom, (int)_paramsAddr["Speed_Scale_Enable"]);
            ipanel = getU16Param(eeprom, (int)_paramsAddr["Speed_Scale_Panel"]);
            if (bShownAtPanle(ipanel, curPanel) && ien == 1)
            {
                iposX = getU16Param(eeprom, (int)_paramsAddr["Speed_Scale_H_Position"]);
                short scaleAlign = getU16Param(eeprom, (int)_paramsAddr["Speed_Scale_Align"]);
                ogl.DrawVScale(whitePen, scaleAlign, iposX, halfheight, 0, 0, 0);
            }

            //climb rate - VSpeed
            ien = getU16Param(eeprom, (int)_paramsAddr["ClimbRate_Enable"]);
            ipanel = getU16Param(eeprom, (int)_paramsAddr["ClimbRate_Panel"]);
            if (bShownAtPanle(ipanel, curPanel) && ien == 1)
            {
                iposX = getU16Param(eeprom, (int)_paramsAddr["ClimbRate_H_Position"]);
                iposY = getU16Param(eeprom, (int)_paramsAddr["ClimbRate_V_Position"]);
                ifont = getU16Param(eeprom, (int)_paramsAddr["ClimbRate_Font_Size"]);
                //ifontalign = getU16Param(eeprom, (int)_paramsAddr["FlightMode_H_Alignment"]);
                //strOffset = ogl.calstring("STAB", font, SIZE_TO_FONT[ifont], whiteBrush, ifontalign);
                ogl.drawstring("0.03ms", font, SIZE_TO_FONT[ifont], whiteBrush, iposX + 5, iposY);
                int arrlen = 12;
                if (ifont != 0)
                    arrlen += 2;
                ogl.DrawLine(whitePen, iposX, iposY, iposX, iposY + arrlen);
                ogl.DrawLine(whitePen, iposX, iposY, iposX-3, iposY + 3);
                ogl.DrawLine(whitePen, iposX, iposY, iposX+3, iposY + 3);
            }

            //RSSI
            ien = getU16Param(eeprom, (int)_paramsAddr["RSSI_Enable"]);
            ipanel = getU16Param(eeprom, (int)_paramsAddr["RSSI_Panel"]);
            if (bShownAtPanle(ipanel, curPanel) && ien == 1)
            {
                iposX = getU16Param(eeprom, (int)_paramsAddr["RSSI_H_Position"]);
                iposY = getU16Param(eeprom, (int)_paramsAddr["RSSI_V_Position"]);
                ifont = getU16Param(eeprom, (int)_paramsAddr["RSSI_Font_Size"]);
                ifontalign = getU16Param(eeprom, (int)_paramsAddr["RSSI_H_Alignment"]);
                strOffset = ogl.calstring("RSSI:100%", font, SIZE_TO_FONT[ifont], whiteBrush, ifontalign);
                ogl.drawstring("RSSI:100%", font, SIZE_TO_FONT[ifont], whiteBrush, iposX - strOffset, iposY);
            }

            //GPS1 status
            ien = getU16Param(eeprom, (int)_paramsAddr["GPSStatus_Enable"]);
            ipanel = getU16Param(eeprom, (int)_paramsAddr["GPSStatus_Panel"]);
            if (bShownAtPanle(ipanel, curPanel) && ien == 1)
            {
                iposX = getU16Param(eeprom, (int)_paramsAddr["GPSStatus_H_Position"]);
                iposY = getU16Param(eeprom, (int)_paramsAddr["GPSStatus_V_Position"]);
                ifont = getU16Param(eeprom, (int)_paramsAddr["GPSStatus_Font_Size"]);
                ifontalign = getU16Param(eeprom, (int)_paramsAddr["GPSStatus_H_Alignment"]);
                strOffset = ogl.calstring("FIX3D-10", font, SIZE_TO_FONT[ifont], whiteBrush, ifontalign);
                ogl.drawstring("FIX3D-10", font, SIZE_TO_FONT[ifont], whiteBrush, iposX - strOffset, iposY+5);
            }

            //GPS1 HDOP
            ien = getU16Param(eeprom, (int)_paramsAddr["GPSHDOP_Enable"]);
            ipanel = getU16Param(eeprom, (int)_paramsAddr["GPSHDOP_Panel"]);
            if (bShownAtPanle(ipanel, curPanel) && ien == 1)
            {
                iposX = getU16Param(eeprom, (int)_paramsAddr["GPSHDOP_H_Position"]);
                iposY = getU16Param(eeprom, (int)_paramsAddr["GPSHDOP_V_Position"]);
                ifont = getU16Param(eeprom, (int)_paramsAddr["GPSHDOP_Font_Size"]);
                ifontalign = getU16Param(eeprom, (int)_paramsAddr["GPSHDOP_H_Alignment"]);
                strOffset = ogl.calstring("HDOP:1.08", font, SIZE_TO_FONT[ifont], whiteBrush, ifontalign);
                ogl.drawstring("HDOP:1.08", font, SIZE_TO_FONT[ifont], whiteBrush, iposX - strOffset, iposY + 5);
            }

            //GPS1 lat
            ien = getU16Param(eeprom, (int)_paramsAddr["GPSLatitude_Enable"]);
            ipanel = getU16Param(eeprom, (int)_paramsAddr["GPSLatitude_Panel"]);
            if (bShownAtPanle(ipanel, curPanel) && ien == 1)
            {
                iposX = getU16Param(eeprom, (int)_paramsAddr["GPSLatitude_H_Position"]);
                iposY = getU16Param(eeprom, (int)_paramsAddr["GPSLatitude_V_Position"]);
                ifont = getU16Param(eeprom, (int)_paramsAddr["GPSLatitude_Font_Size"]);
                ifontalign = getU16Param(eeprom, (int)_paramsAddr["GPSLatitude_H_Alignment"]);
                strOffset = ogl.calstring("30.32444", font, SIZE_TO_FONT[ifont], whiteBrush, ifontalign);
                ogl.drawstring("30.32444", font, SIZE_TO_FONT[ifont], whiteBrush, iposX - strOffset, iposY + 5);
            }

            //GPS1 lon
            ien = getU16Param(eeprom, (int)_paramsAddr["GPSLongitude_Enable"]);
            ipanel = getU16Param(eeprom, (int)_paramsAddr["GPSLongitude_Panel"]);
            if (bShownAtPanle(ipanel, curPanel) && ien == 1)
            {
                iposX = getU16Param(eeprom, (int)_paramsAddr["GPSLongitude_H_Position"]);
                iposY = getU16Param(eeprom, (int)_paramsAddr["GPSLongitude_V_Position"]);
                ifont = getU16Param(eeprom, (int)_paramsAddr["GPSLongitude_Font_Size"]);
                ifontalign = getU16Param(eeprom, (int)_paramsAddr["GPSLongitude_H_Alignment"]);
                strOffset = ogl.calstring("120.06299", font, SIZE_TO_FONT[ifont], whiteBrush, ifontalign);
                ogl.drawstring("120.06299", font, SIZE_TO_FONT[ifont], whiteBrush, iposX - strOffset, iposY + 5);
            }

            //GPS2 status
            ien = getU16Param(eeprom, (int)_paramsAddr["GPS2Status_Enable"]);
            ipanel = getU16Param(eeprom, (int)_paramsAddr["GPS2Status_Panel"]);
            if (bShownAtPanle(ipanel, curPanel) && ien == 1)
            {
                iposX = getU16Param(eeprom, (int)_paramsAddr["GPS2Status_H_Position"]);
                iposY = getU16Param(eeprom, (int)_paramsAddr["GPS2Status_V_Position"]);
                ifont = getU16Param(eeprom, (int)_paramsAddr["GPS2Status_Font_Size"]);
                ifontalign = getU16Param(eeprom, (int)_paramsAddr["GPS2Status_H_Alignment"]);
                strOffset = ogl.calstring("FIX3D-18", font, SIZE_TO_FONT[ifont], whiteBrush, ifontalign);
                ogl.drawstring("FIX3D-18", font, SIZE_TO_FONT[ifont], whiteBrush, iposX - strOffset, iposY + 5);
            }

            //GPS2 HDOP
            ien = getU16Param(eeprom, (int)_paramsAddr["GPS2HDOP_Enable"]);
            ipanel = getU16Param(eeprom, (int)_paramsAddr["GPS2HDOP_Panel"]);
            if (bShownAtPanle(ipanel, curPanel) && ien == 1)
            {
                iposX = getU16Param(eeprom, (int)_paramsAddr["GPS2HDOP_H_Position"]);
                iposY = getU16Param(eeprom, (int)_paramsAddr["GPS2HDOP_V_Position"]);
                ifont = getU16Param(eeprom, (int)_paramsAddr["GPS2HDOP_Font_Size"]);
                ifontalign = getU16Param(eeprom, (int)_paramsAddr["GPS2HDOP_H_Alignment"]);
                strOffset = ogl.calstring("HDOP:0.98", font, SIZE_TO_FONT[ifont], whiteBrush, ifontalign);
                ogl.drawstring("HDOP:0.98", font, SIZE_TO_FONT[ifont], whiteBrush, iposX - strOffset, iposY + 5);
            }

            //GPS2 lat
            ien = getU16Param(eeprom, (int)_paramsAddr["GPS2Latitude_Enable"]);
            ipanel = getU16Param(eeprom, (int)_paramsAddr["GPS2Latitude_Panel"]);
            if (bShownAtPanle(ipanel, curPanel) && ien == 1)
            {
                iposX = getU16Param(eeprom, (int)_paramsAddr["GPS2Latitude_H_Position"]);
                iposY = getU16Param(eeprom, (int)_paramsAddr["GPS2Latitude_V_Position"]);
                ifont = getU16Param(eeprom, (int)_paramsAddr["GPS2Latitude_Font_Size"]);
                ifontalign = getU16Param(eeprom, (int)_paramsAddr["GPS2Latitude_H_Alignment"]);
                strOffset = ogl.calstring("30.32446", font, SIZE_TO_FONT[ifont], whiteBrush, ifontalign);
                ogl.drawstring("30.32446", font, SIZE_TO_FONT[ifont], whiteBrush, iposX - strOffset, iposY + 5);
            }

            //GPS2 lon
            ien = getU16Param(eeprom, (int)_paramsAddr["GPS2Longitude_Enable"]);
            ipanel = getU16Param(eeprom, (int)_paramsAddr["GPS2Longitude_Panel"]);
            if (bShownAtPanle(ipanel, curPanel) && ien == 1)
            {
                iposX = getU16Param(eeprom, (int)_paramsAddr["GPS2Longitude_H_Position"]);
                iposY = getU16Param(eeprom, (int)_paramsAddr["GPS2Longitude_V_Position"]);
                ifont = getU16Param(eeprom, (int)_paramsAddr["GPS2Longitude_Font_Size"]);
                ifontalign = getU16Param(eeprom, (int)_paramsAddr["GPS2Longitude_H_Alignment"]);
                strOffset = ogl.calstring("120.06300", font, SIZE_TO_FONT[ifont], whiteBrush, ifontalign);
                ogl.drawstring("120.06300", font, SIZE_TO_FONT[ifont], whiteBrush, iposX - strOffset, iposY + 5);
            }

            //Time
            ien = getU16Param(eeprom, (int)_paramsAddr["Time_Enable"]);
            ipanel = getU16Param(eeprom, (int)_paramsAddr["Time_Panel"]);
            if (bShownAtPanle(ipanel, curPanel) && ien == 1)
            {
                iposX = getU16Param(eeprom, (int)_paramsAddr["Time_H_Position"]);
                iposY = getU16Param(eeprom, (int)_paramsAddr["Time_V_Position"]);
                ifont = getU16Param(eeprom, (int)_paramsAddr["Time_Font_Size"]);
                ifontalign = getU16Param(eeprom, (int)_paramsAddr["Time_H_Alignment"]);
                strOffset = ogl.calstring("48:51", font, SIZE_TO_FONT[ifont], whiteBrush, ifontalign);
                ogl.drawstring("48:51", font, SIZE_TO_FONT[ifont], whiteBrush, iposX - strOffset, iposY);
            }

            //Throttle
            ien = getU16Param(eeprom, (int)_paramsAddr["Throttle_Enable"]);
            ipanel = getU16Param(eeprom, (int)_paramsAddr["Throttle_Panel"]);
            if (bShownAtPanle(ipanel, curPanel) && ien == 1)
            {
                iposX = getU16Param(eeprom, (int)_paramsAddr["Throttle_H_Position"]);
                iposY = getU16Param(eeprom, (int)_paramsAddr["Throttle_V_Position"]);
                short throScaleEn = getU16Param(eeprom, (int)_paramsAddr["Throttle_Scale_Enable"]);
                strOffset = ogl.calstring("50", font, SIZE_TO_FONT[ifont], whiteBrush, 2);
                ogl.drawstring("50", font, SIZE_TO_FONT[0], whiteBrush, iposX - strOffset, iposY);
                if (throScaleEn == 1)
                {
                    RectangleF frect = new RectangleF(iposX+3, iposY+5, 5, 25);
                    ogl.FillRectangle(whiteBrush, frect);
                    PointF[] plist = new PointF[4];
                    plist[0] = new PointF(iposX + 4, iposY + 5);
                    plist[1] = new PointF(iposX + 7, iposY + 5);
                    plist[2] = new PointF(iposX + 7, iposY -20);
                    plist[3] = new PointF(iposX + 4, iposY - 20);
                    ogl.DrawPolygon(whitePen, plist);
                }
            }

            //Altitude
            ien = getU16Param(eeprom, (int)_paramsAddr["Altitude_TALT_Enable"]);
            ipanel = getU16Param(eeprom, (int)_paramsAddr["Altitude_TALT_Panel"]);
            if (bShownAtPanle(ipanel, curPanel) && ien == 1)
            {
                iposX = getU16Param(eeprom, (int)_paramsAddr["Altitude_TALT_H_Position"]);
                iposY = getU16Param(eeprom, (int)_paramsAddr["Altitude_TALT_V_Position"]);
                ifont = getU16Param(eeprom, (int)_paramsAddr["Altitude_TALT_Font_Size"]);
                ifontalign = getU16Param(eeprom, (int)_paramsAddr["Altitude_TALT_H_Alignment"]);
                strOffset = ogl.calstring("ALT:10m", font, SIZE_TO_FONT[ifont], whiteBrush, ifontalign);
                ogl.drawstring("ALT:10m", font, SIZE_TO_FONT[ifont], whiteBrush, iposX - strOffset, iposY);
            }

            //speed
            ien = getU16Param(eeprom, (int)_paramsAddr["Speed_TSPD_Enable"]);
            ipanel = getU16Param(eeprom, (int)_paramsAddr["Speed_TSPD_Panel"]);
            if (bShownAtPanle(ipanel, curPanel) && ien == 1)
            {
                iposX = getU16Param(eeprom, (int)_paramsAddr["Speed_TSPD_H_Position"]);
                iposY = getU16Param(eeprom, (int)_paramsAddr["Speed_TSPD_V_Position"]);
                ifont = getU16Param(eeprom, (int)_paramsAddr["Speed_TSPD_Font_Size"]);
                ifontalign = getU16Param(eeprom, (int)_paramsAddr["Speed_TSPD_H_Alignment"]);
                strOffset = ogl.calstring("SPD:5ms", font, SIZE_TO_FONT[ifont], whiteBrush, ifontalign);
                ogl.drawstring("SPD:5ms", font, SIZE_TO_FONT[ifont], whiteBrush, iposX - strOffset, iposY);
            }

            //compass
            ien = getU16Param(eeprom, (int)_paramsAddr["CHWDIR_Tmode_Enable"]);
            ipanel = getU16Param(eeprom, (int)_paramsAddr["CHWDIR_Tmode_Panel"]);
            if (bShownAtPanle(ipanel, curPanel) && ien == 1)
            {
                iposY = getU16Param(eeprom, (int)_paramsAddr["CHWDIR_Tmode_V_Position"]);
                iposX = 180;
                float sl = 10;
                float ss = 5;
                float step = 20;
                ogl.DrawLine(whitePen, iposX - 3 * step, iposY, iposX - 3 * step, iposY - sl);
                ogl.drawstring("33", font, SIZE_TO_FONT[0], whiteBrush, iposX - 3 * step-7, iposY);
                ogl.DrawLine(whitePen, iposX - 2 * step, iposY, iposX - 2 * step, iposY - ss);
                ogl.DrawLine(whitePen, iposX - 1 * step, iposY, iposX - 1 * step, iposY - sl);
                ogl.drawstring("N", font, SIZE_TO_FONT[0], whiteBrush, iposX - 1 * step - 7, iposY);
                PointF[] plist = new PointF[4];
                int rw = 15;
                int rh = 10;
                plist[0] = new PointF(iposX - rw, iposY + rh);
                plist[1] = new PointF(iposX - rw, iposY - rh);
                plist[2] = new PointF(iposX + rw, iposY - rh);
                plist[3] = new PointF(iposX + rw, iposY + rh);
                ogl.DrawPolygon(whitePen, plist);
                strOffset = ogl.calstring("018", font, SIZE_TO_FONT[2], whiteBrush, 1);
                ogl.drawstring("018", font, SIZE_TO_FONT[2], whiteBrush, iposX - strOffset, iposY - 10);
                ogl.drawstring("3", font, SIZE_TO_FONT[0], whiteBrush, plist[2].X+1, iposY);
                ogl.DrawLine(whitePen, iposX + 2 * step, iposY, iposX + 2 * step, iposY - ss);
                ogl.DrawLine(whitePen, iposX + 3 * step, iposY, iposX + 3 * step, iposY - sl);
                ogl.drawstring("06", font, SIZE_TO_FONT[0], whiteBrush, iposX + 3 * step - 7, iposY);
                ogl.drawstring("W", font, SIZE_TO_FONT[0], whiteBrush, iposX + 3 * step - 7, iposY+10);
                ogl.DrawLine(whitePen, iposX + 4 * step, iposY, iposX + 4 * step, iposY - ss);
                ogl.drawstring("H", font, SIZE_TO_FONT[0], whiteBrush, iposX + 4 * step - 7, iposY + 10);
            }

            //3D
            ien = getU16Param(eeprom, (int)_paramsAddr["Attitude_3D_Enable"]);
            ipanel = getU16Param(eeprom, (int)_paramsAddr["Attitude_3D_Panel"]);
            if (bShownAtPanle(ipanel, curPanel) && ien == 1)
            {
                iposX = 180;
                iposY = (short)halfheight;
                PointF[] plist = new PointF[8];
                float a = 24;
                float b = 10;
                float c = 20;
                float d = 4;
                plist[0] = new PointF(iposX, iposY - a);
                plist[1] = new PointF(iposX-d, iposY);
                plist[2] = new PointF(iposX+d, iposY);
                plist[3] = new PointF(iposX-c, iposY + a);
                plist[4] = new PointF(iposX+c, iposY + a);
                plist[5] = new PointF(iposX-b, iposY + a);
                plist[6] = new PointF(iposX+b, iposY + a);
                plist[7] = new PointF(iposX, iposY+5);
                ogl.DrawLine(whitePen, plist[0].X, plist[0].Y, plist[5].X, plist[5].Y);
                ogl.DrawLine(whitePen, plist[1].X, plist[1].Y, plist[3].X, plist[3].Y);
                ogl.DrawLine(whitePen, plist[3].X, plist[3].Y, plist[4].X, plist[4].Y);
                ogl.DrawLine(whitePen, plist[0].X, plist[0].Y, plist[7].X, plist[7].Y);
                ogl.DrawLine(whitePen, plist[7].X, plist[7].Y, plist[5].X, plist[5].Y);
                ogl.DrawLine(whitePen, plist[0].X, plist[0].Y, plist[6].X, plist[6].Y);
                ogl.DrawLine(whitePen, plist[6].X, plist[6].Y, plist[7].X, plist[7].Y);
                ogl.DrawLine(whitePen, plist[2].X, plist[2].Y, plist[4].X, plist[4].Y);

            }
        }

        private void cbx_fc_SelectedIndexChanged(object sender, EventArgs e)
        {
            u16toEPPROM(eeprom, (int)_paramsAddr["FC_Type"], Convert.ToInt16(this.cbx_fc.SelectedIndex));
        }

        private void exitToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            CheckNewVersion();
            timer1.Stop();
        }
    }
}
