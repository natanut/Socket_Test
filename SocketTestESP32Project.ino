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
Keypad_I2C keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS, I2CADDR, PCF8574);

//--------------------------------------------------------------------------------------------------------------------------------
#include <PZEM004Tv30.h>

#define PZEM_RX_PIN 18
#define PZEM_TX_PIN 19

#define PZEM_SERIAL Serial2
PZEM004Tv30 pzem(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN);

//--------------------------------------------------------------------------------------------------------------------------------
#include <TinyGPS++.h>  /* นำเข้าไรบลารี่ TinyGPS++.h */
#include <HardwareSerial.h>  /* นำเข้าไรบลารี่ HardwareSerial.h */

#define RXPin (16)  /* กำหนดขา รับข้อมูล */
#define TXPin (17)  /* กำหนดขา ส่งข้อมูล */

static const uint32_t GPSBaud = 9600;  /* กำนดความถี่รับส่งข้อมูลกับ GPS */
TinyGPSPlus GPS;  /* สร้าง GPS ใช้งาน TinyGPSPlus */
HardwareSerial Hardware_Serial(1);  /* สร้าง Hardware_Serial ใช้งาน HardwareSerial */

//--------------------------------------------------------------------------------------------------------------------------------
TaskHandle_t TaskDisplay;  /* กำหนด TaskDisplay เพื่อสร้างงานใน TaskHandle_t */ 
TaskHandle_t TaskKeypad;  /* กำหนด TaskDisplay TaskKeypad TaskHandle_t */
TaskHandle_t TaskGPS; /* กำหนด TaskGPS TaskKeypad TaskHandle_t */
TaskHandle_t TaskPZEM; /* กำหนด TaskProcessing TaskKeypad TaskHandle_t */
TaskHandle_t TaskSocket; /* กำหนด TaskProcessing TaskKeypad TaskHandle_t */

//--------------------------------------------------------------------------------------------------------------------------------
int ELCB_Pin = 13;  // กำหนดขาเทส ELCB

//--------------------------------------------------------------------------------------------------------------------------------
/* 
Detect_GPS 
Detect_PZEM 
Detect_Wifi 
*/

String Buffer_ID = "";  /* ตัวแปรชุดอักขรสำหรับเก็บ ID ผู้ถูกตรวจสอบ */
String Save_ID ="";  

String Buffer_GPS_Lat = "";  /* ตัวแปรชุดอักขรสำหรับเก็บ ตำแหน่งเส้นรุ้ง ที่ได้จาก GPS */
String Buffer_GPS_Lng = "";  /* ตัวแปรชุดอักขรสำหรับเก็บ ตำแหน่งเส้นแวง ที่ได้จาก GPS */
String Save_GPS_Lat = "";
String Save_GPS_Lng = "";

String Buffer_StatusBumblebee = "Fail";
int Buffer_Voltage = 0;
int Buffer_Frequency = 0;
String Save_StatusBumblebee = "Fail";

String Buffer_SocketTest = "";
String Save_SocketTest = "";

String Save_ELCB = "";

//--------------------------------------------------------------------------------------------------------------------------------
String Buffer_IndexMenu[10];
int Count_IndexMenu = 0;

char Buffer_IndexID[7];
int Count_IndexID = 0;

//--------------------------------------------------------------------------------------------------------------------------------
int LED_Pin1 = 25;
int LED_Pin2 = 26;
int LED_Pin3 = 27;

//================================================================================================================================
void setup() {

  //--------------------------------------------------------------------------------------------------------------------------------
  Serial.begin(115200);  /* กำหนดความถี่ Serial Monitor */

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
  xTaskCreatePinnedToCore(Task_Display, "TaskDisplay/Core0", 10000, NULL, 1, &TaskDisplay, 0);
  xTaskCreatePinnedToCore(Task_Keypad, "TaskKeypad/Core0", 10000, NULL, 1, &TaskKeypad, 0);
  xTaskCreatePinnedToCore(Task_GPS, "TaskGPS/Core1", 10000, NULL, 1, &TaskGPS, 1);
  xTaskCreatePinnedToCore(Task_PZEM, "TaskPZEM/Core1", 10000, NULL, 1, &TaskPZEM, 1);
  xTaskCreatePinnedToCore(Task_Socket, "TaskSocket/Core1", 10000, NULL, 1, &TaskSocket, 1);

  //--------------------------------------------------------------------------------------------------------------------------------
  vTaskResume(TaskDisplay);  /* TaskGPS ทำงาน */
  vTaskResume(TaskKeypad);  /* TaskGPS ทำงาน */
  vTaskResume(TaskGPS);  /* TaskGPS ทำงาน */
  vTaskSuspend(TaskPZEM);  /* TaskPZEM หยุดงาน */
  vTaskSuspend(TaskSocket); /* TaskSocketหยุดงาน */

  //--------------------------------------------------------------------------------------------------------------------------------
  pinMode(LED_Pin1, INPUT);
  pinMode(LED_Pin2, INPUT);
  pinMode(LED_Pin3, INPUT);

  //--------------------------------------------------------------------------------------------------------------------------------
  pinMode(ELCB_Pin, OUTPUT);
  digitalWrite(ELCB_Pin, HIGH);

}

//================================================================================================================================
void Task_Display(void* pvParameters)
{
  for (;;)
  {
    //--------------------------------------------------------------------------------------------------------------------------------
    // แสดงผล ตรวจไม่พบโมดูล GPS
    if (
    Buffer_IndexMenu[0] == "Detected Fail" and 
    Buffer_IndexMenu[1] == "" and 
    Buffer_IndexMenu[2] == "" and 
    Buffer_IndexMenu[3] == "" and 
    Buffer_IndexMenu[4] == "" and 
    Buffer_IndexMenu[5] == "" and 
    Buffer_IndexMenu[6] == "")
    {
      lcd.setCursor(0, 0);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("    Status GPS:     ");
      lcd.setCursor(0, 1);
      // --------------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print(String("   ") + Buffer_IndexMenu[0]);
      lcd.setCursor(0, 2);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("   Please Turn OFF  ");
      lcd.setCursor(0, 3);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("And Checking Module!");
    }

    //--------------------------------------------------------------------------------------------------------------------------------
    // แสดงผล กำลังพยายามเชื่อมต่อดาวเทียม
    else if (
    Buffer_IndexMenu[0] == "Satellite Checking" and 
    Buffer_IndexMenu[1] == "" and 
    Buffer_IndexMenu[2] == "" and 
    Buffer_IndexMenu[3] == "" and 
    Buffer_IndexMenu[4] == "" and 
    Buffer_IndexMenu[5] == "" and 
    Buffer_IndexMenu[6] == "")
    {
      lcd.setCursor(0, 0);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("    Status: GPS     ");
      lcd.setCursor(0, 1);
      // --------------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print(String(" ") + Buffer_IndexMenu[0]);
      lcd.setCursor(0, 2);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("    Please wait.    ");
      lcd.setCursor(0, 3);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("                    ");
    }

    //--------------------------------------------------------------------------------------------------------------------------------
    // แสดงผล ตำแหน่งที่ได้รับจากดาวเทียม
    else if (
    Buffer_IndexMenu[0] == "Load Location" and 
    Buffer_IndexMenu[1] == "" and 
    Buffer_IndexMenu[2] == "" and 
    Buffer_IndexMenu[3] == "" and 
    Buffer_IndexMenu[4] == "" and
    Buffer_IndexMenu[5] == "" and 
    Buffer_IndexMenu[6] == "")
    {
    lcd.setCursor(0, 0);
    // -------"XXXXXXXXXXXXXXXXXXXX"
    lcd.print(String("   ") + Buffer_IndexMenu[0]);
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
    else if (
    Count_IndexID == 0 and 
    Buffer_IndexMenu[0] == "Save Location" and 
    Buffer_IndexMenu[1] == "" and 
    Buffer_IndexMenu[2] == "" and 
    Buffer_IndexMenu[3] == "" and 
    Buffer_IndexMenu[4] == "" and 
    Buffer_IndexMenu[5] == "" and 
    Buffer_IndexMenu[6] == "")
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
    else if (
    Count_IndexID > 0 and Count_IndexID < 7 and 
    Buffer_IndexMenu[0] == "Save Location" and 
    Buffer_IndexMenu[1] == "" and 
    Buffer_IndexMenu[2] == "" and 
    Buffer_IndexMenu[3] == "" and 
    Buffer_IndexMenu[4] == "" and 
    Buffer_IndexMenu[5] == "" and 
    Buffer_IndexMenu[6] == "")
    {
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
    else if (
    Count_IndexID == 7 and 
    Buffer_IndexMenu[0] == "Save Location" and 
    Buffer_IndexMenu[1] == "" and 
    Buffer_IndexMenu[2] == "" and 
    Buffer_IndexMenu[3] == "" and 
    Buffer_IndexMenu[4] == "" and 
    Buffer_IndexMenu[5] == "" and 
    Buffer_IndexMenu[6] == "")
    {
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
    // หน้าสถานะเครื่อง Bumblebee
    else if (
    Buffer_IndexMenu[0] == "Save Location" and 
    Buffer_IndexMenu[1] == "Save ID" and 
    Buffer_IndexMenu[2] == "" and 
    Buffer_IndexMenu[3] == "" and 
    Buffer_IndexMenu[4] == "" and 
    Buffer_IndexMenu[5] == "" and 
    Buffer_IndexMenu[6] == "")
    {
      lcd.setCursor(0, 0);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("   Bumblebee Test   ");
      lcd.setCursor(0, 1);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("VAC:" + String(Buffer_Voltage) + "V. Fq:" + String(Buffer_Frequency) + "Hz.");
      lcd.setCursor(0, 2);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("Status:" + Buffer_StatusBumblebee + "         ");
      lcd.setCursor(0, 3);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("'B'Back      'A'Save");
    }

    //--------------------------------------------------------------------------------------------------------------------------------
    // หน้า Socket Test
    else if (
    Buffer_IndexMenu[0] == "Save Location" and 
    Buffer_IndexMenu[1] == "Save ID" and 
    Buffer_IndexMenu[2] == "Bumblebee Test" and 
    Buffer_IndexMenu[3] == "" and 
    Buffer_IndexMenu[4] == "" and 
    Buffer_IndexMenu[5] == "" and 
    Buffer_IndexMenu[6] == "")
    {
      lcd.setCursor(0, 0);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("     Socket Test    ");
      lcd.setCursor(0, 1);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("Status:" + Buffer_SocketTest);
      lcd.setCursor(0, 2);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("                    ");
      lcd.setCursor(0, 3);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("'B'Back      'A'Save");
    }

    //--------------------------------------------------------------------------------------------------------------------------------
    // หน้า ELCB Test
    else if (
    Buffer_IndexMenu[0] == "Save Location" and 
    Buffer_IndexMenu[1] == "Save ID" and 
    Buffer_IndexMenu[2] == "Bumblebee Test" and 
    Buffer_IndexMenu[3] == "Socket Test" and 
    Buffer_IndexMenu[4] == "" and 
    Buffer_IndexMenu[5] == "" and 
    Buffer_IndexMenu[6] == "")
    {
      lcd.setCursor(0, 0);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("ELCB Test  :Warning:");
      lcd.setCursor(0, 1);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print(" After this, cannot ");
      lcd.setCursor(0, 2);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("  be edited again.  ");
      lcd.setCursor(0, 3);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("'B'Back      'A'Save");
    }

    //--------------------------------------------------------------------------------------------------------------------------------
    // หน้า Google Sheet
    else if (
    Buffer_IndexMenu[0] == "Save Location" and 
    Buffer_IndexMenu[1] == "Save ID" and 
    Buffer_IndexMenu[2] == "Bumblebee Test" and 
    Buffer_IndexMenu[3] == "Socket Test" and 
    Buffer_IndexMenu[4] == "ELCB Test" and 
    Buffer_IndexMenu[5] == "" and
    Buffer_IndexMenu[6] == "")
    {
      lcd.setCursor(0, 0);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      // -------"     ID:"XXXXXXX    "
      lcd.print("     ID:" + Save_ID + "    ");
      lcd.setCursor(0, 1);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("ELCB:" + Save_ELCB);
      lcd.setCursor(0, 2);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("'*' Info    '#'Reset");
      lcd.setCursor(0, 3);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print(" 'A'Upload to Cloud ");
    }

    //--------------------------------------------------------------------------------------------------------------------------------
    // หน้า Info
    else if (
    Buffer_IndexMenu[0] == "Save Location" and 
    Buffer_IndexMenu[1] == "Save ID" and 
    Buffer_IndexMenu[2] == "Bumblebee Test" and 
    Buffer_IndexMenu[3] == "Socket Test" and 
    Buffer_IndexMenu[4] == "ELCB Test" and 
    Buffer_IndexMenu[5] == "Information" and
    Buffer_IndexMenu[6] == "")
    {
      lcd.setCursor(0, 0);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("Lat:" + Save_GPS_Lat);
      lcd.setCursor(0, 1);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("Lng:" + Save_GPS_Lng);
      lcd.setCursor(0, 2);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("Bb:" + Save_StatusBumblebee);
      lcd.setCursor(0, 3);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("Sock:" + Save_SocketTest);
    }

    //--------------------------------------------------------------------------------------------------------------------------------
    // หน้า จบการทำงาน
    else if (
    Buffer_IndexMenu[0] == "Save Location" and 
    Buffer_IndexMenu[1] == "Save ID" and 
    Buffer_IndexMenu[2] == "Bumblebee Test" and 
    Buffer_IndexMenu[3] == "Socket Test" and 
    Buffer_IndexMenu[4] == "ELCB Test" and 
    Buffer_IndexMenu[5] == "Pre Upload" and
    Buffer_IndexMenu[6] == "")
    {
      lcd.setCursor(0, 0);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("    Successfully    ");
      lcd.setCursor(0, 1);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print(" Uploaded to cloud  ");
      lcd.setCursor(0, 2);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("with Google Sheets. ");
      lcd.setCursor(0, 3);
      // -------"XXXXXXXXXXXXXXXXXXXX"
      lcd.print("    '#' New Test    ");
    }

    //--------------------------------------------------------------------------------------------------------------------------------
    delay(10);  // หน่วงเวลา

   }
}

//================================================================================================================================
void Task_Keypad(void* pvParameters) {

  for (;;)
  {

    //--------------------------------------------------------------------------------------------------------------------------------
    char Custom_Key = keypad.getKey();  // สร้างตัวแปรชื่อ Custom_Key ชนิด char เพื่อเก็บตัวอักขระที่กด

    //--------------------------------------------------------------------------------------------------------------------------------
    // ถ้าหากตัวแปร Custom_Key มีอักขระ
    if (Custom_Key)
    {  
      Serial.println(String("Key: ") + Custom_Key);

      //--------------------------------------------------------------------------------------------------------------------------------
      // vTaskSuspend(TaskDisplay);
      
      //--------------------------------------------------------------------------------------------------------------------------------
      // หน้า Load Location 
      // เหตุการ: กด Save Location
      if (
      Custom_Key == 'A' and 
      Buffer_IndexMenu[0] == "Load Location" and 
      Buffer_IndexMenu[1] == "" and 
      Buffer_IndexMenu[2] == "" and 
      Buffer_IndexMenu[3] == "" and 
      Buffer_IndexMenu[4] == "" and 
      Buffer_IndexMenu[5] == "" and
      Buffer_IndexMenu[6] == "")
      {
        
        //--------------------------------------------------------------------------------------------------------------------------------
        vTaskSuspend(TaskGPS);  // หยุด TaskGPS

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
        Save_GPS_Lat = Buffer_GPS_Lat;
        Save_GPS_Lng = Buffer_GPS_Lng;

        Serial.println(String("Save_GPS_Lat: ") + Save_GPS_Lat);
        Serial.println(String("Save_GPS_Lng: ") + Save_GPS_Lng);

        // vTaskSuspend(TaskGPS);
        // vTaskResume(TaskGPS);
  
      }

      //--------------------------------------------------------------------------------------------------------------------------------
      // หน้า: ID 
      // เหตุการ: กดกลับหน้า Load Location
      else if ( 
      Custom_Key == 'B' and 
      Buffer_IndexMenu[0] == "Save Location" and
      Buffer_IndexMenu[1] == "" and 
      Buffer_IndexMenu[2] == "" and 
      Buffer_IndexMenu[3] == "" and 
      Buffer_IndexMenu[4] == "" and
      Buffer_IndexMenu[5] == "" and
      Buffer_IndexMenu[6] == "")
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
        Save_GPS_Lat = "";
        Save_GPS_Lng = "";

        Serial.println(String("Save_GPS_Lat: ") + Save_GPS_Lat);
        Serial.println(String("Save_GPS_Lng: ") + Save_GPS_Lng);

        //--------------------------------------------------------------------------------------------------------------------------------
        for (int i = Count_IndexID;  i > 0; i--)
        {
          Buffer_IndexID[i] = NULL;
          Count_IndexID--;
        }

        //--------------------------------------------------------------------------------------------------------------------------------
        Buffer_ID = "";
        Serial.println(String("Buffer_ID: ") + Buffer_ID);

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
        vTaskResume(TaskGPS);  // TaskGPS กลับมาทำงาน
      }

      //--------------------------------------------------------------------------------------------------------------------------------
      // หน้า: ID 
      // เหตุการ: กดหมายเลข ID
      else if ((
      Custom_Key == '0' or Custom_Key == '1' or 
      Custom_Key == '2' or Custom_Key == '3' or
      Custom_Key == '4' or Custom_Key == '5' or 
      Custom_Key == '6' or Custom_Key == '7' or
      Custom_Key == '8' or Custom_Key == '9') and
      Count_IndexID < sizeof(Buffer_IndexID) and 
      Buffer_IndexMenu[0] == "Save Location" and 
      Buffer_IndexMenu[1] == "" and 
      Buffer_IndexMenu[2] == "" and 
      Buffer_IndexMenu[3] == "" and 
      Buffer_IndexMenu[4] == "" and 
      Buffer_IndexMenu[5] == "" and
      Buffer_IndexMenu[6] == "")
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
      // หน้า: ID 
      // เหตุการ: กดลบ ID
      else if (
      Count_IndexID != 0 and 
      Custom_Key == 'D' and 
      Buffer_IndexMenu[0] == "Save Location" and 
      Buffer_IndexMenu[1] == "" and 
      Buffer_IndexMenu[2] == "" and 
      Buffer_IndexMenu[3] == "" and 
      Buffer_IndexMenu[4] == "" and 
      Buffer_IndexMenu[5] == "" and
      Buffer_IndexMenu[6] == "")
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
      }

      //--------------------------------------------------------------------------------------------------------------------------------
      // หน้า: ID
      // เหตุการ: กดเคลียร์ ID
      else if (
      Count_IndexID != 0 and 
      Custom_Key == 'C' and 
      Buffer_IndexMenu[0] == "Save Location" and 
      Buffer_IndexMenu[1] == "" and 
      Buffer_IndexMenu[2] == "" and 
      Buffer_IndexMenu[3] == "" and 
      Buffer_IndexMenu[4] == "" and 
      Buffer_IndexMenu[5] == "" and
      Buffer_IndexMenu[6] == "")
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
      }

      //--------------------------------------------------------------------------------------------------------------------------------
      // หน้า: ID ไปหน้า
      // เหตุการ: บันทึก ID ไปหน้า Bumblebee Test
      else if (
      Count_IndexID == 7 and 
      Custom_Key == 'A' and 
      Buffer_IndexMenu[0] == "Save Location" and 
      Buffer_IndexMenu[1] == "" and 
      Buffer_IndexMenu[2] == "" and 
      Buffer_IndexMenu[3] == "" and 
      Buffer_IndexMenu[4] == "" and 
      Buffer_IndexMenu[5] == "" and
      Buffer_IndexMenu[6] == "")
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
        vTaskResume(TaskPZEM);  // TaskPZEM ทำงาน
      }

      //--------------------------------------------------------------------------------------------------------------------------------
      // หน้า: Bumblebee Test
      // เหตุการ์: กลับหน้า ID
      else if (
      Custom_Key == 'B' and  
      Buffer_IndexMenu[0] == "Save Location" and 
      Buffer_IndexMenu[1] == "Save ID" and 
      Buffer_IndexMenu[2] == "" and 
      Buffer_IndexMenu[3] == "" and 
      Buffer_IndexMenu[4] == "" and 
      Buffer_IndexMenu[5] == "" and
      Buffer_IndexMenu[6] == "")
      {
        //--------------------------------------------------------------------------------------------------------------------------------
        Count_IndexMenu--;
        Buffer_IndexMenu[Count_IndexMenu] = "";
        
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
        vTaskSuspend(TaskPZEM);  // TaskPZEM หยุด
      }
      
      //--------------------------------------------------------------------------------------------------------------------------------
      // หน้า Bumblebee
      // เหตุการณ์: กดบันทึก Bumblebee Test ไปหน้า Socket Test
      else if (
      Custom_Key == 'A' and 
      Buffer_IndexMenu[0] == "Save Location" and 
      Buffer_IndexMenu[1] == "Save ID" and
      Buffer_IndexMenu[2] == "" and 
      Buffer_IndexMenu[3] == "" and 
      Buffer_IndexMenu[4] == "" and 
      Buffer_IndexMenu[5] == "" and
      Buffer_IndexMenu[6] == "")
      {
        Serial.println("--------------------------------------------");
        Save_StatusBumblebee = Buffer_StatusBumblebee;
        Serial.println(String("Save_StatusBumblebee: ") + Save_StatusBumblebee);

        //--------------------------------------------------------------------------------------------------------------------------------
        Buffer_IndexMenu[Count_IndexMenu] = "Bumblebee Test";  /* บันทึกหน้าตำแหน่ง */
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
        vTaskSuspend(TaskPZEM);  // TaskPZEM หยุด
        vTaskResume(TaskSocket);  // TaskSocket ทำงาน
      }

      //--------------------------------------------------------------------------------------------------------------------------------
      // หน้า Socket Test
      // เหตุการณ์: กลับไปหน้า Bumblebee Test 
      else if (
      Count_IndexID == 7 and 
      Custom_Key == 'B' and
      Buffer_IndexMenu[0] == "Save Location" and 
      Buffer_IndexMenu[1] == "Save ID" and 
      Buffer_IndexMenu[2] == "Bumblebee Test" and 
      Buffer_IndexMenu[3] == "" and 
      Buffer_IndexMenu[4] == "" and 
      Buffer_IndexMenu[5] == "" and
      Buffer_IndexMenu[6] == "")
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
        vTaskResume(TaskPZEM);  // TaskPZEM ทำงาน
        vTaskSuspend(TaskSocket);  // TaskSocket หยุดทำงาน
      }

      //--------------------------------------------------------------------------------------------------------------------------------
      // หน้า Socket Test
      // เหตุการณ์: กดบันทึก Socket Test ไปหน้า ELCB Test
      else if (
      Custom_Key == 'A' and
      Buffer_IndexMenu[0] == "Save Location" and 
      Buffer_IndexMenu[1] == "Save ID" and
      Buffer_IndexMenu[2] == "Bumblebee Test" and 
      Buffer_IndexMenu[3] == "" and 
      Buffer_IndexMenu[4] == "" and 
      Buffer_IndexMenu[5] == "" and
      Buffer_IndexMenu[6] == "")
      {

        //--------------------------------------------------------------------------------------------------------------------------------
        // บันทึกค่า Save_SocketTest
        Serial.println("--------------------------------------------");
        Save_SocketTest = Buffer_SocketTest;
        Serial.println(String("Save_SocketTest: ") + Save_SocketTest);

        //--------------------------------------------------------------------------------------------------------------------------------
        // ขบันทึกตำแหน่งหน้า Socket Test
        Buffer_IndexMenu[Count_IndexMenu] = "Socket Test";  /* บันทึกหน้าตำแหน่ง */
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
        vTaskSuspend(TaskSocket);  // TaskSocket หยุดทำงาน
      }

      //--------------------------------------------------------------------------------------------------------------------------------
      // หน้า ELCB Test
      // เหตุการณ์: กลับไปหน้า Socket Test
      else if (  
      Count_IndexID == 7 and 
      Custom_Key == 'B' and 
      Buffer_IndexMenu[0] == "Save Location" and 
      Buffer_IndexMenu[1] == "Save ID" and 
      Buffer_IndexMenu[2] == "Bumblebee Test" and 
      Buffer_IndexMenu[3] == "Socket Test" and 
      Buffer_IndexMenu[4] == "" and 
      Buffer_IndexMenu[5] == "" and
      Buffer_IndexMenu[6] == "")
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
      }

      //--------------------------------------------------------------------------------------------------------------------------------
      // หน้า ELCB Test
      // บันทึก ELCB Test ไปหน้า Pre Upload
      else if ( 
      Custom_Key == 'A' and 
      Buffer_IndexMenu[0] == "Save Location" and 
      Buffer_IndexMenu[1] == "Save ID" and
      Buffer_IndexMenu[2] == "Bumblebee Test" and 
      Buffer_IndexMenu[3] == "Socket Test" and 
      Buffer_IndexMenu[4] == "" and 
      Buffer_IndexMenu[5] == "" and
      Buffer_IndexMenu[6] == "")
      {
        //--------------------------------------------------------------------------------------------------------------------------------
        vTaskSuspend(TaskDisplay);
        lcd.clear();

        //--------------------------------------------------------------------------------------------------------------------------------
        lcd.setCursor(0, 0);
        // -------"XXXXXXXXXXXXXXXXXXXX"
        lcd.print("      ELCB Test     ");
        lcd.setCursor(0, 1);
        // -------"XXXXXXXXXXXXXXXXXXXX"
        lcd.print("   Time Countdown   ");
        lcd.setCursor(0, 2);
        // -------"XXXXXXXXXXXXXXXXXXXX"
        lcd.print("          3         ");
        lcd.setCursor(0, 3);
        // -------"XXXXXXXXXXXXXXXXXXXX"
        lcd.print("    Please wait.    ");

        //--------------------------------------------------------------------------------------------------------------------------------
        delay(1000);

        //--------------------------------------------------------------------------------------------------------------------------------
        lcd.setCursor(0, 0);
        // -------"XXXXXXXXXXXXXXXXXXXX"
        lcd.print("      ELCB Test     ");
        lcd.setCursor(0, 1);
        // -------"XXXXXXXXXXXXXXXXXXXX"
        lcd.print("   Time Countdown   ");
        lcd.setCursor(0, 2);
        // -------"XXXXXXXXXXXXXXXXXXXX"
        lcd.print("          2         ");
        lcd.setCursor(0, 3);
        // -------"XXXXXXXXXXXXXXXXXXXX"
        lcd.print("    Please wait.    ");

        //--------------------------------------------------------------------------------------------------------------------------------
        delay(1000);

        //--------------------------------------------------------------------------------------------------------------------------------
        lcd.setCursor(0, 0);
        // -------"XXXXXXXXXXXXXXXXXXXX"
        lcd.print("      ELCB Test     ");
        lcd.setCursor(0, 1);
        // -------"XXXXXXXXXXXXXXXXXXXX"
        lcd.print("   Time Countdown   ");
        lcd.setCursor(0, 2);
        // -------"XXXXXXXXXXXXXXXXXXXX"
        lcd.print("          1         ");
        lcd.setCursor(0, 3);
        // -------"XXXXXXXXXXXXXXXXXXXX"
        lcd.print("    Please wait.    ");

        //--------------------------------------------------------------------------------------------------------------------------------
        delay(1000);

        //--------------------------------------------------------------------------------------------------------------------------------
        lcd.setCursor(0, 0);
        // -------"XXXXXXXXXXXXXXXXXXXX"
        lcd.print("      ELCB Test     ");
        lcd.setCursor(0, 1);
        // -------"XXXXXXXXXXXXXXXXXXXX"
        lcd.print("       Working      ");
        lcd.setCursor(0, 2);
        // -------"XXXXXXXXXXXXXXXXXXXX"
        lcd.print("                   ");
        lcd.setCursor(0, 3);
        // -------"XXXXXXXXXXXXXXXXXXXX"
        lcd.print("    Please wait.    ");

        //--------------------------------------------------------------------------------------------------------------------------------
        digitalWrite(ELCB_Pin, LOW);
        delay(2000);
        digitalWrite(ELCB_Pin, HIGH);

        delay(500);
        lcd.clear();
        //--------------------------------------------------------------------------------------------------------------------------------
        vTaskResume(TaskSocket);  // TaskSocket หยุดทำงาน
        //--------------------------------------------------------------------------------------------------------------------------------
        vTaskResume(TaskDisplay);
        

        //--------------------------------------------------------------------------------------------------------------------------------
        Serial.println("--------------------------------------------");
        // Save_SocketTest = Buffer_SocketTest;
        // Serial.println(String("Save_SocketTest: ") + Save_SocketTest);

        //--------------------------------------------------------------------------------------------------------------------------------
        Buffer_IndexMenu[Count_IndexMenu] = "ELCB Test";  /* บันทึกหน้าตำแหน่ง */
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

      }

      //--------------------------------------------------------------------------------------------------------------------------------
      // หน้า Pre Upload
      // เหตุการ กดไปหน้า Information
      else if ( 
      Custom_Key == '*' and 
      Buffer_IndexMenu[0] == "Save Location" and 
      Buffer_IndexMenu[1] == "Save ID" and
      Buffer_IndexMenu[2] == "Bumblebee Test" and 
      Buffer_IndexMenu[3] == "Socket Test" and 
      Buffer_IndexMenu[4] == "ELCB Test" and 
      Buffer_IndexMenu[5] == "" and
      Buffer_IndexMenu[6] == "")
      {
        Serial.println("--------------------------------------------");
        // Save_SocketTest = Buffer_SocketTest;
        // Serial.println(String("Save_SocketTest: ") + Save_SocketTest);

        //--------------------------------------------------------------------------------------------------------------------------------
        Buffer_IndexMenu[Count_IndexMenu] = "Information";  /* บันทึกหน้าตำแหน่ง */
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
        lcd.clear();
      }

      //--------------------------------------------------------------------------------------------------------------------------------
      // หน้า Information
      // เหตุการ กดกลับหน้า Pre Upload
      else if (  
      Count_IndexID == 7 and 
      Custom_Key == '*' and 
      Buffer_IndexMenu[0] == "Save Location" and 
      Buffer_IndexMenu[1] == "Save ID" and 
      Buffer_IndexMenu[2] == "Bumblebee Test" and 
      Buffer_IndexMenu[3] == "Socket Test" and 
      Buffer_IndexMenu[4] == "ELCB Test" and 
      Buffer_IndexMenu[5] == "Information" and
      Buffer_IndexMenu[6] == "")
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
      }

      //--------------------------------------------------------------------------------------------------------------------------------
      // หน้า: Pre Upload
      // เหตุการ์: รีเซตค่า
      else if (
      Custom_Key == '#' and 
      Buffer_IndexMenu[0] == "Save Location" and 
      Buffer_IndexMenu[1] == "Save ID" and 
      Buffer_IndexMenu[2] == "Bumblebee Test" and 
      Buffer_IndexMenu[3] == "Socket Test" and 
      Buffer_IndexMenu[4] == "ELCB Test" and 
      Buffer_IndexMenu[5] == "" and
      Buffer_IndexMenu[6] == "")
      {
        
        //--------------------------------------------------------------------------------------------------------------------------------
        vTaskSuspend(TaskDisplay);

        //--------------------------------------------------------------------------------------------------------------------------------
        Serial.println(String("Count_IndexID: ") + Count_IndexID);
        for (int i = Count_IndexID;  i > 0; i--)
        {
          Buffer_IndexID[i] = NULL;
          Count_IndexID--;
          delay(1);
        }

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
        //--------------------------------------------------------------------------------------------------------------------------------
        for (int i = Count_IndexMenu;  i > 0; i--)
        {
          Buffer_IndexMenu[i] = "";
          Count_IndexMenu--;
          delay(1);
        }

        //--------------------------------------------------------------------------------------------------------------------------------
        Serial.println("--------------------------------------------");
        Serial.println(String("Count_IndexMenu: ") + Count_IndexMenu);
        Serial.print("Buffer_IndexID: ");
        for (int i = 0;  i < Count_IndexMenu; i++)
        {
          Serial.print(String("|") + Buffer_IndexMenu[i]);
        }
        Serial.println("|");

        //--------------------------------------------------------------------------------------------------------------------------------
        //--------------------------------------------------------------------------------------------------------------------------------
        // เคลียร์ค่า
        Buffer_ID = "";
        Save_ID = "";  

        Buffer_GPS_Lat = "";
        Buffer_GPS_Lng = "";
        Save_GPS_Lat = "";
        Save_GPS_Lng = "";

        Buffer_StatusBumblebee = "Fail";
        Buffer_Voltage = 0;
        Buffer_Frequency = 0;
        Save_StatusBumblebee = "Fail";

        Buffer_SocketTest = "";
        Save_SocketTest = "";

        Save_ELCB = "";

        //--------------------------------------------------------------------------------------------------------------------------------
        delay(500);
        lcd.clear();

        //--------------------------------------------------------------------------------------------------------------------------------
        vTaskResume(TaskDisplay);  /* TaskGPS ทำงาน */
        vTaskResume(TaskKeypad);  /* TaskGPS ทำงาน */
        vTaskResume(TaskGPS);  /* TaskGPS ทำงาน */
        vTaskSuspend(TaskPZEM);  /* TaskPZEM หยุดงาน */
        vTaskSuspend(TaskSocket); /* TaskSocketหยุดงาน */

      }

      //--------------------------------------------------------------------------------------------------------------------------------
      // หน้า Pre Upload
      // เหตุการณ์ กด Pre Upload ไปหน้า Successfully
      else if ( 
      Custom_Key == 'A' and 
      Buffer_IndexMenu[0] == "Save Location" and 
      Buffer_IndexMenu[1] == "Save ID" and
      Buffer_IndexMenu[2] == "Bumblebee Test" and 
      Buffer_IndexMenu[3] == "Socket Test" and 
      Buffer_IndexMenu[4] == "ELCB Test" and 
      Buffer_IndexMenu[5] == "" and
      Buffer_IndexMenu[6] == "")
      {
        // //--------------------------------------------------------------------------------------------------------------------------------      
        // vTaskSuspend(TaskDisplay);  /* TaskGPS ทำงาน */
        // vTaskSuspend(TaskKeypad);  /* TaskGPS ทำงาน */
        // vTaskSuspend(TaskGPS);  /* TaskGPS ทำงาน */
        // vTaskSuspend(TaskPZEM);  /* TaskPZEM หยุดงาน */
        // vTaskSuspend(TaskSocket); /* TaskSocketหยุดงาน */

        

        //--------------------------------------------------------------------------------------------------------------------------------   


        //--------------------------------------------------------------------------------------------------------------------------------
        Buffer_IndexMenu[Count_IndexMenu] = "Pre Upload";  /* บันทึกหน้าตำแหน่ง */
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
     
      }

      //--------------------------------------------------------------------------------------------------------------------------------
      // หน้า: Successfully
      // เหตุการ์: เริ่มใหม่
      else if (
      Custom_Key == '#' and 
      Buffer_IndexMenu[0] == "Save Location" and 
      Buffer_IndexMenu[1] == "Save ID" and 
      Buffer_IndexMenu[2] == "Bumblebee Test" and 
      Buffer_IndexMenu[3] == "Socket Test" and 
      Buffer_IndexMenu[4] == "ELCB Test" and 
      Buffer_IndexMenu[5] == "Pre Upload" and
      Buffer_IndexMenu[6] == "")
      {
        
        //--------------------------------------------------------------------------------------------------------------------------------
        vTaskSuspend(TaskDisplay);

        //--------------------------------------------------------------------------------------------------------------------------------
        Serial.println(String("Count_IndexID: ") + Count_IndexID);
        for (int i = Count_IndexID;  i > 0; i--)
        {
          Buffer_IndexID[i] = NULL;
          Count_IndexID--;
          delay(1);
        }

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
        //--------------------------------------------------------------------------------------------------------------------------------
        for (int i = Count_IndexMenu;  i > 0; i--)
        {
          Buffer_IndexMenu[i] = "";
          Count_IndexMenu--;
          delay(1);
        }

        //--------------------------------------------------------------------------------------------------------------------------------
        Serial.println("--------------------------------------------");
        Serial.println(String("Count_IndexMenu: ") + Count_IndexMenu);
        Serial.print("Buffer_IndexID: ");
        for (int i = 0;  i < Count_IndexMenu; i++)
        {
          Serial.print(String("|") + Buffer_IndexMenu[i]);
        }
        Serial.println("|");

        //--------------------------------------------------------------------------------------------------------------------------------
        // เคลียร์ค่า
        Buffer_ID = "";
        Save_ID = "";  

        Buffer_GPS_Lat = "";
        Buffer_GPS_Lng = "";
        Save_GPS_Lat = "";
        Save_GPS_Lng = "";

        Buffer_StatusBumblebee = "Fail";
        Buffer_Voltage = 0;
        Buffer_Frequency = 0;
        Save_StatusBumblebee = "Fail";

        Buffer_SocketTest = "";
        Save_SocketTest = "";

        Save_ELCB = "";

        //--------------------------------------------------------------------------------------------------------------------------------
        delay(500);
        lcd.clear();

        //--------------------------------------------------------------------------------------------------------------------------------
        vTaskResume(TaskDisplay);  /* TaskGPS ทำงาน */
        vTaskResume(TaskKeypad);  /* TaskGPS ทำงาน */
        vTaskResume(TaskGPS);  /* TaskGPS ทำงาน */
        vTaskSuspend(TaskPZEM);  /* TaskPZEM หยุดงาน */
        vTaskSuspend(TaskSocket); /* TaskSocketหยุดงาน */

      }

    //--------------------------------------------------------------------------------------------------------------------------------
    delay(100);  // หน่วงเวลา

    }
  }
}

//================================================================================================================================
void Task_GPS(void* pvParameters) {

  for (;;)
  {
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
        Buffer_IndexMenu[0] = "Load Location";
        Buffer_GPS_Lat = String(GPS.location.lat(), 6);
        Buffer_GPS_Lng = String(GPS.location.lng(), 6);

        // Serial.print(Status_GPS + " : ");
        // Serial.print(Buffer_GPS_Lat);
        // Serial.print(F(","));
        // Serial.println(Buffer_GPS_Lng);

      }

      //--------------------------------------------------------------------------------------------------------------------------------
      else 
      {
        Buffer_IndexMenu[0] = "Satellite Checking";
        Serial.println(Buffer_IndexMenu[0]);

      }
    }
    
    //--------------------------------------------------------------------------------------------------------------------------------
    if (millis() > 5000 && GPS.charsProcessed() < 10)
    {
      Buffer_IndexMenu[0] = "Detected Fail";
      Serial.println(Buffer_IndexMenu[0]);
      while(true);
      
    }

    delay(100);  // หน่วงเวลา

  }
}

//================================================================================================================================
void Task_PZEM(void* pvParameters)
{
  for (;;) {
    //--------------------------------------------------------------------------------------------------------------------------------
    // แสดงผล Address เพื่อตวจสอบการเชื่อมต่อ
    // Serial.print("Custom Address:");
    // Serial.println(pzem.readAddress(), HEX);

    //--------------------------------------------------------------------------------------------------------------------------------
    float voltage = pzem.voltage();
    float frequency = pzem.frequency();

    //--------------------------------------------------------------------------------------------------------------------------------
    if (isnan(voltage) or isnan(frequency))
    {
        // Serial.println("Error reading voltage or frequency");
        Buffer_Voltage = 0;
        Buffer_Frequency = 0;
        Buffer_StatusBumblebee = "Fail";
    } 
    else 
    {
      Buffer_StatusBumblebee = "Pass";
      Buffer_Voltage = voltage;
      Buffer_Frequency = frequency;

      // Serial.print("Voltage: ");      Serial.print(voltage);      Serial.println("V");
      // Serial.print("Frequency: ");    Serial.print(frequency, 1); Serial.println("Hz");
      // Serial.println();

    }
   
    delay(100);
    
  }
}

//================================================================================================================================
void Task_Socket(void* pvParameters) {

  for (;;)
  {
    bool LED_Pin1Value = not(digitalRead(LED_Pin1));
    bool LED_Pin2Value = not(digitalRead(LED_Pin2));
    bool LED_Pin3Value = not(digitalRead(LED_Pin3));

    if (Buffer_IndexMenu[0] == "Save Location" and 
    Buffer_IndexMenu[1] == "Save ID" and 
    Buffer_IndexMenu[2] == "Bumblebee Test" and 
    Buffer_IndexMenu[3] == "" and 
    Buffer_IndexMenu[4] == "" and 
    Buffer_IndexMenu[5] == "" and 
    Buffer_IndexMenu[6] == "") 
    {
      if (LED_Pin1Value == 1 and LED_Pin2Value == 1 and LED_Pin3Value == 0)
      {
        Buffer_SocketTest = "Correct";
      }

      else if  (LED_Pin1Value == 1 and LED_Pin2Value == 0 and LED_Pin3Value == 0)
      {
        Buffer_SocketTest = "Open Ground";
      }

      else if  (LED_Pin1Value == 0 and LED_Pin2Value == 1 and LED_Pin3Value == 0)
      {
        Buffer_SocketTest = "Open Neutral";
      }

      else if  (LED_Pin1Value == 0 and LED_Pin2Value == 0 and LED_Pin3Value == 0)
      {
        Buffer_SocketTest = "Open Live";
      }

      else if  (LED_Pin1Value == 0 and LED_Pin2Value == 1 and LED_Pin3Value == 1)
      {
        Buffer_SocketTest = "Live/GRD Reverse";
      }

      else if  (LED_Pin1Value == 1 and LED_Pin2Value == 0 and LED_Pin3Value == 1)
      {
        Buffer_SocketTest = "Live/NEU Reverse";
      }

      else if  (LED_Pin1Value == 1 and LED_Pin2Value == 1 and LED_Pin3Value == 1)
      {
        Buffer_SocketTest = "Live/GRD Reverse,Missing GRD";
      }

      // Serial.println("Buffer_SocketTest: " + Buffer_SocketTest);
      // Serial.println("1:" + String(LED_Pin1Value) + " 2:" + String(LED_Pin2Value) + " 3:" + String(LED_Pin3Value));

    }
    else if(
    Buffer_IndexMenu[0] == "Save Location" and 
    Buffer_IndexMenu[1] == "Save ID" and 
    Buffer_IndexMenu[2] == "Bumblebee Test" and 
    Buffer_IndexMenu[3] == "Socket Test" and 
    Buffer_IndexMenu[4] == "ELCB Test" and 
    Buffer_IndexMenu[5] == "" and
    Buffer_IndexMenu[6] == ""
    )
    {
      if (LED_Pin1Value == 1 or LED_Pin2Value == 1 or LED_Pin3Value == 1)
      {
        Save_ELCB = "ELCB Fail";
      }
      else if (LED_Pin1Value == 1 and LED_Pin2Value == 1 and LED_Pin3Value == 1)
      {
        Save_ELCB = "ELCB Fail";
      }
      else
      {
        Save_ELCB = "ELCB Pass";
      }

      // Serial.println("Save_ELCB: " + Save_ELCB);
    }

    delay(100);
  }
}

//================================================================================================================================
void loop() {

}