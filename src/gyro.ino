#include <SPI.h>
#include <Wire.h>
#include <SparkFunLSM9DS1.h>

LSM9DS1 imu;

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  if (imu.begin() != 0) {
    Serial.println("Failed to communicate with LSM9DS1.");
    while (true) {
        exit(0);
    }
  }
}

void loop() {
  imu.readGyro();

  Serial.print("(");
  Serial.print(imu.calcGyro(imu.gx));
  Serial.print(", ");
  Serial.print(imu.calcGyro(imu.gy));
  Serial.print(", ");
  Serial.print(imu.calcGyro(imu.gz));
  Serial.println(")");

  delay(100);
}