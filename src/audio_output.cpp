/**
 * @file streams-memory_raw-i2s.ino
 * @author Phil Schatzmann
 * @brief Compile with Partition Scheme Hughe APP!
 * @version 0.1
 * @date 2021-01-24
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "AudioTools.h"
#include "StarWars30.h"

AudioInfo info(22050, 1, 16);
I2SStream i2s;  // Output to I2S
MemoryStream music(StarWars30_raw, StarWars30_raw_len);
StreamCopy copier(i2s, music); // copies sound into i2s

void setup(){
    Serial.begin(115200);
    AudioToolsLogger.begin(Serial, AudioToolsLogLevel::Info);

    auto config = i2s.defaultConfig(TX_MODE);
    config.pin_bck = 3;
    config.pin_ws = 4;
    config.pin_data = 5;
    // config.pin_mck = 0; // optional master clock pin
    // config.i2s_format = I2S_STD_FORMAT; // default format
    // config.is_master = true; // default esp32 is master
    i2s.begin(config);
}

void loop(){
    if (!copier.copy()){
      i2s.end();
      stop();
    }
}