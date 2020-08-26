// ****************************************************************************
//       Sketch: Sprinter Radio CANBus Decoder
// Date Created: 11/22/2016
//
//     Comments: Generates 12V Ignition and Reverse Signals for after-market
//               radio integration.
//
//  Programmers: Jamie Robertson, info@robertsonics.com
//
// ****************************************************************************

#include <IFCT.h>
#include <Metro.h>
#include <Snooze.h>

#define IGNITION    7       // Ignition relay output
#define REVERSE     6       // Reverse relay output
#define CAN_SLEEP   5       // CAN low power enable
#define LED         13      // LED

Metro gPulseMetro(10);      // Set up a 10ms system tick

SnoozeDigital digital;
SnoozeBlock config_teensy32(digital);

uint32_t gSysTick = 0;
uint32_t gCanTimeout = 0;

uint32_t gLastLedFlashTicks = 0;

uint8_t gIgnitionState = 0;
uint8_t gReverseState = 0;
uint8_t gLedState = 0;
uint8_t gLedCounter = 0;


// ****************************************************************************
void setup() {

    // Initialise the LED output
    pinMode(LED, OUTPUT);
    digitalWrite(LED, gLedState);
  
    // Initialize the relay outputs
    pinMode(IGNITION, OUTPUT);
    digitalWrite(IGNITION, gIgnitionState);
    pinMode(REVERSE, OUTPUT);
    digitalWrite(REVERSE, gReverseState);

    // Initialise the CAN sleep output to full speed
    pinMode(CAN_SLEEP, OUTPUT);
    digitalWrite(CAN_SLEEP, 0);

    // Initialize the sleep mechanism
    digital.pinMode(21, INPUT_PULLUP, RISING);  //pin, mode, type

    // Initialize CAN
    Can0.setBaudRate(500000);
    Can0.enableFIFO(1);
    Can0.enableFIFOInterrupt(1);
    Can0.onReceive(canSniff);
    Can0.intervalTimer();

    // Set up filters for specific CAN IDs
    Can0.setFIFOFilter( REJECT_ALL );
    Can0.setFIFOFilter( MB0, 853, STD );//
    Can0.setFIFOFilter( MB1, 1049, STD );//
    //Can0.setFIFOFilter( MB2, 0x333, STD );//
    //Can0.setFIFOFilter( MB3, 0x444, STD );//
    //Can0.setFIFOFilter( MB4, 0x555, STD );//
    //Can0.setFIFOFilter( MB5, 0x666, STD );//
    Can0.enhanceFilter( FIFO );
}

// ****************************************************************************
void loop() {

    // On each system tick, bump the tick count, the CAN timeout count and
    //  check the LED on counter.
    if (gPulseMetro.check() == 1) {
        gSysTick++;
        gCanTimeout++;
        if (gLedCounter > 0) {
            if (--gLedCounter == 0) {
                digitalWrite(LED, 0);
            }
        }
    }

    // When time to do so, flash the heartbeat LED
    if ((gSysTick - gLastLedFlashTicks) > 100) {
        gLastLedFlashTicks = gSysTick;
        digitalWrite(LED, 1);
        gLedCounter = 3;
    }

    // Check for a CAN timeout of more than 1 min and go to sleep if so
    if (gCanTimeout > 6000) {
        
        Serial.print("Sleeping now");
        Serial.println();

        // Put the CAN transceiver into low power mode and make sure the
        //  LED is off
        digitalWrite(CAN_SLEEP, 1);
        digitalWrite(LED, 0);
        delay(100);

        // Go to sleep until CAN activity wakes us up
        Snooze.deepSleep( config_teensy32 );
        
        delay(100);
        Serial.print("Waking up");
        Serial.println();

        // Enable the CAN transciever full power mode
        digitalWrite(CAN_SLEEP, 0);

        // And reset our timeout count
        gCanTimeout = 0;
    }
}

// ****************************************************************************
void canSniff(const CAN_message_t &msg) {

    if (msg.id == 853) {

        //Serial.print("853: ");
        //for (int i = 0; i < 8; i++) {
        //    Serial.print(msg.buf[i], HEX);
        //    Serial.print(" ");
        //}
        //Serial.println();
        
        if (((msg.buf[1] > 0) || (msg.buf[2] > 0)) && (gIgnitionState == 0)) {
            Serial.print("Ignition ON");
            Serial.println();
            gIgnitionState = 1;
            digitalWrite(IGNITION, 1);  
        }
        else if ((msg.buf[1] == 0) && (msg.buf[2] == 0) && (gIgnitionState == 1)) {
            Serial.print("Ignition OFF");
            Serial.println();
            gIgnitionState = 0;
            digitalWrite(IGNITION, 0);
        }
        gCanTimeout = 0;
    }

    else if (msg.id == 1049) {
        if ((msg.buf[3] == 4) && (gReverseState == 0)) {
            Serial.print("Reverse");
            Serial.println();
            gReverseState = 1;
            digitalWrite(REVERSE, 1);  
        }
        else if ((msg.buf[3] == 0) && (gReverseState == 1)) {
            Serial.print("Not Reverse");
            Serial.println();
            gReverseState = 0;
            digitalWrite(REVERSE, 0);
        }
        gCanTimeout = 0;
    }
}
