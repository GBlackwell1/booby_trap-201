#include "mbed.h"
#include "Servo.h" // Import for simple R/C Servo
// Standard C++ imports
#include <cstdio>
#include <iostream>

#define Vsupply 5 
#define Threshold 1  

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

/* TODO: If we're using a 12V solenoid valve, we need to control a PMOS transistor that allows
*  the voltage to flow through whenever receiving an output signal, but I'll write a little somethign here
*/
AnalogOut Solenoid(D12);

// PHYSICAL RESET SWITCH
void ResetPressed(void)
{
 cout << "Stop!" << endl;
 OUTPUT = 0;
}
// CONTROLLING MICROPHONE
float getMicVoltage(void) {
    float MicVoltage = Mic.read()*Vsupply;
    return MicVoltage;
}
void CheckMicThreshold(void) {
    // TODO: @jacob, don't know the threshold but finagle it how you want
    // active high should be if less than right?
    if (getMicVoltage() <= Threshold) {
        OUTPUT = 0;
    }
}
// CONTROLLING LASER TRIPWIRE, funny function name lol
float getTriggered(void) {
    float TriggerVoltage = Trigger.read()*Vsupply;
    return TriggerVoltage;
}
void CheckIfTriggered(void) {
    if (getTriggered() <= Threshold) {
        OUTPUT = 5;
        cout << "Operate OUTPUT!" << endl;
    } else {
        // Do not reset, duh
        cout << "Not Triggered!" << endl;
    }
}
// PIR SENSING
void RotateServo(void) {
    // Return upon finding the first signal from the PIR
    for (int i = 0; i < 4; i++) {
        // Delay time for PIRs is 2 seconds so wait maybe? 
        // TODO: could be bad in practice
        ThisThread::sleep_for(1s);
        if (PIRARRAY[i] > 0) {
            RCServo.position(45*i);
            cout << "PIR " << i << " triggered!" << endl;
            return;
        }
    }
    return;
}

int main(void)
{
    OUTPUT = 0; // Start in an off state
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
    while(true) {
        // Check the analog inputs.
        CheckIfTriggered();
        // CheckMicThreshold(); <= @jacob, uncomment when you hook up

        if (OUTPUT > 0) { // Actually do the things
            RotateServo();
            Solenoid = 5; // <= @ryan, this is where you'll have to experiment
        } 
        Solenoid = 0;  // <= @gabe, closing solenoid valve so it doesn't constantly spray, 
                       //    will reopen whenever it detects next move (and it's triggered)
        
        // Print Analog Values to screen
        cout << "\n\rTrigger Voltage: " << getTriggered() << endl;
        cout << "\n\rMicrophone Voltage: " << getMicVoltage() << endl;
        cout << "\n\rOutput Voltage: " << OUTPUT << endl;

        wait_us(100000); // Wait 0.1 second before repeating the loop.
    }
}
