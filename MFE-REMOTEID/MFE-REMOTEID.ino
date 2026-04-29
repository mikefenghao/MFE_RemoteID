#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEAdvertising.h>
#include "opendroneid.h" 
#include "common/mavlink.h" // 确保路径正确
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <HTTPUpdateServer.h>
#include "index_html.h"  // <--- 引入刚才创建的网页文件


WebServer server(80);
HTTPUpdateServer httpUpdater;
Preferences prefs;

int baud_rate = 115200;
uint8_t saved_ua_type = 2; // 默认多旋翼
uint8_t saved_id_type = 1; // 默认序列号

// --- [ 1. 资源定义 ] ---
const uint8_t header[] = { 0x1e, 0x16, 0xfa, 0xff, 0x0d };
BLEAdvertising *pAdvertising;
uint8_t msg_counters[10] = {0}; 
uint8_t legacy_phase = 0;      
uint8_t second_flag = 0;       

// MAVLink 解析变量
mavlink_message_t mav_msg;
mavlink_status_t mav_status;

// --- [ 2. 核心数据结构 ] ---
ODID_BasicID_data    basicID_data[2];  
ODID_Location_data   location_data;    
ODID_SelfID_data     selfID_data;      
ODID_System_data     system_data;      
ODID_OperatorID_data operatorID_data;  

// 网页回调函数
void handleRoot() {
    String html = String(index_html);
    prefs.begin("mfe_config", true);
    String c_uasid = prefs.getString("uasid", "");
    String c_opid = prefs.getString("opid", "");
    String c_self_desc = prefs.getString("self_desc", "MFE RemoteID");
    int c_self_type = prefs.getInt("self_type", 0);
    prefs.end();

    // 替换 HTML 模板中的占位符
    html.replace("name=\"uasid\"", "name=\"uasid\" value=\"" + c_uasid + "\"");
    html.replace("name=\"opid\"", "name=\"opid\" value=\"" + c_opid + "\"");
    html.replace("name=\"self_desc\"", "name=\"self_desc\" value=\"" + c_self_desc + "\"");
    server.send(200, "text/html", html); // 发送替换后的 html 变量
}

void handleSave() {
    String new_uasid = server.arg("uasid");
    String new_opid = server.arg("opid");
    int new_baud = server.arg("baud").toInt();
    int new_ua_type = server.arg("ua_type").toInt();
    int new_id_type = server.arg("id_type").toInt();
    String new_self_desc = server.arg("self_desc");
    int new_self_type = server.arg("self_type").toInt();

if (new_uasid.length() > ODID_ID_SIZE) new_uasid = new_uasid.substring(0, ODID_ID_SIZE);
    if (new_opid.length() > ODID_ID_SIZE) new_opid = new_opid.substring(0, ODID_ID_SIZE);
    if (new_self_desc.length() > ODID_STR_SIZE) new_self_desc = new_self_desc.substring(0, ODID_STR_SIZE);

    prefs.begin("mfe_config", false);
    prefs.putString("uasid", new_uasid);
    prefs.putString("opid", new_opid);
    prefs.putInt("baud", new_baud);
    prefs.putInt("ua_type", new_ua_type);
    prefs.putInt("id_type", new_id_type);
    prefs.putString("self_desc", new_self_desc);
    prefs.putInt("self_type", new_self_type);
    prefs.end();

    server.send(200, "text/html", "<h1>OK! Config Saved. Device Rebooting...</h1>");
    delay(2000);
    ESP.restart();
}



// --- [ 3. MAVLink 解析函数 ] ---
void handle_mavlink() {
    while (Serial2.available() > 0) {
        uint8_t c = Serial2.read();
        if (mavlink_parse_char(MAVLINK_COMM_0, c, &mav_msg, &mav_status)) {
            switch (mav_msg.msgid) {
                case MAVLINK_MSG_ID_GLOBAL_POSITION_INT: {
                    mavlink_global_position_int_t gpi;
                    mavlink_msg_global_position_int_decode(&mav_msg, &gpi);

                    // 将飞控数据映射到 RemoteID 结构体
                    location_data.Latitude = gpi.lat / 10000000.0;
                    location_data.Longitude = gpi.lon / 10000000.0;
                    location_data.AltitudeGeo = gpi.alt / 1000.0f;
                    location_data.Height = gpi.relative_alt / 1000.0f;
                    location_data.AltitudeBaro = location_data.AltitudeGeo;
                    
                    if (location_data.Latitude == 0 && location_data.Longitude == 0) break;
                    // 计算水平速度 (cm/s -> m/s)
                    location_data.SpeedHorizontal = sqrt(pow(gpi.vx, 2) + pow(gpi.vy, 2)) / 100.0f;
                    location_data.Direction = gpi.hdg / 100.0f;
                    
                    // 精度处理
                    location_data.HorizAccuracy = createEnumHorizontalAccuracy(10.0f);
                    //location_data.TimeStamp = (millis() / 100) % 36000;
                    //location_data.TimeStamp = (uint16_t)((millis() / 100) % 36000);
                    location_data.TimeStamp = (float)fmod((millis() / 1000.0), 3600.0);
                    Serial.printf("Current TimeStamp: %.1f s\n", location_data.TimeStamp);
                    //Serial.printf("time is %u\r\n",location_data.TimeStamp);
                    break;
                }
                case MAVLINK_MSG_ID_HEARTBEAT: {
                    mavlink_heartbeat_t hb;
                    mavlink_msg_heartbeat_decode(&mav_msg, &hb);
                   // 判断是否解锁
                    if (hb.base_mode & MAV_MODE_FLAG_SAFETY_ARMED) {
                        location_data.Status = ODID_STATUS_AIRBORNE;
                    } else {
                        location_data.Status = ODID_STATUS_GROUND;
                    }
                    break;
                }
case MAVLINK_MSG_ID_GPS_RAW_INT: {
    mavlink_gps_raw_int_t gps;
    mavlink_msg_gps_raw_int_decode(&mav_msg, &gps);

    // 1. 垂直精度 (注意这里改成了 VertAccuracy)
    float v_acc_m = gps.v_acc / 1000.0f; 
    location_data.VertAccuracy = createEnumVerticalAccuracy(v_acc_m);
   // Serial.printf("GPS 原始垂直精度 (mm): %u\n", gps.v_acc);
    // 2. 水平精度 (顺便检查一下这个，通常是 HorizAccuracy)
    float h_acc_m = gps.h_acc / 1000.0f;
    location_data.HorizAccuracy = createEnumHorizontalAccuracy(h_acc_m);
    //Serial.printf("GPS 原始垂直精度 (mm): %u\n", gps.h_acc);
    // 3. 速度精度 (如果报错，可以去 .h 文件里搜一下 SpeedAcc 关键字)
    float s_acc_ms = gps.vel_acc / 1000.0f;
    location_data.SpeedAccuracy = createEnumSpeedAccuracy(s_acc_ms);
    //Serial.printf("GPS 原始垂直精度 (mm): %u\n", gps.vel_acc);
    //location_data.BaroAccuracy = 
    break;
}
case MAVLINK_MSG_ID_HOME_POSITION: {
    mavlink_home_position_t home;
    mavlink_msg_home_position_decode(&mav_msg, &home);

    // 1. 坐标转换：MAVLink 原始数据是 degE7 (整数)，需要除以 1e7
    system_data.OperatorLatitude = home.latitude / 10000000.0;
    system_data.OperatorLongitude = home.longitude / 10000000.0;
    
    // 2. 高度转换：单位从 mm 转为 m
    system_data.OperatorAltitudeGeo = home.altitude / 1000.0f; 

    // 3. 设置操作员位置类型
    // 在 RemoteID 协议中，1 代表“固定坐标”（即起飞点/Home点）
    system_data.OperatorLocationType = ODID_OPERATOR_LOCATION_TYPE_FIXED;

    // 4. 设置分类（可选：通常默认 0 即可，除非有特定法规需求）
    system_data.ClassificationType = ODID_CLASSIFICATION_TYPE_UNDECLARED;


    break;
}
            }
        }
    }
}
void request_remoteid_messages() {
    mavlink_message_t msg;
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];

    // 设置消息发送频率
    // 参数 1: 消息 ID (12901 代表 OPEN_DRONE_ID_LOCATION)
    // 参数 2: 间隔时间 (单位：微秒。200000us = 200ms = 5Hz)
    mavlink_msg_command_long_pack(
        1, 255, &msg, 
        1, 1,             // 目标系统 ID, 目标组件 ID (通常是 1, 1)
        MAV_CMD_SET_MESSAGE_INTERVAL, 
        0,                // 确认
        12901,            // 参数 1: 消息 ID
        200000,           // 参数 2: 间隔 (微秒)
        0, 0, 0, 0, 0     // 其他参数不用管
    );

    uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    Serial1.write(buf, len); // 假设你的飞控接在 Serial1
    Serial.println("已请求飞控发送 RemoteID 透传包...");
}

// --- [ 4. 广播逻辑 ] ---
void broadcast_odid() {
    uint8_t packet[31];
    memset(packet, 0, 31);
    memcpy(packet, header, 5);
    if (legacy_phase == 5) legacy_phase = 6;
    if (legacy_phase > 6) legacy_phase = 0;
    switch (legacy_phase) {
        case 0: { //发送位置信息
            packet[5] = msg_counters[0]++; 
            // 注意：这里不再执行 odid_initLocationData，否则会擦除 handle_mavlink 存入的数据
            ODID_Location_encoded location_enc;
            encodeLocationMessage(&location_enc, &location_data);
            memcpy(&packet[6], &location_enc, ODID_MESSAGE_SIZE);
            send_packet(packet, 31);

            break;
        }
        case 1: { //发送民航信息飞机信息
            packet[5] = msg_counters[1]++;
            ODID_BasicID_encoded basicID_enc;
            encodeBasicIDMessage(&basicID_enc, &basicID_data[0]);
            memcpy(&packet[6], &basicID_enc, ODID_MESSAGE_SIZE);
            send_packet(packet, 31);
            break;
        }
        case 2: { //发送签名
            packet[5] = msg_counters[2]++;
            ODID_SelfID_encoded selfID_enc;
            encodeSelfIDMessage(&selfID_enc, &selfID_data);
            memcpy(&packet[6], &selfID_enc, ODID_MESSAGE_SIZE);
            send_packet(packet, 31);
            break;
        }
        case 3: { //
            packet[5] = msg_counters[3]++;
            ODID_System_encoded system_enc;
            encodeSystemMessage(&system_enc, &system_data);
            memcpy(&packet[6], &system_enc, ODID_MESSAGE_SIZE);
            send_packet(packet, 31);
            break;
        }
        case 4: { //发送驾驶证信息
            packet[5] = msg_counters[4]++;
            ODID_OperatorID_encoded operatorID_enc;
            encodeOperatorIDMessage(&operatorID_enc, &operatorID_data);
            memcpy(&packet[6], &operatorID_enc, ODID_MESSAGE_SIZE);
            send_packet(packet, 31);
            break;
        }
        case 6: { //发送系统蓝牙数据
            const uint8_t name_header[] = { 0x02, 0x01, 0x06, 11, 0x09 }; 
            memcpy(packet, name_header, 5);
            memcpy(&packet[5], "MFE_S3_FLY", 10);
            send_packet(packet, 15);
            break;
        }
    }

    legacy_phase++;

}

void send_packet(uint8_t* data, int len) {
    BLEAdvertisementData oAdvData;
    String payload = "";
    for (int i = 0; i < len; i++) payload += (char)data[i];
    oAdvData.addData(payload);
    pAdvertising->setAdvertisementData(oAdvData);
}

// --- [ 5. 主程序 ] ---
void setup() {
    // 1. 优先读取配置
    prefs.begin("mfe_config", true);
    String s_id = prefs.getString("uasid", ""); 
    String o_id = prefs.getString("opid", "");
    baud_rate = prefs.getInt("baud", 57600); // 新设备默认 57600
    saved_ua_type = (uint8_t)prefs.getInt("ua_type", 0); // 默认 None
    saved_id_type = (uint8_t)prefs.getInt("id_type", 0); // 默认 None
    String s_desc = prefs.getString("self_desc", "NONE"); // 默认值
    int s_type = prefs.getInt("self_type", 0);
    prefs.end();

    // 2. 启动串口
    Serial.begin(115200);
    Serial2.begin(baud_rate, SERIAL_8N1, 16, 17); 

    // 3. 初始化 ODID 结构体 (只应用配置的值)
    odid_initLocationData(&location_data);

    odid_initBasicIDData(&basicID_data[0]);
    // 注意：你发来的库里枚举是 ODID_idtype_t，不是 ODID_id_type_t
    basicID_data[0].IDType = (ODID_idtype_t)saved_id_type; 
    if (s_id.length() > 0) {
        strncpy(basicID_data[0].UASID, s_id.c_str(), ODID_ID_SIZE);
    }

    odid_initSystemData(&system_data);
    basicID_data[0].UAType = (ODID_uatype_t)saved_ua_type; // 修正后的层级

     odid_initOperatorIDData(&operatorID_data);
    if (o_id.length() > 0) {
        strncpy(operatorID_data.OperatorId, o_id.c_str(), ODID_ID_SIZE);
    }
    odid_initSelfIDData(&selfID_data);
    selfID_data.DescType = (ODID_desctype_t)s_type; // 应用网页选的类型
    if (s_desc.length() > 0) {
        strncpy(selfID_data.Desc, s_desc.c_str(), ODID_STR_SIZE);
    }


    // 4. 启动 WiFi 配置门户
// 1. 获取 6 字节的原始 MAC
uint8_t mac[6];
WiFi.macAddress(mac);

// 2. 使用最后两个字节生成十六进制字符串
char hexID[5];
sprintf(hexID, "%02X%02X", mac[4], mac[5]); // 取最后两个字节，比如 EE 和 FF

// 3. 拼接
String apName = "RemoteID-" + String(hexID);
WiFi.softAP(apName.c_str(), "12345678");
    httpUpdater.setup(&server, "/update");
    server.on("/", handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.begin();
    
    // 5. 启动蓝牙广播
    BLEDevice::init("");
    pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->setAdvertisementType(ADV_TYPE_NONCONN_IND);
    pAdvertising->setScanResponse(false);
    pAdvertising->start();
    
    Serial.println("MFE System Started.");
}

void loop() {
     
    server.handleClient(); // 处理网页请求
    // 1. 尽可能快地解析串口数据
    handle_mavlink();

    // 2. 定时发送广播
    static unsigned long last_ms = 0;
    if (millis() - last_ms > 100) { 
        last_ms = millis();
        broadcast_odid();
    }

}