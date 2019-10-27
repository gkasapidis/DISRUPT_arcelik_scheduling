#include "ProcessStep.h"
#include "MachineClass.h"

//Process Step
process_step::process_step() {
	ID = -1;
	processID = -1;
	actualID = -1;

	machines = vector<machine*>();

#if(RESOURCE_CONSTRAINTS)
	resalarm = new double[NumberOfResources];
	proc_res_load_t = new double*[NumberOfResources];
	for (int r = 0; r<NumberOfResources; r++) {
		proc_res_load_t[r] = new double[planhorend];
	}

	for (int r = 0; r<NumberOfResources; r++) {
		for (int t = 0; t<planhorend; t++) {
			proc_res_load_t[r][t] = 0;
		}
	}
#endif
}

process_step::~process_step() {
	machines.clear();
	vector<machine*>().swap(machines);

#if(RESOURCE_CONSTRAINTS)
	delete[] resalarm;
	for (int r = 0; r < NumberOfResources; r++) delete[] proc_res_load_t[r];
	delete[] proc_res_load_t;
#endif
}

//Assingment operator
process_step& process_step::operator=(const process_step &rhs) {
	this->ID = rhs.ID;
	this->actualID = rhs.actualID;
	this->processID = rhs.processID;

	for (int i = 0; i < (int)this->machines.size(); i++) delete this->machines[i];
	this->machines.clear();

	for (int i = 0; i <(int) rhs.machines.size(); i++) {
		//machine *mc = rhs.mach_avail[i];
		//machine *mc_this = new machine();
		//*mc_this = *mc;

		machine *mc = rhs.machines[i];
		machine *mc_this = new machine();
		*mc_this = *mc;

		this->machines.push_back(mc_this);
	}
	return *this;
}

