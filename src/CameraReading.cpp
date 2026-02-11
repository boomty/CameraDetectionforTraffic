#include "esp_camera.h"
#include <WiFi.h>
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "fb_gfx.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <WebServer.h>

// ================= USER CONFIGURATION =================
const char* ssid = "YOUR_WIFI_SSID";         // <--- ENTER WIFI NAME
const char* password = "YOUR_WIFI_PASSWORD"; // <--- ENTER WIFI PASSWORD

// Paste your Teachable Machine Model URL here (must end with /)
// Example: "https://teachablemachine.withgoogle.com/models/AbCdEfGh/"
const char* URL_TM_MODEL = "https://teachablemachine.withgoogle.com/models/lzTjiQBVN/"; 
// ======================================================

// Pin definition for XIAO ESP32S3 Sense (OV2640)
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     10
#define SIOD_GPIO_NUM     40
#define SIOC_GPIO_NUM     39

#define Y9_GPIO_NUM       48
#define Y8_GPIO_NUM       11
#define Y7_GPIO_NUM       12
#define Y6_GPIO_NUM       14
#define Y5_GPIO_NUM       16
#define Y4_GPIO_NUM       18
#define Y3_GPIO_NUM       17
#define Y2_GPIO_NUM       15
#define VSYNC_GPIO_NUM    38
#define HREF_GPIO_NUM     47
#define PCLK_GPIO_NUM     13

WebServer server(80);

// HTML Page containing the TensorFlow.js & Teachable Machine logic
const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32-S3 x Teachable Machine</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; text-align: center; margin-top: 20px; background-color: #222; color: white; }
        #container { position: relative; display: inline-block; }
        img { width: 100%; max-width: 400px; border-radius: 8px; border: 2px solid #555; }
        #label-container { 
            margin-top: 10px; font-size: 24px; font-weight: bold; 
            padding: 10px; background: #333; border-radius: 5px;
        }
        .bar-container { width: 100%; max-width: 400px; margin: 5px auto; background: #444; height: 20px; border-radius: 5px; overflow: hidden; }
        .bar { height: 100%; background: #00ff00; width: 0%; transition: width 0.3s; }
    </style>
</head>
<body>
    <h2>Crosswalk Detector</h2>
    <div id="container">
        <img id="stream" src="" crossorigin="anonymous">
    </div>
    <div id="label-container">Loading Model...</div>
    <div id="bars"></div>

    <script src="https://cdn.jsdelivr.net/npm/@tensorflow/tfjs@latest/dist/tf.min.js"></script>
    <script src="https://cdn.jsdelivr.net/npm/@teachablemachine/image@latest/dist/teachablemachine-image.min.js"></script>

    <script type="text/javascript">
        // Inject the model URL from C++
        const URL = "%MODEL_URL%"; 
        
        let model, maxPredictions;
        let isModelLoaded = false;

        async function init() {
            const modelURL = URL + "model.json";
            const metadataURL = URL + "metadata.json";

            document.getElementById("label-container").innerText = "Loading Model...";
            
            try {
                model = await tmImage.load(modelURL, metadataURL);
                maxPredictions = model.getTotalClasses();
                isModelLoaded = true;
                document.getElementById("label-container").innerText = "Model Loaded. Starting...";
                
                // Set the stream source AFTER model loads to ensure smooth start
                const streamImg = document.getElementById('stream');
                streamImg.src = window.location.origin + ":81/stream";
                streamImg.onload = () => {
                    requestAnimationFrame(loop);
                };
            } catch (e) {
                document.getElementById("label-container").innerText = "Error loading model: " + e;
            }
        }

        async function loop() {
            if(isModelLoaded) {
                await predict();
            }
            requestAnimationFrame(loop);
        }

        async function predict() {
            const streamImg = document.getElementById('stream');
            // We use the image element directly as input
            const prediction = await model.predict(streamImg);

            // Display results
            const labelContainer = document.getElementById("label-container");
            const barsContainer = document.getElementById("bars");
            
            // Find the class with highest probability
            let bestClass = "";
            let bestProb = 0;

            let barsHtml = "";

            for (let i = 0; i < maxPredictions; i++) {
                const classPrediction = prediction[i].className + ": " + (prediction[i].probability * 100).toFixed(0) + "%";
                const probability = prediction[i].probability * 100;
                
                // Update simple bars
                barsHtml += `<div style="display:flex; justify-content:space-between; width: 400px; margin: 0 auto;">
                    <span>${prediction[i].className}</span>
                    <span>${probability.toFixed(0)}%</span>
                </div>
                <div class="bar-container"><div class="bar" style="width:${probability}%"></div></div>`;

                if (prediction[i].probability > bestProb) {
                    bestProb = prediction[i].probability;
                    bestClass = prediction[i].className;
                }
            }
            
            labelContainer.innerText = bestClass;
            barsContainer.innerHTML = barsHtml;
            
            // Optional: Send result BACK to ESP32 (if you want to light an LED)
            // fetch('/result?label=' + bestClass);
        }

        // Initialize on load
        window.onload = init;
    </script>
</body>
</html>
)rawliteral";

// Function to serve the video stream
void streamHandler(WiFiClient &client) {
  camera_fb_t * fb = NULL;
  char part_buf[64];
  static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=frame";
  static const char* _STREAM_BOUNDARY = "\r\n--frame\r\n";
  static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

  client.print("HTTP/1.1 200 OK\r\nContent-Type: ");
  client.print(_STREAM_CONTENT_TYPE);
  client.print("\r\n\r\n");

  while (true) {
    if (!client.connected()) break;
    
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      continue;
    }

    client.print(_STREAM_BOUNDARY);
    size_t hlen = snprintf(part_buf, 64, _STREAM_PART, fb->len);
    client.write((const uint8_t *)part_buf, hlen);
    client.write(fb->buf, fb->len);
    client.print("\r\n");
    
    esp_camera_fb_return(fb);
    
    // Small delay to allow other tasks
    delay(20); 
  }
}

// Handler for the main web page
void handleRoot() {
  String page = INDEX_HTML;
  // Replace placeholder with actual user Model URL
  page.replace("%MODEL_URL%", URL_TM_MODEL); 
  server.send(200, "text/html", page);
}

// Dedicated server for streaming (Port 81)
WiFiServer streamServer(81);

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  // --- Camera Config ---
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG; // Stream JPEG
  
  // Use lower resolution for faster inference/streaming
  config.frame_size = FRAMESIZE_QVGA;   // 320x240
  config.jpeg_quality = 12;             // 0-63, lower is higher quality
  config.fb_count = 2;

  // Initialize Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // --- WiFi Config ---
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("Camera Stream Ready! Go to: http://");
  Serial.println(WiFi.localIP());

  // --- Start Servers ---
  server.on("/", handleRoot);
  server.begin();
  
  streamServer.begin();
}

void loop() {
  server.handleClient();
  
  WiFiClient streamClient = streamServer.available();
  if (streamClient) {
    Serial.println("New Stream Client");
    streamHandler(streamClient);
    streamClient.stop();
  }
}