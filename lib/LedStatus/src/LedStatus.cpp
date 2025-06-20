#include <LedStatus.h>
#include <math.h>

char *buildLedData(unsigned int batteryPct, int chargeRate) {
  static char ledStatusString[11];
  const int litCount = static_cast<int>(round(batteryPct / 10));
  const int altChargeCount = litCount + chargeRate;
  for (int i = 0; i < 10; i++) {
    char res;
    if (batteryPct > 95 // show all green if nearly fully charged
        // or Add charging on top of current level
        || (chargeRate > 0 && i >= litCount && i < altChargeCount)) {
      res = 'C';
    } else if (chargeRate < 0 && i < litCount && i >= altChargeCount) {
      // if discharging, show the charge rate in red
      res = 'D';
    } else if (i < litCount) {
      // Otherwise just show the battery level in the default colour
      res = 'B';
    } else {
      res = 'O';
    }
    ledStatusString[i] = res;
  }
  ledStatusString[10] = '\0';
  return ledStatusString;
}
