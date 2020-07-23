# efishery_motorcontroller


A.	Deskripsi Solusi

Pada dokumen ini, akan menjelaskan hasil dari skill test eFishery untuk posisi Electrical Embedded Engineer. Berdasarkan blok diagram yang diberikan, maka intepretasi yang diberikan adalah sebegai berikut:
 
 ![alt text](https://github.com/apawitan1995/efishery_motorcontroller/blob/master/blok_diagram_arif.jpg?raw=true)
 
Gambar A.1 Blok Diagram solusi.

Setiap push button dapat dideteksi oleh Atmega328p secara terpisah dan telah menggunakan rangkaian debouncer agar error dari output push-button dapat diminimalisir. Dengan menggunakan push button, pengguna dapat melihat akses dari LCD untuk mengatur setpoint kecepatan motor dalam RPM (round per minute ).  Kecepatan motor dapat diatur dengan PWM dari Timer1 dengan frekuensi switching 25kHz. PWM tersebut akan menjalankan rangkaian pada motor driver IR2110 dengan kendali kecepatan tertutup menggunakan encoder. Perlu diperhatikan bahwa motor hanya bias dijalankan dalam 1 arah. Untuk upload program pada Atmega328p, digunakan koneksi SPI seperti microcontroller pada umumnya. 
ESP-Wroom-02 digunakan untuk menghubungkan alat dengan internet. Selain itu, ESP juga digunakan untuk melakukan penyimpanan data kecepatan pada SD Card. Selain itu, data kecepatan juga akan dikirimkan ke cloud ThingSpeak dengan menggunakan API code yang telah didapatkan. Atmega328p terhubung dengan ESP-Wroom-02 menggunakan jalur komunikasi UART Serial. Karena Jalurnya sama dengan UART Serial untuk melakukan flash, maka digunakan Dip-switch untuk mengatur jalur yang akan digunakan. Pengaturan Dip-switch untuk operasi nominal dan untuk upload program adalah sebagai berikut :
 
  ![alt text](https://github.com/apawitan1995/efishery_motorcontroller/blob/master/dipswitch_arif.jpg?raw=true)
 
Gambar A.2 Pengaturan Dip-switch untuk pemrograman ESP dan untuk operasi nominal.

B.	Deskripsi Rangkaian

PCB yang di desain memiliki dimensi 50mm*100mm dimana design rules yang digunakan mengikuti kemampuan dari vendor pembuatan PCB MultiKarya. Lebar jalur disesuaikan dengan daya akan akan melewati jalur tersebut. Penempatan komponen dilakukan dalam upaya mengurangi gangguan pada antenna ESP-Wroom-02. Sumber daya yang digunakan diasumsikan bertegangan 24V. Tegangan tersebut lalu dibagi menggunakan linear regulator agar memberikan catu daya 12V, 5V dan 3.3V.  Untuk memudahkan komunikasi ESP-Wroom-02 dengan ATmega328p, tegangan yang digunakan untuk menjalankan kedua microprocessor tersebut di set 3.3V sehingga tegangan tersebut perlu dijaga terutama pada saat upload program. Skematik dilampirkan dalam bentuk PDF dan GBR dalam bentuk ZIP. Penyematan gambar dari rangkaian yang telah didesain adalah sebagai berikut:

 
   ![alt text](https://github.com/apawitan1995/efishery_motorcontroller/blob/master/brd_capture_arif.JPG?raw=true)
 
Gambar B.1 Penyematan gambar dari rangkaian yang telah di desain.


C.	Deskripsi Program

Firmware untuk Atmega dan untuk ESP telah dilampirkan dalam file Arduino IDE (.ino). Dalam firmware Atmega digunakan 2 ISR untuk perhitungan kecepatan motor dan kendali kecepatan motor. Untuk proses yang tidak critical seperti pengaturan LCD dan komunikasi serial, fungsi dijalankan pada loop seperti pada umumnya dengan memanfaatkan millis agar waktu dioperasikan bias tetap teratur. FSM (finite state machine) telah digunakan untuk menjalankan sistem operasi dan user interface. Diagram state yang digunakan adalah sebagai berikut :
 
    ![alt text](https://github.com/apawitan1995/efishery_motorcontroller/blob/master/diagramfsm_arif.JPG?raw=true)
 
Gambar C.1 Diagram FSM dari firmware Atmega.

Adanya state machine tersebut memungkinan user dengan mudah mengatur sistem yang diinginkan secara aman, dimana pengaturan disini adalah penentuan setpoint kecepatan pada state MENU. Firmware ESP tidak mempergunakan FSM karena tidak dibutuhkan user interface seperti pada atmega. Fungsi untuk komunikasi serial, pengaturan SD card, dan upload data dimasukan kedalam loop dengan memanfaatkan millis untuk mengatur waktu operasinya.

D.	Kesimpulan

Telah dibuat skematik, PCB, dan firmware sesuai dengan blok diagram yang telah diberikan dengan ditambahkan detail seperti LCD dan pengatur catu daya.  Masing-masing pushbutton dapat dikenali oleh Atmega. Kecepatan motor dapat diatur dan dikendalikan dengan PWM dengan feedback encoder dari motor. Flashing Atmega menggunakan SPI dan ESP menggunakan serial. Agar komunikasi serial tidak bentrok, digunakan Dip-switch untuk mengatur jalur UART saat flashing dan operasi normal.

PCB memiliki ukuran 50mmx100mm dengan informasi lengkap pada tiap pin Input dan Output yang akan digunakan langsung oleh pengguna. Design rule PCB disesuaikan dengan kapabilitas vendor PCB Multikarya Bandung dengan pengaturan komponen dan jalur disesuaikan dengan aliran daya dan frekuensi sinyal yang melewatinya. Diperhatikan juga peletakan komponen agar noise pada ESP-Wroom-02 minimal. 

Firmware Atmega dan ESP-Wroom-02 dibuat pada Arduino IDE. Firmware atmega memungkinan mikrokontroller untuk mengatur kecepatan motor dengan feedback encoder. Setpoint kecepatan yang diatur dari pushbutton atmega akan dibagikan melalui serial kepada ESP-Wroom-02. Firmware ESP-Wroom-02 memungkinkan ESP agar bisa terhubung dengan wifi/hotspot dari HP pengguna, mengupload data setpoint ke cloud (thingspeak cloud), serta menyimpan data setpoint kecepatan pada SD card yang terhubung dengan SPI pada ESP.





