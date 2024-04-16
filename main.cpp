#include "mbed.h"
#include "Servo.h" // Import for simple R/C Servo
// Standard C++ imports
#include <cstdio>
#include <iostream>

#define Vsupply 5 
#define PIRThreshold 0.5f

// Two inputs, reset and set 
AnalogIn Trigger(A0);
AnalogIn Mic(D1);
InterruptIn ResetSwitch(A5); 
// 5V output
AnalogOut OUTPUT(D13);
// Start with 4 PIR sensors, could add more
AnalogIn PIR1(A1), PIR2(A2),  PIR3(A3),  PIR4(A4);
AnalogIn PIRARRAY[] = {PIR1, PIR2, PIR3, PIR4};
// Servo motor, PWM output
Servo RCServo(D0);
// Digital for servo and solenoid
AnalogOut Solenoid(D12);
DigitalOut ServoControl(D4);

// PHYSICAL RESET SWITCH
void ResetPressed(void)
{
 cout << "Stop!" << endl;
 OUTPUT = 0;
 Solenoid = 0;
 ServoControl = 0;
}
// CONTROLLING MICROPHONE
float getMicVoltage(void) {
    float MicVoltage = Mic.read()*Vsupply;
    return MicVoltage;
}
void CheckMicThreshold(void) {
    if (getMicVoltage() > 2.5) {
        OUTPUT = 0;
        Solenoid = 0;
        ServoControl = 0;
        cout << "\n\rThreshold triggered with microphone: " << getMicVoltage() << endl;
        ThisThread::sleep_for(5s);
    }
}
// CONTROLLING LASER TRIPWIRE, funny function name lol
float getTriggered(void) {
    float TriggerVoltage = Trigger.read()*Vsupply;
    return TriggerVoltage;
}
void CheckIfTriggered(void) {
    if (getTriggered() <= 2) {
        OUTPUT = 5;
        Solenoid = 1;
        ServoControl = 1;
        cout << "Operate OUTPUT!" << endl;
    } else {
        // Do not reset, duh
        cout << "Not Triggered!" << endl;
    }
}
// PIR SENSING
bool RotateServo(void) {
    // Return upon finding the first signal from the PIR
    for (int i = 1; i < 5; i++) {
        if (PIRARRAY[i-1] > PIRThreshold) {
            RCServo.write(i*0.2);
            cout << "PIR" << i << " triggered at level " << PIRARRAY[i-1].read() << endl;
            cout << "Rotate servo to " << 45*i << " degrees!" << endl;
            return true;
        }
    }
    return false;
}

int main(void)
{
    OUTPUT = 0; // Start in an off state
    Solenoid = 0;
    ServoControl = 0;
    // Setup an event queue to handle event requests for the ISR
    // and issue the callback in the event thread.
    EventQueue event_queue;
    // Setup an event thread to update the motor
    Thread event_thread(osPriorityNormal);
    // Start the event on a loop ready to receive event calls from
    // the event queue
    event_thread.start(callback(&event_queue, &EventQueue::dispatch_forever));
    // Attach the functions to the hardware interrupt pins to be inserted into
    // the event queue and executed on button press.
    ResetSwitch.rise(event_queue.event(&ResetPressed));
    
    RCServo.calibrate(0.001, 0.5);
    ThisThread::sleep_for(2s);

    while(true) {
        // Check the analog inputs.
        CheckIfTriggered();
        CheckMicThreshold(); // <= @jacob, uncomment when you hook up
        //Solenoid = 1;
        if (OUTPUT > 0) { // Actually do the things
            bool ANNIHILATE_HUMAN = RotateServo();
            if (ANNIHILATE_HUMAN) {
                // Send a signal to open the solenoid valve through PMOS
                Solenoid = 1;
                cout << "\n\rMovement detected, spray human" << endl;
            }
            // Then close after human found (or continuously keep NCSolenoid closed)
            Solenoid = 0;
            ThisThread::sleep_for(1s);
            
        } 
        // Print Analog Values to screen
        // cout << "\n\rTrigger Voltage: " << getTriggered() << endl;
        // cout << "\n\rMicrophone Voltage: " << getMicVoltage() << endl;
        cout << "\n\rOutput Voltage: " << OUTPUT << endl;

        wait_us(100000); // Wait 0.1 second before repeating the loop.
    }
}
