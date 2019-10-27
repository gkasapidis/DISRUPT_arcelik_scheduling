#include "MachineUnav.h"

//Machine Unavailability Class Methods
mach_unav::mach_unav() {
	start_unav = 0;
	end_unav = 0;
}

mach_unav::mach_unav(const mach_unav &copyin) {
	start_unav = copyin.start_unav;
	end_unav = copyin.end_unav;
}

mach_unav& mach_unav::operator=(const mach_unav &rhs) {
	this->start_unav = rhs.start_unav;
	this->end_unav = rhs.end_unav;
	return *this;
}



