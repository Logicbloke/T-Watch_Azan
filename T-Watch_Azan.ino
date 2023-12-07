#include "config.h"
#include "PrayerTimes.h"     
#include "DateHelper.h"     

// RGB565 Colors
#define BLACK       0x0000
#define DARK_GREY   0x10A2
#define GREEN       0x07E0
#define ORANGE      0xFDA0
#define RED         0xF800
#define SILVER      0xC618

TTGOClass           *watch  = NULL;
byte                counterToPowOff = 1;
bool                irq = false;
String              currentDateTime, PrayerHour, PrayerMinute;
String              prayerNames[6] = {"Fajr", "Shurooq","Duhr","Asr","Maghrib","Isha"};
String              daysOfWeek[7]  = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
String              year, month, date, minute, hour;       
int16_t             TodaysPrayers[6], dayOfYear, dayOfWeek, currentAbsMinute, elapsedMinutes;
int16_t             xTouch = 0, yTouch = 0, batteryPct;

void setup()
{
     //Serial.begin(115200);
     watch = TTGOClass::getWatch();
     watch->begin();
     watch->openBL();

     watch->power->adc1Enable(AXP202_VBUS_VOL_ADC1 | 
     AXP202_VBUS_CUR_ADC1 | AXP202_BATT_CUR_ADC1 | 
     AXP202_BATT_VOL_ADC1, true);  

     pinMode(AXP202_INT, INPUT_PULLUP);
     attachInterrupt(AXP202_INT, [] {irq = true;}, FALLING);
    
     watch->power->enableIRQ(AXP202_PEK_SHORTPRESS_IRQ, true);
     watch->power->clearIRQ();

     watch->tft->setTextSize(1);    
     watch->tft->setFreeFont(&FreeSans18pt7b);                           
     

     currentDateTime = watch->rtc->formatDateTime(PCF_TIMEFORMAT_YYYY_MM_DD_H_M_S); 
     year      = currentDateTime.substring(0,4);
     month     = currentDateTime.substring(5,7);
     date      = currentDateTime.substring(8,10);
     hour      = currentDateTime.substring(11,13);
     minute    = currentDateTime.substring(14,16);

     month     = month.length()>1?month:"0"+month;
     date      = date.length()>1?date:"0"+date;
     hour      = hour.length()>1?hour:"0"+hour;
     minute    = minute.length()>1?minute:"0"+minute;
        
     currentAbsMinute = (hour).toInt()*60+(minute).toInt();
     dayOfYear = getDayOfYear((date).toInt(), (month).toInt(), (year).toInt());
     dayOfWeek = getWeekDay((date).toInt(), (month).toInt(), (year).toInt());

     for(int i=0; i<6; i++){
          watch->tft->setTextColor(DARK_GREY, BLACK);   // FOREGROUND, BACKGROUND
          TodaysPrayers[i] = PrayerTimes[dayOfYear][i];
          PrayerHour     =    String(TodaysPrayers[i]/60);
          PrayerHour     =    PrayerHour.length() > 1?PrayerHour : "0"+PrayerHour;
          PrayerMinute   =    String(TodaysPrayers[i]%60);
          PrayerMinute   =    PrayerMinute.length() > 1?PrayerMinute : "0"+PrayerMinute;
          elapsedMinutes =    currentAbsMinute-TodaysPrayers[i];

          if(elapsedMinutes >= -15 && elapsedMinutes < 0) {      // Silver 15 min before prayer time
               watch->tft->setTextColor(SILVER, BLACK);
               watch->motor_begin();
               watch->shake();               
          } 
          else if(elapsedMinutes >= 0 && elapsedMinutes <= 60)   // Green on prayer time and up until 60 min after
               watch->tft->setTextColor(GREEN, BLACK);
          else if(elapsedMinutes > 60 &&                                   // Orange after 60 minutes except for Isha
               ((i < 5 && currentAbsMinute < TodaysPrayers[i+1]-15) ||     // Orange as long as before the next prayer up
               (i == 5 && currentAbsMinute > TodaysPrayers[i])))           // Isha is orange until midnight
               watch->tft->setTextColor(ORANGE, BLACK);
          
          watch->tft->drawString(prayerNames[i],0,35*i);
          watch->tft->drawString(PrayerHour + ":" + PrayerMinute,150,35*i);
     }

     // watch->tft->setFreeFont(&FreeSans12pt7b);    
     watch->tft->setTextColor(DARK_GREY, BLACK);                      
     watch->tft->drawString(hour+":"+minute,150,35*6);

     batteryPct = watch->power->getBattPercentage();
     if(watch->power->isVBUSPlug()) {
          if(batteryPct > 99)
               watch->tft->setTextColor(GREEN, BLACK);                      
          else
               watch->tft->setTextColor(ORANGE, BLACK);                      
          watch->tft->drawString(String(batteryPct)+"%",0,35*6);
          delay(2300);
     } else if(batteryPct < 33) {
          watch->tft->setTextColor(RED, BLACK);                      
          watch->tft->drawString(String(batteryPct)+"%",0,35*6);
          delay(1300);
     }   
}

void lightPowerOff()
{
     watch->tft->fillScreen(TFT_BLACK);
     watch->displaySleep();
     watch->closeBL();  
}

// shut off the watch completly
void powerOff()
{
     lightPowerOff();
     watch->powerOff();
     esp_sleep_enable_ext1_wakeup(GPIO_SEL_35, ESP_EXT1_WAKEUP_ALL_LOW);
     esp_deep_sleep_start();  
}

void loop()
{
     watch->tft->setTextColor(DARK_GREY, BLACK);                      
     watch->tft->drawString(hour+(counterToPowOff%2?':':' ')+minute,150,35*6);
     
     if(counterToPowOff < 3)
          watch->tft->drawString(date+"-"+month,0,35*6);
     else if(counterToPowOff < 6)
          watch->tft->drawString(daysOfWeek[dayOfWeek],0,35*6);
     
     delay(1000);

     if(irq) { // Poweroff on button press
          irq = false;
          watch->power->readIRQ();
          if (watch->power->isPEKShortPressIRQ()) {
               watch->power->clearIRQ();
               counterToPowOff = 9;
          }
     watch->power->clearIRQ();
     }
  
     // Reset sleep timer if screen touched
     if(watch->getTouch(xTouch, yTouch)) {
          counterToPowOff = xTouch = yTouch = 0;
     }
          
     // Put to sleep if time elapsed     
     if (++counterToPowOff > 9)                          
               powerOff(); 
}