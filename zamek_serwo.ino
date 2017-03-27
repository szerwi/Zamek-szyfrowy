#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <DS3231.h>
#include <SPI.h>
#include <MFRC522.h>

#define czujnik 23 //mikroprzelacznik sprawdzjacy otwarcie drzwi
#define button 8 //otwieranie drzwi z wewnatrz
#define czujruch 10

String password = "1234"; //aktualne haslo
String nowe = ""; //ustawianie nowego hasla
String wpisane = ""; //wpisane z klawiatury
String powtorz = ""; //powtarzanie hasla przy wpisywaniu nowego

const byte ROWS = 4;
const byte COLS = 3;
// Define the Keymap
char keys[ROWS][COLS] = 
{
  {'1','2','3'},  
  {'4','5','6'},              //inicjowanie klawiatury
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = { 22,24,26,28 };
byte colPins[COLS] = { 30,32,34 };

Servo servo; //nazwa serwa

DS3231 clock;
RTCDateTime dt;

byte proba = 0; //proba zlego wpisania hasla
byte czuj = 0; //stan czujnika zamkniecia drzwi
unsigned long czas = millis();
unsigned long czasekran = millis();
unsigned long czasdata = millis();

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS ); //inicjowanie klawiatury
LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);           //inicjowanie wyswietlacza

const byte UID[] = {0xAF, 0x0E, 0x50, 0x39}; //UID

MFRC522 rfid(53, 5); //Definiowanie RFID
MFRC522::MIFARE_Key key;

void setup()
{
  clock.begin();
  dt = clock.getDateTime();
  SPI.begin();
  rfid.PCD_Init();
  Serial.begin(9600);
  Serial1.begin(9600); //serial dla bluetooth
  Serial.println(clock.dateFormat("l d-m-Y H:i:s", dt));
  lcd.begin (20,4);
  lcd.home();
  lcd.print("     Wpisz kod:    ");
  lcd.setCursor(1,2);
  lcd.print(clock.dateFormat("l d-m-Y", dt));
  lcd.setCursor(0,3);
  lcd.print(clock.dateFormat("H:i:s", dt));
  lcd.setCursor(0,1); 
  servo.attach(9);
  servo.write(0);
  clock.begin();
  keypad.addEventListener(wpisz); //funkcja ktora wykonuje sie po wcisnieciu przycisku na klawiaturze
  pinMode(czujnik, INPUT_PULLUP);
  pinMode(button, INPUT_PULLUP);
  pinMode(czujruch, INPUT);   
}

void wyczysc() 
{
  dt = clock.getDateTime(); lcd.clear();  lcd.home(); lcd.print("     Wpisz kod:    "); //czysci ekran i wypisuje procedure wpisywania hasla
  lcd.setCursor(3,2); lcd.print(clock.dateFormat("l d-m-Y", dt)); lcd.setCursor(3,3); lcd.print(clock.dateFormat("H:i:s", dt)); lcd.setCursor(0,1);
}

void loop()
{
   czas = millis();
   keypad.getKey();
   otworz(); 
   //ruch();
   bluetooth();
   zmienczas();
   nfc();
}

void wpisz(KeypadEvent eKey)
{
  switch(keypad.getState())
  {
    case PRESSED:
    switch (eKey)
    {
      case '#': sprawdz(); break;
      case '*': wpisane = ""; wyczysc(); break;
      default: lcd.print('*'); wpisane += eKey; break;
    }
  }
}

void sprawdz()
{
  byte stan = 0;
  byte sek = 30;
  
  if (wpisane == password) //jezeli haslo jest poprawne otwiera drzwi
  {
    lcd.clear();  lcd.home();
    czuj=0;
    lcd.print("      POPRAWNE      ");
    lcd.setCursor(0,1);
    lcd.print("       HASLO        ");
    otworz();
    wyczysc();
    wpisane = "";
    proba = 0;
  }
  
  else if(wpisane == "0") //jezeli haslo jest rowne 0 przechodzi do procedury zmiany hasla
  {
    wpisane = "";
    lcd.clear();  lcd.home();
    lcd.print("    ZMIANA HASLA    ");
    lcd.setCursor(0,1);
    lcd.print("Stare: ");
    keypad.addEventListener(nowehaslo);
  }
  
  else //jezeli haslo jest zle odmowa dostepu
  {
    proba++;
    lcd.clear();  lcd.home();
    lcd.print("     ZLE HASLO      ");
    lcd.setCursor(0,1);
    lcd.print("To twoja ");
    lcd.print(proba);
    lcd.print(" proba");
    delay(1000);
    wyczysc();
    wpisane = "";
    if(proba>=5) //jezeli haslo jest zle po raz 5 i wiecej czeka 30sekund
    {
     sek = 30;
     lcd.clear();  lcd.home();
     for(sek!=0; sek--;)
     { 
        lcd.print("  Musisz poczekac  ");
        lcd.setCursor(4,1);
        lcd.print(sek);
        lcd.print("   sekund!    ");
        delay(1000);
     }
    wyczysc();
   }
  }
}

void nowehaslo(KeypadEvent eKey) //ustawianie nowego hasla - stare haslo
{
  switch (keypad.getState())
  {
   case PRESSED:
      switch (eKey)
      {
        case '#':
        if(wpisane == password)
        {
          wpisane = ""; nowe = "";
          lcd.clear();  lcd.home();
          lcd.print("    ZMIANA HASLA    ");
          lcd.setCursor(0,1);
          lcd.print("Nowe: ");
          keypad.addEventListener(nowehaslo2);
        }
        else
        {
          wpisane = ""; nowe = "";
          lcd.clear();  lcd.home();
          lcd.print("   Odmowa dostepu    ");
          lcd.setCursor(0,1);
          lcd.print("     Zle haslo       ");
          delay(2000);
          wyczysc();
          keypad.addEventListener(wpisz);
          wyczysc();
        }
       break;
       case '*': 
        wpisane = ""; nowe = "";
        lcd.clear();  lcd.home();
        lcd.print("    ZMIANA HASLA    ");
        lcd.setCursor(0,1);
        lcd.print("Stare: ");
        break;
       default: lcd.print('*'); wpisane += eKey; break;
       break;
      }
  }
}

void nowehaslo2(KeypadEvent eKey) //ustawianie nowego hasla - nowe haslo
{
switch (keypad.getState())
  {
   case PRESSED:
      switch (eKey)
      {
        case '#':
          wpisane = ""; powtorz = "";
          lcd.clear();  lcd.home();
          lcd.print("    ZMIANA HASLA    ");
          lcd.setCursor(0,1);
          lcd.print("Powtorz: ");
          keypad.addEventListener(nowehaslo3);
        
        break;
       case '*': 
        wpisane = ""; nowe = ""; powtorz = "";
        lcd.clear();  lcd.home();
        lcd.print("    ZMIANA HASLA    ");
        lcd.setCursor(0,1);
        lcd.print("Nowe: ");
        break;
       default: lcd.print('*'); nowe += eKey; break;
       break;
      }
  }
}

void nowehaslo3(KeypadEvent eKey) //ustawienie nowego hasla - powtorz haslo
{
  switch (keypad.getState())
  {
   case PRESSED:
      switch (eKey)
      {
        case '#':
        if(powtorz == nowe)
        {
          password = nowe;
          lcd.clear(); lcd.home();
          lcd.print("     NOWE HASLO     ");
          lcd.setCursor(0,1);
          lcd.print("      ZAPISANE      ");
          delay(2000);
          wyczysc();
          keypad.addEventListener(wpisz);
        }
        else
        {
          wpisane = ""; nowe = ""; powtorz = "";
          lcd.clear();  lcd.home();
          lcd.print("   Odmowa dostepu     ");
          lcd.setCursor(0,1);
          lcd.print("   Brak spojnosci     ");
          delay(2000);
          wyczysc();
          keypad.addEventListener(wpisz);
          wyczysc();
        }
        
        break;
       case '*': 
        wpisane = ""; nowe = ""; powtorz = "";
        lcd.clear();  lcd.home();
        lcd.print("    ZMIANA HASLA    ");
        lcd.setCursor(0,1);
        lcd.print("Powtorz: ");
        break;
       default: lcd.print('*'); powtorz += eKey; break;
       break;
      }
  }
}

void bluetooth() //otwieranie bluetooth
{
  if(Serial1.read() == '1')
  {
    lcd.clear(); lcd.home();
    lcd.print("  OTRZYMANO SYGNAL  ");
    lcd.setCursor(0,1);
    lcd.print("     BLUETOOTH      ");
    czuj = 0;
    otworz();
    wyczysc();
  }
}

void otworz() //otwieranie drzwi
{
   if(digitalRead(button) == LOW) //otwieranie drzwi z przycisku
   {
    czuj = 0;
    lcd.clear(); lcd.home();
    lcd.print("  OTRZYMANO SYGNAL  ");
    lcd.setCursor(0,1);
    lcd.print("    Z PRZYCISKU     ");
   }
  if(czuj==0)
  {
  servo.write(90);
  delay(5000);
  wyczysc();
  czuj=1;
    }
  else if(czuj==1)
  {
  if(digitalRead(czujnik) == HIGH)
  {
  delay(1000);  
  servo.write(0);
  }
  }
}

void ruch() //wykrywanie ruchu i wylaczanie wyswietlacza po 30 sekundach bez ruchu
{
  if(digitalRead(czujruch)==HIGH)
  {
    lcd.backlight();
    czasekran = millis();
  }
  if(czas - czasekran >= 30000)
  {
    lcd.noBacklight();
    czasekran = millis();
  }
}

void zmienczas()
{
  

  if(czas - czasdata >= 1000)
  {
    czasdata = millis();
    dt = clock.getDateTime();
    lcd.setCursor(3,2); 
    lcd.print(clock.dateFormat("l d-m-Y", dt)); 
    lcd.setCursor(3,3); 
    lcd.print(clock.dateFormat("H:i:s", dt)); 
    lcd.setCursor(0,1);
  }
}

void nfc() {
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial())
  {
    if (rfid.uid.uidByte[0] == UID[0] && 
        rfid.uid.uidByte[1] == UID[1] &&
        rfid.uid.uidByte[2] == UID[2] &&
        rfid.uid.uidByte[3] == UID[3])
    {
      lcd.clear(); lcd.home();
      lcd.print("  OTRZYMANO SYGNAL  ");
      lcd.setCursor(0,1);
      lcd.print("        NFC         ");
      czuj = 0;
      otworz();
      wyczysc();
    } 
    else
    {
      lcd.clear(); lcd.home();
      lcd.print("     ZLY TOKEN      ");
      lcd.setCursor(0,1);
      lcd.print("        NFC         ");
      delay(2000);
      wyczysc();
    }
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
}

