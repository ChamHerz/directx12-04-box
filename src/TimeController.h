//***************************************************************************************
// TimeController.h by ChamHerz (C) 2020
//***************************************************************************************

#pragma once
class TimeController
{
public:
	TimeController();

	void reset();				// inicia el tiempo
	void update();				// se actualiza por frame

	float totalTime() const;	// en segundos

private:
	__int64 baseTime;
	__int64 prevTime;
	__int64 stopTime = 0;
	__int64 currentTime;
	__int64 pausedTime;

	bool stopped;

	double deltaTime;	// variacion del tiempo
	double secondsPerCount;
};

