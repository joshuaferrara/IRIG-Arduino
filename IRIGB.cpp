/**
* IRIG-B Demodulator for Arduino
* Author: Joshua Ferrara <josh@ferrara.space>
* Copyright: MIT
*/ 

#include <Arduino.h>
#include <QueueList.h>

class IRIGB {
public:
    IRIGB(int _inputPin) : inputPin(_inputPin) {
        pinMode(inputPin, INPUT);
    }

    void setOnTime(void (*_onTimeFunc)()) {
        onTimeFunc = _onTimeFunc;
    }

    void run() {
        if (digitalRead(inputPin) == HIGH && !pulse) {          // Rising edge
            pStart = millis();                                  // Set start time of current pulse
            pulse = true;                                       // Let everyone know we're recording a pulse
        } else if (digitalRead(inputPin) == LOW && pulse) {     // Falling edge
            int cPType = pType(millis() - pStart);              // Type of current pulse

            if (lPType == 'R' && cPType == 'R') {               // Start of frame
                if (onTimeFunc != nullptr) onTimeFunc();
                clearQueue(bits);                               // Empty buffer
                pos = 0;                                        // Set current position to start of frame
            } else if (pos >= 0) {                              // Add bit to queue if in a frame
                if (cPType == 'R') {                            // Position marker
                    if (pos == 4) bits.push(cPType);
                    if (++pos == 9) pos = 0;                    // Increment/reset pos
                    parseBits();                                // Parse frame
                } else {
                    bits.push(cPType);
                }
            }

            lPType = cPType;                                    // Set last pulse type equal to current pulse type
            pulse = false;                                      // Let everyone know we're done recording a pulse
        }
    }

    int getSeconds() {
      return seconds;
    }

    int getMinutes() {
      return minutes;
    }

    int getHours() {
      return hours;
    }

    int getDays() {
      return days;
    }

    int getYears() {
      return years;
    }

    int sumOfDays(int month) {
      int i = 0, cSum = 0;
      for (; i < month - 1; i++, cSum += daysOfMonth[i]);
      return cSum;
    }

    const int daysOfMonth[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int getMonth() {
      int i = 0, cSum = 0;
      for (; i < 12; i++, cSum += daysOfMonth[i]) {
        if (getDays() < cSum) break;
      }
      return i;
    }

    int getDayOfMonth() {
      int month = getMonth();
      return getDays() - sumOfDays(getMonth());
    }
private:
    QueueList<int> bits;                // Stores bits for current bit group, identified by position marker

    void (*onTimeFunc)();               // Executed when PPS received

    int pos = -1;                       // Stores index of last position marker within frame
    int lPType = -1;                    // Last pulse type
    unsigned long pStart;               // Start time of pulse in ms
    bool pulse = false;                 // Is pulse being recorded?

    unsigned int inputPin;              // Digital input pin

    unsigned long lastUpdatedAt = 0;    // Last time in millis the time variables were updated at. Used to calculate offset.

    // Last known time - sorta. Still needs some work to be useful...
    unsigned int seconds = 0;
    unsigned int secondM = 0;
    unsigned int minutes = 0;
    unsigned int hours   = 0;
    unsigned int days    = 0;
    unsigned int years   = 0;

    // Temporary time variables
    unsigned int tSeconds = 0;
    unsigned int tSecondM = 0;
    unsigned int tMinutes = 0;
    unsigned int tHours   = 0;
    unsigned int tDays    = 0;
    unsigned int tYears   = 0;

    bool isLeapYear(int y) {
      return (((1970 + y) > 0) && !((1970 + y) % 4) && (((1970 + y) % 100) || !((1970 + y) % 400)));
    }

    // Sums up bits stored within the bits queue
    const int pVal[12] = {1, 2, 4, 8, 0, 10, 20, 40, 80, 0, 100, 200};
    int queueSum(int len = 12) {
        int cSum = 0, n = 0;
        while (!bits.isEmpty()) {
            int cBit = bits.pop();
            cSum += (cBit * (n < len ? pVal[n++] : 0));
        }
        return cSum;
    }

    // Converts length of a pulse to binary 0/1 or a 'R' for a reference/position frame
    int pType(unsigned long len) {
        if (len <= 9 && len >= 7) return 'R';
        return (len <= 3 && len >= 1) ? 0 : 1;
    }

    // Clears bits queue
    void clearQueue(QueueList<int> &q) {
        while (!q.isEmpty()) q.pop();
    }

    void parseBits() {
        switch (pos) {                  // Parse data received until this position, at this point we should parse...
            case 0:                     // ...nothing, or binary seconds, or print to console                
                queueSum();
                break;
            case 1:                     // ...seconds
                tSeconds = queueSum(9);
                break;
            case 2:                     // ...minutes
                tMinutes = queueSum(9);
                break;
            case 3:                     // ...hours
                tHours   = queueSum(9);
                break;
            case 4:                     // ...nothing - still decoding days
                break;
            case 5:                     // ...days
                // Summing stuff here is a bit special...
                // The first 11 (sorta, we're missing a reference bit. This is handled by separate for loops below) bits have to do with days of year,
                // while the next 6 have to do with a seconds multiplier
                // 11100001000 0000000 - Example bits between positions 3 and 4 for Arbiter 1093B. Here, days = 47 and seconds*0.1 = 0

                tDays = 0;
                for (int i = 0; i < 9; i++) {
                  tDays += bits.pop() * pVal[i];
                }
                for (int i = 0; i < 2; i++) {
                  tDays += bits.pop() * pVal[9 + i];
                }

                tSecondM = queueSum(4);
                break;
            case 6:                     // ...years
                tYears   = queueSum(8);
                break;
            case 7:                     // At this point, we have all time data recorded. Set variables based on anticipated time.
                if ((tSeconds += 1) == 60) {
                  tSeconds = 0;
                  tMinutes++;
                }
                seconds = tSeconds;
                secondM = tSecondM;

                if (tMinutes == 60) {
                  tMinutes = 0;
                  tHours++;
                }
                minutes = tMinutes;

                if (tHours == 24) {
                  tHours = 0;
                  tDays++;
                }
                hours = tHours;
                
                if (tDays == 367) { // This does not take into account leap years, and has yet to be tested.
                  tDays = 0;
                  tYears++;
                }
                days = tDays;
                
                years = tYears;

                // lastUpdatedAt = millis() - 700; // Time last received at current time, -700 ms for position marker 7
                break;
        }
    }
};
