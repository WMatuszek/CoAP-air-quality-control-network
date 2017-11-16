/*
 * BMP280Controller.cpp
 *
 *  Created on: 16.11.2017
 *      Author: Witold
 */

#include "BMP280Controller.h"

namespace BMP280 {

BMP280Controller::BMP280Controller(SPIClass *spi, uint8_t pin_cs)
	: bus_SPI(spi), pin_cs(pin_cs) {}

BMP280Controller::~BMP280Controller() {}

bool BMP280Controller::begin(uint8_t addr, uint8_t chipid) {
	deselect();
	pinMode(pin_cs, OUTPUT);

	// hardware SPI
	bus_SPI->begin();

	if (read8(Register_t::ID) != chipid) {
		bus_SPI->end();
		return false;
	}

	readCoefficients();

	write8(Register_t::CONTROL, 0x3F);
	return true;
}

float BMP280Controller::readTemperature(void) {
	int32_t var1, var2;
	int32_t adc_T = read24(Register_t::TEMPDATA);
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

	  int32_t adc_P = read24(Register_t::PRESSUREDATA);
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
	cData.dig_T1 = read16_LE(Register_t::DIG_T1);
	cData.dig_T2 = readS16_LE(Register_t::DIG_T2);
	cData.dig_T3 = readS16_LE(Register_t::DIG_T3);
	cData.dig_P1 = read16_LE(Register_t::DIG_P1);
	cData.dig_P2 = readS16_LE(Register_t::DIG_P2);
	cData.dig_P3 = readS16_LE(Register_t::DIG_P3);
	cData.dig_P4 = readS16_LE(Register_t::DIG_P4);
	cData.dig_P5 = readS16_LE(Register_t::DIG_P5);
	cData.dig_P6 = readS16_LE(Register_t::DIG_P6);
	cData.dig_P7 = readS16_LE(Register_t::DIG_P7);
	cData.dig_P8 = readS16_LE(Register_t::DIG_P8);
	cData.dig_P9 = readS16_LE(Register_t::DIG_P9);
}

void BMP280Controller::write8(byte reg, byte value) {
	bus_SPI->beginTransaction(SPISettings(BMP280::SPI_BUS_SPEED, MSBFIRST, SPI_MODE0));
	select();
	bus_SPI->transfer(reg & ~0x80);
	bus_SPI->transfer(value);
	deselect();
	bus_SPI->endTransaction();
}

uint8_t BMP280Controller::read8(byte reg) {
	bus_SPI->beginTransaction(SPISettings(BMP280::SPI_BUS_SPEED, MSBFIRST, SPI_MODE0));
	select();
	bus_SPI->transfer(reg | 0x80);
	uint8_t val = bus_SPI->transfer(0x00);
	deselect();
	bus_SPI->endTransaction();

	return val;
}

uint16_t BMP280Controller::read16(byte reg) {
	bus_SPI->beginTransaction(SPISettings(BMP280::SPI_BUS_SPEED, MSBFIRST, SPI_MODE0));
	select();
	bus_SPI->transfer(reg | 0x80);
	uint16_t val = bus_SPI->transfer(0x00);
	val <<= 8;
	val |= bus_SPI->transfer(0x00);
	deselect();
	bus_SPI->endTransaction();

	return val;
}

uint32_t BMP280Controller::read24(byte reg) {
	bus_SPI->beginTransaction(SPISettings(BMP280::SPI_BUS_SPEED, MSBFIRST, SPI_MODE0));
	select();
	bus_SPI->transfer(reg | 0x80);
	uint32_t val = bus_SPI->transfer(0x00);
	val <<= 8;
	val |= bus_SPI->transfer(0x00);
	val <<= 8;
	val |= bus_SPI->transfer(0x00);
	deselect();
	bus_SPI->endTransaction();

	return val;
}

int16_t BMP280Controller::readS16(byte reg) {
	return (int16_t)read16(reg);
}

uint16_t BMP280Controller::read16_LE(byte reg) {
	  uint16_t tmp = read16(reg);
	  return (tmp >> 8) | (tmp << 8);
}

int16_t BMP280Controller::readS16_LE(byte reg) {\
	return (int16_t)read16_LE(reg);
}



} /* namespace BMP280 */


