#include <iostream>
#include <ostream>
#include "Parameters.h"
#include "MachineClass.h"


#ifndef BUFFER_H
#define BUFFER_H

using namespace std;
class buffer : public machine
{
public:
	int waiting_time;
	
	buffer();
	~buffer();
	buffer(const buffer &copyin);
	buffer &operator=(const buffer &rhs);
	friend ostream &operator<<(ostream &output, const buffer &mc);
	
	//Operation Related Methos
	void reportResidents();

};
#endif