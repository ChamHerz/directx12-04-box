#include <windows.h>				//QueryPerformanceCounter
#include "TimeController.h"

TimeController::TimeController()
	: secondsPerCount(0.0), deltaTime(-1.0), baseTime(0),
	pausedTime(0), prevTime(0), currentTime(0), stopped(false)
{
	__int64 countsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
	secondsPerCount = 1.0 / (double)countsPerSec;
}

float TimeController::totalTime() const
{
	if (stopped)
	{
		return (float)(((stopTime - pausedTime) - baseTime) * secondsPerCount);
	}
	else
	{
		return (float)(((currentTime - pausedTime) - baseTime) * secondsPerCount);
	}
}

void TimeController::update()
{
	if (stopped)
	{
		deltaTime = 0.0;
		return;
	}

	// obtengo y guardo nuevamente el timer del sistema
	__int64 currentTimeAux;
	QueryPerformanceCounter((LARGE_INTEGER*)&currentTimeAux);
	currentTime = currentTimeAux;

	// Obtengo la diferencia entre ambos tiempos
	deltaTime = (currentTime - prevTime) * secondsPerCount;

	// Preparo para el siguiente frame
	prevTime = currentTime;

	// Puede llegar a ser negativo
	if (deltaTime < 0.0)
	{
		deltaTime = 0.0;
	}
}

void TimeController::reset()
{
	// obtengo la hora del sistema
	__int64 currentTimeAux;
	QueryPerformanceCounter((LARGE_INTEGER*)&currentTimeAux);

	baseTime = currentTimeAux;
	prevTime = currentTimeAux;
	stopped = 0;
	stopped = false;
}