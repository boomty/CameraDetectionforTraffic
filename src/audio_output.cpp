#include "AudioTools.h"
#include "opening.h"

uint8_t channels = 2;
uint16_t sample_rate = 22050;
uint8_t bits_per_sample = 16;

MemoryStream music(StarWars30_raw, StarWars30_raw_len);
I2SStream i2s;  // Output to I2S
StreamCopy copier(i2s, music); // copies sound into i2s

void setup(){
    Serial.begin(115200);
    auto config = i2s.defaultConfig(TX_MODE);
    config.pin_ws = 44;
    config.pin_bck = 7;
    config.pin_data = 43;
    config.sample_rate = sample_rate;
    config.channels = channels;
    config.bits_per_sample = bits_per_sample;
    i2s.begin(config);

    music.begin();
}

void loop(){
    copier.copy();
}
