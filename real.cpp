#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <ESP32Servo.h>
#include <math.h>

Adafruit_MPU6050 mpu;

float roll = 0.0;
float pitch = 0.0;
float yaw = 0.0;

const int GAIT_SPEED = 3, STEP_SWING = 20, STEP_LIFT = 25;

const int numServos = 8;
Servo myServos[numServos];

const int servoPins[numServos] = {13, 14, 26, 33, 12, 27, 25, 32}; // Pins for the servos

unsigned long previousMillis = 0;
unsigned long lasttime = 0; // for mpu6050
const long interval = 20; // 50Hz update loop (every 20ms)

char mode = 's'; // 'w' = walk, 's' = stop, 't' = tall, 'c' = crouch

float hips[4]  = {90, 90, 90, 90};   // CURRENT servo angles
float knees[4] = {45, 45, 45, 45};

float phase = 0;

void setLeg(int idx, float hipTarget, float kneeTarget, float dt, float maxSpeed = 60) {
    float maxStep = maxSpeed * dt;
    // constantly these have to move to the assigned goal, but it cant driceltly moove all the angles at once , so we project them into the smallest maximum posisble step
    hips[idx]  += constrain(hipTarget  - hips[idx],  -maxStep, maxStep);
    knees[idx] += constrain(kneeTarget - knees[idx], -maxStep, maxStep);

    //convert to angles
    hips[idx]  = constrain(hips[idx],  0, 180);
    knees[idx] = constrain(knees[idx], 0, 180);

    myServos[2 * idx].write((int)hips[idx]);
    myServos[2 * idx + 1].write((int)knees[idx]);
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

void getIMU(float dt){
    //initialsie the imu, wake it up, read data from it, calcualte the yaw and pitch from the data dn return it
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    //maths copied from wikipedia
    roll = atan2(a.acceleration.y, a.acceleration.z) * 180 / PI;
    pitch = atan2(-a.acceleration.x, sqrt(a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z)) * 180 / PI;
    
    //yaw cannot be calulated accuratedlyl
    // close methods maybe Yawcurrent=Yawprevious+(wz x dt) or

    float gyroZ_degrees = g.gyro.z * 180.0 / PI;
    yaw += gyroZ_degrees * dt;// yaw will slowly drift even if sensor is perfectly still as it relies on dt  and sesor noise adds up
    // we should use a mpu9250 as it includes  a magnetometer to correct for yaw drift, but we will not do that here as mpu6050 is mentioned in assignment

}

void dogtall(float dt) {
    for (int i = 0; i < 4; i++) {
        setLeg(i, 90, 60, dt);   // hip neutral, knee extended (tall)
    }
}

void dogcrouch(float dt) {
    for (int i = 0; i < 4; i++) {
        setLeg(i, 90, 35, dt);   // hip neutral, knee bent (crouch)
    }
}

void dogwalk(float dt){  
    phase += dt * GAIT_SPEED * 2 * PI; // Update phase based on gait speed
    float legphase[4];
    legphase[0] = phase; // FrontLeft
    legphase[1] = phase + PI/2; // Front Right
    legphase[2] = phase + PI; // Back Left
    legphase[3] = phase + 3*PI/2; // Bck Right

    //we will make it output range (-1 , 0 ,1 )

    for( int i = 0; i < 4; i ++){
        float hipTarget  = 90 + sin(legphase[i]) * STEP_SWING;
        float kneeTarget = 45;
        if (cos(legphase[i]) > 0) {
            kneeTarget = -(cos(legphase[i]) * STEP_LIFT); 
            // if leg in swing phase , we lift the knee so it doenst hit grnd
        }
        setLeg(i, hipTarget, kneeTarget, dt);
    }
}
    //run the gait and balance code, and set the leg positions

void setup(){

    Serial.begin(115200);
    Wire.begin(21,22);// to begin the i2c communcition between on the esp
    mpu.begin();

    //setup the serial, setup imu, setup servos, and set the initial leg positions
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);

    for (int i = 0; i < numServos; i++) {
        myServos[i].setPeriodHertz(50);
        myServos[i].attach(servoPins[i], 500, 2400);
  }
    mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
    mpu.setGyroRange(MPU6050_RANGE_250_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    // this is juust example code copied to initalse and setup the imu, the sesitivity values 
    
    //setting servo to neutral posotion
    for (int i = 0; i < 4; i++){
       myServos[2*i].write(90);
       myServos[2*i+1].write(45);
   }


}
void loop(){
    readSerial();

    unsigned long currentMillis = millis();
    
    


    if (currentMillis - previousMillis >= interval) {
        float dt = (currentMillis - previousMillis) / 1000.0;
        previousMillis = currentMillis;

        getIMU(dt);

        Serial.print("Roll: ");
        Serial.print(roll);
        Serial.print("  Pitch: ");
        Serial.println(pitch);

        if (mode == 'w') {
            dogwalk(dt); 
        } else if (mode == 'c') {
            dogcrouch(dt);
        } else if (mode == 't') {
            dogtall(dt);
        } else {
            for(int i= 0; i<4; i++){
                setLeg(i,90,45,dt);
            }
        }
    }
}