/*
 * BMP280Controller.h
 *
 *  Created on: 16.11.2017
 *      Author: Witold
 */

#ifndef BMP280CONTROLLER_H_
#define BMP280CONTROLLER_H_

#include "Arduino.h"
#include "Wire.h"

namespace BMP280 {

const uint32_t SPI_BUS_SPEED = 500000;

const uint8_t ADDRESS_I2C = 0x77;
const uint8_t CHIP_ID = 0x60;

const uint8_t REG_CONTROL_PMODE_Pos = 0;
const uint8_t REG_CONTROL_TOVSAMP_Pos = 2;
const uint8_t REG_CONTROL_POVSAMP_Pos = 5;

enum RegAddr_t {
	DIG_T1 = 0x88,
	DIG_T2 = 0x8A,
	DIG_T3 = 0x8C,
	DIG_P1 = 0x8E,
	DIG_P2 = 0x90,
	DIG_P3 = 0x92,
	DIG_P4 = 0x94,
	DIG_P5 = 0x96,
	DIG_P6 = 0x98,
	DIG_P7 = 0x9A,
	DIG_P8 = 0x9C,
	DIG_P9 = 0x9E,
	ID = 0xD0,
	VERSION = 0xD1,
	SOFTRESET = 0xE0,
	CAL26 = 0xE1,  // R calibration stored in 0xE1-0xF0
	CONTROL = 0xF4,
	CONFIG = 0xF5,
	PRESSUREDATA = 0xF7,
	TEMPDATA = 0xFA,
};

struct CalibrationData {
  uint16_t dig_T1;
  int16_t  dig_T2;
  int16_t  dig_T3;
  uint16_t dig_P1;
  int16_t  dig_P2;
  int16_t  dig_P3;
  int16_t  dig_P4;
  int16_t  dig_P5;
  int16_t  dig_P6;
  int16_t  dig_P7;
  int16_t  dig_P8;
  int16_t  dig_P9;
  uint8_t  dig_H1;
  int16_t  dig_H2;
  uint8_t  dig_H3;
  int16_t  dig_H4;
  int16_t  dig_H5;
  int8_t   dig_H6;
};


class BMP280Controller {
private:
	TwoWire *bus_I2C;

	CalibrationData cData;
	int64_t t_fine = 0;

public:
	BMP280Controller(TwoWire *i2c);
	virtual ~BMP280Controller();

    bool  begin(uint8_t addr = BMP280::ADDRESS_I2C, uint8_t chipid = BMP280::CHIP_ID);
    float readTemperature(void);
    float readPressure(void);
    float readAltitude(float seaLevelhPa = 1013.25);

private:

    void readCoefficients(void);
    //uint8_t spixfer(uint8_t x);

    void      	write8(byte reg, byte value);
    uint8_t   	read8(byte reg);
    uint16_t  	read16(byte reg);
    uint32_t  	read24(byte reg);
    int16_t   	readS16(byte reg);
    uint16_t  	read16_LE(byte reg); // little endian
    int16_t   	readS16_LE(byte reg); // little endian

//    void 		select(void) { digitalWrite(pin_cs, LOW); }
//    void 		deselect(void) { digitalWrite(pin_cs, HIGH); }
};

} /* namespace BMP280 */

#endif /* BMP280CONTROLLER_H_ */
