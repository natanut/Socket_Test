//--------------------------------------------------------------------------------------------------------------------------------
#include <PZEM004Tv30.h>

#if defined(ESP32)

#if !defined(PZEM_RX_PIN) && !defined(PZEM_TX_PIN)

#define PZEM_RX_PIN 18
#define PZEM_TX_PIN 19
#endif

#define PZEM_SERIAL Serial2
#define CONSOLE_SERIAL Serial
PZEM004Tv30 pzem(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN);

#elif defined(ESP8266)

#define PZEM_SERIAL Serial
#define CONSOLE_SERIAL Serial1
PZEM004Tv30 pzem(PZEM_SERIAL);
#else

#define PZEM_SERIAL Serial2
#define CONSOLE_SERIAL Serial
PZEM004Tv30 pzem(PZEM_SERIAL);
#endif

//--------------------------------------------------------------------------------------------------------------------------------
#include <Wire.h>  /* นำเข้าไรบลารี่ Wire.h */
#include <LiquidCrystal_I2C.h>  /* นำเข้าไรบลารี่ LiquidCrystal_I2C.h */
LiquidCrystal_I2C lcd(0x27, 20, 4);  /* กำหนดค่า lcd ที่ i2C ตำแหน่ง 0x27 ที่ขนาด 20 ตัวอักษร 4 แถว*/

//--------------------------------------------------------------------------------------------------------------------------------
#include <Keypad_I2C.h>
#include <Keypad.h>
#define I2CADDR 0x20

const byte ROWS = 4; // กำหนดจำนวนของ Rows
const byte COLS = 4;  // กำหนดจำนวนของ Columns

// กำหนด Key ที่ใช้งาน (4x4)
char keys[ROWS][COLS] = { {'1','2','3','A'},
                          {'4','5','6','B'},
                          {'7','8','9','C'},
                          {'*','0','#','D'} };

// กำหนด Pin ที่ใช้งาน (4x4)
byte rowPins[ROWS] = {0, 1, 2, 3}; // เชื่อมต่อกับ Pin แถวของปุ่มกด
byte colPins[COLS] = {4, 5, 6, 7}; // เชื่อมต่อกับ Pin คอลัมน์ของปุ่มกด

// makeKeymap(keys) : กำหนด Keymap
// rowPins : กำหนด Pin แถวของปุ่มกด
// colPins : กำหนด Pin คอลัมน์ของปุ่มกด
// ROWS : กำหนดจำนวนของ Rows
// COLS : กำหนดจำนวนของ Columns
// I2CADDR : กำหนด Address ขอ i2C
// PCF8574 : กำหนดเบอร์ IC
Keypad_I2C keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS, I2CADDR, PCF8574 );

//--------------------------------------------------------------------------------------------------------------------------------
#include <TinyGPS++.h>  /* นำเข้าไรบลารี่ TinyGPS++.h */
#include <HardwareSerial.h>  /* นำเข้าไรบลารี่ HardwareSerial.h */

#define RXPin (16)  /* กำหนดขา รับข้อมูล */
#define TXPin (17)  /* กำหนดขา ส่งข้อมูล */

static const uint32_t GPSBaud = 9600;  /* กำนดความถี่รับส่งข้อมูลกับ GPS */
TinyGPSPlus GPS;  /* สร้าง GPS ใช้งาน TinyGPSPlus */
HardwareSerial Hardware_Serial(2);  /* สร้าง Hardware_Serial ใช้งาน HardwareSerial */

//--------------------------------------------------------------------------------------------------------------------------------
TaskHandle_t TaskDisplay;  /* กำหนด TaskDisplay เพื่อสร้างงานใน TaskHandle_t */ 
TaskHandle_t TaskKeypad;  /* กำหนด TaskDisplay TaskKeypad TaskHandle_t */
TaskHandle_t TaskGPS; /* กำหนด TaskGPS TaskKeypad TaskHandle_t */
TaskHandle_t TaskProcessing; /* กำหนด TaskProcessing TaskKeypad TaskHandle_t */

//--------------------------------------------------------------------------------------------------------------------------------
/* 
Detect_GPS 
Detect_PZEM 
Detect_Wifi 
*/

// String Status_Working = "Ready";  /* ตัวแปรชุดอักขรสำหรับเก็บ สถานะการทำงาน */
String Status_Working = "Load Location";  /* ตัวแปรชุดอักขรสำหรับเก็บ สถานะการทำงาน */
String Buffer_ID = "";  /* ตัวแปรชุดอักขรสำหรับเก็บ ID ผู้ถูกตรวจสอบ */
String Save_ID ="";

String Status_GPS = "";  /* ตัวแปรชุดอักขรสำหรับเก็บ สถานะ GPS */
String Buffer_GPS_Lat = "";  /* ตัวแปรชุดอักขรสำหรับเก็บ ตำแหน่งเส้นรุ้ง ที่ได้จาก GPS */
String Buffer_GPS_Lng = "";  /* ตัวแปรชุดอักขรสำหรับเก็บ ตำแหน่งเส้นแวง ที่ได้จาก GPS */
String Save_Location = "";

//--------------------------------------------------------------------------------------------------------------------------------
String Buffer_IndexMenu[10];
int Count_IndexMenu = 0;

char Buffer_IndexID[7];
int Count_IndexID = 0;



//================================================================================================================================
void setup() {

  //--------------------------------------------------------------------------------------------------------------------------------
  Serial.begin(115200);  /* กำหนดความถี่ Serial Monitor */

  //--------------------------------------------------------------------------------------------------------------------------------
  // Pzem
  CONSOLE_SERIAL.begin(115200);

  //--------------------------------------------------------------------------------------------------------------------------------
  Wire.begin();  // เรียกการเชื่อมต่อ Wire
  keypad.begin(makeKeymap(keys));  // เรียกกาเชื่อมต่อ

  //--------------------------------------------------------------------------------------------------------------------------------
  lcd.begin();  /* เริ่มใช้งาน lcd */
  lcd.clear();

  lcd.setCursor(0, 0);
  // -------"XXXXXXXXXXXXXXXXXXXX"
  lcd.print("                    ");
  lcd.setCursor(0, 1);
  lcd.print(" TestSocket Project ");
  lcd.setCursor(0, 2);
  lcd.print(" ------------------ ");
  lcd.setCursor(0, 3);
  lcd.print("                    ");
  delay(3000);
  lcd.clear();

  //--------------------------------------------------------------------------------------------------------------------------------
  Hardware_Serial.begin(GPSBaud, SERIAL_8N1, RXPin, TXPin, false);  /* เริ่มเชื่อมต่อ Hardware_Serial */
  Serial.println(String("GPS Library Version: ") + TinyGPSPlus::libraryVersion());  /* แสดงผล library Version */
  
  //--------------------------------------------------------------------------------------------------------------------------------
  // Task1code,   /* Task function. */
  //                   "Task1",     /* name of task. */
  //                   10000,       /* Stack size of task */
  //                   NULL,        /* parameter of the task */
  //                   1,           /* priority of the task */
  //                   &Task1,      /* Task handle to keep track of created task */
  //                   0);          /* pin task to core 0 */    

  //create a task that executes the Task0code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(Task_Display, "Task0", 10000, NULL, 1, &TaskDisplay, 0);
  xTaskCreatePinnedToCore(Task_Keypad, "Task1", 10000, NULL, 1, &TaskKeypad, 0);
  xTaskCreatePinnedToCore(Task_GPS, "Task2", 10000, NULL, 1, &TaskGPS, 1);
  xTaskCreatePinnedToCore(Task_Processing, "Task3", 10000, NULL, 1, &TaskProcessing, 1);

}



//================================================================================================================================
void Task_Display(void* pvParameters) {
  for (;;) {
    //--------------------------------------------------------------------------------------------------------------------------------
    // แสดงผล ตรวจไม่พบโมดูล GPS
    if (Status_GPS == "Detected Fail" and Buffer_IndexMenu[0] == "")
    {
      lcd.setCursor(0, 0);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("    Status GPS:     ");
      lcd.setCursor(0, 1);
      // --------------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print(String("   ") + Status_GPS);
      lcd.setCursor(0, 2);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("   Please Turn OFF  ");
      lcd.setCursor(0, 3);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("And Checking Module!");

    }

    //--------------------------------------------------------------------------------------------------------------------------------
    // แสดงผล กำลังพยายามเชื่อมต่อดาวเทียม
    else if (Status_GPS == "Satellite Checking" and Buffer_IndexMenu[0] == "")
    {
      lcd.setCursor(0, 0);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("    Status: GPS     ");
      lcd.setCursor(0, 1);
      // --------------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print(String(" ") + Status_GPS);
      lcd.setCursor(0, 2);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("    Please wait.    ");
      lcd.setCursor(0, 3);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("                    ");

    }

    //--------------------------------------------------------------------------------------------------------------------------------
    // แสดงผล ตำแหน่งที่ได้รับจากดาวเทียม
    else if (Status_GPS == "Load Location" and Buffer_IndexMenu[0] == "")
    {
    lcd.setCursor(0, 0);
    // -------"XXXXXXXXXXXXXXXXXXXX"
    lcd.print(String("   ") + Status_GPS);
    lcd.setCursor(0, 1);
    // -------"XXXXXXXXXXXXXXXXXXXX"
    lcd.print("Lat: " + Buffer_GPS_Lat + "      ");
    lcd.setCursor(0, 2);
    // -------"XXXXXXXXXXXXXXXXXXXX"
    lcd.print("Long: " + Buffer_GPS_Lng + "    ");
    lcd.setCursor(0, 3);
    // -------"XXXXXXXXXXXXXXXXXXXX"
    lcd.print("             'A'Save");
    
    }

    //--------------------------------------------------------------------------------------------------------------------------------
    else if (Buffer_IndexMenu[0] == "Save Location" and Buffer_IndexMenu[1] == "" and Count_IndexID == 0)
    {
    
    lcd.setCursor(0, 0);
    // -------"XXXXXXXXXXXXXXXXXXXX"
    lcd.print("    Please Enter    ");
    lcd.setCursor(0, 1);
    // -------"XXXXXXXXXXXXXXXXXXXX"
    lcd.print("    ID:________     ");
    lcd.setCursor(0, 2);
    // -------"XXXXXXXXXXXXXXXXXXXX"
    lcd.print("'B'Back             ");
    lcd.setCursor(0, 3);
    // -------"XXXXXXXXXXXXXXXXXXXX"
    lcd.print("                    ");
    
    }

    //--------------------------------------------------------------------------------------------------------------------------------
    else if (Buffer_IndexMenu[0] == "Save Location" and Buffer_IndexMenu[1] == "" and Count_IndexID > 0 and Count_IndexID < 7)
    {
      //--------------------------------------------------------------------------------------------------------------------------------
      lcd.setCursor(0, 0);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("    Please Enter    ");
      lcd.setCursor(0, 1);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("     ID:" + Buffer_ID);
      lcd.setCursor(0, 2);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("'B'Back    'D'Delete");
      lcd.setCursor(0, 3);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("'C'Clear            ");

    }

    //--------------------------------------------------------------------------------------------------------------------------------
    else if (Buffer_IndexMenu[0] == "Save Location" and Buffer_IndexMenu[1] == "" and Count_IndexID == 7)
    {
      //--------------------------------------------------------------------------------------------------------------------------------
      lcd.setCursor(0, 0);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("    Please Enter    ");
      lcd.setCursor(0, 1);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("     ID:" + Buffer_ID);
      lcd.setCursor(0, 2);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("'B'Back    'D'Delete");
      lcd.setCursor(0, 3);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("'C'Clear     'A'Save");

    }

    //--------------------------------------------------------------------------------------------------------------------------------
    else if (Buffer_IndexMenu[0] == "Save Location" and Buffer_IndexMenu[1] == "Save ID")
    {
      //--------------------------------------------------------------------------------------------------------------------------------
      lcd.setCursor(0, 0);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("      BB Test       ");
      lcd.setCursor(0, 1);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("VAC: 220V. Fq: 50Hz");
      lcd.setCursor(0, 2);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("'*'Info             ");
      lcd.setCursor(0, 3);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("'B'Back      'A'Save");

    }

    //--------------------------------------------------------------------------------------------------------------------------------
    delay(100);  // หน่วงเวลา

   }
}

//================================================================================================================================
void Task_Keypad(void* pvParameters) {

  for (;;) {

    //--------------------------------------------------------------------------------------------------------------------------------
    char Custom_Key = keypad.getKey();  // สร้างตัวแปรชื่อ Custom_Key ชนิด char เพื่อเก็บตัวอักขระที่กด

    //--------------------------------------------------------------------------------------------------------------------------------
    // ถ้าหากตัวแปร Custom_Key มีอักขระ
    if (Custom_Key)
    {  
      // Serial.println(String("Key: ") + Custom_Key);

      //--------------------------------------------------------------------------------------------------------------------------------
      // ถ้ากด A เมื่ออยู่หน้า โหลดตำแหน่ง
      if (Status_GPS == "Load Location" and 
      Buffer_IndexMenu[0] == "" and 
      Buffer_IndexMenu[1] == "" and 
      Custom_Key == 'A')
      {
        
        //--------------------------------------------------------------------------------------------------------------------------------
        Buffer_IndexMenu[Count_IndexMenu] = "Save Location";  /* บันทึกหน้าตำแหน่ง */
        Count_IndexMenu++;

        //--------------------------------------------------------------------------------------------------------------------------------
        Serial.println("--------------------------------------------");
        Serial.println(String("Count_IndexMenu: ") + Count_IndexMenu);
        Serial.print("Buffer_IndexMenu: ");
        for (int i = 0;  i < Count_IndexMenu; i++)
        {
          Serial.print(String("|") + Buffer_IndexMenu[i]);

        }
        Serial.println("|");

        //--------------------------------------------------------------------------------------------------------------------------------
        Save_Location = Buffer_GPS_Lat + String(",") + Buffer_GPS_Lng;
        Serial.println(String("Save_Location: ") + Save_Location);

        //--------------------------------------------------------------------------------------------------------------------------------
        lcd.clear();
       
      }

      //--------------------------------------------------------------------------------------------------------------------------------
      // กดหลับหน้าตำแหน่ง
      else if (Buffer_IndexMenu[0] == "Save Location" and
      Buffer_IndexMenu[1] == "" and
      Custom_Key == 'B')
      {
        //--------------------------------------------------------------------------------------------------------------------------------
        Count_IndexMenu--;
        Buffer_IndexMenu[Count_IndexMenu] = "";  /* บันทึกหน้าตำแหน่ง */
        

        //--------------------------------------------------------------------------------------------------------------------------------
        Serial.println("--------------------------------------------");
        Serial.println(String("Count_IndexMenu: ") + Count_IndexMenu);
        Serial.print("Buffer_IndexMenu: ");
        for (int i = 0;  i < Count_IndexMenu; i++)
        {
          Serial.print(String("|") + Buffer_IndexMenu[i]);

        }
        Serial.println("|");

        //--------------------------------------------------------------------------------------------------------------------------------
        Save_Location = "";
        Serial.println(String("Save_Location: ") + Save_Location);

        //--------------------------------------------------------------------------------------------------------------------------------
        Buffer_ID = "";
        Serial.println(String("Buffer_ID: ") + Save_Location);

        //--------------------------------------------------------------------------------------------------------------------------------
        for (int i = Count_IndexID;  i > 0; i--)
        {
          Buffer_IndexID[i] = NULL;
          Count_IndexID--;

        }

        Buffer_ID = "";

        //--------------------------------------------------------------------------------------------------------------------------------
        Serial.println("--------------------------------------------");
        Serial.println(String("Count_IndexID: ") + Count_IndexID);
        Serial.print("Buffer_IndexID: ");
        for (int i = 0;  i < Count_IndexID; i++)
        {
          Serial.print(String("|") + Buffer_IndexID[i]);

        }
        Serial.println("|");

        // //--------------------------------------------------------------------------------------------------------------------------------
        // lcd.clear();

      }

      //--------------------------------------------------------------------------------------------------------------------------------
      // กดหมายเลข ID
      else if (Buffer_IndexMenu[0] == "Save Location" and 
      Buffer_IndexMenu[1] == "" and
      Count_IndexID < sizeof(Buffer_IndexID) and 
      (Custom_Key == '0' or Custom_Key == '1' or Custom_Key == '2' or Custom_Key == '3' or
      Custom_Key == '4' or Custom_Key == '5' or Custom_Key == '6' or Custom_Key == '7' or
      Custom_Key == '8' or Custom_Key == '9'))
      {
        //--------------------------------------------------------------------------------------------------------------------------------
        Buffer_IndexID[Count_IndexID] = Custom_Key;  /* บันทึกหน้าตำแหน่ง */
        Count_IndexID++;

        Buffer_ID = String(Buffer_IndexID);

        //--------------------------------------------------------------------------------------------------------------------------------
        Serial.println("--------------------------------------------");
        Serial.println(String("Count_IndexID: ") + Count_IndexID);
        Serial.print("Buffer_IndexID: ");
        for (int i = 0;  i < Count_IndexID; i++)
        {
          Serial.print(String("|") + Buffer_IndexID[i]);

        }
        Serial.println("|");

      }

      //--------------------------------------------------------------------------------------------------------------------------------
      // ลบหมายเลข ID
      else if (Buffer_IndexMenu[0] == "Save Location" and 
      Buffer_IndexMenu[1] == "" and
      Count_IndexID != 0 and 
      Custom_Key == 'D')
      {
        //--------------------------------------------------------------------------------------------------------------------------------
        Count_IndexID--;
        Buffer_IndexID[Count_IndexID] = NULL;  /* บันทึกหน้าตำแหน่ง */

        Buffer_ID = String(Buffer_IndexID);

        //--------------------------------------------------------------------------------------------------------------------------------
        Serial.println("--------------------------------------------");
        Serial.println(String("Count_IndexID: ") + Count_IndexID);
        Serial.print("Buffer_IndexID: ");
        for (int i = 0;  i < Count_IndexID; i++)
        {
          Serial.print(String("|") + Buffer_IndexID[i]);

        }
        Serial.println("|");

        //--------------------------------------------------------------------------------------------------------------------------------
        lcd.clear();

      }

      //--------------------------------------------------------------------------------------------------------------------------------
      // เคลียร์หมายเลข ID
      else if (Buffer_IndexMenu[0] == "Save Location" and 
      Buffer_IndexMenu[1] == "" and
      Count_IndexID != 0 and 
      Custom_Key == 'C')
      {

        //--------------------------------------------------------------------------------------------------------------------------------
        for (int i = Count_IndexID;  i > 0; i--)
        {
          Buffer_IndexID[i] = NULL;
          Count_IndexID--;

        }

        Buffer_ID = "";

        //--------------------------------------------------------------------------------------------------------------------------------
        Serial.println("--------------------------------------------");
        Serial.println(String("Count_IndexID: ") + Count_IndexID);
        Serial.print("Buffer_IndexID: ");
        for (int i = 0;  i < Count_IndexID; i++)
        {
          Serial.print(String("|") + Buffer_IndexID[i]);

        }
        Serial.println("|");

        //--------------------------------------------------------------------------------------------------------------------------------
        lcd.clear();

      }

      //--------------------------------------------------------------------------------------------------------------------------------
      // เซฟหมายเลข ID
      else if (Buffer_IndexMenu[0] == "Save Location" and 
      Buffer_IndexMenu[1] == "" and
      Count_IndexID == 7 and 
      Custom_Key == 'A')
      {
        Serial.println("--------------------------------------------");
        Save_ID = Buffer_ID;
        Serial.println(String("Save_ID: ") + Save_ID);

        //--------------------------------------------------------------------------------------------------------------------------------
        Buffer_IndexMenu[Count_IndexMenu] = "Save ID";  /* บันทึกหน้าตำแหน่ง */
        Count_IndexMenu++;

        //--------------------------------------------------------------------------------------------------------------------------------
        Serial.println("--------------------------------------------");
        Serial.println(String("Count_IndexMenu: ") + Count_IndexMenu);
        Serial.print("Buffer_IndexMenu: ");
        for (int i = 0;  i < Count_IndexMenu; i++)
        {
          Serial.print(String("|") + Buffer_IndexMenu[i]);

        }
        Serial.println("|");

        //--------------------------------------------------------------------------------------------------------------------------------
        lcd.clear();

      }
    }

    delay(100);  // หน่วงเวลา
  }
}

//================================================================================================================================
void Task_GPS(void* pvParameters) {

  for (;;) {

    //--------------------------------------------------------------------------------------------------------------------------------
    while (Hardware_Serial.available() > 0)

    //--------------------------------------------------------------------------------------------------------------------------------
    if (GPS.encode(Hardware_Serial.read()))
    {
      //--------------------------------------------------------------------------------------------------------------------------------
      // Serial.print(F("Location: "));

      //--------------------------------------------------------------------------------------------------------------------------------
      if (GPS.location.isValid())
      {
        Status_GPS = "Load Location";
        Buffer_GPS_Lat = String(GPS.location.lat(), 6);
        Buffer_GPS_Lng = String(GPS.location.lng(), 6);

        // Serial.print(Status_GPS + " : ");
        // Serial.print(Buffer_GPS_Lat);
        // Serial.print(F(","));
        // Serial.print(Buffer_GPS_Lng);

      }

      //--------------------------------------------------------------------------------------------------------------------------------
      else 
      {
        Status_GPS = "Satellite Checking";
        Serial.println(Buffer_IndexMenu[0]);

      }
    }
    
    //--------------------------------------------------------------------------------------------------------------------------------
    if (millis() > 5000 && GPS.charsProcessed() < 10)
    {
      Status_GPS = "Detected Fail";
      Serial.println(Buffer_IndexMenu[0]);
      while(true);
      
    }

    delay(1000);  // หน่วงเวลา   
  }
}

//================================================================================================================================
void Task_Processing(void* pvParameters) {

  for (;;) {
    
    float voltage = pzem.voltage();
    // float current = pzem.current();
    // float power = pzem.power();
    // float energy = pzem.energy();
    // float frequency = pzem.frequency();
    // float pf = pzem.pf();

    // Check if the data is valid
    if(isnan(voltage)){
        Serial.println("Error reading voltage");
    // } else if (isnan(current)) {
    //     Serial.println("Error reading current");
    // } else if (isnan(power)) {
    //     Serial.println("Error reading power");
    // } else if (isnan(energy)) {
    //     Serial.println("Error reading energy");
    // } else if (isnan(frequency)) {
    //     Serial.println("Error reading frequency");
    // } else if (isnan(pf)) {
    //     Serial.println("Error reading power factor");
    } else {

        // Print the values to the Serial console
        Serial.print("Voltage: ");      Serial.print(voltage);      Serial.println("V");
        // Serial.print("Current: ");      Serial.print(current);      Serial.println("A");
        // Serial.print("Power: ");        Serial.print(power);        Serial.println("W");
        // Serial.print("Energy: ");       Serial.print(energy,3);     Serial.println("kWh");
        // Serial.print("Frequency: ");    Serial.print(frequency, 1); Serial.println("Hz");
        // Serial.print("PF: ");           Serial.println(pf);

    }
    delay(1000);  // หน่วงเวลา   
  }
}

//================================================================================================================================
void loop() {

}