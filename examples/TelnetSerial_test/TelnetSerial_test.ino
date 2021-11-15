
#include <ESP8266WiFi.h>

#ifndef APSSID
#define APSSID "HONEYPOT"
#define APPSK  "thereisnospoon"
#endif


/********************************************************************************************
 *  TELNET Tty ALLOW LOGGING TO Tty AND/OR TELNET CONNECTION
 ********************************************************************************************/

#include <TelnetSerial.h>

TelnetSerial Tty(115200);

unsigned long start_time;

void setup() {

  //
  // Soft AP
  //
  Tty.println("\n\n\n\n");
  Tty.printf("Configuring access point SSID=%s password %s\n", APSSID, APPSK);
  WiFi.softAP(APSSID, APPSK); // Remove the password parameter if you want the AP to be open. 
  IPAddress myIP = WiFi.softAPIP();
  Tty.print("AP started. IP address: ");
  Tty.println(myIP);
  //
  //
  //
  
  start_time = millis();
}

/********************************************************************************************
 ********************************************************************************************
 ** MAIN LOOP
 ********************************************************************************************
 ********************************************************************************************/

const int TEST_TYPE = 2;

void loop(void)
{
  switch (TEST_TYPE) {
  
    case 1:
        if (Tty.available())
          Tty.print(Tty.read());
        break;

    case 2:
      if (Tty.cmd_available())
        process_command();
      break;
  }
  
  if ( (millis()-start_time) > 10000 ) {
    Tty.printf("Time=%lu\n", millis());
    start_time = millis();
  }
}

void process_command() {
  Tty.println("---COMMAND---");
  int num_par = Tty.cmd_num_params();
  Tty.printf("Command: %s -- %d parameters\n", Tty.cmd_verb(), Tty.cmd_num_params());
  for (int i=0; i<=Tty.cmd_num_params(); i++) {
    Tty.printf("Parameter %d: \"%s\" [%d/%f]\n", i, Tty.cmd_param_str(i), Tty.cmd_param_int(i), Tty.cmd_param_double(i));
  }
  Tty.cmd_init(); // Clear for a new command
}
