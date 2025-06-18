#include <math.h>
#include <LedStatus.h>

char* buildLedData(unsigned int batteryPct, int chargeRate){
    static char ledStatusString[11];
    int litCount =(int)round(batteryPct/10);
    int altChargeCount = litCount + chargeRate;
    for(int i=0;i<10;i++) {
        if( (batteryPct>95)){ // (batteryPct >=90 && chargeRate > 0) ||
            // show all green if nearly fully charged
            ledStatusString[i] = 'C';
        }else if(chargeRate > 0 && i >= litCount && i < altChargeCount){
            // Add charging on top of current level
            ledStatusString[i] = 'C';
        }else if(chargeRate < 0 && i < litCount && i >= altChargeCount ){
            // if discharging, show the charge rate in red
            ledStatusString[i] = 'D';
        }else if(i < litCount){
            // Otherwise just show the batery level in the default colour
            ledStatusString[i] = 'B';
        }else{
            ledStatusString[i] = 'O';
        }
    }
    ledStatusString[10] = '\0';
    return ledStatusString;
}

