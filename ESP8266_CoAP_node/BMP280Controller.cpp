/*
 * BMP280Controller.cpp
 *
 *  Created on: 16.11.2017
 *      Author: Witold
 */

#include "BMP280Controller.h"

namespace BMP280 {

BMP280Controller::BMP280Controller(TwoWire *i2c)
	: SensorInterface(Sensor_t::TEMP_PRESS), bus_I2C(i2c) {}

BMP280Controller::~BMP280Controller() {}

bool BMP280Controller::begin(uint8_t addr, uint8_t chipid) {
	// hardware SPI
	bus_I2C->begin();

	if (read8(RegAddr_t::ID) != chipid) {
		Serial.println(read8(RegAddr_t::ID), 16);
		return false;
	}

	readCoefficients();

	write8(RegAddr_t::CONTROL,
			(0x01 << REG_CONTROL_POVSAMP_Pos)
					| (0x07 << REG_CONTROL_TOVSAMP_Pos)
					| (0x03 << REG_CONTROL_PMODE_Pos));
	return true;
}

float BMP280Controller::readTemperature(void) {
	int32_t var1, var2;
	int32_t adc_T = read24(RegAddr_t::TEMPDATA);
	adc_T >>= 4;

	var1 = ((((adc_T >> 3) - ((int32_t) cData.dig_T1 << 1)))
			* ((int32_t) cData.dig_T2)) >> 11;

	var2 = (((((adc_T >> 4) - ((int32_t) cData.dig_T1))
			* ((adc_T >> 4) - ((int32_t) cData.dig_T1))) >> 12)
			* ((int32_t) cData.dig_T3)) >> 14;
	t_fine = var1 + var2;
	float T = (t_fine * 5 + 128) >> 8;
	return T / 100;
}

float BMP280Controller::readPressure(void) {
	  int64_t var1, var2, p;

	  // Must be done first to get the t_fine variable set up
	  readTemperature();

	  int32_t adc_P = read24(RegAddr_t::PRESSUREDATA);
	  adc_P >>= 4;

	  var1 = ((int64_t)t_fine) - 128000;
	  var2 = var1 * var1 * (int64_t)cData.dig_P6;
	  var2 = var2 + ((var1*(int64_t)cData.dig_P5)<<17);
	  var2 = var2 + (((int64_t)cData.dig_P4)<<35);
	  var1 = ((var1 * var1 * (int64_t)cData.dig_P3)>>8) +
	    ((var1 * (int64_t)cData.dig_P2)<<12);
	  var1 = (((((int64_t)1)<<47)+var1))*((int64_t)cData.dig_P1)>>33;

	  if (var1 == 0) {
	    return 0;  // avoid exception caused by division by zero
	  }
	  p = 1048576 - adc_P;
	  p = (((p<<31) - var2)*3125) / var1;
	  var1 = (((int64_t)cData.dig_P9) * (p>>13) * (p>>13)) >> 25;
	  var2 = (((int64_t)cData.dig_P8) * p) >> 19;

	  p = ((p + var1 + var2) >> 8) + (((int64_t)cData.dig_P7)<<4);
	  return (float)p/256;
}

float BMP280Controller::readAltitude(float seaLevelhPa) {
	  float altitude;
	  float pressure = readPressure(); // in Si units for Pascal
	  pressure /= 100;
	  altitude = 44330 * (1.0 - pow(pressure / seaLevelhPa, 0.1903));
	  return altitude;
}

void BMP280Controller::readCoefficients(void) {
	cData.dig_T1 = read16_LE(RegAddr_t::DIG_T1);
	cData.dig_T2 = readS16_LE(RegAddr_t::DIG_T2);
	cData.dig_T3 = readS16_LE(RegAddr_t::DIG_T3);
	cData.dig_P1 = read16_LE(RegAddr_t::DIG_P1);
	cData.dig_P2 = readS16_LE(RegAddr_t::DIG_P2);
	cData.dig_P3 = readS16_LE(RegAddr_t::DIG_P3);
	cData.dig_P4 = readS16_LE(RegAddr_t::DIG_P4);
	cData.dig_P5 = readS16_LE(RegAddr_t::DIG_P5);
	cData.dig_P6 = readS16_LE(RegAddr_t::DIG_P6);
	cData.dig_P7 = readS16_LE(RegAddr_t::DIG_P7);
	cData.dig_P8 = readS16_LE(RegAddr_t::DIG_P8);
	cData.dig_P9 = readS16_LE(RegAddr_t::DIG_P9);
}

void BMP280Controller::write8(uint8_t reg, uint8_t value) {
	bus_I2C->beginTransmission(BMP280::ADDRESS_I2C);
	bus_I2C->write(reg);
	bus_I2C->write(value);
	bus_I2C->endTransmission();
}

uint8_t BMP280Controller::read8(uint8_t reg) {
	bus_I2C->beginTransmission(BMP280::ADDRESS_I2C);
	bus_I2C->write((uint8_t)reg);
	bus_I2C->endTransmission();
	bus_I2C->requestFrom(BMP280::ADDRESS_I2C, (uint8_t)1);
    uint8_t value = bus_I2C->read();

	return value;
}

uint16_t BMP280Controller::read16(uint8_t reg) {
	bus_I2C->beginTransmission(BMP280::ADDRESS_I2C);
	bus_I2C->write((uint8_t)reg);
	bus_I2C->endTransmission();
	bus_I2C->requestFrom(BMP280::ADDRESS_I2C, (uint8_t)2);
    uint16_t value = (bus_I2C->read() << 8) | bus_I2C->read();

	return value;
}

uint32_t BMP280Controller::read24(uint8_t reg) {
	bus_I2C->beginTransmission(BMP280::ADDRESS_I2C);
	bus_I2C->write((uint8_t)reg);
	bus_I2C->endTransmission();
	bus_I2C->requestFrom(BMP280::ADDRESS_I2C, (uint8_t)3);
	uint32_t value = (bus_I2C->read() << 16) | (bus_I2C->read() << 8) | bus_I2C->read();

	return value;
}

int16_t BMP280Controller::readS16(uint8_t reg) {
	return (int16_t)read16(reg);
}

uint16_t BMP280Controller::read16_LE(uint8_t reg) {
	  uint16_t tmp = read16(reg);
	  return (tmp >> 8) | (tmp << 8);
}

int16_t BMP280Controller::readS16_LE(uint8_t reg) {\
	return (int16_t)read16_LE(reg);
}



} /* namespace BMP280 */


