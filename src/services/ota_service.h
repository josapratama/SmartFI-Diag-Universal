/**
 * @file ota_service.h
 * @brief OTA (Over-The-Air) Firmware Update Service via WiFi.
 */

#ifndef OTA_SERVICE_H
#define OTA_SERVICE_H

#include <Arduino.h>
#include <Update.h>
#include <ESPAsyncWebServer.h>

class OTAService {
public:
    OTAService() : _progress(0), _isUpdating(false), _lastError("") {}

    /**
     * Register OTA upload endpoint on existing web server
     */
    void init(AsyncWebServer& server) {
        // Serve OTA upload page
        server.on("/ota", HTTP_GET, [](AsyncWebServerRequest *request) {
            String html = R"rawliteral(
<!DOCTYPE html><html><head><title>OTA Update</title>
<style>
body{font-family:sans-serif;max-width:600px;margin:50px auto;padding:20px}
.progress{width:100%;height:30px;background:#eee;border-radius:5px;overflow:hidden}
.bar{height:100%;background:#4CAF50;transition:width 0.3s}
</style></head><body>
<h2>Smart FI - Firmware Update</h2>
<form method="POST" action="/ota/upload" enctype="multipart/form-data" id="form">
<input type="file" name="firmware" accept=".bin" required>
<br><br><button type="submit">Upload & Flash</button>
</form>
<div class="progress"><div class="bar" id="bar" style="width:0%"></div></div>
<p id="status"></p>
<script>
document.getElementById('form').addEventListener('submit',function(e){
  e.preventDefault();
  var f=new FormData(this);
  var xhr=new XMLHttpRequest();
  xhr.open('POST','/ota/upload');
  xhr.upload.onprogress=function(e){
    if(e.lengthComputable){
      var pct=Math.round(e.loaded/e.total*100);
      document.getElementById('bar').style.width=pct+'%';
      document.getElementById('status').textContent='Uploading: '+pct+'%';
    }
  };
  xhr.onload=function(){
    document.getElementById('status').textContent=xhr.responseText;
    if(xhr.status==200) setTimeout(function(){location.reload()},3000);
  };
  xhr.send(f);
});
</script></body></html>
)rawliteral";
            request->send(200, "text/html", html);
        });

        // Handle firmware upload
        server.on("/ota/upload", HTTP_POST,
            // Response handler (called after upload completes)
            [this](AsyncWebServerRequest *request) {
                _isUpdating = false;
                if (Update.hasError()) {
                    _lastError = "Update FAILED: " + String(Update.errorString());
                    request->send(500, "text/plain", _lastError);
                } else {
                    request->send(200, "text/plain", "Update OK! Rebooting...");
                    delay(1000);
                    ESP.restart();
                }
            },
            // Upload handler (called for each chunk)
            [this](AsyncWebServerRequest *request, const String& filename, size_t index, 
                   uint8_t *data, size_t len, bool final) {
                if (index == 0) {
                    Serial.printf("[OTA] Starting update: %s\n", filename.c_str());
                    _isUpdating = true;
                    _progress = 0;
                    
                    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                        Serial.println("[OTA] Not enough space!");
                        Update.printError(Serial);
                    }
                }

                if (Update.isRunning()) {
                    if (Update.write(data, len) != len) {
                        Update.printError(Serial);
                    }
                    _progress = index + len;
                }

                if (final) {
                    if (Update.end(true)) {
                        Serial.printf("[OTA] Update complete: %u bytes\n", index + len);
                    } else {
                        Update.printError(Serial);
                    }
                    _isUpdating = false;
                }
            }
        );

        Serial.println("[OTA] Service initialized at /ota");
    }

    bool isUpdating() const { return _isUpdating; }
    size_t getProgress() const { return _progress; }
    String getLastError() const { return _lastError; }

private:
    size_t _progress;
    bool _isUpdating;
    String _lastError;
};

#endif // OTA_SERVICE_H
