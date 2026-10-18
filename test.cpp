#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <ESP32Servo.h>

Adafruit_MPU6050 mpu;

// 0: Front-Left, 1: Front-Right, 2: Back-Left, 3: Back-Right
Servo hips[4];
Servo knees[4];

const int hipPins[4]  = {13, 14, 26, 33};
const int kneePins[4] = {12, 27, 25, 32};

// Flip to -1 if your right-side servos are mirrored in Wokwi
const int hipDir[4]  = {1, 1, 1, 1};
const int kneeDir[4] = {1, 1, 1, 1};

char mode = 's'; // 'w' = walk, 's' = stop, 't' = tall, 'c' = crouch

// Base angles
const float HIP_HOME    = 90.0;
const float KNEE_STAND  = 90.0;
const float KNEE_TALL   = 60.0;
const float KNEE_CROUCH = 125.0;

// Gait settings
const float STEP_SWING = 22.0; // hip swing (deg)
const float STEP_LIFT  = 25.0; // knee lift (deg)
const float GAIT_SPEED = 1.8;  // trot cycles per second

// Filter & balance variables
float pitch = 0.0;
float roll  = 0.0;
const float alpha = 0.96;

const float kpPitch = 0.85;
const float kpRoll  = 0.85;

unsigned long lastTick = 0;
unsigned long lastPrint = 0;
float phase = 0.0;

void setLeg(int idx, float hipOffset, float kneeOffset) {
  int h = constrain(round(HIP_HOME + hipOffset * hipDir[idx]), 20, 160);
  int k = constrain(round(KNEE_STAND + kneeOffset * kneeDir[idx]), 20, 160);

  hips[idx].write(h);
  knees[idx].write(k);
}

void readSerial() {
  while (Serial.available() > 0) {
    char c = tolower(Serial.read());
    if (c == 'w' || c == 's' || c == 't' || c == 'c') {
      mode = c;
      Serial.print("Switched mode to: ");
      Serial.println(mode);
    }
  }
}

void updateIMU(float dt) {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Accelerometer angles in degrees
  float accRoll  = atan2(a.acceleration.y, a.acceleration.z) * RAD_TO_DEG;
  float accPitch = atan2(-a.acceleration.x, sqrt(a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z)) * RAD_TO_DEG;

  // Gyro rates come in rad/s from Adafruit library, convert to deg/s
  float gyroRollRate  = g.gyro.x * RAD_TO_DEG;
  float gyroPitchRate = g.gyro.y * RAD_TO_DEG;

  // Complementary filter
  roll  = alpha * (roll  + gyroRollRate * dt)  + (1.0 - alpha) * accRoll;
  pitch = alpha * (pitch + gyroPitchRate * dt) + (1.0 - alpha) * accPitch;
}

void runGaitAndBalance(float dt) {
  float baseKnee = 0.0;
  if (mode == 't') baseKnee = KNEE_TALL - KNEE_STAND;
  if (mode == 'c') baseKnee = KNEE_CROUCH - KNEE_STAND;

  float pComp = pitch * kpPitch;
  float rComp = roll  * kpRoll;

  // Tilt adjustments for FL, FR, BL, BR
  float tiltAdj[4] = {
    +pComp - rComp, // FL
    +pComp + rComp, // FR
    -pComp - rComp, // BL
    -pComp + rComp  // BR
  };

  float hOut[4] = {0, 0, 0, 0};
  float kOut[4] = {baseKnee, baseKnee, baseKnee, baseKnee};

  if (mode == 'w') {
    phase += 2.0 * PI * GAIT_SPEED * dt;
    if (phase >= 2.0 * PI) phase -= 2.0 * PI;

    // Trot pairs: (FL + BR) in phase, (FR + BL) shifted by PI
    float legPhase[4] = {phase, phase + PI, phase + PI, phase};

    for (int i = 0; i < 4; i++) {
      hOut[i] = sin(legPhase[i]) * STEP_SWING;

      // Lift knee during forward swing, apply balance while foot is on the ground
      if (cos(legPhase[i]) > 0) {
        kOut[i] -= cos(legPhase[i]) * STEP_LIFT;
        kOut[i] += tiltAdj[i] * 0.3;
      } else {
        kOut[i] += tiltAdj[i];
      }
    }
  } else {
    // Standing / tall / crouch: all 4 legs adjust for tilt
    for (int i = 0; i < 4; i++) {
      kOut[i] += tiltAdj[i];
    }
  }

  for (int i = 0; i < 4; i++) {
    setLeg(i, hOut[i], kOut[i]);
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      yield();
    }
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  for (int i = 0; i < 4; i++) {
    hips[i].setPeriodHertz(50);
    knees[i].setPeriodHertz(50);
    hips[i].attach(hipPins[i], 500, 2400);
    knees[i].attach(kneePins[i], 500, 2400);
    setLeg(i, 0, 0);
  }

  lastTick = millis();
  Serial.println("RoboDog Ready. Send w, s, t, or c.");
}

void loop() {
  readSerial();

  unsigned long now = millis();

  // 50Hz update loop (every 20ms)
  if (now - lastTick >= 20) {
    float dt = (now - lastTick) / 1000.0;
    lastTick = now;

    updateIMU(dt);
    runGaitAndBalance(dt);
  }

  // Print pitch/roll every 300ms so serial monitor doesn't lag Wokwi
  if (now - lastPrint >= 300) {
    lastPrint = now;
    Serial.print("Pitch: ");
    Serial.print(pitch, 1);
    Serial.print(" | Roll: ");
    Serial.println(roll, 1);
  }
}