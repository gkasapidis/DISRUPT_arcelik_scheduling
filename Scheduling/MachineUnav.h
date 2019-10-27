#include <iostream>
#include <ostream>

#ifndef MACHINEUNAV_H
#define MACHINEUNAV_H

class mach_unav
{
public:
	int start_unav;
	int end_unav;

	mach_unav();
	mach_unav(const mach_unav &copyin);
	mach_unav &operator=(const mach_unav &rhs);
};


#endif



