/*
*   This file is part of firstperson-telecontrol.
*
*    firstperson-telecontrol is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    firstperson-telecontrol is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with firstperson-telecontrol.  If not, see <http://www.gnu.org/licenses/>.
*
*	 Authors: Lars Fritsche, Felix Unverzagt, Roberto Calandra
*	 Created: July, 2015
*/

#include <yarp/os/Network.h>
#include <yarp/os/Time.h>
#include <yarp/os/ResourceFinder.h>
#include <yarp/dev/PolyDriver.h>
#include <yarp/dev/ITorqueControl.h>
#include <yarp/dev/IControlMode.h>
#include <yarp/dev/IPositionControl.h>
#include <yarp/dev/IPositionDirect.h>
#include <yarp/dev/IEncoders.h>
#include <yarp/sig/Matrix.h>
#include <yarp/sig/Vector.h>
#include <yarp/math/Math.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <YarpAdapter.h>
#include <vector>
#include <SensorGloveInterface.h>

using namespace std;

using namespace yarp::os;
using namespace yarp::dev;
using namespace yarp::math;

const double pi = 3.141592654;
const double g = 9.81;

Network yarpNet;

Property params;
std::string robotName;

IControlMode *controlMode[4];
ITorqueControl *torque[4];
IPositionDirect *positionDirect[4];
IPositionControl *positionControl[4];
IEncoders *encoders[4];

BufferedPort<Bottle> inPort;
BufferedPort<Bottle> outPort;

double oldGloveVibrationInput[] = {0, 0, 0, 0, 0};
bool gloveVibrationInitialized;

int activatedJoints[] = { 0, 1, 2, 3 };
bool activated[] = { false, false, true, false };
bool useDirect[] = { true, true, true, true };

PolyDriver robotDevice[4];

int dof[] = { 16, 16, 6, 3 };


/// <summary>
/// Constructor
/// </summary>
YarpAdapter::YarpAdapter() {
	//resetVibration();
	initialize();
	//setMode(false);
}
YarpAdapter::~YarpAdapter() {
//	setMode(true);
}

void YarpAdapter::setMode(bool finished) {
	for (int i = 0; i < 4; i++) {
		if (!activated[i])
			continue;

		if (useDirect[i] && !finished)
			positionDirect[i]->setPositionDirectMode();
		else
			positionControl[i]->setPositionMode();
	}
}

void YarpAdapter::initializeDevice(int bodyPart, string remotePorts,
		string localPorts, string device) {
	Property options;
	options.put("device", "remote_controlboard");
	options.put("local", localPorts.c_str());   //local port names
	options.put("remote", remotePorts.c_str());         //where we connect to

	// create a device
	robotDevice[bodyPart].open(options);
	if (!robotDevice[bodyPart].isValid()) {
		printf("Device not available.  Here are the known devices:\n");
		printf("%s", Drivers::factory().toString().c_str());
		string devices = Drivers::factory().toString().c_str();
		return;
	}

	bool ok;
	ok = robotDevice[bodyPart].view(controlMode[bodyPart]);
	ok = ok && robotDevice[bodyPart].view(torque[bodyPart]);
	ok = ok && robotDevice[bodyPart].view(positionDirect[bodyPart]);
	ok = ok && robotDevice[bodyPart].view(positionControl[bodyPart]);
	ok = ok && robotDevice[bodyPart].view(encoders[bodyPart]);

	if (!ok) {
		printf("Problems acquiring interfaces\n");
		return;
	}
}

void YarpAdapter::initializePorts() {
	outPort.open("/hapticGloveController/glove_raw/out");
	inPort.open("/hapticGloveController/vibration/in");
}

void YarpAdapter::setSingleVelocityNormal(int bodyPart) {
	if (useDirect[bodyPart] || !activated[bodyPart])
		return;

	switch (bodyPart) {
	case MOVE_LEFT_ARM:
	case MOVE_RIGHT_ARM:
		positionControl[bodyPart]->setRefSpeed(0, 5);
		positionControl[bodyPart]->setRefSpeed(1, 5);
		positionControl[bodyPart]->setRefSpeed(2, 5);
		positionControl[bodyPart]->setRefSpeed(3, 5);
		positionControl[bodyPart]->setRefSpeed(10, 0);
		positionControl[bodyPart]->setRefSpeed(11, 0);
		positionControl[bodyPart]->setRefSpeed(12, 0);
		positionControl[bodyPart]->setRefSpeed(13, 0);
		positionControl[bodyPart]->setRefSpeed(14, 0);
		positionControl[bodyPart]->setRefSpeed(15, 0);
		return;
	case MOVE_TORSO:
		positionControl[bodyPart]->setRefSpeed(0, 10);
		positionControl[bodyPart]->setRefSpeed(1, 10);
		positionControl[bodyPart]->setRefSpeed(2, 10);
		return;
	case MOVE_HEAD:
		positionControl[bodyPart]->setRefSpeed(0, 5);
		positionControl[bodyPart]->setRefSpeed(1, 5);
		positionControl[bodyPart]->setRefSpeed(2, 5);
		return;
	}
}

void YarpAdapter::setSingleVelocityLow(int bodyPart) {
	if (useDirect[bodyPart] || !activated[bodyPart])
		return;

	switch (bodyPart) {
	case MOVE_LEFT_ARM:
	case MOVE_RIGHT_ARM:
		positionControl[bodyPart]->setRefSpeed(0, 3);
		positionControl[bodyPart]->setRefSpeed(1, 3);
		positionControl[bodyPart]->setRefSpeed(2, 3);
		positionControl[bodyPart]->setRefSpeed(3, 3);

		positionControl[bodyPart]->setRefSpeed(10, 00);
		positionControl[bodyPart]->setRefSpeed(11, 00);
		positionControl[bodyPart]->setRefSpeed(12, 00);
		positionControl[bodyPart]->setRefSpeed(13, 00);
		positionControl[bodyPart]->setRefSpeed(14, 00);
		positionControl[bodyPart]->setRefSpeed(15, 00);
		return;
	case MOVE_TORSO:
		positionControl[bodyPart]->setRefSpeed(0, 3);
		positionControl[bodyPart]->setRefSpeed(1, 3);
		positionControl[bodyPart]->setRefSpeed(2, 3);
		return;
	case MOVE_HEAD:
		positionControl[bodyPart]->setRefSpeed(0, 1);
		positionControl[bodyPart]->setRefSpeed(1, 1);
		positionControl[bodyPart]->setRefSpeed(2, 1);
		return;
	}
}

void YarpAdapter::initialize() {

	initializePorts();

	return;
}

void YarpAdapter::setVelocityNormal() {
	setSingleVelocityNormal(MOVE_LEFT_ARM);
	setSingleVelocityNormal(MOVE_RIGHT_ARM);
	setSingleVelocityNormal(MOVE_TORSO);
}

void YarpAdapter::setVelocityLow() {
	setSingleVelocityLow(MOVE_LEFT_ARM);
	setSingleVelocityLow(MOVE_RIGHT_ARM);
	setSingleVelocityLow(MOVE_TORSO);
}

int YarpAdapter::move(int bodyPart, int jointId, double angle) {
	if (!activated[bodyPart])
		return -1;

	if (useDirect[bodyPart])
		positionDirect[bodyPart]->setPosition(jointId, angle);
	else
		positionControl[bodyPart]->positionMove(jointId, angle);

	return 0;
}

int YarpAdapter::move(int bodyPart, double *refs) {
	if (!activated[bodyPart])
		return -1;

	if (useDirect[bodyPart]) {
		if (bodyPart == YarpAdapter::MOVE_TORSO || bodyPart == YarpAdapter::MOVE_HEAD) {
			int activatedTorsoJoints[] = { 0, 1, 2 };
			//positionDirect[bodyPart]->setPositions(3, activatedTorsoJoints,refs);
			positionDirect[bodyPart]->setPositions(refs);
		} else {
			positionDirect[bodyPart]->setPositions(4, activatedJoints, refs);
		}

	} else
		positionControl[bodyPart]->positionMove(refs);

	return 0;
}

double* YarpAdapter::getJoints(int bodyPart) {
	double* values = new double[dof[bodyPart]];
	encoders[bodyPart]->getEncoders(values);
	return values;
}

int YarpAdapter::getDof(int bodyPart) {
	return dof[bodyPart];
}

bool YarpAdapter::isDirect(int bodyPart) {
	return useDirect[bodyPart];
}

bool YarpAdapter::isActivated(int bodyPart) {
	return activated[bodyPart];
}

int YarpAdapter::close() {
	robotDevice[0].close();
	robotDevice[1].close();
	robotDevice[2].close();
	robotDevice[3].close();

	return 0;
}

double* YarpAdapter::readGloveVibrationInput() {
	Bottle* in = inPort.read(false);
	if (in == NULL) {
		double currentTime = ((double)(clock()));
		if((currentTime - lastTime) > 1000){
			//resetVibration();
		}
		return oldGloveVibrationInput;
	}

	lastTime = ((double) clock());

	double* inVals = new double[in->size()];
	for (int i = 0; i < 6; i++)
		inVals[i] = in->get(i).asDouble();

	//oldGloveVibrationInput = inVals;
	gloveVibrationInitialized = true;
	return inVals;
}


void YarpAdapter::writeToOutput(double* handValues) {
	Bottle& out = outPort.prepare();
	out.clear();

	//add hand values € [0, 1024)
	for(int i=0; i < SensorGloveInterface::NUM_OF_FINGERS; i++) {
		out.addDouble(handValues[i]);
	}

	outPort.write();
}

void YarpAdapter::resetVibration() {
	if(gloveVibrationInitialized) {
		//oldGloveVibrationInput = new double[SensorGloveInterface::NUM_OF_FINGERS];
		gloveVibrationInitialized = true;
	}

	for(int i=0; i < SensorGloveInterface::NUM_OF_FINGERS; i++)
		oldGloveVibrationInput[i] = 0;
}

/**
 *
 */

