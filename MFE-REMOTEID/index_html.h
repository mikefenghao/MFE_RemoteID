#ifndef INDEX_HTML_H
#define INDEX_HTML_H

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>MFE RemoteID 配置终端</title>
    <style>
        body { font-family: -apple-system, sans-serif; background: #f0f2f5; margin: 0; padding: 20px; display: flex; justify-content: center; }
        .card { background: white; padding: 24px; border-radius: 16px; box-shadow: 0 4px 20px rgba(0,0,0,0.1); width: 100%; max-width: 450px; }
        h2 { color: #007aff; text-align: center; margin-bottom: 24px; font-size: 20px; border-bottom: 2px solid #007aff; padding-bottom: 10px; }
        .item { margin-bottom: 18px; }
        label { display: block; margin-bottom: 6px; font-size: 14px; color: #444; font-weight: bold; }
        input, select { width: 100%; padding: 12px; border: 1px solid #ccc; border-radius: 8px; box-sizing: border-box; font-size: 15px; background: #fff; }
        input:focus, select:focus { border-color: #007aff; outline: none; box-shadow: 0 0 0 3px rgba(0,122,255,0.1); }
        button { width: 100%; background: #007aff; color: white; padding: 14px; border: none; border-radius: 8px; font-size: 16px; font-weight: 600; margin-top: 10px; cursor: pointer; transition: 0.3s; }
        button:hover { background: #0056b3; }
        .footer { text-align: center; margin-top: 20px; font-size: 11px; color: #999; }
    </style>
</head>
<body>
    <div class="card">
        <h2> 终端配置 Terminal</h2>
        <form action="/save" method="POST">
            
            <div class="item">
                <label>BASIC ID (产品唯一识别码)</label>
                <input type="text" name="uasid" placeholder="例如: 1581F..." maxlength="20" required>
            </div>

            <div class="item">
                <label>ID TYPE (识别码类型)</label>
                <select name="id_type">
                    <option value="1">SERIAL_NUMBER (厂商序列号)</option>
                    <option value="2">CAA_REGISTRATION_ID (民航注册码)</option>
                    <option value="3">UTM_ASSIGNED_UUID (UTM分配ID)</option>
                    <option value="4">SPECIFIC_SESSION_ID (特定任务ID)</option>
                    <option value="0">NONE (无)</option>
                </select>
            </div>

            <div class="item">
                <label>UA TYPE (机型类型)</label>
                <select name="ua_type">
                    <option value="1">AEROPLANE (固定翼飞机)</option>
                    <option value="2">HELICOPTER_OR_MULTIROTOR (直升机/多旋翼)</option>
                    <option value="3">GYROPLANE (旋翼机)</option>
                    <option value="4">HYBRID_LIFT (垂直起降 VTOL)</option>
                    <option value="5">ORNITHOPTER (扑翼机)</option>
                    <option value="6">GLIDER (滑翔机)</option>
                    <option value="7">KITE (风筝)</option>
                    <option value="8">FREE_BALLOON (自由气球)</option>
                    <option value="9">CAPTIVE_BALLOON (系留气球)</option>
                    <option value="10">AIRSHIP (飞艇)</option>
                    <option value="11">FREE_FALL_PARACHUTE (降落伞)</option>
                    <option value="12">ROCKET (火箭)</option>
                    <option value="13">TETHERED_POWERED_AIRCRAFT (有动力系留机)</option>
                    <option value="14">GROUND_OBSTACLE (地面障碍物)</option>
                    <option value="0">NONE (无)</option>
                </select>
            </div>

            <div class="item">
                <label>OPERATOR ID (操作员识别码)</label>
                <input type="text" name="opid" placeholder="例如: CN-123..." maxlength="20" required>
            </div>

            <div class="item">
                <label>BAUD RATE (串口波特率)</label>
                <select name="baud">
                    <option value="115200">115200</option>
                    <option value="57600">57600</option>
                </select>
            </div>
           <div class="item">
                <label>SELF ID TYPE (签名性质)</label>
                <select name="self_type">
                    <option value="0">TEXT (普通文本)</option>
                    <option value="1">EMERGENCY (紧急警报)</option>
                    <option value="2">EXTENDED (扩展状态)</option>
                </select>
            </div>

            <div class="item">
                <label>SELF ID DESC (个性签名/任务描述)</label>
                <input type="text" name="self_desc" placeholder="例如: 正在电力巡检..." maxlength="23">
            </div>

            <button type="submit">保存并应用 Save & Apply</button>
        </form>
        <div class="item" style="margin-top: 20px; border-top: 1px solid #eee; padding-top: 10px;">
    <label>固件系统 (Firmware Update)</label>
    <button type="button" onclick="location.href='/update'" style="background: #666;">进入 OTA 升级模式</button>
     </div>
        <div class="footer">MFE RemoteID Project v1.0</div>
    </div>
</body>
</html>
)rawliteral";

#endif