/*
 * SensorInterface.h
 *
 *  Created on: 03.01.2018
 *      Author: Witold
 */

#ifndef SENSORINTERFACE_H_
#define SENSORINTERFACE_H_

#include "CoAP_node.h"
#include <vector>
#include <algorithm>

struct SensorDevice {
	SType_t type;
	SState_t state;
	SMeasureMode_t mode;
};

class SensorInterface {

protected:
	std::vector<SensorDevice> sensors;

public:
	SensorInterface() {}
	virtual ~SensorInterface() {}

	bool setState(SType_t stype, SState_t newState) {
		SensorDevice *s = getSensorByType(stype);
		if (s == nullptr) return false;

		if (s->state == newState) return true;
		if (newState == SState_t::ON) {
			bool out = setOn(stype);
			if (out) s->state = newState;
			return out;
		}
		if (newState == SState_t::OFF) {
			bool out = setOff(stype);
			if (out) s->state = newState;
			return out;
		}
	}

	bool setMeasureMode(SType_t stype, SMeasureMode_t newMode) {
		SensorDevice *s = getSensorByType(stype);
		if (s == nullptr) return false;

		if (s->mode == newMode) return true;
		if (newMode == SMeasureMode_t::CONTINOUS) {
			bool out = setModeContinous(stype);
			if (out) s->mode = newMode;
			return out;
		}
		if (newMode == SMeasureMode_t::ON_DEMAND) {
			bool out = setModeOnDemand(stype);
			if (out) s->mode = newMode;
			return out;
		}
	}

	bool isSensorOn(SType_t stype) {
		SensorDevice *s = getSensorByType(stype);
		if (s == nullptr) return false;
		return s->state == SState_t::ON;
	}
	bool isModeContinous(SType_t stype) {
		SensorDevice *s = getSensorByType(stype);
		if (s == nullptr) return false;
		return s->mode == SMeasureMode_t::CONTINOUS;
	}

private:

	SensorDevice *getSensorByType(SType_t stype) {
		SensorDevice *retval = nullptr;
		std::vector<SensorDevice>::iterator it;
		for (it = sensors.begin(); it != sensors.end(); ++it) {
			if (it->type == stype) {
				retval = &(*it);
				break;
			}
		}
		return retval;
	}

	virtual bool setOn(SType_t stype) = 0;
	virtual bool setOff(SType_t stype) = 0;
	virtual bool setModeContinous(SType_t stype) = 0;
	virtual bool setModeOnDemand(SType_t stype) = 0;

};


#endif /* SENSORINTERFACE_H_ */
