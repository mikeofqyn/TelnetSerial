#include "TelnetSerial.h"

/////////////////////////////////////////////////////////////////////////////////////////
// TelnetServer aux class
/////////////////////////////////////////////////////////////////////////////////////////


/** Constructor with specific port
*
*/
TelnetServer_::TelnetServer_(uint16_t wport)
	: WiFiServer(wport)
{
	port = wport;
};


/** Defaul constructor
*
*/
TelnetServer_::TelnetServer_()
	: TelnetServer_(TELNET_SERIAL_DEFAULT_PORT)
{
	//
};

/////////////////////////////////////////////////////////////////////////////////////////
// TelnetSerial -- General
/////////////////////////////////////////////////////////////////////////////////////////


/** Returns true if a telnet connection is active. Checks for incoming connections
*
*/
bool TelnetSerial::check_conn() { // Check for active or new TELNET connection. True if conection exists
	if (client && client.connected())
		return true;
	if (client)
		client.stop();
	client = server.available();
	return (client && client.connected());
}


/** Returns telnet server port number
*
*/
uint16_t TelnetSerial::portnum()
{
	return server.port;
}


/** Returns true if at least one source is active. Additionally checks for incoming telnet connections
*
*/
bool TelnetSerial::check() {
	bool b = false, c = false;
	if (Serial)
		b = true;
	c = check_conn();
	return b || c;
}


/** Allow 'if (object) ...' idiom to check initialization
*
*/
TelnetSerial::operator bool() {
	return this->check();
}


/** Begin console
*
*/
bool TelnetSerial::begin(long speed) {
	if (!begun) {
		server.begin();
		Serial.begin(speed);
		for (int itimes = 0; (itimes < 20) && (!Serial); itimes++);
			delay(100);
		this->printf("Server listening on port %d\n", this->portnum());
		this->printf("Serial port %s started (speed=%ld)\n\n", Serial ? "" : "NOT", speed);
	}
	begun = true;
	return this->check();
}


/** Constructor with Serial speed
*
*/
TelnetSerial::TelnetSerial(long speed) {
	//
	begun = false;
	which_in = TS_NONE_IN;
	//
	autocr_set = true;
	autocr_sent = false;
	inputecho_serial_set = true;
	inputecho_telnet_set = false;
	crossecho_set = true;
	//
	command_timeout = 0;
	//
	this->begin(speed);
	//
	this->cmd_init();
	//
	// Get sync and nodelay default values for all clients
	//
	WiFiClient c;
	c.setDefaultNoDelay(true);
}


/** Default constructor
*
*/
TelnetSerial::TelnetSerial() : TelnetSerial::TelnetSerial(TELNETSERIAL_DEFAULT_SPEED) {
}


/** Enable/disable auto CR on LF
*
*/
void TelnetSerial::autocr(bool set) {
	autocr_set = set;
}


/** Enable/disable keystroke echo for each source
*
*/
void TelnetSerial::inputecho(bool set_serial, bool set_telnet) {
	inputecho_serial_set = set_serial;
	inputecho_telnet_set = set_telnet;
}


/** Enable/disable keystroke echo in both sources
*
*/
void TelnetSerial::inputecho(bool option) {
	inputecho(option, option);
}


/** Enable/disable echoing input data to between sources
*
*/
void TelnetSerial::crossecho(bool onoff) {
	crossecho_set = onoff;
}


/** Enable/disable sync (behave more like a (slow) serial connection
*
*/
void TelnetSerial::telnetsync(bool onoff) {
	client.setDefaultSync(onoff);
	client.setSync(onoff);
}



/** Enable/disable sync (behave more like a (slow) serial connection
*
*/
bool TelnetSerial::is_synced() {
	return client.getDefaultSync();
}


/////////////////////////////////////////////////////////////////////////////////////////
// TelnetSerial -- Print
/////////////////////////////////////////////////////////////////////////////////////////


/** Write byte
*
*/
size_t TelnetSerial::write(uint8_t x) {
	size_t s = 0;
	if ((x == '\r') && autocr_sent) {
		autocr_sent = false;
		return 1;
	}
	if (Serial)
		s = Serial.write(x);
	if (check_conn())
		s = client.write(x);
	if (autocr_set && (x == '\n')) {
		this->write('\r');
		autocr_sent = true;
	}
	else {
		autocr_sent = false;
	}
	return s;
}


/** Write character array
*
*/
size_t TelnetSerial::write(char* x) {
	size_t s = 0;
	if (Serial)
		s = Serial.write(x);
	if (check_conn())
		s = client.write(x);
	return s;
}


/** Write byte array
*
*/
size_t TelnetSerial::write(uint8_t* x, size_t sz) {
	size_t s = 0;
	if (Serial)
		s = Serial.write(x, sz);
	if (check_conn())
		s = client.write(x, sz);

	return s;
}


/** Wait for output to cmplete
 *
 */
void TelnetSerial::flush() {
	if (Serial)
		Serial.flush();
	if (check_conn())
		client.flush();
}

/////////////////////////////////////////////////////////////////////////////////////////
// TelnetSerial -- Stream
/////////////////////////////////////////////////////////////////////////////////////////

static void write_with_ln(Print &p, int c) {
	if (c == '\n') {
		p.write('\n');
		p.write('\r');
	}
	else
		p.write(c);
}

/** Read Serial with keystroke echo
*
*/int TelnetSerial::readSerialWithEcho() {
	int r = Serial.read();
	if (r > 0) {
		if (inputecho_serial_set)
			write_with_ln(Serial,r);
		if (crossecho_set)
			write_with_ln(client,r);
	}
	return r;
}


/** Read telnet client with keystroke echo
*
*/
int TelnetSerial::readClientWithEcho() {
	int r = client.read();
	if (r > 0) {
		if (inputecho_telnet_set)
			write_with_ln(client, r);
		if (crossecho_set) {
			write_with_ln(Serial, r);
		}
	}
	return r;
}


/** Return next character, -1 if none
*
*/
int TelnetSerial::read() {
	int r = -1;
	if (which_in == TS_NONE_IN) {  // Try first serial, then telnet
		if (Serial.available() > 0)
			r = readSerialWithEcho();
		else if (check_conn() && (client.available() > 0))
			r = readClientWithEcho();
	}
	else if (which_in == TS_SERIAL_IN) { // Already doing serial
		r = readSerialWithEcho();
	}
	else {                             // Fallback to telnet
		if (check_conn())
			r = readClientWithEcho();
	}
	if (r < 0)
		which_in == TS_NONE_IN;          // Source exhausted
	return r;
}


/** Huw many characters in input buffer?
*
*/
int TelnetSerial::available() {
	//
	// Once chars are found in a source, next calls to available()
	// read() or peek() stick to this source until exhausted
	//
	int r = 0;
	if ((which_in == TS_SERIAL_IN) || (which_in == TS_NONE_IN)) {
		if (Serial)
			r = Serial.available();
		if (r > 0)
			which_in = TS_SERIAL_IN;
		else
			which_in = TS_NONE_IN;
	}
	if ((which_in == TS_TELNET_IN) || (which_in == TS_NONE_IN)) {
		if (check_conn())
			r = client.available();
		if (r > 0)
			which_in = TS_TELNET_IN;
		else
			which_in = TS_NONE_IN;
	}
	return r;
}


/** Get next character in input or -1 fi none, keeping it for read()
*
*/
int TelnetSerial::peek() {
	// 
	// once successfully peeked from a source, succesive calls to the Stream
	// functions stick to it until exhausted
	// 
	int r = -1;
	if ((which_in == TS_SERIAL_IN) || (which_in == TS_NONE_IN)) {
		if (Serial)
			r = Serial.peek();
		if (r > 0)
			which_in = TS_SERIAL_IN;
		else
			which_in = TS_NONE_IN;
	}
	if ((which_in == TS_TELNET_IN) || (which_in == TS_NONE_IN)) {
		if (check_conn())
			r = client.peek();
		if (r > 0)
			which_in = TS_TELNET_IN;
		else
			which_in = TS_NONE_IN;
	}
	return r;
}


/////////////////////////////////////////////////////////////////////////////////////////
// TelnetSerial -- Command listener
/////////////////////////////////////////////////////////////////////////////////////////


/** Set timeout
*
*/
void TelnetSerial::cmd_set_timeout(unsigned long ms) {
	if (ms && (ms < COMMAND_MIN_TIMEOUT))
		ms = COMMAND_MIN_TIMEOUT;
	command_timeout = ms;
	command_half_timeout = ms / 2;
}

static bool TO_warning_issued = false;

/** Return true if a complete command line has been read (until EOL)
*
*/
bool TelnetSerial::cmd_available() {
	/*-- if command verb and EOL are present, command is ready --*/
	if (eolfound && (cmdpos > 0))
	{
		// To do: Set timeout on non-consumed commands?
		this->check_conn(); // Just in case a telnet connection is being attempted
		return true;
	}
	
	/*-- any input?  --*/
	int n = this->available();

	/*-- No => Check for input timeouts --*/ 
	if (n == 0)  {
		/*-- No keustrokes pending --*/
		if ((scanpos > 0) && (command_timeout > 0UL))  {
			/*--  Command buffer non-empty anf timeout checking active --*/
			unsigned long elaps = millis() - command_last_key_ms;
			if (elaps > (command_half_timeout)) {
				/*-- Warn user at 1/2 time out --*/
				if (!TO_warning_issued) {
					this->print("<WAITING FOR INPUT>");
					TO_warning_issued = true;
				}
				if (elaps > (command_timeout)) {
					/*-- flush input if timeout occurred --*/
					cmd_init();
					this->println("<TIMEOUT, INPUT DISCARDED. PLEASE RETYPE>\n:");
				}
			}
		}
		return false; // No command available yet
	}	

	/*--Process pending keystrokes --*/
	char c = 0; // Next char in input
	while (n > 0)
	{
		TO_warning_issued = false;
		command_last_key_ms = millis();
		/*--prevent command buffer overflow --*/
		if ((scanpos) >= COMMAND_BUFFER_SIZE) {
			send_error_message(error_command_toolong);
			cmd_init();
			return false;
		}
		/*-- read and process next character --*/
		c = this->read();
		switch (c)
		{
		/*-- End Of Line --*/
		case 0:
		case '\n':
		case '\r':
			if (cmdpos < 0) {  // empty line
				cmd_init();
				this->println();
				return false;
			}
			cmdbuf[++scanpos] = 0;
			eolfound = true;
			return true;
			break;
		/*-- Whitespace (only first WS char is stored) --*/
		case ' ':
		case '\t':
			if (!whitespace) {
				whitespace = true;
				cmdbuf[scanpos++] = 0;
			}
			break;
		/*-- Text, first position after whitespace is stored --*/
		default:
			if (cmdpos < 0) {
				/*-- first word is command verb --*/
				cmdpos = scanpos;
			}
			else if (whitespace) {
				if (nparams >= COMMAND_MAX_PARAMS) {
					send_error_message(error_too_many_params);
					cmd_init();
					return false;
				}
				else {
					parampos[++nparams] = scanpos;
				}
			}
			whitespace = false;
			cmdbuf[scanpos++] = c;
			// this->printf("\n\rc=%c scanpos=%d cmdpos=%d np=%d\n\r", c, (int)scanpos, (int)cmdpos, (int)nparams);
			break;
		}
		/*-- more data? --*/
		n = this->available();
	}  // while(n>0)
}

/** Get number of parameters
*
*/
int TelnetSerial::cmd_num_params() {
	return nparams;
}

/** Initialize for a new command line
*
*/
void TelnetSerial::cmd_init(const char *prompt) {
	bzero(cmdbuf, COMMAND_BUFFER_SIZE);
	cmdpos = -1; // << NOT YET FOUND
	scanpos = 0;
	nparams = 0;
	eolfound = false;
	whitespace = false;
	command_last_key_ms = 0;
	if (prompt)
		this->print(prompt);
	client.flush();
	Serial.flush();
}


/** Return true if a command is being typed.
* 
* Might be used to inhibit output during command processing at the application level
*/
bool  TelnetSerial::cmd_in_progress() {
	// this->printf("<%d>", (int)scanpos);  
	return (scanpos > 0);
}

/** Get pointer to command verb
*
*/
char* TelnetSerial::cmd_verb() {
	if (cmdpos >= 0)
		return &(cmdbuf[cmdpos]);
	else
		return NULL;
}


/** Is the command just read equal to the fiven text and the number of parameters is in range?
*
*/
bool TelnetSerial::cmd_equal(const char *expected, int minparams, int maxparams) {
	char* actual = cmd_verb();
	if (maxparams < 0)
		maxparams = minparams;
	if (expected && actual && !strcmp(expected, actual))
		if (minparams < 0)
			return true;
		else {
			bool r = (cmd_num_params() >= minparams) && (cmd_num_params() <= maxparams);
			if (!r)
				send_error_message(error_params_number);
			return r;
		}
	else
		return false;
}


/** Get pointer to n-th parameter string
*
*/
char* TelnetSerial::cmd_param_str(uint8_t npar) {
	if (cmd_verb()) {
		if (npar == 0)
			return cmd_verb();
		else if (npar <= nparams)
			return &(cmdbuf[parampos[npar]]);
	}
	return NULL;
}


/** Interpret n-th parameter as integer
*
*/
int TelnetSerial::cmd_param_int(uint8_t npar) {
	char *s = cmd_param_str(npar);
	if (s)
		return (atoi(s));
	else
		return 0;
}


/** Interpret n-th parameter as double
*
*/
double TelnetSerial::cmd_param_double(uint8_t npar) {
	char *s = cmd_param_str(npar);
	if (s)
		return (atof(s));
	else
		return 0.0;
}


/** Send error message
*
*/
void TelnetSerial::send_error_message(const char* msg) {
	this->print("ERROR: ");
	this->println(msg);
	return;
}


