#include <Arduino.h>

/* Private #defines */
#define IDLE 0
#define MOVING 1

/* Declare Variables */
struct encoderVars
{
    uint8_t pin_A;
    uint8_t pin_B;
    volatile int8_t position;
    volatile uint8_t state;
};

encoderVars encoder = {0, 0, 0, IDLE};

/* Interrupt Service Routine */
void IRAM_ATTR readEncoder()
{
    static uint8_t lastStateA = LOW;
    uint8_t stateA = digitalRead(encoder.pin_A);
    uint8_t stateB = digitalRead(encoder.pin_B);

    if (stateA != lastStateA)
    {
        if (stateA == HIGH)
        {
            if (stateB == LOW)
                encoder.position++;
            else
                encoder.position--;
        }
    }

    lastStateA = stateA;
}

/* Setup encoder */
void encoder_begin(uint8_t pin_A, uint8_t pin_B)
{
    encoder.pin_A = pin_A;
    encoder.pin_B = pin_B;

    pinMode(encoder.pin_A, INPUT_PULLUP);
    pinMode(encoder.pin_B, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(encoder.pin_A), readEncoder, CHANGE);
}

/* Get encoder position */
int8_t encoder_data()
{
    int8_t pos;
    noInterrupts();
    pos = encoder.position;
    encoder.position = 0;
    interrupts();
    return pos;
}
