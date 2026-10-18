#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <ESP32Servo.h>
#include <math.h>

Adafruit_MPU6050 mpu;

float roll = 0.0;
float pitch = 0.0;
float yaw = 0.0;

const int GAIT_SPEED,STEP_SWING,STEP_LIFT;
GAIT_SPEED = 3;



const int numServos = 8;
Servo myServos[numServos];

const int servoPins[numServos] = {13, 14, 26, 33, 12, 27, 25, 32}; // Pins for the servos

unsigned long previousMillis = 0;
unsigned long lasttime = 0; // for mpu6050
const long interval = 20; // 50Hz update loop (every 20ms)

char mode = 's'; // 'w' = walk, 's' = stop, 't' = tall, 'c' = crouch


float hips[4] = {0, 0, 0, 0};
float knees[4] = {0, 0, 0, 0};


void setLeg(int idx, float armOffset, float elbowOffset){
    //set servo to that posootion 


    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    
    myServos[idx].write(pos);
    // Calculate the next position
    pos += step;
    if (pos >= 180 || pos <= 0) {
      step = -step;
    }
  }
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


    roll = atan2(a.acceleration.y, a.acceleration.z) * 180 / PI;
    pitch = atan2(-a.acceleration.x, sqrt(a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z)) * 180 / PI;
    
    //yaw cannot be calulated accuratedlyl
    // close methods maybe Yawcurrent=Yawprevious+(wz x dt) or

    float gyroZ_degrees = g.gyro.z * 180.0 / PI;
    yaw += gyroZ_degrees * dt;// yaw will slowly drift even if sensor is perfectly still as it relies on dt  and sesor noise adds up
    // we should use a mpu9250 as it includes  a magnetometer to correct for yaw drift, but we will not do that here as mpu6050 is mentioned in assignment

}

void dogtall(float dt, float current_pos_hips[4], float current_pos_knee[4]  ){
    //set the legs to a tall position



    for (int i = 0; i < 4; i++) {
        for(int j = current_pos ; j >-30, ){
        setLeg(i, 0, j); 
    }
  }   
}

void dogcrouch(float dt, float current_pos_hips[4], float current_pos_knee[4] ){
    //set the legs to a crouch position
    for (int i = 0; i < 4; i++) {
        for(int j = current_pos ; j >-30, ){
        setLeg(i, 0, j); 
    }
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
        hips[i] = sin(legphase[i]) * STEP_SWING;

        if (cos(legPhase[i]) > 0) {
            knees[i] = -(cos(legPhase[i]) * STEP_LIFT); 
            // if leg in swing phase , we lift the knee so it doenst hit grnd
        } else {
            //gorundtouching phase
            knees[i] = 0.0; 

        }
    }

    for (int i = 0; i < 4; i++) {
        setLeg(i, hOut[i], kOut[i]); 
    }
  }

    //run the gait and balance code, and set the leg positions




void setup(){

    Serial.begin(115200);
    Wire.begin(21,22);// to begin the i2c communcition between on the esp
    mpu.begin()

    
    //setup the serial, setup imu, setup servos, and set the initial leg positions
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);

    // A "for loop" that counts from 0 to 7 to set up all 8 servos instantly
    for (int i = 0; i < numServos; i++) {
        myServos[i].setPeriodHertz(50);
        myServos[i].attach(servoPins[i], 500, 2400);
  }
    mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
    mpu.setGyroRange(MPU6050_RANGE_250_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    // this is juust example code copied to initalse and setup the imu, the sesitivity values 
    
    lastTime = millis();


}


void loop(){
    readSerial();

    unsigned long currentMillis = millis();
    
    if (currentMillis - previousMillis >= interval) {
        float dt = (currentMillis - previousMillis) / 1000.0;
        previousMillis = currentMillis;

        getIMU(dt);

        if (mode == 'w') {
            dogwalk(dt, 2); 
        } else if (mode == 't') {
            dogcrouch(dt,hips,knees);
        } else if (mode == 'c') {
            dogtall(dt,hips,knees);
        } else {
            for(int i; i<4; i++){
                setLeg(i,90,45);
            }
        }
    }
}
