#define TIME_OUT 5000

// Motor Pins
#define MTL_1 5   // Left Motor Direction 1
#define MTL_2 6   // Left Motor Direction 2
#define MTR_1 9   // Right Motor Direction 1
#define MTR_2 10  // Right Motor Direction 2
#define ENA 11    // Left Motor Enable (PWM)
#define ENB 3     // Right Motor Enable (PWM)

// Encoder Pins
#define ENC_LEFT 1  // Left Encoder
#define ENC_RIGHT 2 // Right Encoder

// IR Sensor Pins
#define IR_FRONT 8  // Middle IR Sensor
#define IR_RIGHT 12 // Right IR Sensor
#define IR_LEFT 7   // Left IR Sensor

// Global Variables for Encoders
volatile unsigned long leftEncoderTicks = 0;
volatile unsigned long rightEncoderTicks = 0;

// Interrupt Service Routines for Encoders
void leftEncoderISR() {
    leftEncoderTicks++;
}

void rightEncoderISR() {
    rightEncoderTicks++;
}

// IR Sensor Setup
struct IR_Sensor {
    uint8_t pin;
};

struct IR_Sensor irFront, irRight, irLeft;

void ir_init() {
    irFront.pin = IR_FRONT;
    irRight.pin = IR_RIGHT;
    irLeft.pin = IR_LEFT;

    pinMode(irFront.pin, INPUT);
    pinMode(irRight.pin, INPUT);
    pinMode(irLeft.pin, INPUT);
}

int readIRSensor(struct IR_Sensor sensor) {
    return digitalRead(sensor.pin); // Return 1 for no obstacle, 0 for obstacle
}

// Motor Control Functions
void motor_init() {
    pinMode(MTL_1, OUTPUT);
    pinMode(MTL_2, OUTPUT);
    pinMode(MTR_1, OUTPUT);
    pinMode(MTR_2, OUTPUT);
    pinMode(ENA, OUTPUT);
    pinMode(ENB, OUTPUT);

    pinMode(ENC_LEFT, INPUT);
    pinMode(ENC_RIGHT, INPUT);

    attachInterrupt(digitalPinToInterrupt(ENC_LEFT), leftEncoderISR, RISING);
    attachInterrupt(digitalPinToInterrupt(ENC_RIGHT), rightEncoderISR, RISING);
}

void setMotorSpeed(int leftSpeed, int rightSpeed) {
    analogWrite(ENA, abs(leftSpeed)); // Set left motor speed
    analogWrite(ENB, abs(rightSpeed)); // Set right motor speed

    // Left motor direction
    digitalWrite(MTL_1, leftSpeed > 0 ? HIGH : LOW);
    digitalWrite(MTL_2, leftSpeed > 0 ? LOW : HIGH);

    // Right motor direction
    digitalWrite(MTR_1, rightSpeed > 0 ? HIGH : LOW);
    digitalWrite(MTR_2, rightSpeed > 0 ? LOW : HIGH);
}

void stop() {
    analogWrite(ENA, 0); // Stop left motor
    analogWrite(ENB, 0); // Stop right motor

    digitalWrite(MTL_1, LOW);
    digitalWrite(MTL_2, LOW);
    digitalWrite(MTR_1, LOW);
    digitalWrite(MTR_2, LOW);
    delay(200); // Small pause for stabilization
}

void goStraightWithEncoders(int baseSpeed, int duration) {
    leftEncoderTicks = 0;
    rightEncoderTicks = 0;

    unsigned long startTime = millis();

    while (millis() - startTime < duration) {
        unsigned long leftTicks = leftEncoderTicks;
        unsigned long rightTicks = rightEncoderTicks;

        int error = leftTicks - rightTicks;

        // Adjust speeds to minimize the error
        int leftSpeed = baseSpeed - error * 2; // Proportional control
        int rightSpeed = baseSpeed + error * 2;

        setMotorSpeed(leftSpeed, rightSpeed);

        delay(50); // Small delay to allow stable control
    }

    stop(); // Stop after the duration
}

void turnRight() {
    setMotorSpeed(-100, 100); // Reverse left motor and forward right motor
    delay(150);               // Adjust delay for 90-degree turn
    stop();
}

void turnLeft() {
    setMotorSpeed(100, -100); // Forward left motor and reverse right motor
    delay(150);               // Adjust delay for 90-degree turn
    stop();
}

// Wall-Following Logic
void rightWallFollower(int baseSpeed) {
    int frontObstacle = readIRSensor(irFront); // Front sensor
    int rightObstacle = readIRSensor(irRight); // Right sensor
    int leftObstacle = readIRSensor(irLeft);   // Left sensor

    if (frontObstacle == 0) { 
        // Obstacle in front
        stop();
        if (rightObstacle == 1) {
            // Turn right if there's space
            turnRight();
        } else if (leftObstacle == 1) {
            // Turn left if no space on the right
            turnLeft();
        }
    } else if (rightObstacle == 0) {
        // Maintain wall-following on the right
        setMotorSpeed(baseSpeed, baseSpeed - 30); // Slight left adjustment
    } else if (rightObstacle == 1) {
        // No wall on the right
        setMotorSpeed(baseSpeed - 30, baseSpeed); // Slight right adjustment
    } else {
        // Default: Go straight
        setMotorSpeed(baseSpeed, baseSpeed);
    }
    delay(50); // Small delay to stabilize control
}

void setup() {
    Serial.begin(9600);
    motor_init();
    ir_init();
    delay(5000); // Initial delay before starting
}

void loop() {
    rightWallFollower(64); // Follow the right wall at a slow speed
}
