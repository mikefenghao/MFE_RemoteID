#include "BLE_TX.h"
#include "common/mavlink.h" // 确保路径正确


#include "GB46750-2025.h"
#include "opendroneid.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "mavlink_GB46750.h"
#include <ElegantOTA.h>
#include <Preferences.h>
#include <index_html.h>
Preferences prefs;
WebServer server(80);
char productID[21] = "SN123456789012345678";
char regMark[9] = "ABCDEFGH";
int opCategory = 0; // 0:未定义, 1:开放, 2:特定, 3:审定
int uaClass = 0;    // 0:微型, 1:轻型...
int baudRate = 57600;

static BLE_TX ble;
ODID_UAS_Data UAS_data;
GB46750_Data GB46750_Data_T;
// MAVLink 解析变量
mavlink_message_t mav_msg;
mavlink_status_t mav_status;
uint8_t get_gb46750_speed_accuracy(uint32_t vel_acc_mms) {
    // 如果值为 0 或未知
    if (vel_acc_mms == 0) return 0;

    float acc_ms = (float)vel_acc_mms / 1000.0f; // 毫米/秒 转为 米/秒

    if (acc_ms < 0.3f)  return 4; // 小于 0.3 m/s
    if (acc_ms < 1.0f)  return 3; // 小于 1 m/s
    if (acc_ms < 3.0f)  return 2; // 小于 3 m/s
    if (acc_ms < 10.0f) return 1; // 小于 10 m/s

    return 0; // 大于或等于 10 m/s 或未知
}
struct time_last_t{
    uint32_t system_time_lasttime;
}time_last;

uint8_t get_gb46750_vert_accuracy(uint32_t v_acc_mm) {
    // 如果值为 0，通常代表未知
    if (v_acc_mm == 0) return 0; 

    float acc_m = (float)v_acc_mm / 1000.0f; // 毫米转为米

    if (acc_m < 1.0f)   return 6; // 小于 1 m
    if (acc_m < 3.0f)   return 5; // 小于 3 m
    if (acc_m < 10.0f)  return 4; // 小于 10 m
    if (acc_m < 25.0f)  return 3; // 小于 25 m
    if (acc_m < 45.0f)  return 2; // 小于 45 m
    if (acc_m < 150.0f) return 1; // 小于 150 m

    return 0; // 大于或等于 150 m 或未知
}
uint8_t get_gb46750_horiz_accuracy(uint32_t h_acc_mm) {
    if (h_acc_mm == 0) return 0; // 未知

    float acc_m = (float)h_acc_mm / 1000.0f; // 转换为米

    if (acc_m < 1.0f)       return 12; // 小于 1 m
    if (acc_m < 3.0f)       return 11; // 小于 3 m
    if (acc_m < 10.0f)      return 10; // 小于 10 m
    if (acc_m < 30.0f)      return 9;  // 小于 30 m
    if (acc_m < 92.6f)      return 8;  // 小于 92.6 m (0.05 n mile)
    if (acc_m < 185.2f)     return 7;  // 小于 185.2 m (0.1 n mile)
    if (acc_m < 555.6f)     return 6;  // 小于 555.6 m (0.3 n mile)
    if (acc_m < 926.0f)     return 5;  // 小于 926 m (0.5 n mile)
    if (acc_m < 1852.0f)    return 4;  // 小于 1852 m (1 n mile)
    if (acc_m < 3704.0f)    return 3;  // 小于 3704 m (2 n mile)
    if (acc_m < 7408.0f)    return 2;  // 小于 7408 m (4 n mile)
    if (acc_m < 18520.0f)   return 1;  // 小于 18520 m (10 n mile)
    
    return 0; // 大于等于 18.52 km 或未知
}
void handle_mavlink() {
    while (Serial2.available() > 0) {
        uint8_t c = Serial2.read();
        if (mavlink_parse_char(MAVLINK_COMM_0, c, &mav_msg, &mav_status)) {
            switch (mav_msg.msgid) {
                case MAVLINK_MSG_ID_GLOBAL_POSITION_INT: {
                    mavlink_global_position_int_t gpi;
                    mavlink_msg_global_position_int_decode(&mav_msg, &gpi);
                    GB46750_Data_T.ua_lon = gpi.lon;
                    GB46750_Data_T.ua_lat = gpi.lat;
                    GB46750_Data_T.alt_geo = gpi.alt / 1000.0f;
                    GB46750_Data_T.alt_rel = gpi.relative_alt / 1000.0f;
                    //GB46750_Data_T.alt_baro = location_data.AltitudeGeo;
                    GB46750_Data_T.track_angle = gpi.hdg / 100.0f;
                    GB46750_Data_T.speed_gs = sqrt(pow(gpi.vx, 2) + pow(gpi.vy, 2)) / 100.0f;
                    GB46750_Data_T.speed_v = -gpi.vz/100.0f;

                    break;
                }
                case MAVLINK_MSG_ID_HEARTBEAT: {
                    mavlink_heartbeat_t hb;
                    mavlink_msg_heartbeat_decode(&mav_msg, &hb);
                   // 判断是否解锁
                    if (hb.base_mode & MAV_MODE_FLAG_SAFETY_ARMED) {
                        //location_data.Status = ODID_STATUS_AIRBORNE;
                        GB46750_Data_T.op_status = 2;//空中
                    } else {
                        GB46750_Data_T.op_status = 1;//地面
                    }
                    break;
                }
                case MAVLINK_MSG_ID_SCALED_PRESSURE: {
                    mavlink_scaled_pressure_t sp;
                    mavlink_msg_scaled_pressure_decode(&mav_msg,&sp);
                    GB46750_Data_T.alt_baro = (int)(44330.0f * (1.0f - pow((sp.press_abs / 1013.25f), 0.190295f)));

                    break;
                }
                case MAVLINK_MSG_ID_SYSTEM_TIME: {
                mavlink_system_time_t tb;
                mavlink_msg_system_time_decode(&mav_msg, &tb);
                //printf("Original Unix (us): %llu\n", tb.time_unix_usec);
                GB46750_Data_T.timestamp_ms = tb.time_unix_usec/1000;
                time_last.system_time_lasttime = millis();
                    break;
                }
case MAVLINK_MSG_ID_GPS_RAW_INT: {
    mavlink_gps_raw_int_t gps;
    mavlink_msg_gps_raw_int_decode(&mav_msg, &gps);
    GB46750_Data_T.horiz_accuracy = get_gb46750_horiz_accuracy(gps.h_acc);
    GB46750_Data_T.vert_accuracy = get_gb46750_vert_accuracy(gps.v_acc);
    GB46750_Data_T.speed_accuracy = get_gb46750_speed_accuracy(gps.vel_acc);

    break;
}
case MAVLINK_MSG_ID_HOME_POSITION: {
    mavlink_home_position_t home;
    mavlink_msg_home_position_decode(&mav_msg, &home);
        GB46750_Data_T.station_lat = home.latitude;
        GB46750_Data_T.station_lon = home.longitude;
        GB46750_Data_T.station_alt_geo = home.altitude/1000.0f;



    break;
}
            }
        }
    }
}
// 辅助函数：生成指定范围的随机浮点数
static float randomFloat(float min, float max) {
    return min + (float)rand() / ((float)RAND_MAX / (max - min));
}

// 辅助函数：生成指定范围的随机整数
static int randomInt(int min, int max) {
    return min + rand() % (max - min + 1);
}

// 主函数：填充模拟的无人机数据
void fillODID_ExampleData(ODID_UAS_Data &UAS_data) 
{
    // 清空整个结构体
    memset(&UAS_data, 0, sizeof(ODID_UAS_Data));
    
    // ============ Basic ID [0] ============
    UAS_data.BasicIDValid[0] = 1;
    UAS_data.BasicID[0].UAType = ODID_UATYPE_HELICOPTER_OR_MULTIROTOR;
    UAS_data.BasicID[0].IDType = ODID_IDTYPE_SERIAL_NUMBER;
    strcpy(UAS_data.BasicID[0].UASID, "DRONE12345678901234");
    
    // ============ Basic ID [1] (可选) ============
    UAS_data.BasicIDValid[1] = 1;
    UAS_data.BasicID[1].UAType = ODID_UATYPE_HELICOPTER_OR_MULTIROTOR;
    UAS_data.BasicID[1].IDType = ODID_IDTYPE_CAA_REGISTRATION_ID;
    strcpy(UAS_data.BasicID[1].UASID, "REG87654321ABCDEFGH");
    
    // ============ Location ============
    UAS_data.LocationValid = 1;
    UAS_data.Location.Status = ODID_STATUS_AIRBORNE;
    
    // 模拟飞行中的位置（例如：美国硅谷附近）
    UAS_data.Location.Latitude = 37.422 + randomFloat(-0.01, 0.01);    // 北纬37.422度
    UAS_data.Location.Longitude = -122.084 + randomFloat(-0.01, 0.01); // 西经122.084度
    UAS_data.Location.AltitudeBaro = 120.5f;   // 气压高度 120.5米
    UAS_data.Location.AltitudeGeo = 125.3f;    // 大地高度 125.3米
    UAS_data.Location.HeightType = ODID_HEIGHT_REF_OVER_TAKEOFF;
    UAS_data.Location.Height = 100.0f;         // 相对起飞点高度 100米
    
    // 飞行方向（0-359度，正北为0）
    UAS_data.Location.Direction = 45.0f;       // 向东北方向飞行
    UAS_data.Location.SpeedHorizontal = 12.5f; // 水平速度 12.5 m/s
    UAS_data.Location.SpeedVertical = 0.5f;    // 垂直速度 0.5 m/s（缓慢上升）
    
    // 精度信息
    UAS_data.Location.HorizAccuracy = createEnumHorizontalAccuracy(5.0f);  // 5米水平精度
    UAS_data.Location.VertAccuracy = createEnumVerticalAccuracy(3.0f);     // 3米垂直精度
    UAS_data.Location.BaroAccuracy = createEnumVerticalAccuracy(1.0f);     // 1米气压精度
    UAS_data.Location.SpeedAccuracy = createEnumSpeedAccuracy(1.0f);       // 1 m/s速度精度
    UAS_data.Location.TSAccuracy = createEnumTimestampAccuracy(0.1f);      // 0.1秒时间精度
    
    // 时间戳（UTC时间，从整点开始计算的秒数）
    time_t now = time(NULL);
    struct tm *utc_time = gmtime(&now);
    UAS_data.Location.TimeStamp = utc_time->tm_min * 60 + utc_time->tm_sec; // 从整点开始秒数
    
    // ============ Self ID ============
    UAS_data.SelfIDValid = 1;
    UAS_data.SelfID.DescType = ODID_DESC_TYPE_TEXT;
    strcpy(UAS_data.SelfID.Desc, "Search and Rescue Drone"); // 描述文本（最大23字符）
    
    // ============ System ============
    UAS_data.SystemValid = 1;
    UAS_data.System.OperatorLocationType = ODID_OPERATOR_LOCATION_TYPE_TAKEOFF;
    UAS_data.System.ClassificationType = ODID_CLASSIFICATION_TYPE_EU;
    
    // 操作者位置（起飞点）
    UAS_data.System.OperatorLatitude = 37.420;   // 起飞点纬度
    UAS_data.System.OperatorLongitude = -122.080; // 起飞点经度
    UAS_data.System.OperatorAltitudeGeo = 25.0f; // 起飞点海拔高度
    
    UAS_data.System.AreaCount = 1;
    UAS_data.System.AreaRadius = 500;     // 操作区域半径500米
    UAS_data.System.AreaCeiling = 150.0f; // 区域限高150米
    UAS_data.System.AreaFloor = 0.0f;     // 区域限低0米
    
    // EU分类信息
    UAS_data.System.CategoryEU = ODID_CATEGORY_EU_SPECIFIC;
    UAS_data.System.ClassEU = ODID_CLASS_EU_CLASS_3;
    
    UAS_data.System.Timestamp = (uint32_t)time(NULL); // Unix时间戳
    
    // ============ Operator ID ============
    UAS_data.OperatorIDValid = 1;
    UAS_data.OperatorID.OperatorIdType = ODID_OPERATOR_ID;
    strcpy(UAS_data.OperatorID.OperatorId, "OP123456789012345678");
    
    // ============ Auth (可选，这里不填充) ============
    // 大多数情况下不需要认证数据
    memset(&UAS_data.Auth, 0, sizeof(UAS_data.Auth));
    memset(UAS_data.AuthValid, 0, sizeof(UAS_data.AuthValid));
}
void printODIDData(const ODID_UAS_Data &data) {
    Serial.println("=== Open Drone ID Data ===");
    
    if (data.BasicIDValid[0]) {
        Serial.printf("Basic ID[0]: %s\n", data.BasicID[0].UASID);
    }
    
    if (data.LocationValid) {
        Serial.printf("Lat: %.6f, Lon: %.6f\n", 
                     data.Location.Latitude, 
                     data.Location.Longitude);
        Serial.printf("Alt: %.1fm, Height: %.1fm\n",
                     data.Location.AltitudeGeo,
                     data.Location.Height);
    }
}
void request_system_time_once() {
    mavlink_message_t msg;
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];

    // 使用 MAV_CMD_REQUEST_MESSAGE (ID: 512) 来请求特定的消息 ID
    mavlink_msg_command_long_pack(
        1,                          // 本机 System ID
        255,                        // 本机 Component ID (广播模块通常设为 255)
        &msg,
        1,                          // 目标 System ID (飞控一般为 1)
        1,                          // 目标 Component ID (飞控一般为 1)
        MAV_CMD_REQUEST_MESSAGE,    // 命令 ID: 512
        0,                          // 确认标志
        MAVLINK_MSG_ID_SYSTEM_TIME, // 参数 1: 想要请求的消息 ID (这里是 2)
        0, 0, 0, 0, 0, 0            // 参数 2-7: 无效
    );

    uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    Serial2.write(buf, len); // 通过连接飞控的串口发送
    
    //Serial.println("Sent request for SYSTEM_TIME to Flight Controller.");
}

void loadSettings() {
    prefs.begin("remoteid", true);
    String p = prefs.getString("pid", "SN000000000000000000");
    memset(GB46750_Data_T.uas_id, 0, sizeof(GB46750_Data_T.uas_id)); // 清空
    strncpy(GB46750_Data_T.uas_id, p.c_str(), 20); // 最多拷贝20字节

    String r = prefs.getString("reg", "00000000");
    memset(GB46750_Data_T.registration, 0, sizeof(GB46750_Data_T.registration));
    strncpy(GB46750_Data_T.registration, r.c_str(), 8);

    strncpy(productID, p.c_str(), 20);
    strncpy(regMark, r.c_str(), 8);
    opCategory = prefs.getInt("cat", 0);
    uaClass = prefs.getInt("uac", 0);

    GB46750_Data_T.run_category = (uint8_t)prefs.getInt("cat", 0);
    GB46750_Data_T.ua_category = (uint8_t)prefs.getInt("uac", 0);
    baudRate = prefs.getInt("baud", 57600);
    prefs.end();
    
}

void handleSave() {
    prefs.begin("remoteid", false);
    if (server.hasArg("pid")) prefs.putString("pid", server.arg("pid"));
    if (server.hasArg("reg")) prefs.putString("reg", server.arg("reg"));
    if (server.hasArg("cat")) prefs.putInt("cat", server.arg("cat").toInt());
    if (server.hasArg("uac")) prefs.putInt("uac", server.arg("uac").toInt());
    if (server.hasArg("baud")) prefs.putInt("baud", server.arg("baud").toInt());
    prefs.end();
    
    String message = "<center><h2 style='font-family:sans-serif;'>保存成功，设备正在重启...</h2><p>请在 5 秒后重新连接 WiFi 并刷新页面。</p></center>";
    server.send(200, "text/html; charset=utf-8", message);
    delay(2000);
    ESP.restart();
}

void handleRoot() {
    String s = index_html;
    // 简单的字符串替换来填充当前值（实际开发建议用更高效的方式）
    s.replace("%PID%", productID); s.replace("%REG%", regMark);
    s.replace("%C" + String(opCategory) + "%", "selected");
    s.replace("%A" + String(uaClass) + "%", "selected");
    s.replace("%B" + String(baudRate == 57600 ? 1 : 2) + "%", "selected");
    server.send(200, "text/html; charset=utf-8", s);
}

void initWiFi() {
    WiFi.mode(WIFI_AP); 
    delay(100); 

    String macStr = WiFi.softAPmacAddress();
    String suffix = macStr;
    suffix.replace(":", "");
    suffix = suffix.substring(suffix.length() - 6);
    suffix.toUpperCase();

    char ssid[32];
    sprintf(ssid, "RemoteID_%s", suffix.c_str());
    
    // 启动 AP
    if(WiFi.softAP(ssid, "12345678")) {
        Serial.print("Access Point Started. SSID: ");
        Serial.println(ssid);
        Serial.print("IP Address: ");
        Serial.println(WiFi.softAPIP());

        // --- 必须添加以下代码来启动 Web 服务 ---
        server.on("/", handleRoot);            // 绑定主页
        server.on("/save", HTTP_POST, handleSave); // 绑定保存接口
        ElegantOTA.begin(&server);             // 启动 OTA 接口
        server.begin();                        // 真正开启服务器
        Serial.println("HTTP server started");
        // ------------------------------------
    }
}
void setup()
{
    Serial.begin(115200);
    gb46750_fill_mock(&GB46750_Data_T);
    Serial.printf("Broadcast ID: %s, Reg: %s\n", GB46750_Data_T.uas_id, GB46750_Data_T.registration);
    loadSettings(); // 1. 先读配置
    srand(time(NULL));

 Serial2.begin(baudRate, SERIAL_8N1, 16, 17); 
 initWiFi(); // 3. 启动 Web 服务
//odid_initUasData(&UAS_data);
  //  fillODID_ExampleData(UAS_data);
    //gb46750_fill_mock(&GB46750_Data_T);
    // 打印数据用于验证
   
    
}
void loop()
{
    server.handleClient(); // 处理网页请求
    ElegantOTA.loop();     // 处理 OTA 任务
    handle_mavlink();
    static uint32_t last_send_time = 0;
    if (millis() - last_send_time >= 1000) {
        last_send_time = millis();
      //ble.transmit_longrange(UAS_data);
     ble.GB46750_LongSend(GB46750_Data_T);
     
    }
    //delay(100);
    if((millis()-time_last.system_time_lasttime)>=2000)
    {
        time_last.system_time_lasttime = millis();
     request_system_time_once();
    }
    //handle_mavlink();


}