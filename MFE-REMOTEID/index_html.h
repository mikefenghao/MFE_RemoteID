#ifndef INDEX_HTML_H
#define INDEX_HTML_H

#include <Arduino.h>

const char index_html[] PROGMEM = R"rawliteral(

<!DOCTYPE HTML><html><head>
  <title>RemoteID Config</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; text-align: center; background-color: #f4f4f4; }
    .card { background: white; padding: 20px; border-radius: 10px; display: inline-block; box-shadow: 0 4px 8px rgba(0,0,0,0.1); margin-top: 20px; width: 90%; max-width: 400px; }
    input, select { width: 90%; padding: 10px; margin: 10px 0; border: 1px solid #ccc; border-radius: 5px; }
    button { background: #007bff; color: white; padding: 12px; border: none; border-radius: 5px; width: 95%; cursor: pointer; }
    button:hover { background: #0056b3; }
    .ota-link { display: block; margin-top: 15px; color: #666; font-size: 0.8em; }
  </style>
</head><body>
  <div class="card">
    <h2>RemoteID 设置</h2>
    <form action="/save" method="POST">
      <input type="text" name="pid" maxlength="20" placeholder="20位唯一产品识别码" value="%PID%">
      <input type="text" name="reg" maxlength="8" placeholder="8位实名登记标志" value="%REG%">
      <select name="cat">
        <option value="0" %C0%>未定义</option><option value="1" %C1%>开放类</option>
        <option value="2" %C2%>特定类</option><option value="3" %C3%>审定类</option>
      </select>
      <select name="uac">
        <option value="0" %A0%>微型</option><option value="1" %A1%>轻型</option>
        <option value="2" %A2%>小型</option><option value="3" %A3%>中型</option>
        <option value="4" %A4%>大型</option>
      </select>
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
