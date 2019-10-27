#include <iostream>
#include <ostream>
#include "Parameters.h"
#include "ProcessStep.h"
#include "ListObject.h"

#ifndef STAGE_H
#define STAGE_H

using namespace std;
class stage
{
public:
	int ID;
	vector<process_step*> psteps;

	stage();
	~stage();
	stage(const stage &copyin);
	stage &operator=(const stage &rhs);
	friend ostream &operator<<(ostream &output, const stage &mc);
};
#endif