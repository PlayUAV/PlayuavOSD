using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Collections;
using OSD;

namespace OSD
{
    class Language
    {
        readonly Hashtable _langdspzh = new Hashtable();
        readonly Hashtable _langdspen = new Hashtable();

        public Language()
        {
            setDspLang();
            
        }

        public string getLangStr(string strkey)
        {
            switch (PlayuavOSD.langid){
                case PlayuavOSD.LanguageEnum.LANG_EN:
                    return _langdspen[strkey].ToString();
                case PlayuavOSD.LanguageEnum.LANG_ZH:
                    return _langdspzh[strkey].ToString(); 
            }
            return "";
        }

        private void setDspLang()
        {
            //these for messagebox
            _langdspen["lab_port"] = "Port:";
            _langdspzh["lab_port"] = "端口：";

            _langdspen["lab_fc"] = "FlightControl:";
            _langdspzh["lab_fc"] = "飞控类型：";

            _langdspen["Load_from_OSD"] = "Read params";
            _langdspzh["Load_from_OSD"] = "读取参数";

            //these for GUI widget
            _langdspen["Load_from_OSD"] = "Read params";
            _langdspzh["Load_from_OSD"] = "读取参数";

            _langdspen["Save_To_OSD"] = "Save to memory";
            _langdspzh["Save_To_OSD"] = "保存参数到内存";

            _langdspen["Save_To_OSD_Tip"] = "Save params to memory temporary avoiding erase flash on chip. Lost after power off ";
            _langdspzh["Save_To_OSD_Tip"] = "把参数暂时保存在芯片的内存中，避免多次擦写FLASH，断电后将会丢失";

            _langdspen["Sav_To_EEPROM"] = "Save to flash";
            _langdspzh["Sav_To_EEPROM"] = "保存参数到FLASH";

            _langdspen["Sav_To_EEPROM_Tip"] = "Save params to flash on chip. Won't be lost after power off ";
            _langdspzh["Sav_To_EEPROM_Tip"] = "把参数暂时保存在芯片的FLASH，断电后不会丢失";

            _langdspen["btn_up_fw"] = "Firmware update";
            _langdspzh["btn_up_fw"] = "固件更新";

            _langdspen["List_col0"] = "Name";
            _langdspzh["List_col0"] = "参数名";

            _langdspen["List_col1"] = "Value";
            _langdspzh["List_col1"] = "值";

            _langdspen["List_col2"] = "Unit";
            _langdspzh["List_col2"] = "单位";

            _langdspen["List_col3"] = "Range";
            _langdspzh["List_col3"] = "范围";

            _langdspen["List_col4"] = "description";
            _langdspzh["List_col4"] = "描述";

            _langdspen["menu_file"] = "File";
            _langdspzh["menu_file"] = "文件";

            _langdspen["menu_file_save"] = "Save OSD file...";
            _langdspzh["menu_file_save"] = "保存参数文件...";

            _langdspen["menu_file_load"] = "Load OSD file...";
            _langdspzh["menu_file_load"] = "加载参数文件...";

            _langdspen["menu_file_default"] = "Load default params";
            _langdspzh["menu_file_default"] = "加载默认参数";

            _langdspen["menu_file_custom"] = "Load custom firmware...";
            _langdspzh["menu_file_custom"] = "加载自定义固件";

            _langdspen["menu_file_exit"] = "Exit";
            _langdspzh["menu_file_exit"] = "退出";

            _langdspen["menu_opt"] = "Options";
            _langdspzh["menu_opt"] = "选项";

            _langdspen["menu_opt_lang"] = "Language";
            _langdspzh["menu_opt_lang"] = "语言";

            _langdspen["menu_opt_lang_en"] = "英语";
            _langdspzh["menu_opt_lang_en"] = "English";

            _langdspen["menu_opt_lang_zh"] = "中文";
            _langdspzh["menu_opt_lang_zh"] = "Chinese";

            _langdspen["menu_opt_help"] = "Help";
            _langdspzh["menu_opt_help"] = "帮助";

            _langdspen["menu_opt_help_Manual"] = "Getting started";
            _langdspzh["menu_opt_help_Manual"] = "帮助文档";

            _langdspen["menu_opt_help_update"] = "Check updates";
            _langdspzh["menu_opt_help_update"] = "检查更新";

            _langdspen["menu_opt_help_about"] = "About";
            _langdspzh["menu_opt_help_about"] = "关于";

            //these are common
            _langdspen["enable"] = "0:disable, 1:enable";
            _langdspzh["enable"] = "0:禁用, 1:启用";

            _langdspen["panel"] = "Which panel will be displayed. Multi-panels separated by comma like 1,3";
            _langdspzh["panel"] = "在哪个页面显示，多个页面以英文逗号隔开，比如1，3";

            _langdspen["hpos"] = "Horizontal position";
            _langdspzh["hpos"] = "水平位置";

            _langdspen["vpos"] = "Vertical position. maximum is 230 of NTSC, and 250 of PAL";
            _langdspzh["vpos"] = "垂直位置 230为NTSC制式最大，PAL制式可到250";

            _langdspen["font"] = "0:small, 1:normal, 2:large";
            _langdspzh["font"] = "0:小号, 1:正常, 2:大号";

            _langdspen["halign"] = "0:left, 1:middle, 2:right";
            _langdspzh["halign"] = "0:左对齐,  1:居中, 2:右对齐";

            _langdspen["alarmval"] = "Threshold";
            _langdspzh["alarmval"] = "警戒值";

            //these are special
            _langdspen["Attitude"] = "UAV attitude";
            _langdspzh["Attitude"] = "飞行姿态";

            _langdspen["Attitude_MP_Enable"] = "MissionPlanner style HUD, 0:disable, 1:enable";
            _langdspzh["Attitude_MP_Enable"] = "MissionPlanner地面站类似的界面,0:禁用, 1:启用";

            _langdspen["Attitude_MP_Mode"] = "0:NATO, 1:Russia";
            _langdspzh["Attitude_MP_Mode"] = "0:北约, 1:俄制";

            _langdspen["Attitude_3D_Enable"] = "3D style HUD, 0:disable, 1:enable";
            _langdspzh["Attitude_3D_Enable"] = "3D界面,0:禁用, 1:启用";

            _langdspen["Misc"] = "Miscellaneous";
            _langdspzh["Misc"] = "杂项";

            _langdspen["Misc_Units_Mode"] = "0:Metric, 1:Imperial";
            _langdspzh["Misc_Units_Mode"] = "0:公制 1:英制";

            _langdspen["Misc_Max_Panels"] = "Max panels";
            _langdspzh["Misc_Max_Panels"] = "最大显示页面";

            _langdspen["PWM"] = "The chanel used to switch panel and video.";
            _langdspzh["PWM"] = "切换视频，页面。以下只是参考值，根据自己的遥控器测试并设置";

            _langdspen["PWM_Video_Chanel"] = "The chanel used to switch video.";
            _langdspzh["PWM_Video_Chanel"] = "根据飞机类型及遥控，设置合适的通道";

            _langdspen["PWM_Video_Value"] = "Toggle video switch when the chanel input above this value. Default 1200 be suitalbe for most RC";
            _langdspzh["PWM_Video_Value"] = "当通道输出由低位大于这个值的时候，触发一次切换，默认1200适用于大多数遥控";

            _langdspen["PWM_Panel_Chanel"] = "The chanel used to switch panel.";
            _langdspzh["PWM_Panel_Chanel"] = "根据飞机类型及遥控，设置合适的通道";

            _langdspen["PWM_Panel_Value"] = "Toggle panel switch when the chanel input above this value. Default 1200 be suitalbe for most RC";
            _langdspzh["PWM_Panel_Value"] = "当通道输出由低位大于这个值的时候，触发一次切换，默认1200适用于大多数遥控";

            _langdspen["ArmState"] = "";
            _langdspzh["ArmState"] = "解锁状态";

            _langdspen["BatteryVoltage"] = "";
            _langdspzh["BatteryVoltage"] = "电池电压";

            _langdspen["BatteryCurrent"] = "";
            _langdspzh["BatteryCurrent"] = "电池电流";

            _langdspen["BatteryConsumed"] = "Battery remaining";
            _langdspzh["BatteryConsumed"] = "剩余电量，百分比";

            _langdspen["FlightMode"] = "";
            _langdspzh["FlightMode"] = "飞行模式";

            _langdspen["GPSStatus"] = "GPS1's status";
            _langdspzh["GPSStatus"] = "GPS1 状态";

            _langdspen["GPSHDOP"] = "a measure of the GPS1's position accuracy";
            _langdspzh["GPSHDOP"] = "GPS1 水平精度";

            _langdspen["GPSLatitude"] = "GPS1 Latitude";
            _langdspzh["GPSLatitude"] = "GPS1 纬度";

            _langdspen["GPSLongitude"] = "GPS1 Longitude";
            _langdspzh["GPSLongitude"] = "GPS1 经度";

            _langdspen["GPS2Status"] = "GPS2's status";
            _langdspzh["GPS2Status"] = "GPS2 状态";

            _langdspen["GPS2HDOP"] = "a measure of the GPS2's position accuracy";
            _langdspzh["GPS2HDOP"] = "GPS2 水平精度";

            _langdspen["GPS2Latitude"] = "GPS2 Latitude";
            _langdspzh["GPS2Latitude"] = "GPS2 纬度";

            _langdspen["GPS2Longitude"] = "GPS2 Longitude";
            _langdspzh["GPS2Longitude"] = "GPS2 经度";

            _langdspen["Time"] = "";
            _langdspzh["Time"] = "飞行时间";

            _langdspen["Altitude"] = "";
            _langdspzh["Altitude"] = "高度";

            _langdspen["Altitude_TALT_Enable"] = "Traditional hud. 0:disable, 1:enable";
            _langdspzh["Altitude_TALT_Enable"] = "传统样式. 0:禁用, 1:启用";

            _langdspen["Altitude_Scale_Enable"] = "If show scale bar or not. 0:no, 1:yes";
            _langdspzh["Altitude_Scale_Enable"] = "是否要显示滚动条. 0:禁用, 1:启用";

            _langdspen["Scale_Align"] = "0:left, 1:right";
            _langdspzh["Scale_Align"] = "0:左 1:右";

            _langdspen["Speed"] = "";
            _langdspzh["Speed"] = "速度";

            _langdspen["Speed_TSPD_Enable"] = "Traditional hud. 0:disable, 1:enable";
            _langdspzh["Speed_TSPD_Enable"] = "传统样式. 0:禁用, 1:启用";

            _langdspen["Speed_Scale_Enable"] = "If show scale bar or not. 0:no, 1:yes";
            _langdspzh["Speed_Scale_Enable"] = "是否要显示滚动条. 0:禁用, 1:启用";

            _langdspen["Throttle"] = "";
            _langdspzh["Throttle"] = "油门";

            _langdspen["Throttle_Scale_Enable"] = "If show the throttle scroll bar or not. 0:disable, 1:enable";
            _langdspzh["Throttle_Scale_Enable"] = "是否显示油门滚动条. 0:禁用, 1:启用";

            _langdspen["HomeDistance"] = "";
            _langdspzh["HomeDistance"] = "家的距离";

            _langdspen["WPDistance"] = "Distance to next way-point";
            _langdspzh["WPDistance"] = "下个航点的距离";

            _langdspen["CHWDIR"] = "The display style of the direction of compass, home and way-point";
            _langdspzh["CHWDIR"] = "指南针，航点，家的方向的显示模式";

            _langdspen["CHWDIR_Tmode_Enable"] = "Use scale style or not. 0:No, 1:Yes";
            _langdspzh["CHWDIR_Tmode_Enable"] = "是否显示刻度条样式。0:否, 1:是";

            _langdspen["CHWDIR_Nmode_Enable"] = "Use animation style or not. 0:No, 1:Yes";
            _langdspzh["CHWDIR_Nmode_Enable"] = "是否显示动画样式。 0:否, 1:是";

            _langdspen["CHWDIR_Nmode_Radius"] = "The radio of the circle";
            _langdspzh["CHWDIR_Nmode_Radius"] = "圆圈的半径";

            _langdspen["CHWDIR_Nmode_Home_Radius"] = "The home display position from the center";
            _langdspzh["CHWDIR_Nmode_Home_Radius"] = "把家显示在离圆心多少距离的圆上";

            _langdspen["CHWDIR_Nmode_WP_Radius"] = "The way-point display position from the center";
            _langdspzh["CHWDIR_Nmode_WP_Radius"] = "把航点显示在离圆心多少距离的圆上";

            _langdspen["Alarm"] = "Alarm configuration";
            _langdspzh["Alarm"] = "警告设置";

            _langdspen["Alarm_GPS_Status_Enable"] = "Alarm when GPS no fixed. 0:disable, 1:enable ";
            _langdspzh["Alarm_GPS_Status_Enable"] = "GPS未锁定报警. 0:禁用, 1:启用";

            _langdspen["Alarm_Low_Batt_Enable"] = "Alarm when battery low.0:disable, 1:enable";
            _langdspzh["Alarm_Low_Batt_Enable"] = "电量过低报警.0:禁用, 1:启用";

            _langdspen["Alarm_Under_Speed_Enable"] = "Alarm when speed too low.0:disable, 1:enable";
            _langdspzh["Alarm_Under_Speed_Enable"] = "速度过低报警.0:禁用, 1:启用";

            _langdspen["Alarm_Over_Speed_Enable"] = "Alarm when speed too high.0:disable, 1:enable";
            _langdspzh["Alarm_Over_Speed_Enable"] = "速度过高报警.0:禁用, 1:启用";

            _langdspen["Alarm_Under_Alt_Enable"] = "Alarm when altitude too low.0:disable, 1:enable";
            _langdspzh["Alarm_Under_Alt_Enable"] = "高度过低报警.0:禁用, 1:启用";

            _langdspen["Alarm_Over_Alt_Enable"] = "Alarm when altitude too high.0:disable, 1:enable";
            _langdspzh["Alarm_Over_Alt_Enable"] = "高度过低报警.0:禁用, 1:启用";

            _langdspen["ClimbRate"] = "vertical speed";
            _langdspzh["ClimbRate"] = "爬升率，即垂直速度";

            _langdspen["RSSI"] = "First set the Raw_Enable to 1, then turn on and turn off your RC. Take the value from OSD to RSSI_Max and RSSI_Min. After that set Raw_Enable to 0";
            _langdspzh["RSSI"] = "首先MP里设置好，然后把Raw_Enable设为1.在OSD里观察打开，关闭遥控器得到的RSSI原始值，分别填到RSSI_Max, RSSI_Min,之后再把Raw_Enable设为0";

            _langdspen["RSSI_Min"] = "If Raw_Enable is 1, turn off your RC. Take the value of the screen to here";
            _langdspzh["RSSI_Min"] = "启用RSSI原始值的时候，关掉遥控器，屏幕上显示的值填到这里";

            _langdspen["RSSI_Max"] = "If Raw_Enable is 1, turn on your RC. Take the value of the screen to here";
            _langdspzh["RSSI_Max"] = "启用RSSI原始值的时候，打开遥控器，屏幕上显示的值填到这里";

            _langdspen["RSSI_Raw_Enable"] = "If show the raw value of RSSI or not. 0:percentage, 1:raw value";
            _langdspzh["RSSI_Raw_Enable"] = "是否显示RSSI原始值.0:百分比, 1:原始值";

            _langdspen["Wind_speed_dir"] = "Wind Speed and direction";
            _langdspzh["Wind_speed_dir"] = "风速,风向";

            _langdspen["Time_Type"] = "Time count start from 0:power on, 1:last heartbeat 2:armed";
            _langdspzh["Time_Type"] = "从哪里开始计时：0：上电 1：上次心跳 2：解锁";

            _langdspen["Throttle_Scale_Type"] = "0:Vertical, 1:Horizontal";
            _langdspzh["Throttle_Scale_Type"] = "0:垂直, 1:水平";
        }
    }
}
