#ifndef GB46750_2025_H
#define GB46750_2025_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// --- 运行类别枚举 ---
// --- 003: 运行类别枚举 (Operating Category) ---
// 反映无人机当前的运行管理属性
typedef enum {
    GB_RUN_UNDEFINED = 0, // 未定义
    GB_RUN_OPEN      = 1, // 开放类：风险较低，通常指微型/轻型无人机在视距内运行
    GB_RUN_SPECIFIC  = 2, // 特定类：中等风险，需经过特定运行风险评估
    GB_RUN_CERTIFIED = 3  // 审定类：高风险，类似于民航载人飞机的适航管理标准
} GB_RunCategory;

// --- 004: 航空器分类枚举 (UA Category) ---
// 根据重量、速度等物理特性进行的分类
typedef enum {
    GB_UA_MICRO  = 0, // 微型无人机：空机重量小于0.25kg，设计性能极低
    GB_UA_LIGHT  = 1, // 轻型无人机：空机重量小于4kg，或起飞重量小于7kg
    GB_UA_SMALL  = 2, // 小型无人机：起飞重量在7kg至25kg之间
    GB_UA_MEDIUM = 3, // 中型无人机：起飞重量在25kg至150kg之间
    GB_UA_LARGE  = 4  // 大型无人机：起飞重量大于150kg
} GB_UACategory;

// --- 005: 遥控站位置类型枚举 (Location Type) ---
// 定义 006 项经纬度坐标代表的是哪里
typedef enum {
    GB_LOC_TYPE_TAKEOFF = 0, // 起飞点位置：无人机起飞时的坐标
    GB_LOC_TYPE_REMOTE  = 1  // 遥控器位置：当前操控人员/遥控站的实时坐标
} GB_LocationType;

// --- 015: 运行状态枚举 (Operational Status) ---
// 反映航空器当前的实时动力和安全状态
typedef enum {
    GB_STATUS_UNREPORTED = 0, // 未报告/未知
    GB_STATUS_GROUND     = 1, // 地面运行：在地面滑行或未起飞状态
    GB_STATUS_AIRBORNE   = 2, // 飞行：已离地，处于正常飞行过程中
    GB_STATUS_EMERGENCY  = 3, // 紧急情况：如失控、进入禁飞区等告警状态
    GB_STATUS_FAIL_NORM  = 4, // 普通失效：部分非核心传感器或链路异常
    GB_STATUS_FAIL_EMER  = 5  // 严重失效：可能导致坠毁的核心系统故障
} GB_OpStatus;

// --- 016: 坐标系枚举 (Coordinate System) ---
// 决定了报文中经纬度数据的基准
typedef enum {
    GB_COORD_WGS84    = 0, // WGS-84：全球卫星定位系统通用坐标系（默认常用）
    GB_COORD_CGCS2000 = 1  // CGCS2000：中国国家大地坐标系（国内高精度推荐）
} GB_CoordSystem;

// --- 数据结构体 (符合 GB 46750-2025 规范) ---
typedef struct {
    // --- 基础信息 ---
    char     uas_id[20];          // 001: 唯一产品识别码 (SN)，标准长度20字节
    char     registration[8];     // 002: 实名登记号，标准长度8字节
    uint8_t  run_category;        // 003: 运行类别 (0:未定义, 1:开放, 2:特定, 3:审定)
    uint8_t  ua_category;         // 004: 航空器分类 (0:微型, 1:轻型, 2:小型, 3:中型, 4:大型)

    // --- 遥控站/起飞点信息 ---
    uint8_t  station_loc_type;    // 005: 坐标类型 (0:起飞点, 1:遥控器/站)
    int32_t  station_lon;         // 006: 站经度，单位 1e-7 度
    int32_t  station_lat;         // 006: 站纬度，单位 1e-7 度
    float    station_alt_geo;     // 007: 站大地高度(海拔)，单位m (编码需+1000m偏移)

    // --- 航空器实时位置与姿态 ---
    int32_t  ua_lon;              // 008: 航空器经度，单位 1e-7 度
    int32_t  ua_lat;              // 008: 航空器纬度，单位 1e-7 度
    float    track_angle;         // 009: 航迹角，范围 0.0~359.9 度
    float    speed_gs;            // 010: 地速，单位 m/s
    float    alt_rel;             // 011: 相对高度(相对于起飞点)，单位m (编码需+9000m偏移)
    float    speed_v;             // 012: 垂直速度，单位 m/s (正为升，负为降)
    float    alt_geo;             // 013: 航空器大地高度(海拔)，单位m (编码需+1000m偏移)
    float    alt_baro;            // 014: 航空器气压高度，单位m (编码需+1000m偏移)

    // --- 状态与精度 ---
    uint8_t  op_status;           // 015: 运行状态 (1:地面, 2:空中, 3:紧急等)
    uint8_t  coord_system;        // 016: 坐标系 (0:WGS-84, 1:CGCS2000)
    uint8_t  horiz_accuracy;      // 017: 水平精度，查表值 (如11代表<3m)
    uint8_t  vert_accuracy;       // 018: 垂直精度，查表值
    uint8_t  speed_accuracy;      // 019: 速度精度，查表值
    uint64_t timestamp_ms;        // 020: 毫秒级时间戳 (Unix Epoch)
    uint8_t  ts_accuracy;         // 021: 时间戳精度，查表值
} GB46750_Data;

int gb46750_encode_full(const GB46750_Data *data, uint8_t *buffer);
void gb46750_fill_mock(GB46750_Data *data);

#ifdef __cplusplus
}
#endif
#endif