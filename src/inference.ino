#include <SSCMA_Micro_Core.h>
#include <Arduino.h>
#include <esp_camera.h>
#include <queue>

#include "IsNotAlignedAudioCode.ino"
#include "IsAlignedAudioCode.ino"

SET_LOOP_TASK_STACK_SIZE(40 * 1024);

SSCMAMicroCore instance;
SSCMAMicroCore::VideoCapture capture;

std::queue<int> results;
int sum = 0;

bool wasNotAligned = false;

void setup() {
    Serial.begin(115200);

    MA_RETURN_IF_UNEXPECTED(capture.begin(SSCMAMicroCore::VideoCapture::DefaultCameraConfigXIAOS3));

    MA_RETURN_IF_UNEXPECTED(instance.begin(SSCMAMicroCore::Config::DefaultConfig));

    audioBegin();

    Serial.println("Init done");
}

void loop() {
    audioUpdate();

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

    bool isNotAligned = (sum >= 3);

    if (isNotAligned) {
        Serial.println("Not aligned");

        if (!wasNotAligned) {
            IsAlignedAudio();
        }
    } else {
        Serial.println("Aligned");

        if (wasNotAligned) {
            NotAlignedAudio();
        }

        wasNotAligned = isNotAligned;
    }

    auto perf = instance.getPerf();
    Serial.printf(
        "Perf: preprocess=%dms, inference=%dms, postprocess=%dms\n",
        perf.preprocess,
        perf.inference,
        perf.postprocess
    );
}
