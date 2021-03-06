#include "AD5790.h"

SPISettings AD5790_spi_settings(8000000, MSBFIRST, SPI_MODE1); //Sets SPI Settings


/* Sends a 24 bit code to the device 
 *
 * pin: the arduino pin that the device is connected to
 * bitcode: the 24 bit code to send to the device
 * 
 * Returns: the 24 bit code sent to the device
 */


static long write_register(int pin, long bitcode)
{ 
    byte b1, b2, b3; //declare bytes (8 x 3 = 24)
    SPI.beginTransaction(AD5790_spi_settings); //Necessary for SPI communication
    digitalWrite(pin, LOW); 
    SPI.transfer(BYTE1(bitcode)); //Transfer the first, second, and third bytes of a 24 bit code (Line 28 of AD5790.h)
    SPI.transfer(BYTE2(bitcode)); 
    SPI.transfer(BYTE3(bitcode)); 
    digitalWrite(pin, HIGH);
    digitalWrite(pin, LOW);
    b1 = SPI.transfer(0x0); 
    b2 = SPI.transfer(0x0);
    b3 = SPI.transfer(0x0);
    digitalWrite(pin, HIGH);
    SPI.endTransaction();
    return BITCODE(b1, b2, b3);
}

/* Reads from a specified register 
 *
 * pin: the arduino pin that the device is connected to
 * register_code: the register code for the register to read from (see lines 16-19 of AD5790.h)
 * 
 * Returns: the 24 bit code read from the register
 */
static long read_register(int pin, byte register_code)
{
    byte b1, b2, b3;

    SPI.beginTransaction(AD5790_spi_settings);
    digitalWrite(pin, LOW);
    b1 = SPI.transfer((READ|(0x7&register_code))<<4); //
    b2 = SPI.transfer(0x0);
    b3 = SPI.transfer(0x0);
    digitalWrite(pin, HIGH);
    digitalWrite(pin, LOW);
    b1 = SPI.transfer(0x0);
    b2 = SPI.transfer(0x0);
    b3 = SPI.transfer(0x0);
    digitalWrite(pin, HIGH);
    SPI.endTransaction();

  return BITCODE(b1, b2, b3);
}

void AD5790::set_gcurrval(long input)
{
  //Serial.println("Updating gcurrval...");
  gcurrval = input;
  //Serial.print("Updated gcurrval: ");
  //Serial.println(gcurrval);
}

AD5790::AD5790(int sync)
{
    //sync = 4;
    _sync = sync;
    /*Serial.print("_sync ");
    Serial.println(_sync);*/
}


/* Initializes the device with the INIT_CODE specified in line 22 of AD5790.h */
//void AD5790::initialize_DAC() {
//    pinMode(_sync, OUTPUT);
//    long init_code = INIT_CODE;
//    write_register(_sync, init_code);
//    set_value(131072);
//    Serial.println(read_CTRL_register()); 
//}
//int _sync = 4;
void AD5790::initialize_DAC(int pin, int _sync) { 
    Serial.print("pin is ");
    Serial.println(pin);
    pinMode(_sync, OUTPUT);
    long init_code = INIT_CODE0;
    write_register(_sync, init_code);
    Serial.println(read_CTRL_register()); 
    set_value(_sync, 131071);
    init_code = INIT_CODE;
    write_register(_sync, init_code);
    Serial.println(read_CTRL_register()); 
    set_value(_sync, 131071); 
    
}


/* Sets the value of the DAC register
 *
 * bitcode: the 18 bit code to set the DAC register to (0-262143)
 * 
 * Returns: the value written to the DAC register
 */
long AD5790::set_value(int _sync, long bitcode) 
{
    long dac_code = DAC_CODE(bitcode); // Given a value to write to the DAC register (0-262143), constructs the 24 bit code to send to the device 
    long rc = write_register(_sync, dac_code); //write to dac, return 24 bitcode
    Serial.print("_sync is ");
    Serial.println(_sync);
    _dac_reg = BITCODE_TO_DAC(rc); //Current value of DAC register = Given rc (24 bit code), returns the value read from or written to the DAC register (0-262143)
    AD5790::set_gcurrval(bitcode);
    return rc;
}


/* Ramps from the current value of 
 *
 * finval: the final value of the DAC register
 * step_size: the size of the step to take
 * delta_t: the time between steps
 * 
 */

void AD5790::ramp(int _sync, long finval, long step_size, int delta_t) 
{
  // _dac_reg is supposed to be the current value of the DAC register
  // this could be the issue if _dac_reg is not being updated properly by other functions 
  // _dac_reg is currently updated by set_value() and read_DAC_register() (lines 86 & 130)
//  long currval = _dac_reg; 
//  long currval = (20/(262144-1))*gcurrval-10; //send global currentvalue to local currentvalue, convert from bit value to voltage value
  long currval = gcurrval; //conversion of finval into bit value is done in python script
  long n_steps = abs(currval-finval)/abs(step_size);
  if (finval > currval) {
    step_size = abs(step_size);
  } else {
    step_size = -abs(step_size);
  }
  for(int j= 0; abs(currval-finval) > step_size; j++) {
    if (j > n_steps) break;
    unsigned long timer = millis();
    long newval = (currval + j*step_size)&0x3FFFF;
//    long newval = (currval + j*step_size);
    set_value(_sync, newval);
    while(millis() <= timer + delta_t);
  }
  //last step to get to the actual value you want
  set_value(_sync, finval);
}

/* These functions wrap read_register for specific registers */
long AD5790::read_CTRL_register()
{
    return read_register(_sync, CTRL);
}



long AD5790::read_DAC_register(int _sync)
{
    long rc = read_register(_sync, DAC);
    _dac_reg = BITCODE_TO_DAC(rc);
    return rc;
}


long AD5790::read_CLR_register()
{
    return read_register(_sync, CLR);
}


long AD5790::read_SCTRL_register()
{
    return read_register(_sync, SCTRL);
}
