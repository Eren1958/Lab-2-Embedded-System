#include "mbed.h"

// ---------------- INPUT BUTTONS (0-5) ----------------
DigitalIn btn0(D2, PullDown);
DigitalIn btn1(D3, PullDown);
DigitalIn btn2(D4, PullDown);
DigitalIn btn3(D5, PullDown);
DigitalIn btn4(D6, PullDown);
DigitalIn btn5(D7, PullDown);

// ---------------- LED OUTPUTS ----------------
// Normal meanings:
// LED1 -> warning blink / counter bit 0
// LED2 -> lockdown steady / counter bit 1
// LED3 -> lockdown blink / counter bit 2
DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);

#define CODE_LENGTH 4

// ---------------- PASSCODES ----------------
int userCode[CODE_LENGTH]  = {1, 1, 1, 1};
int adminCode[CODE_LENGTH] = {4, 4, 4, 4};

int enteredCode[CODE_LENGTH];
int indexCode = 0;

int wrongAttempts = 0;
int eventCounter = 0;

bool warningMode = false;
bool lockdownMode = false;

// ---------------- READ BUTTON ----------------
int readButton() {
    if (btn0 == 1) return 0;
    if (btn1 == 1) return 1;
    if (btn2 == 1) return 2;
    if (btn3 == 1) return 3;
    if (btn4 == 1) return 4;
    if (btn5 == 1) return 5;
    return -1;
}

// ---------------- WAIT FOR BUTTON RELEASE ----------------
void waitRelease() {
    while (btn0 == 1 || btn1 == 1 || btn2 == 1 ||
           btn3 == 1 || btn4 == 1 || btn5 == 1) {
        thread_sleep_for(10);
    }
}

// ---------------- CHECK CODE ----------------
bool checkCode(int correctCode[]) {
    for (int i = 0; i < CODE_LENGTH; i++) {
        if (enteredCode[i] != correctCode[i]) {
            return false;
        }
    }
    return true;
}

// ---------------- CLEAR ENTERED CODE ----------------
void clearEnteredCode() {
    indexCode = 0;
    for (int i = 0; i < CODE_LENGTH; i++) {
        enteredCode[i] = -1;
    }
}

// ---------------- SHOW EVENT COUNTER USING LEDs ----------------
// Uses 3-bit binary display:
// LED1 = 1, LED2 = 2, LED3 = 4
void showEventCounter() {
    led1 = (eventCounter & 0x01) ? 1 : 0;
    led2 = (eventCounter & 0x02) ? 1 : 0;
    led3 = (eventCounter & 0x04) ? 1 : 0;
}

// ---------------- WARNING MODE ----------------
// 30 seconds, slow blink, no inputs accepted
void warningPhase() {
    warningMode = true;
    clearEnteredCode();

    for (int i = 0; i < 30; i++) {
        led1 = !led1;
        thread_sleep_for(1000);
    }

    led1 = 0;
    led2 = 0;
    led3 = 0;
    warningMode = false;
}

// ---------------- MAIN ----------------
int main() {
    Timer blinkTimer;
Timer lockdownTimer;

bool blinkTimerStarted = false;
bool lockdownTimerStarted = false;
    led1 = 0;
    led2 = 0;
    led3 = 0;

    while (true) {

        // ---------------- LOCKDOWN MODE ----------------
       if (lockdownMode) {

    if (!lockdownTimerStarted) {
        lockdownTimer.start();
        lockdownTimerStarted = true;
    }

    if (!blinkTimerStarted) {
        blinkTimer.start();
        blinkTimerStarted = true;
    }

    if (lockdownTimer.elapsed_time() < 60s) {

        led2 = 1;

        if (blinkTimer.elapsed_time() >= 500ms) {
            led3 = !led3;
            blinkTimer.reset();
        }

    } else {
        showEventCounter();
    }

    int button = readButton();

    if (button != -1) {
        enteredCode[indexCode++] = button;
        waitRelease();
        thread_sleep_for(150);
    }

    if (indexCode == CODE_LENGTH) {
        if (checkCode(adminCode)) {

            lockdownMode = false;
            wrongAttempts = 0;

            blinkTimer.stop();
            blinkTimer.reset();
            blinkTimerStarted = false;

            lockdownTimer.stop();
            lockdownTimer.reset();
            lockdownTimerStarted = false;

            led1 = 0;
            led2 = 0;
            led3 = 0;
        }

        clearEnteredCode();
    }

    thread_sleep_for(10);
    continue;
}
        // ---------------- WARNING MODE ----------------
        // Inputs blocked during warning
        if (warningMode) {
            thread_sleep_for(10);
            continue;
        }

        // ---------------- NORMAL MODE ----------------
        int button = readButton();

        if (button != -1) {
            enteredCode[indexCode] = button;
            indexCode++;
            waitRelease();
            thread_sleep_for(150);
        }

        if (indexCode == CODE_LENGTH) {

            if (checkCode(userCode)) {
                // Correct user code: reset system and counter
                wrongAttempts = 0;
                eventCounter = 0;
                led1 = 0;
                led2 = 0;
                led3 = 0;
            } else {
                wrongAttempts++;

                if (wrongAttempts == 3) {
                    warningPhase();
                } else if (wrongAttempts == 4) {
                    lockdownMode = true;
                    eventCounter++;

                    blinkTimer.stop();
                    blinkTimer.reset();

                    lockdownTimer.stop();
                    lockdownTimer.reset();

                    led1 = 0;
                    led2 = 0;
                    led3 = 0;
                }
            }

            clearEnteredCode();
        }

        thread_sleep_for(10);
    }
}
