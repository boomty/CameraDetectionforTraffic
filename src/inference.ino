#include <SSCMA_Micro_Core.h>
#include <Arduino.h>
#include <esp_camera.h>
#include <queue>

#include "audio_output.ino"

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

    // Decide alignment after processing all classes
    bool isNotAligned = (sum >= 3);

    if (hasClass) {
        if (isNotAligned) {
            Serial.println("Not aligned");

            // Only start sound when state changes from aligned -> not aligned
            if (!wasNotAligned) {
                audioPlay();
            }
        } else {
            Serial.println("Aligned");

            // Optional: stop sound once aligned again
            if (wasNotAligned) {
                audioStop();
            }
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