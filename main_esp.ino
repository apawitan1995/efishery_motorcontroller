// MAIN CODE ESP-WROOM-02
// Made by : Gusti Arif Hanifah Pawitan

// Menggunakan ESP8266 core : http://arduino.esp8266.com/stable/package_esp8266com_index.json
// Board : ThaiEasyElec’s ESPino
// Upload speed : 115200
// CPU Freq : 80MHz
// Flash size : 4MB (FS:2MB OTA:~ 1019KB
// Debug port : disabled
// debug level : None
// v2 Lower memory
// Flash
// Legacy
// Only Sketch
// All SSL ciphers

//Library
#include <ESP8266WiFi.h> //Library untuk connect ke WIFI
#include <SPI.h> // library komunikasi SPI agar bisa terhubung dengan SD card
#include <SD.h> // library penggunaan SD Card

// pin yang digunakan untuk LED indikasi wifi
int pin_led = 16;

//variabel untuk penggunaan SD card
const int chipSelect = 15; // CS pada GPIO15
File root;
int flag_sd = 0;
int sd_stat = 0;
char buf_in_sd[40];
int buf_sd;
int flag_update_sd = 0;

//variabel untuk koneksi wifi dan pengiriman data ke cloud
int flag_wifi = 0;
int wifi_stat = 0;
String apiKey = "FMV3YAYST4H2S4ZU";// API key dari ThingSpeak
const char *ssid =  "Arif"; // Ganti dengan nama wifi hotspot dari HP
const char *pass =  "12345678";
const char* server = "api.thingspeak.com";
WiFiClient client;

//variabel setpoint kecepatan motor
int setpoint = 0;

// variabel untuk komunikasi serial 
char buf_in[40],buf_out[40],buf[40]; // untuk buffer komunikasi masuk, komunikasi keluar, dan untuk merubah integer ke string
const char s[2] = ";"; // ini terima aja buat memotong2 data dari atmega 
char *token; /// ini terima aja buat memotong2 data dari atmega
int buf_num[10]; // data yang dah dipotong nanti ditaro di sini
int increment; // ini variabel buat increment
int flag_uart_send = 0; // variabel yang mengatur kapan harus mengirimkan data melalui UART

//variabel data dalam komunikasi serial yang akan di share dengan ESP
int   data_1 = 0; //data set point kecepatan
int   data_2 = 0; //data kondisi state machine atmega
int   data_3 = 0; //data status wifi
int   data_4 = 0; //data status SD card
int   data_5 = 0; //reserve (tidak dipakai)
int   data_6 = 0; //reserve (tidak dipakai)
int   data_7 = 0; //reserve (tidak dipakai)

//variabel data yang disimpan setelah data dikirimkan ke ESP melalui serial
int   data_1_old = 0;
int   data_2_old = 0;
int   data_3_old = 0;
int   data_4_old = 0;
int   data_5_old = 0;
int   data_6_old = 0;
int   data_7_old = 0;

// variabel untuk menyimpan data setpoint baru dari atmega
int   data_1_new = 0;

//variabel timer 
unsigned long  time_serial = 0;
unsigned long  time_sd = 0;
unsigned long  time_status = 0;
unsigned long  time_upload = 0;


void setup() 
{
  // inisiasi komunikasi serial
    Serial.begin(9600); // baudrate harus sama dengan atmega

  // set pin led wifi ke output
    pinMode(pin_led, OUTPUT);

  //inisiasi hubungan dengan wifi
    WiFi.begin(ssid, pass); // lakukan koneksi wifi
    delay(1); // tunggu 1 detik

  //cek status wifi untuk inisisasi pertama
    if (WiFi.status() != WL_CONNECTED) {
      wifi_stat = 0; // connecting failed
      flag_wifi = 1; // try again next loop
      digitalWrite(pin_led, LOW);
      }
    else{
      wifi_stat = 1; // initialization success
      flag_sd = 0; // stop trying
      digitalWrite(pin_led, HIGH);
      }
      
  //cek status SD card untuk inisisasi pertama
    if (!SD.begin(chipSelect)) { 
      sd_stat = 0; // initialization failed 
      }
     else {
      sd_handler(); // update data setpoint dari SD card apabila ada
      sd_stat = 1; // initialization sucesss
      }

    //kirimkan data serial pertama ke atmega
    serial_handler();

    //inisiasi nilai untuk timer
    time_serial = millis();
    time_sd = millis();
    time_upload = millis();
    time_status = millis();
    
}


void loop() 
{

  // fungsi untuk mengecek komunikasi serial dengan atmega, dilakukan setiap 100ms
  if ( (unsigned long)(millis() - time_serial) >= 100 ){
    serial_handler(); //fungsi untuk melakukan update print LCD
    time_serial = millis();
    }

  // fungsi untuk mengecek status SD card dan Wifi, dilakukan setiap 5 detik
  if ( (unsigned long)(millis() - time_status) >= 5000 ){
    
    if (!SD.begin(chipSelect)) { 
      sd_stat = 0; // initialization failed 
      }
     else {
      sd_stat = 1; // initialization sucesss
      }

    if (WiFi.status() != WL_CONNECTED) {
      wifi_stat = 0; // connecting failed
      flag_wifi = 1; // try again next loop
      digitalWrite(pin_led, LOW);
      }
    else{
      wifi_stat = 1; // initialization success
      flag_wifi = 0; // stop trying
      digitalWrite(pin_led, HIGH);
      }

    // apabila koneksi wifi tidak menyala, inisiasi kembali koneksi setiap 5 detik sampai kembali menyala
    if ( flag_wifi == 1 ){
      WiFi.begin(ssid, pass);
      }
      
    time_status = millis();  
    }
          
  // fungsi untuk mengupdate data ke SD card, dicek setiap 10 detik, hanya di update apabila ada perubahan data
  if ( (unsigned long)(millis() - time_sd) >= 10000 ){
    //cek apakah flag perubahan data menyala
    if ( flag_update_sd == 1 ){
      sd_handler();
      }
    time_sd = millis();
    }

  //fungsi untuk meng-upload data ke cloud, dilakukan setiap 60 detik
  if ( (unsigned long)(millis() - time_upload) >= 60000 ){  
     if (client.connect(server,80)){  //connect ke server thingspeak
                                
        String postStr = apiKey;
        postStr +="&field1=";
        postStr += String(setpoint);
        postStr += "\r\n\r\n";
    
        client.print("POST /update HTTP/1.1\n");
        client.print("Host: api.thingspeak.com\n");
        client.print("Connection: close\n");
        client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n");
        client.print("Content-Type: application/x-www-form-urlencoded\n");
        client.print("Content-Length: ");
        client.print(postStr.length());
        client.print("\n\n");
        client.print(postStr);
     
        }
    client.stop();
    time_upload = millis();    
    }
}


//fungsi untuk mengatur penyimpanan data di sd card, setiap dipanggil, file akan di baca, disimpan datanya,
// di delete, di update datanya apabila perlu, lalu disimpan kembali di file baru
void sd_handler(){
  root = SD.open("/");
  root.rewindDirectory();
  root.close();
  
  if (SD.exists("speed_data.txt")) {
      File speed_data = SD.open("speed_data.txt", FILE_READ);
  
        while (speed_data.available()) {
         buf_in[increment] = (speed_data.read());
         increment++;
        }
      speed_data.close();  
      increment=0;
      buf_sd = atof(buf_in_sd);
      
      SD.remove("speed_data.txt");
    }
   else {
    buf_sd = setpoint;
   }
    
  root = SD.open("/");
  root.rewindDirectory();
  root.close();

  if( flag_update_sd == 1 ){
    buf_sd = setpoint;
    flag_update_sd = 0;
  }

  File speed_data = SD.open("speed_data.txt", FILE_WRITE); 
  if (speed_data) {
      speed_data.print(String(buf_sd));
  }
  speed_data.close();
  
  speed_data = SD.open("speed_data.txt", FILE_READ);
  
  while (speed_data.available()) {
   buf_in[increment] = (speed_data.read());
   increment++;
  }
  increment=0;

  setpoint = atof(buf_in);
  
  // close the file:
  speed_data.close();
  
}


//fungsi untuk mengatur komunikasi serial yang masuk dan keluar dari atmega
void serial_handler(){

  data_1 = setpoint;
  data_2 = 0;
  data_3 = wifi_stat;
  data_4 = sd_stat;
  data_5 = 0; //reserve
  data_6 = 0; //reserve
  data_7 = 0; //reserve

  // cek apakah ada perubahan pada data yang ingin dikirimkan
  if((data_1 != data_1_old)||(data_2 != data_2_old)||(data_3 != data_3_old)||(data_4 != data_4_old)){
    flag_uart_send = 1;
  }

 
  if(flag_uart_send == 1){
    // untuk mengirim data serial
    strcpy(buf_out,"");
    sprintf(buf,"%d;",data_1);
    strcpy(buf_out, buf);
    sprintf(buf,"%d;",data_2);
    strcat(buf_out, buf);
    sprintf(buf,"%d;",data_3);  
    strcat(buf_out, buf);
    sprintf(buf,"%d;",data_4);
    strcat(buf_out, buf);
    sprintf(buf,"%d;",data_5);
    strcat(buf_out, buf);
    sprintf(buf,"%d;",data_6);
    strcat(buf_out, buf);
    sprintf(buf,"%d;\n",data_7);
    strcat(buf_out, buf);
    Serial.print(buf_out);
    flag_uart_send = 0;
    //simpan data yang terakhir dikirimkan melalui serial
    data_1_old = data_1;
    data_2_old = data_2;
    data_3_old = data_3;
    data_4_old = data_4;
    data_5_old = data_5;
    data_6_old = data_6;
    data_7_old = data_7;
  }
  
    // untuk ambil data serial
  increment=0;
  while (Serial.available()) {
    buf_in[increment] = Serial.read();
    increment++;
    }

     // untuk mencacah data string
  increment=0;
  token = strtok(buf_in,s);
  while(token){
    buf_num[increment]=atof(token);
    token = strtok(NULL,s);
    increment++;
    }

   // simpan data baru
   data_1_new = buf_num[0]; // setpoint motor
   data_2 = buf_num[1]; // state dari atmega saat ini

    data_1_old = data_1_new;
    data_2_old = data_2;

  // apabila setpoint berubah, flag untuk menyimpan data ke SD card akan menyala
  if(data_1 != data_1_new){
    flag_update_sd = 1;
  }

   
}
