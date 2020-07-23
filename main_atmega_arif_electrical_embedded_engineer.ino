// MAIN CODE ATMEGA328P
// Made by : Gusti Arif Hanifah Pawitan

// menggunakan MINICORE : https://mcudude.github.io/MiniCore/package_MCUdude_MiniCore_index.json
// Board : Atmega328
// Freq : Ext. 8MHz
// BOD : 2.7V
// LTO Disabled
// Variant : 328P
// Bootloader : Yes (UART0)

// Library LCD I2C : https://gitlab.com/tandembyte/LCD_I2C

//Library
#include <Wire.h> // Library untuk komunikasi I2C ke LCD
#include <LiquidCrystal_I2C.h> // Library untuk menggunakan LCD

//Variabel untuk LCD, default address adalah 0x27, digunakan LCD 16x2
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2);
char kal1[15]; // variabel yang menyimpan karakter pada baris pertama lcd
char kal2[15]; // variabel yang menyimpan karakter pada baris kedua lcd
char kal1_old[15]; // variabel yang menyimpan karakter pada baris pertama lcd ketika lcd telah diupdate
char kal2_old[15]; // variabel yang menyimpan karakter pada baris kedua lcd ketika lcd telah diupdate

// variabel untuk komunikasi serial 
char buf_in[40],buf_out[40],buf[40]; // untuk buffer komunikasi masuk, komunikasi keluar, dan untuk merubah integer ke string
const char s[2] = ";"; // ini terima aja buat memotong2 data dari ESP 
char *token; /// ini terima aja buat memotong2 data dari ESP
int buf_num[10]; // data yang dah dipotong nanti ditaro di sini
int increment; // ini variabel buat increment aja
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

//Lokasi pin untuk ke-4 tombol yang digunakan
int pin_sw1 = 16;
int pin_sw2 = 17;
int pin_sw3 = 14;
int pin_sw4 = 15;

//variabel untuk mengolah data tombol yang telah ditekan
int button_raw[4];
int button[4];
int flag_push[4];
int flag_button = 0;

//lokasi pin untuk pengendalian motor
int pin_pwm = 9; // pin output PWM ke IR2110
int pin_enc = 2; // bisa pakai pin 2 ataupun pin 3, untuk menghitung putaran encoder
int coefficient_rpm = 100; //koefisien untuk menghitung RPM motor, tergantung spesifikasi motor ( RPM*coefficient_rpm = tick/second )

//variabel untuk menghitung kecepatan motor ( switching freq : 25kHz, control freq : 100Hz )
unsigned long tick_shared = 0; // perhitungan pulse pada ISR
unsigned long tick_motor = 0; // salinan perhitungan pulse pada ISR
int tick_measure = 0; // total pulse dalam sampling time, di konversi agar menjadi tick/second
int tick_setpoint = 0 ;// setpoint kecepatan dalam tick/second
int setpoint = 0; // setpoint dalam RPM
int error = 0; // error antara kecepatan yang diukur dan setpoint
float Ki = 0.5; // konstanta integrator ( harus di tune )
float Kp = 5; // konstanta proportional ( harus di tune )
float integ_error = 0; // variabel yang menyimpan hasil integrasi error
int output = 0; // output kontrol kecepatan PI ( 0-160 )
int output_old = 0;
int min_RPM = 0; // setpoint kecepatan minimum dari motor 
int max_RPM = 500; // setpoint kecepatan maksimum dari motor ( sesuai spesifikasi motor )

//variabel untuk mengatur apakah motor menyala atau tidak
int flag_motor = 0;

// variabel untuk finite state machine
int state_fsm = 0; // dimulai dari state 0
int prev_state_fsm = 0; // state sebelum terjadi perubahan state

// variabel untuk menu 
int state_menu =0; //variabel yang menyimpan kondisi state pada fsm menu
int min_menu = 0; // karena pada menu hanya untuk merubah kecepatan motor saja, jadi hanya ada 1 halaman menu
int max_menu = 0;
int flag_menu = 0;
int temp_menu = 0;

//variabel untuk timing 
unsigned long  time_serial = 0;
unsigned long  time_motor = 0;
unsigned long  time_fsm = 0;
unsigned long  time_lcd = 0;
int flag_timer = 0 ;
int timer_start = 0 ;
int timeout = 0;



//fungsi untuk interrupt pada encoder motor
void tick_count()
{
  tick_shared=tick_shared+1;
}

//fungsi untuk menghitung kecepatan motor, harus dilakukan dengan frekuensi 100Hz
ISR(TIMER2_COMPA_vect){
  tick_measure = (tick_shared - tick_motor)*100; // variabel = tick/second
  tick_setpoint= setpoint*coefficient_rpm; // konversi RPM ke tick/second

  error = tick_setpoint - tick_measure;

  integ_error = integ_error + Ki*error*100; 
  // anti windup saturation function, agar nilai integrator tidak overflow
    if( integ_error > 5000 ) integ_error =5000;
    if( integ_error < -5000 ) integ_error = -5000;
  
  output = Kp*error+ integ_error; 
  //sesuaikan output agar tidak melebihi batas PWM ( 0 - 160 )
    if( output > 150 ) output = 150; // dutycycle maximum tidak boleh 1, maksimal 0.95
    if( output < 5 ) output = 0;

  //output hanya akan dilanjutkan apabila flag_motor = 1 ( state fsm 3 - Running )
  if ( flag_motor != 1 ){
    output = 0;
  }
  
  if (output != output_old){
    analogWrite(pin_pwm, output);
    output_old = output;
  }
  
  tick_motor = tick_shared;
}

void setup() {
  //Inisiasi penggunaan LCD
  lcd.init();
  lcd.backlight();

  //inisiasi pin dan interrupt untuk encoder
  pinMode(pin_enc, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pin_enc), tick_count, CHANGE); // interrupt akan menyala setiap ada perbuahan nilai pada pin encoder

  //inisiasi pin PWM
  pinMode(pin_pwm, OUTPUT);  // set pin pwm menjadi output

  cli(); // matikan dahulu semua interrupt
  //ganti frekuensi pin PWM ke 25kHz, maksimal PWM 0-160
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  TCCR1A = _BV(COM1A1)  // non-inverted PWM on ch. A
      | _BV(COM1B1)  // same on ch. B
      | _BV(WGM11);  // mode 10: ph. correct PWM, TOP = ICR1
  TCCR1B = _BV(WGM13)   // ditto
      | _BV(CS10);   // prescaler = 1
  ICR1   = 160;

  //set timer untuk interrupt perhitungan kecepatan motor
  //set timer2 interrupt at 100Hz
  TCCR2A = 0;// set entire TCCR2A register to 0
  TCCR2B = 0;// same for TCCR2B
  TCNT2  = 0;//initialize counter value to 0
  // set compare match register for 100hz increments
  OCR2A = 155;// = (8*10^6) / (100*1024) - 1 (must be <256)
  // turn on CTC mode
  TCCR2A |= (1 << WGM21);
  // Set CS22, CS21, and CS20 bit for 1024 prescaler
  TCCR2B |= (1 << CS22)|(1 << CS21)|(1 << CS20);   
  // enable timer compare interrupt
  TIMSK2 |= (1 << OCIE2A);
  sei(); // nyalakan kembali interrupt

  //Inisiasi 4 tombol yang akan digunakan
  pinMode(pin_sw1, INPUT);
  pinMode(pin_sw2, INPUT);
  pinMode(pin_sw3, INPUT);
  pinMode(pin_sw4, INPUT);
  

  //Inisiasi komunikasi UART ke ESP
  Serial.begin(9600); // set baud-rate 9600 (default)

  //lakukan tes serial pertama untuk synchronisasi data dengan SD card pada ESP
  inisiasi_data();


  //inisiasi nilai untuk timer
  time_serial = millis();
  time_motor = millis();
  time_fsm = millis();
  time_lcd = millis();

  //nilai awal tick motor
  tick_motor = tick_shared;

}


void loop() {
  
  //fungsi pada loop bisa di interrupt kapan saja
  if ( (unsigned long)(millis() - time_serial) >= 1 ){
    serial_handler(); //fungsi untuk melakukan update print LCD
    time_serial = millis();
    }

  if ( (unsigned long)(millis() - time_fsm) >= 20 ){
    FSM(); // fungsi yang memuat finite state machine dari sistem
    time_fsm = millis();
    }

  if ( (unsigned long)(millis() - time_lcd) >= 40 ){
    print_lcd(); //fungsi untuk melakukan update print LCD
    time_lcd = millis();
    }

}

// fungsi untuk melakukan inisiasi dan sinkronisasi data dengan ESP
void inisiasi_data(){

  // tuliskan status pada LCD
  strcat(kal1,"Preparation");
  lcd.setCursor(0,0);
  lcd.print("                ");
  lcd.setCursor(0,0);
  lcd.print(kal1);

  // tunggu adanya komunikasi serial dari ESP
  while (!Serial.available()) { // wait for serial data
    }

  // data berhasil didapatkan dari ESP, print status pada LCD
  strcat(kal1,"Synching");
  lcd.setCursor(0,0);
  lcd.print("                ");
  lcd.setCursor(0,0);
  lcd.print(kal1);
  
  // untuk ambil data serial
  increment=0;
  while (Serial.available()) {
    buf_in[increment] = Serial.read();
    increment++;
    }

  // untuk memecah data dari string
  increment=0;
  token = strtok(buf_in,s);
  while(token){
    buf_num[increment]=atof(token);
    token = strtok(NULL,s);
    increment++;
    }
  //masukan data yang di dapat ke variabel data
   data_1 = buf_num[0]; // setpoint motor
   data_2 = buf_num[1]; // state dari atmega saat ini
   data_3 = buf_num[2]; // kondisi wifi ( menyala atau tidak )
   data_4 = buf_num[3]; // kondisi SD card ( ada atau tidak )
   data_5 = buf_num[4]; // reserve
   data_6 = buf_num[5]; // reserve
   data_7 = buf_num[6]; // reserve

   data_1_old = data_1;
   data_2_old = data_2;

  // inisiasi setpoint kecepatan motor dan state machine
   setpoint = data_1;
   state_fsm = data_2;

  
}



//fungsi untuk mengatur komunikasi serial yang masuk dan keluar dari atmega
void serial_handler(){

  //update data terbaru kecepatan dan FSM
  data_1 = setpoint;
  data_2 = state_fsm; 

  //bila terjadi perubahan data dari terakhir dikirimkan, kirimkan data ke ESP untuk di update di SD card
  if((data_1 != data_1_old)||(data_2 != data_2_old)){
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
    // simpan data yang dikirimkan
    data_1_old = data_1;
    data_2_old = data_2;
  }
  
    // untuk ambil data serial
  increment=0;
  while (Serial.available()) {
    buf_in[increment] = Serial.read();
    increment++;
    }

     //untuk mencacah data dari string 
  increment=0;
  token = strtok(buf_in,s);
  while(token){
    buf_num[increment]=atof(token);
    token = strtok(NULL,s);
    increment++;
    }

   data_1 = buf_num[0]; // setpoint motor
   data_2 = buf_num[1]; // state dari atmega saat ini
   data_3 = buf_num[2]; // kondisi wifi ( menyala atau tidak )
   data_4 = buf_num[3]; // kondisi SD card ( ada atau tidak )
   data_5 = buf_num[4]; // reserve
   data_6 = buf_num[5]; // reserve
   data_7 = buf_num[6]; // reserve

   
}


// fungsi untuk menghitung timer di fsm, input yang masuk berupa total waktu yang akan dilewati sampai timeout = 1
void timer_state(int count_num){
  int count;
  if (flag_timer==0){
    timer_start = millis();
    flag_timer=1;
  }
    count = millis() - timer_start;
    if (count >= count_num ){
      timeout=1;

    }
  }

// fungsi yang memuat finite state machine dari sistem
void FSM(){

  button_read(); // cek tombol yang sedang ditekan dan dilepas ( bisa menekan banyak tombol sekaligus )

  // state standby, motor tidak menyala
  // tekan tombol ke-3 untuk pindah ke state menu
  // tekan tombol ke-1 dan ke-2 untuk masuk me state transisi
  if(state_fsm == 0){

    strcat(kal2," Standby");
    
    flag_motor = 0; // pastikan flag = 0 agar motor tidak menyala
    
    if ( button[0] == '0' && button[1] == '0'){
      flag_button=0;
      }
    if ( button[2] == '1'){
      state_fsm = 1;
      flag_push[2]='1';
      }
    if ( button[0] == '1' && button[1] == '1' && flag_button == 0){
       prev_state_fsm = 1;
       state_fsm = 2;
      }
    }

  // state menu, untuk mengganti setpoint kecepatan motor
  // penjelasan akan diberikan pada fungsi menu
  // pengguna dapat keluar dari state menu dengan menekan tombol ke-4 atau menunggu 30 detik sampai timeout
  // penekanan tombol ke-1, ke-2, dan ke-3 akan mereset timer 
  else if ( state_fsm == 1){
    menu();
    timer_state(30000);
    if ( button[0] == '1' || button[1] == '1' || button[2] == '1'){
      flag_timer = 0;
      }
    if ( button[3] == '1' || timeout == 1){
       state_fsm = 1;
       flag_timer =0;
       flag_menu = 0;
       timeout =0;
       sprintf(kal2,"");
      }
    }

    //state transisi, tahan tombol ke-1 dan ke-2 selama 2 detik agar bisa berpindah dari state 0 ke state 3 ataupun sebaliknya
    //apabila tombol dilepas sebelum timeout ( kurang dari 2 detik ), state akan kembali pada state sebelum masuk state transisi
  else if ( state_fsm == 2){
    sprintf(kal1,"Hold 2 Sec");
    sprintf(kal2,"");
    timer_state(2000);
    if ( button[0] == '0' || button[1] == '0'){
      state_fsm = prev_state_fsm;
      flag_timer = 0;
      sprintf(kal2,"");
      }
    if (timeout == 1){
      if ( prev_state_fsm == 0){
        state_fsm = 3;
        }
       else if ( prev_state_fsm == 3){
        state_fsm=0;
        }
       flag_timer = 0;
       timeout=0;
       flag_button=1;
       sprintf(kal2,"");
       }
    }
  // state standby, motor menyala
  // tekan tombol ke-3 untuk pindah ke state menu
  // tekan tombol ke-1 dan ke-2 untuk masuk me state transisi   
 else if(state_fsm == 3){
    
    flag_motor = 1; // nyalakan flag, output akan mengikuti setpoint kecepatan yang diberikan
    
    strcat(kal2," Running");
    if ( button[0] == '0' && button[1] == '0'){
      flag_button=0;
      }
    if ( button[2] == '1'){
      state_fsm = 1;
      flag_push[2]='1';
      }
    if ( button[0] == '1' && button[1] == '1' && flag_button == 0){
       prev_state_fsm = 3;
       state_fsm = 2;
      }
    }
}

// fungsi untuk menu merubah kecepatan motor
void menu(){

  // dapat menampilkan dan merubah berbagai nilai, untuk saat ini hanya digunakan untuk merubah kecepatan motor
  if (state_menu == 0){
    temp_menu = setpoint;
    }

  // untuk merubah nilai setpoint, terlebih dahulu tekan tombol ke-3
  if (button[2] == '1'&& flag_push[2]=='0'){
    kal1[0]= 0b00000111;

    //ketika masih menekan tombol ke 3, tekan tombol ke-1 untuk menurunkan setpoint atau tombol ke-2 untuk menaikkanset point
      if ((button[0] == '1' )&&(flag_push[0] == '0')){
        temp_menu=temp_menu-10;
        flag_push[0]='1';
      }
      if (( button[1] == '1')&&(flag_push[1] == '0')){
        temp_menu=temp_menu+10;
        flag_push[1]='1';
      }

      // masukan setpoint kecepatan terbaru
    if (state_menu == 0){
      if (temp_menu <= min_RPM){
        temp_menu = min_RPM;
        }
      else if (temp_menu > max_RPM){
        temp_menu = max_RPM;
        }
      setpoint=temp_menu;
      sprintf(kal2,"    %4.0d RPM",temp_menu);
      }

  }
  // apabila tombol yang ditekan hanya tombol ke-1 atau ke-2 tanpa menahan tombol ke-3
  else if (button[2] == '0'){
    //tombol ke-1 atau ke-2 digunakan untuk masuk ke variabel yang ingin diganti, namun karena hanya ada variabel kecepatan
    //maka tampilan variabel hanya ada 1
    if ((button[0] == '1' )&&(flag_push[0] == '0')){
      state_menu=state_menu-1;
      flag_push[0]='1';
      if (state_menu <= min_menu){
        state_menu = min_menu;
        }
    }
    if (( button[1] == '1')&&(flag_push[1] == '0')){
      state_menu=state_menu+1;
      flag_push[1]='1';
      if (state_menu > max_menu){
        state_menu = max_menu;
        }
    }
  // apabila tidak ada tombol yang ditekan, lcd akan menampilkan settingan setpoint terakhir
    if (state_menu == 0){
      temp_menu = setpoint;
      sprintf(kal1,"    SET  SPEED");
      sprintf(kal2,"    %4.0d RPM  ",temp_menu);
      }

  }
}



//Fungsi ini digunakan agar LCD hanya di update ketika harus ada yang dirubah pada layarnya
void print_lcd(void){

  int flag_refresh_1=0;
  int flag_refresh_2=0;
  int i;
  
  for(i=0;i<15;i++){
    if (kal1_old[i] != kal1[i]){
      flag_refresh_1 =1;
    }
    if (kal2_old[i] != kal2[i]){
      flag_refresh_2 =1;
    }
    if (flag_refresh_1 == 1 || flag_refresh_2 == 1){
      i=15;
    }
  }
  
  if ( flag_refresh_1 == 1){
    lcd.setCursor(0,0);
    lcd.print("                ");
    lcd.setCursor(0,0);
    lcd.print(kal1);
    for(i=0;i<15;i++){
      kal1_old[i] = kal1[i];
      }
  }
  
  if ( flag_refresh_2 == 1){
    lcd.setCursor(0,1);
    lcd.print("                ");
    lcd.setCursor(0,1);
    lcd.print(kal2);
    for(i=0;i<15;i++){
      kal2_old[i] = kal2[i];
      }
  }
}

//fungsi untuk mengolah informasi tombol yang telah ditekan oleh pengguna
void button_read()
{
  
  button_raw[0] = digitalRead(pin_sw1);
  button_raw[1] = digitalRead(pin_sw2);
  button_raw[2] = digitalRead(pin_sw3);
  button_raw[3] = digitalRead(pin_sw4);
  
  if((button_raw[0] == LOW) && (button_raw[1] == LOW) && (button_raw[2] == LOW) && (button_raw[3] == LOW) ){
    button[0] = '0';
    button[1] = '0';
    button[2] = '0';
    button[3] = '0';
    flag_push[0] = '0';
    flag_push[1] = '0';
    flag_push[2] = '0';
    flag_push[3] = '0';
  }
  
  if( button_raw[0] == HIGH ) {button[0] = '1';}
  else { button[0] = '0';flag_push[0] = '0';}
  
  if( button_raw[1] == HIGH ) {button[1] = '1';}
  else { button[1] = '0';flag_push[1] = '0';}
  
  if( button_raw[2] == HIGH ) {button[2] = '1';}// else {BTN = 0;}
  else { button[2] = '0';flag_push[2] = '0';}
  
  if( button_raw[3] == HIGH ) {button[3] = '1';}// else {BTN = 0;}
  else { button[3] = '0';flag_push[3] = '0';}
}
