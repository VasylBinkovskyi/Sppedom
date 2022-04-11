#include "GyverEncoder.h"
#define CLK 6
#define DT 5
#define SW 4
#define PRECISE_ALGORITHM
#include <EEPROM.h>
#include <iarduino_OLED_txt.h>
iarduino_OLED_txt myOLED(0x3C);
extern uint8_t MediumFont[];
extern uint8_t SmallFont[];
extern uint8_t MegaNumbers[];
#include <Servo.h>
Servo Servostrelka;
Encoder enc(CLK, DT, SW, TYPE1);

//........................................Variable......................................................................................
volatile const float stala = 0.4530; //передаточне відношення редуктора
volatile float timer_SPD, timer1;
volatile int SPD;
volatile long odo_counter = 1, odo1_counter = 1, serv_counter = 1;
unsigned long timer_strelka;
float ServEE, OdoEE;
float odometer, odometer1 = 25.0, servis, odoTemp;
float svitlo_Timer = 0;
float TimerFiltrSpd = 0;
float timer_Rozg = 0, rozg_60 = 100, rozg_100 = 100, max_Rozg60 = 100, max_Rozg100 = 100;
float oil_Temp = 0, oil_Servis = 0;
float timerVccOLED = 0;
int svitlo_flag, fara_State = 0;
int servo_SPD = 0;
int FiltrSpeed = 0;
int rozg_Flag = 0;
int timer_save = 2000;
int in_SPD = 3;
int ign_pin = 9, ign_flag;
int save_pin = 8;
int saveFlag = 1;
bool ign, save;
int SPD_temp = 0, max_SPD;
int cont_meny = 1, clcMeny = 0, holMeny = 0, presMeny = 0, douMeny = 0;
int segment_0 = 0, segment_1 = 0, segment_2 = 0, SPD_in = 0;
int fara_out = 11;
int dispVcc = 10, flagOLEDVcc = 3;

//_______________________________________________________________________________________________________________________________________
///////////////////////////<FUNCTIONS>/////////////////////////////////////////////////////////////////////////////////////
int GetFiltrSpeed(int spd, int filtSpeed)
{

  if (spd > 0 && spd < 160 && spd > filtSpeed)
  {
    filtSpeed++;
  }
  else if (spd >= 0 && spd < 160 && spd < filtSpeed)
  {
    filtSpeed--;
  }

  return filtSpeed;
}

//-----------------------------------------pririvaniya-----------------------------------------------------------------------------------
void RAPID()
{
  timer_SPD = micros() - timer1;
  timer1 = micros();
  if (ign_flag == 2 && timer_SPD > 10000)
  {
    odo_counter++;
    odo1_counter++;
    serv_counter++;
  }
}

///#####################################################SETUP############################################################################
void setup()
{

  // Serial.begin(9600);
  pinMode(save_pin, INPUT_PULLUP); // 8-й вхід збереженя пробіга при вимкнені живлення коли HIGH записуєм данні в еепром
  pinMode(ign_pin, INPUT_PULLUP);  // pin9 вход включення запаленя
  pinMode(in_SPD, INPUT_PULLUP);   // pin3 вхід датчика Холла
  pinMode(fara_out, OUTPUT);       // pin 11 вихід включення денних ходових вогнів
  pinMode(dispVcc, OUTPUT);        // pin 10 виход живлення OLED дисплея
  attachInterrupt(1, RAPID, RISING);
  Servostrelka.attach(7);
  enc.setType(TYPE1);
  enc.setPinMode(HIGH_PULL);
  //////////////////////////////////////////////////
  // EEPROM.put(0,25000.125);//Загальний одометер   //
  // EEPROM.put(5,50);//максимальна швидкість       //
  // EEPROM.put(40,25000.00);//загальний одометер   //Відладка программи запис стартових значень в eeprom
  // EEPROM.put(10,100.0);//мінімальний розгін 1   //
  // EEPROM.put(15,100.0);//мінімальний розгін 2   //
  // EEPROM.put(20,9700.0);//сервіс масла          //
  // EEPROM.put(25,1);//Флаг світла                //
  //////////////////////////////////////////////////
  EEPROM.get(0, odoTemp);
  EEPROM.get(0, OdoEE);
  EEPROM.get(5, max_SPD);
  EEPROM.get(10, max_Rozg60);
  EEPROM.get(15, max_Rozg100);
  EEPROM.get(20, oil_Temp);
  EEPROM.get(20, ServEE);
  EEPROM.get(25, svitlo_flag);
}
//###################################################LOOP#################################################################################
void loop()
{
  enc.tick();
  //.....................................<DIAGNOSTICS BLOCK>................................................................
  // tone(13, 60);///92 km/h
  // Serial.print("SPDe=   ");
  // Serial.println(FiltrSpeed);
  // Serial.print("micros()-timer1=   ");
  // Serial.println(micros()-timer1);
  // Serial.print("timer1=   ");
  // Serial.println(timer1);
  // Serial.print("SPD_in= ");
  // Serial.println(SPD_in);
  // Serial.print("millis=          ");
  // Serial.println(millis());
  //___________________________________________________________________________________________________________________
  //.....................................Strelka_Test...................................................................
  ign = digitalRead(ign_pin); //обробка включення запалення
  if (ign == 0 && ign_flag == 0)
  {
    timer_strelka = millis();
    Servostrelka.write(0);

    ign_flag = 1;
  }
  if (ign == 0 && ign_flag == 1 && millis() - timer_strelka > 1000)
  {
    Servostrelka.write(180);
    ign_flag = 2;
  }
  if (ign == 1)
  {
    ign_flag = 0;
    timer_SPD = 0;
    Servostrelka.write(180);
  }
  //_______________________________________________________________________________________________________________________
  //.....................................виключення і виключення OLED дисплея...................................................................

  if (ign_flag == 2)
  {
    timerVccOLED = millis();
  }

  if (flagOLEDVcc == 3)
  {
    digitalWrite(dispVcc, HIGH);
    if (millis() > 1000 && flagOLEDVcc == 3) // перше включення після подачі напруги
    {
      myOLED.begin();
      flagOLEDVcc = 1;
    }
  }

  if (ign_flag == 2 && flagOLEDVcc == 0)
  {
    myOLED.wakeUp(); // просинаємся
    flagOLEDVcc = 1;
  }

  if (ign == 1 && flagOLEDVcc == 1 && (millis() - timerVccOLED > 300000))
  {
    myOLED.sleap(); // засинаєм
    flagOLEDVcc = 0;
  }

  //_______________________________________________________________________________________________________________________
  //............................................вимір швидкості і одометер............................................................

  if (micros() - timer1 > 1000000.0)
  {
    timer_SPD = 0;
    SPD = 0;
  }
  SPD = ((stala / (timer_SPD / 1000000)) * 3.60);

  if (millis() - TimerFiltrSpd > 50.0)
  {
    FiltrSpeed = GetFiltrSpeed(SPD, FiltrSpeed);
    TimerFiltrSpd = millis();
  }

  servo_SPD = map(FiltrSpeed, 10, 120, 180, 0);
  if (ign_flag == 2 && FiltrSpeed > 0 && FiltrSpeed < 120)
  {
    Servostrelka.write(servo_SPD);
  }
  if (ign_flag == 2 && FiltrSpeed > max_SPD)
  {
    max_SPD = FiltrSpeed;
  }

  ///розбиваємо швидкість на сегменти
  segment_0 = FiltrSpeed / 100;
  segment_1 = (FiltrSpeed % 100) / 10;
  segment_2 = FiltrSpeed % 10;

  odometer = odoTemp + (stala * odo_counter) / 1000; //вираховуєм загальний пробіг
  odometer1 = (stala * odo1_counter) / 1000;         // вираховуєм суточний пробіг
  save = digitalRead(save_pin);                      // зберігаєм загальний пробіг при вимкненні живлення
  if (save == 0 && millis() > 10000 && saveFlag == 1)
  {
    EEPROM.put(5, max_SPD);
    if (odometer > OdoEE)
    {
      EEPROM.put(0, odometer);
    }
    if (oil_Servis < ServEE)
    {
      EEPROM.put(20, oil_Servis);
    }
    saveFlag = 2;
  }

  //________________________________________________________________________________________________________________________
  //...............................заміри розгону...........................................................................
  if (FiltrSpeed == 0 && ign_flag == 2)
  {
    rozg_Flag = 0;
    timer_Rozg = millis();
  }
  if (FiltrSpeed >= 60 && rozg_Flag == 0 && ign_flag == 2)
  {
    rozg_60 = (millis() - timer_Rozg) / 1000;
    if (rozg_60 < max_Rozg60)
    {
      max_Rozg60 = rozg_60;
      EEPROM.put(10, rozg_60);
    }
    rozg_Flag = 1;
  }
  if (FiltrSpeed >= 100 && rozg_Flag == 1 && ign_flag == 2)
  {
    rozg_100 = (millis() - timer_Rozg) / 1000;
    if (rozg_100 < max_Rozg100)
    {
      max_Rozg100 = rozg_100;
      EEPROM.put(15, rozg_100);
    }
    rozg_Flag = 2;
  }

  //________________________________________________________________________________________________________________________
  //.............................Сервіси....................................................................................

  oil_Servis = oil_Temp - ((stala * serv_counter) / 1000); //вираховуєм загальний пробіг

  if (oil_Servis < 1000 && cont_meny == 1)
  {
    myOLED.invText(true);
    myOLED.setFont(MediumFont);
    myOLED.print("OIL!", 75, 4);
    myOLED.invText(false);
  }
  //__________________________________________________________________________________________________________________________________
  //..........................Ходові вогні............................................................................................

  if (ign_flag == 2)
  {
    svitlo_Timer = millis();
  }

  if (svitlo_flag == 1 && ign_flag == 2 && FiltrSpeed > 10 && fara_State == 0)
  {
    digitalWrite(fara_out, HIGH);
    fara_State = 1;
  }
  if (svitlo_flag == 1 && ign_flag == 0 && fara_State == 1 && millis() - svitlo_Timer > 25000)
  {
    digitalWrite(fara_out, LOW);
    fara_State = 0;
  }

  if (svitlo_flag == 0)
  {
    digitalWrite(fara_out, LOW);
  }

  //_________________________________________________________________________________________________________________________
  //............................функція енкодер реалізація інкремента і декремента меню обробка кнопок........................

  if (enc.isRight()) // якщо був поворот направо
  {
    myOLED.clrScr();
    cont_meny--;
  }

  if (enc.isLeft()) // якщо був поворот наліво
  {
    myOLED.clrScr();
    cont_meny++;
  }

  if (enc.isSingle()) //одинарний клік
  {
    myOLED.clrScr();
    presMeny = 1;
    clcMeny++;
    if (clcMeny > 1)
      clcMeny = 0;
  }
  else
  {
    presMeny = 0;
  }
  if (enc.isDouble()) //подвійний клік
  {
    myOLED.clrScr();
    if (cont_meny > 9)
    {
      cont_meny /= 10;
    }
    else
    {
      cont_meny *= 10;
    }
    douMeny++;
    if (douMeny > 1)
      holMeny = 0;
  }

  if (enc.isHolded()) //довге утримування
  {
    myOLED.clrScr();
    holMeny = 1;
  }
  else
  {
    holMeny = 0;
  }

  if (cont_meny == 0)
    cont_meny = 4;
  if (cont_meny == 5)
    cont_meny = 1;

  if (cont_meny == 9)
    cont_meny = 11;
  if (cont_meny == 12)
    cont_meny = 10;
  if (cont_meny == 22)
    cont_meny = 20;
  if (cont_meny == 19)
    cont_meny = 21;
  if (cont_meny == 29)
    cont_meny = 30;
  if (cont_meny == 31)
    cont_meny = 30;
  if (cont_meny == 40)
    cont_meny = 4;

  //_________________________________________________________________________________________________________________________
  //....................MENU...............................................................................................

  switch (cont_meny)
  {

  case 1: //тут виводимо швидкість і одометер, можна при натиснені на кнопку енкодера ще суточний або загальний одометер
    myOLED.setFont(MegaNumbers);
    myOLED.print(segment_0, 0, 7);
    myOLED.print(segment_1, 24, 7);
    myOLED.print(segment_2, 48, 7);
    myOLED.setFont(MediumFont);
    myOLED.print("KM/H", 80, 7);
    myOLED.setFont(SmallFont);
    myOLED.print("----------------------", 0, 2);

    if (clcMeny == 0)
    {
      myOLED.setFont(MediumFont);
      myOLED.print(odometer, 0, 1);
    }
    else
    {
      myOLED.setFont(SmallFont);
      myOLED.print("Odo1:", 0, 1);
      myOLED.setFont(MediumFont);
      myOLED.print(odometer1, 35, 1);
    }
    break;
  case 10: //тут виводимо max швидкість
    myOLED.setFont(SmallFont);
    myOLED.print("----------------------", 0, 2);
    myOLED.setFont(MegaNumbers);
    myOLED.print(max_SPD, 0, 7);
    myOLED.setFont(MediumFont);
    myOLED.print("Max speed:", 0, 1);
    myOLED.setFont(MediumFont);
    myOLED.print("KM/H", 75, 7);
    if (holMeny == 1)
    {
      max_SPD = 0;
      EEPROM.put(5, 0);
    }

    break;

  case 11: //тут виводимо одометри і скидаєм суточний
    myOLED.setFont(MediumFont);
    myOLED.print("Odometer1:", 0, 1);
    myOLED.setFont(MediumFont);
    myOLED.print(odometer1, 0, 3);
    myOLED.setFont(MediumFont);
    myOLED.print("Odometer:", 0, 5);
    myOLED.setFont(MediumFont);
    myOLED.print(odometer, 0, 7);
    if (holMeny == 1)
    {
      odo1_counter = 0;
    }

    break;

  case 2: //другий екран меню

    myOLED.setFont(SmallFont);
    myOLED.print("Razgon ot 0 do 60:", 0, 0);
    myOLED.setFont(SmallFont);
    myOLED.print("Seconds.", 80, 2);
    myOLED.setFont(MediumFont);
    myOLED.print(rozg_60, 0, 2);

    myOLED.setFont(SmallFont);
    myOLED.print("Razgon ot 0 do 100:", 0, 4);
    myOLED.setFont(SmallFont);
    myOLED.print("Seconds.", 80, 6);
    myOLED.setFont(MediumFont);
    myOLED.print(rozg_100, 0, 6);

    break;

  case 20: //третій екран меню

    myOLED.setFont(SmallFont);
    myOLED.print("Minimal ot 0 do 60:", 0, 0);
    myOLED.setFont(SmallFont);
    myOLED.print("Sec", 0, 2);
    myOLED.setFont(MediumFont);
    myOLED.print(max_Rozg60, 50, 2);

    myOLED.setFont(SmallFont);
    myOLED.print("Minimal ot 0 do 100:", 0, 4);
    myOLED.setFont(SmallFont);
    myOLED.print("Sec", 0, 6);
    myOLED.setFont(MediumFont);
    myOLED.print(max_Rozg100, 50, 6);

    break;

  case 21: //тут скидаємо динамічні заміри
    myOLED.setFont(MediumFont);
    myOLED.print("Reset razgon", 0, 1);
    myOLED.setFont(SmallFont);
    myOLED.print("To reset pres and hold", 0, 3);
    myOLED.print("booton", 0, 4);
    if (holMeny == 1)
    {
      rozg_60 = 110;
      rozg_100 = 110;
      max_Rozg60 = 110;
      max_Rozg100 = 110;
      EEPROM.put(10, 100.0); //мінімальний розгін 1   //
      EEPROM.put(15, 100.0); //мінімальний розгін 2   //
      myOLED.setFont(MediumFont);
      myOLED.print("Reset OK!", 0, 7);
    }
    break;

  // 3-й екран меню
  case 3: // servises
    myOLED.setFont(SmallFont);
    myOLED.print("SERVISES...", 0, 1);
    myOLED.print("----------------------", 0, 2);
    if (oil_Servis < 1000 && cont_meny == 3)
    {
      myOLED.invText(true);
      myOLED.setFont(MediumFont);
      myOLED.print("Change Oil", 0, 4);
      myOLED.invText(false);
    }
    else
    {
      myOLED.setFont(MediumFont);
      myOLED.print("Change Oil", 0, 4);
    }
    myOLED.setFont(MediumFont);
    myOLED.print("Km", 100, 7);

    myOLED.setFont(MediumFont);
    myOLED.print(oil_Servis, 0, 7);

    break;

  case 30: //тут скидаємо servis oil
    myOLED.setFont(SmallFont);
    myOLED.print("----------------------", 0, 2);
    myOLED.setFont(MediumFont);
    myOLED.print("Motor Oil", 0, 1);
    myOLED.print(oil_Servis, 0, 4);
    myOLED.print("Km", 100, 4);
    myOLED.setFont(SmallFont);
    myOLED.print("To reset pres and hold", 0, 6);
    myOLED.print("booton", 0, 7);
    if (holMeny == 1)
    {
      oil_Temp = 10000.0;
      serv_counter = 0;
      EEPROM.put(20, 10000.0);
      myOLED.setFont(SmallFont);
      myOLED.print("Reset OK!", 60, 7);
    }
    break;

  case 4: // ДХО

    myOLED.setFont(MediumFont);
    myOLED.print("HODOVIE", 25, 1);
    myOLED.print("OGNI", 40, 3);
    myOLED.print("|", 60, 7);
    myOLED.setFont(SmallFont);
    myOLED.print("----------------------", 0, 4);
    myOLED.setFont(MediumFont);
    if (svitlo_flag == 1)
    {
      myOLED.invText(true);
      myOLED.print("ON", 20, 7);
      myOLED.invText(false);
    }
    else
    {
      myOLED.print("ON", 20, 7);
    }

    if (svitlo_flag == 0)
    {
      myOLED.invText(true);
      myOLED.print("OFF", 80, 7);
      myOLED.invText(false);
    }
    else
    {
      myOLED.print("OFF", 80, 7);
    }

    if (holMeny == 1)
    {
      svitlo_flag = svitlo_flag == 0 ? 1 : 0;
      EEPROM.put(25, svitlo_flag);
    }
    break;
  }
}
//##################################################END####################################################################

////////////////////////////////////////////////////bitmap_image////////////////////////////////////////////////////////////