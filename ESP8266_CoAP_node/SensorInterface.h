/*
 * SensorInterface.h
 *
 *  Created on: 03.01.2018
 *      Author: Witold
 */

#ifndef SENSORINTERFACE_H_
#define SENSORINTERFACE_H_

#include "CoAP_node.h"

class SensorInterface {

protected:
	Sensor_t type;
	SensorState_t state = SensorState_t::ON;
	MeasureMode_t mode = MeasureMode_t::CONTINOUS;

public:
	SensorInterface(Sensor_t type) : type(type) {}
	virtual ~SensorInterface() {}

	bool setState(SensorState_t newState) {
		if (state == newState) return true;
		if (newState == SensorState_t::ON) {
			bool out = setOn();
			if (out) state = newState;
			return out;
		}
		if (newState == SensorState_t::OFF) {
			bool out = setOff();
			if (out) state = newState;
			return out;
		}
	}

	bool setMeasureMode(MeasureMode_t newMode) {
		if (mode == newMode) return true;
		if (newMode == MeasureMode_t::CONTINOUS) {
			bool out = setModeContinous();
			if (out) mode = newMode;
			return out;
		}
		if (newMode == MeasureMode_t::ON_DEMAND) {
			bool out = setModeOnDemand();
			if (out) mode = newMode;
			return out;
		}
	}

private:

	virtual bool setOn() = 0;
	virtual bool setOff() = 0;
	virtual bool setModeContinous() = 0;
	virtual bool setModeOnDemand() = 0;

};


#endif /* SENSORINTERFACE_H_ */
