#include <TimeLib.h>
#include "IRIGB.cpp"

IRIGB irig(7);

void onTime() {
  Serial.print(irig.getHours()); Serial.print(":"); Serial.print(irig.getMinutes()); Serial.print(":"); Serial.print(irig.getSeconds());
  Serial.print(" 20"); Serial.print(irig.getYears()); Serial.print("/"); Serial.print(irig.getMonth()); Serial.print("/"); Serial.print(irig.getDayOfMonth()); Serial.println(" UTC");
  
  setTime(irig.getHours(), irig.getMinutes(), irig.getSeconds(), irig.getDayOfMonth(), irig.getMonth(), irig.getYears());
  
  Serial.print("Unix: "); Serial.println(now());
}

void setup() {
  Serial.begin(115200);
  irig.setOnTime(&onTime);
}

void loop() {
  irig.run();
}
