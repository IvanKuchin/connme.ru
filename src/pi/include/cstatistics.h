#ifndef	__CSTATISTICS__H__
#define	__CSTATISTICS__H__

#include <chrono>
#include <time.h>
#include "clog.h"

class CStatistics
{
	private:
		clock_t						startClock, endClock;
		int							startTime, endTime;
		string						logFilename;
		std::chrono::microseconds	startChrono, endChrono;
	public:

				CStatistics();
		void	SetLogFile(string param) { logFilename = param; };
				~CStatistics();
};

#endif