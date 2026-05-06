#include "GB46750-2025.h"
#include <string.h>
#include <math.h>

// 辅助写入函数
static void write_le16(uint8_t **p, uint16_t v) { memcpy(*p, &v, 2); *p += 2; }
static void write_le32(uint8_t **p, uint32_t v) { memcpy(*p, &v, 4); *p += 4; }

// --- 虚拟赋值函数：填入符合规范的所有参数 ---
void gb46750_fill_mock(GB46750_Data *data) {
    if (!data) return;
    memset(data, 0, sizeof(GB46750_Data));

    // --- 基础信息段 (001-004) ---
    // 唯一产品识别码 (SN): 20字节。必须唯一，包含生产商代码等
    strncpy(data->uas_id, "00000000000000000000", 20); 
    // 实名登记号: 8字节。在民航局登记系统获取的编号
    strncpy(data->registration, "12345678", 8); 
    // 运行类别: 0-开放类, 1-特定类, 2-审定类[cite: 2]
    data->run_category = GB_RUN_OPEN; 
    // 航空器分类: 0-微型, 1-轻型, 2-小型, 3-中型, 4-大型[cite: 2]
    data->ua_category = GB_UA_LIGHT;

    // --- 遥控站/起飞点信息 (005-007) ---[cite: 2]
    // 位置类型: 0-动态(遥控器), 1-静态(起飞点), 2-固定位置[cite: 2]
    data->station_loc_type = GB_LOC_TYPE_TAKEOFF;
    // 经纬度精度: 1e-7 度。 1214737000 = 121.4737°[cite: 2]
    data->station_lon = 0; 
    data->station_lat = 0;  
    // 大地高度 (海拔): 单位米(m)。 偏移1000m, 分辨率0.5m[cite: 2]
    data->station_alt_geo = 0;

    // --- 航空器动态位置与姿态 (008-014) ---[cite: 2]
    // 航空器当前位置 (经纬度): 1e-7 度精度[cite: 2]
    data->ua_lon = 1214740000; 
    data->ua_lat = 312310000;
    // 航迹角: 顺时针方向 0.0-359.9度。单位0.1度[cite: 2]
    data->track_angle = 0;
    // 地速: 航空器相对于地面的水平速度。单位0.1m/s[cite: 2]
    data->speed_gs = 0;
    // 相对高度: 相对于起飞点的高度。单位m, 偏移9000m, 分辨率0.5m[cite: 2]
    data->alt_rel = 0;
    // 垂直速度: 单位m/s。 正数表示上升，负数表示下降[cite: 2]
    data->speed_v = 0;   
    // 大地高度: 海拔高度。单位m, 偏移1000m, 分辨率0.5m[cite: 2]
    data->alt_geo = 0;
    // 气压高度: 基于标准气压(101.325kPa)测得的高度[cite: 2]
    data->alt_baro = 0;

    // --- 状态与精度 (015-021) ---[cite: 2]
    // 运行状态: 0-地面, 1-空中, 2-紧急, 3-失效[cite: 2]
    data->op_status = GB_STATUS_UNREPORTED;
    // 坐标系: 0-WGS84, 1-CGCS2000[cite: 2]
    data->coord_system = GB_COORD_WGS84;
    // 水平精度: 查表值 (例如: 11代表误差 < 3m)[cite: 2]
    data->horiz_accuracy = 0; 
    // 垂直精度: 查表值 (例如: 5代表误差 < 3m)[cite: 2]
    data->vert_accuracy = 0;   
    // 速度精度: 查表值 (例如: 3代表误差 < 1m/s)[cite: 2]
    data->speed_accuracy = 0;  
    // 时间戳: 1970年至今的毫秒数 (Unix Epoch in ms)[cite: 2]
    data->timestamp_ms = 1714454400000ULL; //1714454400000ULL
    // 时间戳精度: 查表值 (例如: 8代表误差 < 10ms)[cite: 2]
    data->ts_accuracy = 5;     
}
//gb46750_encode_full
// --- 编码函数：严格遵循 5.2 章节协议格式 ---
/**
 * @brief 按照 GB 46750-2025 强制打包 001-021 所有数据项
 */
int gb46750_encode_full(const GB46750_Data *data, uint8_t *buffer) {
    uint8_t *p = buffer;

    // 1. 报文头
    *p++ = 0xFF; // 类型: 255
    *p++ = 0x20; // 协议版本 V1.0
    uint8_t *len_ptr = p++; // 长度占位

    // 2. 数据标识 (DI) - 声明 001 到 021 全部存在
    *p++ = 0xFF; // 字节1: 包含001-007, 扩展位1
    *p++ = 0xFF; // 字节2: 包含008-014, 扩展位1[cite: 2]
    *p++ = 0xFE; // 字节3: 包含015-021, 扩展位0 (结束)[cite: 2]
uint8_t *data_start = p; // 记录正文的“起跑线”，此时 data_start 指向 001 项
    // 3. 数据内容 (按顺序一个都不能少)
    
    // --- 001-007 ---
    memcpy(p, data->uas_id, 20); p += 20;            // 001: SN
    memcpy(p, data->registration, 8); p += 8;        // 002: 实名登记
    *p++ = data->run_category;                       // 003: 运行类别
    *p++ = data->ua_category;                        // 004: 分类
    *p++ = data->station_loc_type;                   // 005: 站位置类型
    write_le32(&p, data->station_lon);               // 006: 站经度 (小端)[cite: 2]
        write_le32(&p, data->station_lat);               // 008: 站纬度 (小端)[cite: 2]
        write_le16(&p, (uint16_t)((data->station_alt_geo + 1000) * 2)); // 007: 站高度[cite: 2]
    write_le32(&p, data->ua_lon);                    // 008: 航空器经度[cite: 2]
    write_le32(&p, data->ua_lat);                    // 008: 航空器纬度[cite: 2]


    // --- 008-014 ---
        write_le16(&p, (uint16_t)(data->track_angle * 10)); // 009: 航迹角[cite: 2]
            write_le16(&p, (uint16_t)(data->speed_gs * 10));    // 010: 地速[cite: 2]
                write_le16(&p, (uint16_t)((data->alt_rel + 9000) * 2)); // 011: 相对高度[cite: 2]




    
    // 012: 垂直速度 (1字节: 符号1位 + 数值7位)
    uint8_t vs_val = (uint8_t)(fabsf(data->speed_v) * 2);
    if (vs_val > 0x7F) vs_val = 0x7F;
    if (data->speed_v < 0) vs_val |= 0x80; // 下降为1[cite: 2]
    *p++ = vs_val;

    write_le16(&p, (uint16_t)((data->alt_geo + 1000) * 2));  // 013: 大地高度[cite: 2]
    write_le16(&p, (uint16_t)((data->alt_baro + 1000) * 2)); // 014: 气压高度[cite: 2]

    // --- 015-021 ---
    *p++ = data->op_status;      // 015: 运行状态
    *p++ = data->coord_system;   // 016: 坐标系
    *p++ = data->horiz_accuracy; // 017: 水平精度
    *p++ = data->vert_accuracy;  // 018: 垂直精度
    *p++ = data->speed_accuracy; // 019: 速度精度
    
    // 020: 时间戳 (6字节小端)[cite: 2]
    uint64_t ts = data->timestamp_ms;
    for(int i=0; i<6; i++) {
        *p++ = (uint8_t)(ts & 0xFF);
        ts >>= 8;
    }
    *p++ = data->ts_accuracy;    // 021: 时间戳精度

    // 4. 长度回填 (从DI开始到最后的字节总数)
    //*len_ptr = (uint8_t)(p - (len_ptr + 1)); 
*len_ptr = (uint8_t)(p - data_start);
    return (int)(p - buffer); // 返回总长度，大概 80+ 字节
}