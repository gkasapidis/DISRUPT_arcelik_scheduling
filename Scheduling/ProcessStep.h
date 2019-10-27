#include "CObject.h"
#include "Parameters.h"
#include "ListObject.h"
#include "MachineClass.h"

#ifndef PROCSTEP_H
#define PROCSTEP_H

using namespace std;

class process_step
{
public:
	int ID;
	int actualID;
	int processID;

	vector<machine*> machines;

#if(RESOURCE_CONSTRAINTS)
	double *resalarm;
	double **proc_res_load_t;
#endif

	process_step();
	~process_step();
	process_step &operator=(const process_step &rhs);
};

#endif