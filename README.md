
# TelnetSerial: A Serial + Telnet console class.
Extends `Print` class allowing `Serial`-like I/O to the serial port and/or a Telnet connection. The `TelnetSerial` object reads from any channel that has data available (`Serial` first). The writes are sent to all the connected channels at any given time.

 Additional primitives provide command line parsing. 
* See a more versatile telnet implementation at
      https://github.com/jandrassy/TelnetStream

* See also
       https://arduino.stackexchange.com/questions/62783/create-new-serial-class-inheriting-from-stream

## Usage

* Create a `TelnetSerial` object and use it as you wolud use a `Serial` object:
```arduino
#include <TelnetSerial.h>
int incomingByte = 0; // for incoming serial data
TelnetSerial Tty;

void setup() {

    // opens serial port, sets data rate to 9600 bps
    Tty.begin(9600);
    // Soft AP
    WiFi.softAP("MY_APSSID", "mypassword"); // Remove password parameter if you want the AP to be open.
    IPAddress myIP = WiFi.softAPIP();
    Tty.print("AP started. IP address: ");
    Tty.println(myIP);
}
void loop() {
    // send data only when you receive data:
    if (Tty.available() > 0) {
    // read the incoming byte:
    incomingByte = Tty.read();
    // say what you got:
    Tty.print("I received: ");
    Tty.println(incomingByte, DEC);
}
```

* Add frequent calls to `check_conn()` to ensure that incoming Telnet connections are accepted. Other methods also call `check_conn()`: `check()` (test whether any console is attached) and `available()` (check if there is input data available for reading, see example above).
 
* The class provides some handy methods to handle command line inputs:
``` arduino
      if (Tty.cmd_available())
         int num_par = Tty.cmd_num_params(); 
         Tty.printf("Command: %s  with  %d parameters\n", Tty.cmd_verb(), Tty.cmd_num_params());  
         for  (int i=0; i<=Tty.cmd_num_params(); i++)  { 
             Tty.printf("Parameter %d: \"%s\" [%d/%f]\n", 
                i, Tty.cmd_param_str(i), Tty.cmd_param_int(i), Tty.cmd_param_double(i));  
         }
         Tty.cmd_init();  // Clear for a new command
      }
  ```
## API
`TelnetSerial`derives from the  [Stream](https://www.arduino.cc/reference/en/language/functions/communication/stream/) base class, so it implements all its functions ([available()](https://www.arduino.cc/reference/en/language/functions/communication/stream/streamavailable) ,  [read()](https://www.arduino.cc/reference/en/language/functions/communication/stream/streamread), [flush()](https://www.arduino.cc/reference/en/language/functions/communication/stream/streamflush), [find()](https://www.arduino.cc/reference/en/language/functions/communication/stream/streamfind), [findUntil()](https://www.arduino.cc/reference/en/language/functions/communication/stream/streamfinduntil), [peek()](https://www.arduino.cc/reference/en/language/functions/communication/stream/streampeek), [readBytes()](https://www.arduino.cc/reference/en/language/functions/communication/stream/streamreadbytes),  [readBytesUntil()](https://www.arduino.cc/reference/en/language/functions/communication/stream/streamreadbytesuntil),  [readString()](https://www.arduino.cc/reference/en/language/functions/communication/stream/streamreadstring), [readStringUntil()](https://www.arduino.cc/reference/en/language/functions/communication/stream/streamreadstringuntil), [parseInt()](https://www.arduino.cc/reference/en/language/functions/communication/stream/streamparseint),  [parseFloat()](https://www.arduino.cc/reference/en/language/functions/communication/stream/streamparsefloat), [setTimeout()](https://www.arduino.cc/reference/en/language/functions/communication/stream/streamsettimeout)) as well as those of the [Print](https://playground.arduino.cc/Code/Printclass/) base class ([print()](https://www.arduino.cc/reference/en/language/functions/communication/serial/print/), [println()](https://www.arduino.cc/reference/en/language/functions/communication/serial/println/), [write()](https://www.arduino.cc/reference/en/language/functions/communication/serial/write/)).

### Constructors
**`TelnetSerial()`**
Create and begin with default speed

**`TelnetSerial(long speed)`**
Create and begin serial with given speed

### Configuration
**`bool check_conn()`**
Test for incomming telnet connections (call frequently)

**`bool check()`**
Is any console connected? (`calls check_conn()`)

**`operator bool()`**
Allows use of the  `if(TelnetSerialObject) ...` idiom

**`uint16_t portnum()`**
Return numbre of port to use

**`bool begin(long speed = 9600)`**
For compatibility witj `Serial`object

**`void autocr(bool onoff)`**
 Enable (*true*) or disable (*false*) auto CR on LF 
 
**`void inputecho(bool on_off)`**
Enable (*true*) or disable (*false*) keystroke echo on al inputs

**`void inputecho(bool onoff_serial, bool onoff_telnet)`**
Enable (*true*) or disable (*false*) keystroke echo on serial and telnet

**`void crossecho(bool on_off)`**
Enable (*true*) or disable (*false*) keystroke echo on the other source (Telnet <-> Serial)

**`void telnetsync(bool onoff)`**
Enable/disable sync (behave more like a (slow) character oriented serial connection

**`bool is_synced()`**
Is sync enabled?

### Command line processing

`void cmd_set_timeout(unsigned long ms)`          
 Set input timeout (time before the command line being entered is ignored
 
`void cmd_init(const char *prompt = NULL)`        
 Initialize for a new command line
 
`void send_error_message(const char* message)`    
 Send an error message (just `println` it)
 
`bool cmd_in_progress()`                          
 Is user is currently typing a command?

`bool cmd_available()`                 
 Is a complete command line read and parsed in buffer?
 
`int cmd_num_params()`                
 Get number of parameters in command line
 
`char* cmd_verb()`                      
 Pointer to command verb in commns line buffer
 
`bool cmd_equal(const char *expect, int minpar = -1, int maxpar = -1)` 
 Is the given command present in the input buffer and has enough parameters?                                         
 (`min` defaults to don't care, `max` defaults to `min`)
 
`char*  cmd_param_str(uint8_t n)`         
 Get pointer to `n`-th parameter string (starting with 1)
 
`int    cmd_param_int(uint8_t n)`         
 reinterpret `n`-th parameter as integer
 
`double cmd_param_double(uint8_t n)`      
 reinterpret `n`-th parameter as double
	