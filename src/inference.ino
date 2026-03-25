#include <SSCMA_Micro_Core.h>

#include <Arduino.h>
#include <esp_camera.h>
#include <queue>

SET_LOOP_TASK_STACK_SIZE(40 * 1024);


SSCMAMicroCore instance;
SSCMAMicroCore::VideoCapture capture;

std::queue<int>results;
int sum=0;
void setup() {

    // Init serial port
    Serial.begin(115200);

    // Init video capture
    MA_RETURN_IF_UNEXPECTED(capture.begin(SSCMAMicroCore::VideoCapture::DefaultCameraConfigXIAOS3));

    // Init SSCMA Micro Core
    MA_RETURN_IF_UNEXPECTED(instance.begin(SSCMAMicroCore::Config::DefaultConfig));

    Serial.println("Init done");

}

void loop() {
    auto frame = capture.getManagedFrame();

    MA_RETURN_IF_UNEXPECTED(instance.invoke(frame));

    for (const auto& cls : instance.getClasses()) {
        if (results.size()>=5){
            if (results.front()==1){
                sum-=1;
            }
            results.pop();
        }
        if (cls.score>0.5){
            results.push(1);
            sum+=1;
        }
        else{
            results.push(0);
        }
        if(sum>=3){
            Serial.println("Not aligned");
        }
        else{
            Serial.println("Aligned");
        }
    }

    

    auto perf = instance.getPerf();
    Serial.printf("Perf: preprocess=%dms, inference=%dms, postprocess=%dms\n", perf.preprocess, perf.inference, perf.postprocess);

}