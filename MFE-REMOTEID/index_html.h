#ifndef INDEX_HTML_H
#define INDEX_HTML_H

#include <Arduino.h>

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>RemoteID Config</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; text-align: center; background-color: #f4f4f4; padding: 20px; }
    .card { background: white; padding: 20px; border-radius: 10px; display: inline-block; box-shadow: 0 4px 8px rgba(0,0,0,0.1); width: 100%; max-width: 400px; text-align: left; }
    h2 { text-align: center; color: #333; }
    label { display: block; margin-top: 15px; font-weight: bold; color: #555; font-size: 0.9em; }
    input, select { width: 100%; padding: 10px; margin-top: 5px; border: 1px solid #ccc; border-radius: 5px; box-sizing: border-box; }
    button { background: #007bff; color: white; padding: 12px; border: none; border-radius: 5px; width: 100%; cursor: pointer; margin-top: 25px; font-size: 1em; }
    button:hover { background: #0056b3; }
    .ota-link { display: block; text-align: center; margin-top: 20px; color: #666; font-size: 0.85em; text-decoration: none; }
    .ota-link:hover { text-decoration: underline; }
  </style>
</head><body>
  <div class="card">
    <h2>RemoteID 设置</h2>
    <form action="/save" method="POST">
      
      <label>唯一产品识别码 (20位):</label>
      <input type="text" name="pid" maxlength="20" placeholder="请输入PID" value="%PID%">
      
      <label>实名登记标志 (8位):</label>
      <input type="text" name="reg" maxlength="8" placeholder="请输入登记号" value="%REG%">
      
      <label>运行类别:</label>
      <select name="cat">
        <option value="0" %C0%>未定义</option>
        <option value="1" %C1%>开放类</option>
        <option value="2" %C2%>特定类</option>
        <option value="3" %C3%>审定类</option>
      </select>
      
      <label>航空器分类:</label>
      <select name="uac">
        <option value="0" %A0%>微型</option>
        <option value="1" %A1%>轻型</option>
        <option value="2" %A2%>小型</option>
        <option value="3" %A3%>中型</option>
        <option value="4" %A4%>大型</option>
      </select>
      
      <label>串口波特率:</label>
      <select name="baud">
        <option value="57600" %B1%>57600</option>
        <option value="115200" %B2%>115200</option>
      </select>
      
      <button type="submit">保存并重启</button>
    </form>
    <a class="ota-link" href="/update">固件更新 (OTA)</a>
  </div>
</body></html>)rawliteral";

#endif