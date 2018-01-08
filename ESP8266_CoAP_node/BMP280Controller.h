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

#include "SensorInterface.h"

namespace BMP280 {


const uint32_t SPI_BUS_SPEED = 500000;
const uint8_t ADDRESS_I2C = 0x77;
const uint8_t CHIP_ID = 0x60;

const uint8_t REG_CONTROL_PMODE_Pos = 0;
const uint8_t REG_CONTROL_TOVSAMP_Pos = 2;
const uint8_t REG_CONTROL_POVSAMP_Pos = 5;
const uint8_t REG_STATUS_MEASURING_Pos = 3;

const uint8_t REG_CONTROL_PMODE_Msk = 0x03 << REG_CONTROL_PMODE_Pos;
const uint8_t REG_CONTROL_TOVSAMP_Msk = 0x07 << REG_CONTROL_TOVSAMP_Pos;
const uint8_t REG_CONTROL_POVSAMP_Msk = 0x07 << REG_CONTROL_POVSAMP_Pos;
const uint8_t REG_STATUS_MEASURING_Msk = 0x01 << REG_STATUS_MEASURING_Pos;


enum Mode_t {
	SLEEP = 0, FORCED = 1, NORMAL = 3
};

enum State_t {
	ENABLED = 1, DISABLED = 0
};

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
	STATUS = 0xF3,
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


class BMP280Controller : public SensorInterface {

private:

	static const uint8_t POVSAMP_DEFAULT = 0x01;
	static const uint8_t TOVSAMP_DEFAULT = 0x07;

	TwoWire *bus_I2C;

	CalibrationData cData;
	int64_t t_fine = 0;
	Mode_t currentMode = Mode_t::NORMAL;
	uint8_t currentTempState = State_t::ENABLED;
	uint8_t currentPressState = State_t::ENABLED;

public:
	BMP280Controller(TwoWire *i2c);
	virtual ~BMP280Controller();

	bool begin(uint8_t addr = BMP280::ADDRESS_I2C, uint8_t chipid = BMP280::CHIP_ID);

	float measureTemperature(void);
	float measurePressure(void);
	bool setMode(Mode_t newMode);
	bool isSleeping() { return currentMode == Mode_t::SLEEP; }
	bool setTempState(State_t newState);
	bool setPressState(State_t newState);

private:
	float readTemperature(void);
	float readPressure(void);
	float readAltitude(float seaLevelhPa = 1013.25);
	void readCoefficients(void);
	//uint8_t spixfer(uint8_t x);

	void write8(byte reg, byte value);
	void setWrite8(byte reg, byte value, byte mask);
	uint8_t read8(byte reg);
	uint16_t read16(byte reg);
	uint32_t read24(byte reg);
	int16_t readS16(byte reg);
	uint16_t read16_LE(byte reg); // little endian
	int16_t readS16_LE(byte reg); // little endian

private:
	bool setOn(SType_t stype) {
		if (stype == SType_t::TEMP) return setTempState(State_t::ENABLED);
		if (stype == SType_t::PRESS) return setPressState(State_t::ENABLED);
		return false;
	}
	bool setOff(SType_t stype) {
		if (stype == SType_t::TEMP) return setTempState(State_t::DISABLED);
		if (stype == SType_t::PRESS) return setPressState(State_t::DISABLED);
		return false;
	}
	bool setModeContinous(SType_t stype) {
		for (uint8_t i=0; i<sensors.size(); ++i) sensors[i].mode = SMeasureMode_t::CONTINOUS; // One sets mode for all
		return setMode(Mode_t::NORMAL);
	}
	bool setModeOnDemand(SType_t stype) {
		for (uint8_t i=0; i<sensors.size(); ++i) sensors[i].mode = SMeasureMode_t::ON_DEMAND; // One sets mode for all
		return setMode(Mode_t::SLEEP);
	}

	bool getMode(SType_t stype) {
		return isSleeping() ? SMeasureMode_t::ON_DEMAND : SMeasureMode_t::CONTINOUS;
	}
};

} /* namespace BMP280 */

#endif /* BMP280CONTROLLER_H_ */
