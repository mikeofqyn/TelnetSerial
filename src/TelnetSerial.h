#pragma once
#ifndef TELNETSERIAL_H
#define TELNETSERIAL_H

/* A Serial+Telnet Class.
 * Extends Print, writes to Serial and/or Telnet connection they are available
 * Reads from whichever has input, Serial first. Use with care, simultaneous
 * inputs will get intermixed.
 * 
 * See a more versatile telnet implemebtation at
 *		https://github.com/jandrassy/TelnetStream
 * See also
 *		https://arduino.stackexchange.com/questions/62783/create-new-serial-class-inheriting-from-stream
 */
 
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>

// Telnet
#define TELNET_SERIAL_DEFAULT_PORT 23 
// Serial
#define TELNETSERIAL_DEFAULT_SPEED 9600
// Command buffer
#define COMMAND_BUFFER_SIZE      256   // Max command + parameters length
#define COMMAND_MAX_PARAMS        20   // Max number of parameters per command
#define COMMAND_MIN_TIMEOUT     3000   // 3 seconds


static const char error_command_toolong[] = "Command too long. Discarded.";
static const char error_too_many_params[] = "Too many params. Ignored.";
static const char error_invalid_command[] = "Invalid command.";
static const char error_params_number[]   = "Not enough or too many parameters.";

class TelnetServer_ : public WiFiServer {
public:
	uint16_t port;
	TelnetServer_(uint16_t wport);
	TelnetServer_();
};

typedef enum {
	TS_NONE_IN,
	TS_SERIAL_IN,
	TS_TELNET_IN
} TelnetServer_input_t;

class TelnetSerial : public Print {
protected:
	TelnetServer_         server;
	WiFiClient            client;
	TelnetServer_input_t  which_in;    // Which input has been tested 
	bool                  begun;       // Begin() not needed
	bool                  whitespace;  // last char read is whitespace

	
	int readSerialWithEcho();
	int readClientWithEcho();

	//
	// Command line handler
	//
private:
	char                  cmdbuf[COMMAND_BUFFER_SIZE];   // command buffer
	unsigned int          scanpos;                       // Scanning position in cmd buffer
	uint8_t				  nparams;                       // Number of command parameters
	int                   cmdpos;                        // Pos of cmd strinh; WARNING -1 -> not found yet
	uint8_t               parampos[COMMAND_MAX_PARAMS];  // Positions of parameters
	bool                  eolfound;                      // Found end-of-line?
	unsigned long         command_timeout;               // Command input timeout in ms. 0-no timeout
	unsigned long         command_half_timeout;          // Half of timeout period. warn before cancelling
	unsigned long         command_last_key_ms;           // Time of last keystroke, to check for timeouts
	// Options
	bool                  autocr_set;                    // Auto CR on LF set? (default ON)
	bool                  autocr_sent;                   // If auto CR sent on LF skip adjacent CR
	bool                  inputecho_serial_set;          // Echo keystrokes on serial input? (default true) 
	bool                  inputecho_telnet_set;          // Echo keystrokes on telnet input? (default false) 
	bool                  crossecho_set;                 // Echo input from one source to the other? (default true


public:
	TelnetSerial();                       // Create and begin with default speed
	TelnetSerial(long speed);             // Create and begin serial with given speed
	bool check_conn();                    // Test for incomming telnet connections (call frequently)
	bool check();                         // Is any console connected? (calls check_conn())
	operator bool();                      // Allows use of 'if(TelnetSerialObject) ...' idiom
	uint16_t portnum();                   // Return numbre of port to use
	bool begin(long speed = 9600);        // for compatibility
	void autocr(bool onoff);              // enable (true) or disable (false) auto CR con LF 
	void inputecho(bool onoff);           // enable (true) or disable (false) keystroke echo on al inputs
	void inputecho(bool eser, bool etel); // enable (true) or disable (false) keystroke echo on serial and telnet
	void crossecho(bool onoff);           // enable (true) or disable (false) keystroke echo on the other source (telnet<->Serial)
	void telnetsync(bool onoff);           // Enable/disable sync (behave more like a (slow) serial connection
	bool is_synced();                     // Is synchroniced?

	//
	// Print utility class implementation
	//
	virtual size_t write(uint8_t x);
	virtual size_t write(char* x);
	virtual size_t write(uint8_t* x, size_t sz);
	virtual void flush();
	//
	// Stream utility class implementation (see Stream base class)
	// 
	//
	virtual int read();
	virtual int available();
	virtual int peek();
	//
	// Command listener
	//
	void   cmd_set_timeout(unsigned long ms);          // Set input timeout                      
	void   cmd_init(const char *prompt = NULL);              // Initialize for a new command line
	void   send_error_message(const char* message);    // Send an error message (just println it).
	bool   cmd_in_progress();                          // User is currently typing a command

	//
	bool   cmd_available();                 // Is a complete command line read and parsed in buffer?
	int    cmd_num_params();                // Get number of parameters of command line
	char*  cmd_verb();                      // Pointer to command verb
	bool   cmd_equal(const char *expect, int minpar = -1, int maxpar = -1); 
	                                            // Is a command present, matches given one and has enough parameters?
	                                            // min defaults to don't care, max defaults to min
	char*  cmd_param_str(uint8_t npar);         // Get pointer to nth parameter string [starting with 1]
	int    cmd_param_int(uint8_t npar);         // reinterpret n-th parameter as integer
	double cmd_param_double(uint8_t npar);      // reinterpret nth parameter as double
	
};


#endif