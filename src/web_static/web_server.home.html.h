static const char CONTENT_HOME_HTML[] PROGMEM = 
  "<!doctype html> <html lang=\"en\"> <head> <meta charset=\"utf-8\"> <title>emonESP</title> <meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"> <meta name=\"viewport\" content=\"width=device-width,user-scalable=no\"> <meta name=\"description\" content=\"emonESP\"> <meta name=\"author\" content=\"emonESP\"> <meta name=\"theme-color\" content=\"#000000\"> <link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\"> </head> <body> <div id=\"page\"> <div class=\"box\"> <h1 data-bind=\"text: config.node_name\">emonESP</h1> <h3 data-bind=\"text: config.node_description\">WiFi EmonCMS Link for CircuitSetup Energy Meters</h3> </div> <div class=\"loading\" data-bind=\"visible: !initialised()\"> Loading, please wait... </div> <div id=\"container\" data-bind=\"visible: initialised\" style=\"display: none\"> <div id=\"one\"> <h2 onclick=\"toggle('wifi');\"><span>+</span> WiFi Network</h2> <div id=\"wifi\" data-bind=\"visible: !status.isWifiClient()\" class=\"itembody-wrapper\" style=\"display:none\"> <p><b>Mode:</b> <span id=\"mode\" data-bind=\"text: status.fullMode\"></span></p> <div id=\"client-view\" data-bind=\"visible: !wifi.canConfigure() &amp;&amp; !wifi.wifiConnecting()\"> <table> <tr> <th>Network</th> <th>RSSI dBm</th> </tr> <tbody id=\"sta-ssid\"> <tr> <td data-bind=\"text: config.ssid\"></td> <td data-bind=\"text: status.srssi\"></td> </tr> </tbody> </table> <button data-bind=\"click: function () { wifi.forceConfig(true); }\">Change WiFi network</button> <p><b>IP Address:</b><br><a data-bind=\"text: status.ipaddress, attr: {href: 'http://'+status.ipaddress()}\"></a></p> <button id=\"apoff\" data-bind=\"visible: status.isWifiAccessPoint\">Turn off Access Point</button> </div> <div id=\"ap-view\" data-bind=\"visible: wifi.canConfigure() &amp;&amp; !wifi.wifiConnecting()\"> <p>Connect to network:</p> <div id=\"wifiList\"> <ul class=\"list-group\" data-bind=\"foreach: scan.filteredResults, visible: 0 != scan.results().length\"> <li class=\"list-group-item\" data-bind=\"click: $root.wifi.select, css: { active: $root.wifi.selectedNet() === $data }\"> <span data-bind=\"text: ssid\"></span> <img width=\"24px\" height=\"24px\" data-bind=\"attr:{src: 'wifi_signal_'+strength()+'.svg'}\"> </li> </ul> </div> <div data-bind=\"visible: 0 == scan.results().length\"> Scanning... </div> <p> <b>SSID:</b><br> <input type=\"text\" autocapitalize=\"none\" data-bind=\"textInput: config.ssid\"> </p> <p> <b>Passkey:</b><br> <input type=\"text\" autocapitalize=\"none\" data-bind=\"textInput: wifiPassword.value, attr: { type: wifiPassword.show() ? 'text' : 'password' }\"><br> </p><div> <input id=\"wifipassword\" type=\"checkbox\" data-bind=\"checked: wifiPassword.show\"> <label for=\"wifipassword\">Show password</label> </div> <p></p> <p> <button data-bind=\"click: wifi.saveNetwork, text: (wifi.saveNetworkFetching() ? 'Saving' : (wifi.saveNetworkSuccess() ? 'Saved' : 'Connect')), disable: wifi.saveNetworkFetching\">Connect</button> </p> </div> <div data-bind=\"visible: wifi.wifiConnecting\"> <p>Connecting to WiFi Network...Please wait 10s</p> </div> </div> </div> <div id=\"four\"> <h2 onclick=\"toggle('latest');\"><span>+</span> Latest Data</h2> <div id=\"latest\" data-bind=\"visible: status.isWifiClient() && last.lastValues()\" class=\"itembody-wrapper\" style=\"display:none\"> <table> <tr> <th>Key</th> <th>Value</th> </tr> <tbody data-bind=\"foreach: last.values\"> <tr> <td data-bind=\"text: key\"></td> <td data-bind=\"text: value\"></td> </tr> </tbody> </table><br> <div class=\"small-text\"> Input key value pairs received via serial UART (e.g \"CT1:3935,CT2:325\") or HTTP input API e.g.: <br> <a data-bind=\"attr: {href: 'http://'+status.ipaddress()+'/input?string=CT1:3935,CT2:325,T0:20.5'}\">http://<span data-bind=\"text: status.ipaddress\"></span>/input?string=CT1:3935,CT2:325,T0:20.5</a> </div> </div> </div> <div id=\"two\"> <h2 onclick=\"toggle('emoncms');\"><span>+</span> Emoncms</h2> <div id=\"emoncms\" class=\"itembody-wrapper\" style=\"display:none\"> <p> <input type=\"checkbox\" id=\"emoncms_enabled\" data-bind=\"checked: config.emoncms_enabled\"> <label for=\"emoncms_enabled\"><b>Enable Emoncms</b></label> </p> <p data-bind=\"visible: config.emoncms_enabled\"> <b>Emoncms Server*:</b><br> <input type=\"text\" data-bind=\"textInput: config.emoncms_server\"><br> <span> e.g '<a href=\"https://emoncms.org\">emoncms.org</a>', 'emonpi', or '192.168.1.4' </span><br> </p> <p data-bind=\"visible: config.emoncms_enabled\"> <b>Emoncms Path:</b><br> <input type=\"text\" data-bind=\"textInput: config.emoncms_path\"><br> <span> e.g '/emoncms', or empty for emoncms.org </span><br> </p> <p data-bind=\"visible: config.emoncms_enabled\"> <b>Emoncms Node Name*:</b><br> <input type=\"text\" data-bind=\"textInput: config.emoncms_node\"> </p> <p data-bind=\"visible: config.emoncms_enabled\"> <b>Write apikey*:</b><br> <input type=\"password\" autocapitalize=\"none\" data-bind=\"textInput: emoncmsApiKey.value, attr: { type: emoncmsApiKey.show() ? 'text' : 'password' }\"><br> <span> <input id=\"emoncmsApiKey\" type=\"checkbox\" data-bind=\"checked: emoncmsApiKey.show\"> <label for=\"emoncmsApiKey\">Show password</label> </span> </p> <p data-bind=\"visible: config.emoncms_enabled\"> <b>SSL SHA-1 Fingerprint:</b><br> <input type=\"text\" data-bind=\"textInput: config.emoncms_fingerprint\"><br> <br>HTTPS will be enabled if present e.g:<br> <br> <span class=\"small-text\"> 7D:82:15:BE:D7:BC:72:58:87:7D:8E:40:D4:80:BA:1A:9F:8B:8D:DA <br> </span></p> <p> <button data-bind=\"click: saveEmonCms, text: (saveEmonCmsFetching() ? 'Saving' : (saveEmonCmsSuccess() ? 'Saved' : 'Save')), disable: saveEmonCmsFetching\">Save</button> <span data-bind=\"visible: config.emoncms_enabled\"> <b>&nbsp; Connected:&nbsp;<span data-bind=\"text: 1 === status.emoncms_connected() ? 'Yes' : 'No'\"></span></b> <span data-bind=\"visible: 1 === status.emoncms_connected()\"><br><br><b>&nbsp; Successful messages:&nbsp;<span data-bind=\"text: status.packets_success()+'/'+status.packets_sent()+' '+((status.packets_success()/status.packets_sent())*100)+'%'\"></span></b></span> </span> </p> </div> </div> <div id=\"three\"> <h2 onclick=\"toggle('mqtt');\"><span>+</span> MQTT</h2> <div id=\"mqtt\" class=\"itembody-wrapper\" style=\"display:none\"> <p> <input type=\"checkbox\" id=\"mqtt_enabled\" data-bind=\"checked: config.mqtt_enabled\"> <label for=\"mqtt_enabled\"><b>Enable MQTT</b></label> </p> <p data-bind=\"visible: config.mqtt_enabled\"> <b>MQTT Server*:</b><br> <input data-bind=\"textInput: config.mqtt_server\" type=\"text\"><br> <span class=\"small-text\">e.g 'emonpi', 'test.mosquitto.org' or '192.168.1.4'</span><br> </p> <p data-bind=\"visible: config.mqtt_enabled\"> <b>MQTT Port:</b><br> <input data-bind=\"textInput: config.mqtt_port\" type=\"text\"><br> <span class=\"small-text\">e.g 1883</span><br> </p> <p data-bind=\"visible: config.mqtt_enabled\"> <b>MQTT Base-topic*:</b><br> <input data-bind=\"textInput: config.mqtt_topic\" type=\"text\"><br> <span class=\"small-text\"> e.g 'emon/emonesp', 'emon/heatpump'<br> Data will be published to sub topic e.g. 'emon/emonesp/CT1' </span> </p> <p data-bind=\"visible: config.mqtt_enabled\"> <b>MQTT Feed-name prefix:</b><br> <input data-bind=\"textInput: config.mqtt_feed_prefix\" type=\"text\"><br> <span class=\"small-text\"> Value to prefix the feed names with: &#60;base-topic&#62;/&#60;prefix&#62;&#60;feed-name&#62;. Required for posting to AdafruitIO. Leave blank for no prefix. </span> </p> <p data-bind=\"visible: config.mqtt_enabled\"> <b>Username:</b><br> <input data-bind=\"textInput: config.mqtt_user\" type=\"text\"><br> <span class=\"small-text\">Leave blank for authentication</span> </p> <p data-bind=\"visible: config.mqtt_enabled\"> <b>Password:</b><br> <input type=\"password\" autocapitalize=\"none\" data-bind=\"textInput: mqttPassword.value, attr: { type: mqttPassword.show() ? 'text' : 'password' }\"><br> <span> <input id=\"mqttPassword\" type=\"checkbox\" data-bind=\"checked: mqttPassword.show\"> <label for=\"mqttPassword\">Show password</label> </span> <span class=\"small-text\">Leave blank for no authentication</span><br> </p> <p> <button data-bind=\"click: saveMqtt, text: (saveMqttFetching() ? 'Saving' : (saveMqttSuccess() ? 'Saved' : 'Save')), disable: saveMqttFetching\">Save</button> <b>&nbsp; Connected:&nbsp;<span data-bind=\"text: 1 === status.mqtt_connected() ? 'Yes' : 'No'\"></span></b> </p> </div> </div> <div id=\"nine\"> <h2 onclick=\"toggle('calibration');\"><span>+</span> Calibration Settings</h2> <div id=\"calibration\" class=\"itembody-wrapper\" style=\"display:none\"> <br> <p> <b>Voltage:</b><br> <input type=\"text\" value=\"voltage_cal\" data-bind=\"textInput: config.voltage_cal\"><br> <span class=\"small-text\">37106 - 9v AC Transformer - Jameco 157041 <br>38302 - 9v AC Transformer - Jameco 112336 <br>29462 - 12v AC Transformer - Jameco 167151 <br>For meter versions >= 1.4 rev3: <br>3920 - 9v AC Transformer - Jameco 157041 (default) </span><br> </p> <p> <b>CT1:</b><br> <input type=\"text\" data-bind=\"textInput: config.ct1_cal\"><br> <span class=\"small-text\">39571 - SCT-010 80A/26.6mA <br>25498 - SCT-013-000 100A/50mA <br>39473 - SCT-016 120A/40mA (default) <br>46539 - Magnalab 100A <br>26315 - SCT-024 200A/100mA</span><br> </p> <p> <b>CT2:</b><br> <input type=\"text\" data-bind=\"textInput: config.ct2_cal\"><br> <span class=\"small-text\">39571 - SCT-010 80A/26.6mA <br>25498 - SCT-013-000 100A/50mA <br>39473 - SCT-016 120A/40mA (default) <br>46539 - Magnalab 100A <br>26315 - SCT-024 200A/100mA</span><br> </p> <p> <b>Frequency:</b><br> <input type=\"text\" data-bind=\"textInput: config.freq_cal\"><br> <span class=\"small-text\">60 Hz - North America: 4485 (default) <br>50 Hz - Europe: 389</span> </p> <p> <b>CT Gain (PGA):</b><br> <input type=\"text\" data-bind=\"textInput: config.gain_cal\"><br> <span class=\"small-text\"> 1x: 0 <br>2x: 21 (default) <br>4x: 42</span> <br><br> <button id=\"save_cal\" data-bind=\"click: saveCal, text: (saveCalFetching() ? 'Saving' : (saveCalSuccess() ? 'Saved' : 'Save')), disable: saveCalFetching\">Save</button><br> <span>After saving, the ESP will be restarted and values sent to the energy meter</span> </p> <br><br> </div> </div> <div id=\"nine\"> <h2 onclick=\"toggle('control');\"><span>+</span> Control</h2> <div id=\"control\" data-bind=\"visible: status.isWifiClient() && (config.node_type()=='hpmon' || config.node_type()=='smartplug')\" class=\"itembody-wrapper\" style=\"display:none; text-align:center\"> <br> <div class=\"btn-group\"> <button id=\"on\" data-bind=\"click: ctrlMode.bind($data,'On'), css: (status.ctrl_mode()=='On'?'green':'')\">On</button><button id=\"off\" data-bind=\"click: ctrlMode.bind($data,'Off'), css: (status.ctrl_mode()=='Off'?'red':'')\">Off</button><button id=\"timer\" data-bind=\"click: ctrlMode.bind($data,'Timer'), css: (status.ctrl_mode()=='Timer'?(status.ctrl_state()==1?'green':'red'):'')\">Timer</button> </div> <p><b>Time now:</b> <span data-bind=\"text: status.time\">12:34:56</span></p> <div style=\"\"><b>Period 1</b></div> <div style=\"display:inline-block; margin:10px\">Start: <input type=\"time\" data-bind=\"textInput: config.f_timer_start1\"></div> <div style=\"display:inline-block; margin:10px\">Stop: <input type=\"time\" data-bind=\"textInput: config.f_timer_stop1\"></div> <br><br> <div style=\"\"><b>Period 2</b></div> <div style=\"display:inline-block; margin:10px\">Start: <input type=\"time\" data-bind=\"textInput: config.f_timer_start2\"></div> <div style=\"display:inline-block; margin:10px\">Stop: <input type=\"time\" data-bind=\"textInput: config.f_timer_stop2\"></div> <br><br> <p data-bind=\"visible: config.node_type()=='hpmon'\"> Flow temperature: <input type=\"text\" data-bind=\"textInput: config.flowT\"> </p> <p>Time offset (seconds): <input type=\"text\" data-bind=\"textInput: config.time_offset\" style=\"width:80px\"></p> <button data-bind=\"click: saveTimer, text: (saveTimerFetching() ? 'Saving' : (saveTimerSuccess() ? 'Saved' : 'Save')), disable: saveTimerFetching\">Save</button> <br><br> </div> </div> <div id=\"five\"> <h2 onclick=\"toggle('admin');\"><span>+</span> Admin</h2> <div id=\"admin\" class=\"itembody-wrapper\" style=\"display:none\"> <p> <b>Username:</b><br> <input type=\"text\" value=\"admin\" data-bind=\"textInput: config.www_username\"> </p> <p> <b>Password:</b><br> <input type=\"password\" autocapitalize=\"none\" data-bind=\"textInput: wwwPassword.value, attr: { type: wwwPassword.show() ? 'text' : 'password' }\" pattern=\".{15}\" title=\"The field requires 15 characters\" maxlength=\"15\" required=\"\"><br> <span> <input id=\"wwwPassword\" type=\"checkbox\" data-bind=\"checked: wwwPassword.show\"> <label for=\"wwwPassword\">Show password</label> </span><br> <span>Web interface HTTP authentication.</span><br><br> <button data-bind=\"click: saveAdmin, text: (saveAdminFetching() ? 'Saving' : (saveAdminSuccess() ? 'Saved' : 'Save')), disable: saveAdminFetching\">Save</button> </p> </div> </div> <div id=\"six\"> <h2 onclick=\"toggle('firmware');\"><span>+</span> Firmware</h2> <div id=\"firmware\" class=\"itembody-wrapper\" style=\"display:none\"> <p><b>Version:</b> <a href=\"https://github.com/CircuitSetup/EmonESP/releases\">v<span data-bind=\"text: config.version\"></span></a></p> <div data-bind=\"visible: !updateFetching() && !updateComplete()\"> <form action=\"#\" method=\"post\" enctype=\"multipart/form-data\" id=\"upload_form\" data-bind=\"submit: otaUpdate\"> <input type=\"file\" name=\"update\" data-bind=\"value: updateFilename\"> <button data-bind=\"click: otaUpdate, text: (updateFetching() ? 'Updating...' : 'Update'), disable: ('' == updateFilename())\">Update</button> </form> </div> <div data-bind=\"visible: (updateError() !== '')\" class=\"box error\"> <h4>Error:</h4> <p data-bind=\"text: updateError\"></p> </div> <div data-bind=\"visible: updateFetching() && !updateComplete()\"> Updating... <br> <div id=\"progressBack\"> <div id=\"progressBar\" data-bind=\"style: { width: updateProgress()+'%' }\"></div> </div> </div> <p data-bind=\"visible: updateComplete\"> Firmware update completed ok </p> </div> </div> <div id=\"seven\"> <h2 onclick=\"toggle('system');\"><span>+</span> System</h2> <div id=\"system\" class=\"itembody-wrapper\" style=\"display:none\"> <p><b>Free RAM:</b> <span data-bind=\"text: scaleString(status.free_heap(), 1024, 0) + 'K'\"></span></p> <p><b>Flash Size:</b> <span data-bind=\"text: scaleString(config.espflash(), 1024, 0) + 'K'\"></span></p> <p><b>Version:</b> <a href=\"https://github.com/CircuitSetup/EmonESP/releases\">v<span data-bind=\"text: config.version\"></span></a></p> <button id=\"restart\">Restart</button> <button id=\"reset\">Factory Reset</button> </div> </div> <div id=\"eight\"> <h2 onclick=\"toggle('logs');\"><span>+</span> Logs</h2> <div id=\"logs\" class=\"itembody-wrapper scrollable-logs\" style=\"display:none\"> <table> <tr> <th>Timestamp</th> <th>Log</th> </tr> <tbody data-bind=\"foreach: last.entries\"> <tr> <td data-bind=\"text: timestamp\"></td> <td data-bind=\"text: log\"></td> </tr> </tbody> </table><br> <h3>Serial Consoles</h3> <div> <button onclick=\"window.open('term.html?debug');\">Debug</button> </div> </div> </div> </div> </div> <div style=\"clear:both\"></div> <div id=\"footer-small-scrn\"> Powered by <a href=\"http://openenergymonitor.org\"><span>Open</span>EnergyMonitor.org</a> </div> <div id=\"footer-large-scrn\"> Powered by <a href=\"http://openenergymonitor.org\"><span>Open</span>EnergyMonitor.org</a> </div> <script src=\"lib.js\" type=\"text/javascript\"></script> <script src=\"config.js\" type=\"text/javascript\"></script> </body> </html> \n";
