#include <SSCMA_Micro_Core.h>
#include <Arduino.h>
#include <esp_camera.h>
#include <queue>

#include "AudioTools.h"
#include "Aligned.h"

SET_LOOP_TASK_STACK_SIZE(40 * 1024);

SSCMAMicroCore instance;
SSCMAMicroCore::VideoCapture capture;

std::queue<int> results;
int sum = 0;

bool wasNotAligned = false;

uint8_t channels = 1;
uint16_t sample_rate = 22050;
uint8_t bits_per_sample = 16;

MemoryStream music(__440_raw, __440_raw_len);
I2SStream i2s;  // Output to I2S
StreamCopy copier(i2s, music); // copies sound into i2s
VolumeStream volume(i2s);
LSM9DS1 imu;
int x,y,z;


void setup() {
    Serial.begin(115200);
    AudioToolsLogger.begin(Serial, AudioToolsLogLevel::Info);   

    MA_RETURN_IF_UNEXPECTED(capture.begin(SSCMAMicroCore::VideoCapture::DefaultCameraConfigXIAOS3));

    MA_RETURN_IF_UNEXPECTED(instance.begin(SSCMAMicroCore::Config::DefaultConfig));

    Serial.println("Init done");

    auto config = i2s.defaultConfig(TX_MODE);
    config.pin_ws = 44; //44 , 7 , 43 ESP-32S3 Sense
    config.pin_bck = 7;
    config.pin_data = 43;
    config.sample_rate = sample_rate;
    config.channels = channels;
    config.bits_per_sample = bits_per_sample;

    i2s.begin(config);
    music.begin();

    auto vcfg = volume.defaultConfig();
    vcfg.copyFrom(config);
    volume.begin(vcfg); // we need to provide the bits_per_sample and channels
    volume.setVolume(30);

    if (imu.begin() != 0) {
    Serial.println("Failed to communicate with LSM9DS1.");
    while (true) {
        exit(0);
    }
  }

}

void loop() {
    imu.readGyro();
    bool isNotAligned = (sum >= 3);

    if (isNotAligned) {
        Serial.println("Not aligned");
        //check past xyz, and use that to determine which way
        if(imu.calcMag(imu.mx)-x<0){
            Serial.println("Turn left");
        }
        else if(imu.calcMag(imu.mx)-x>0){
            Serial.println("Turn right");
        }
    }
    else {
        Serial.println("Aligned!");
        if (!copier.copy()){
            i2s.end();
            stop();
        }
        x=imu.calcMag(imu.mx);
        y=imu.calcMag(imu.my);
        z=imu.calcMag(imu.mz);
     }
        
    wasNotAligned = isNotAligned;
    

    auto perf = instance.getPerf();
    Serial.printf(
        "Perf: preprocess=%dms, inference=%dms, postprocess=%dms\n",
        perf.preprocess,
        perf.inference,
        perf.postprocess
    );


    auto frame = capture.getManagedFrame();
    MA_RETURN_IF_UNEXPECTED(instance.invoke(frame));

    bool hasClass = false;

    for (const auto& cls : instance.getClasses()) {
        hasClass = true;

        if (results.size() >= 5) {
            if (results.front() == 1) {
                sum -= 1;
            }
            results.pop();
        }

        if (cls.score > 0.5) {
            results.push(1);
            sum += 1;
        } else {
            results.push(0);
        }
    }
}
