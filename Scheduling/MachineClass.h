#include <iostream>
#include <ostream>
#include "Parameters.h"
#include "CObject.h"
#include "ListObject.h"
#include "MachineUnav.h"
#include "Resource.h"



#ifndef MACHINE_H
#define MACHINE_H

using namespace std;
class operation;
class ProductionLine;
class machine : public listObject<machine>
{
public:
	int ID;
	int actualID;
	int actualprocess_stepID;
	int process_stepID; //This is bound to the line type ID
	//int typeID; //This holds the machine type ID
	ProductionLine *line;
    vector<int> embeddedMachineIDs; //This array holds the sub-machine IDs that are included in this machine

	int setup_time;
	int cleanup_time;
	int cleanup_frequency;

	int priority;
	int weight;

	int *setup_t;
	int *processing_t;
	int *cleanup_t;


	double *capacity;

	double speed;
	int *parallel_processors; //Array that stores the parallel processors of a machine per process

#if(RESOURCE_CONSTRAINTS)
	double *res_consum_setup;
	double *res_consum_processing;
	double *res_consum_cleanup;
	double **mach_res_load_t;
	//CObArray *resources;
	vector<resource*> resources;
#endif

    vector<mach_unav*> unavailability;
    bool checkAvail(operation* op);
    void applyUnavWindow(mach_unav* w);

	//CObArray *operations;
	vector<operation*> operations;

	machine();
	~machine();
	machine(const machine &copyin);
	machine &operator=(const machine &rhs);
	bool operator==(const machine &rhs);
	bool operator!=(const machine &rhs);
	friend ostream &operator<<(ostream &output, const machine &mc);
	int containsOP(operation* op);
	bool fixTimes();
	void setDoneStatus(bool temp, bool status);
	bool isEqual(machine *mc);
	
	//Operation Related Methos
	void insertOp(operation* op, int index);
	void removeOp(int index);


	//Resources Report
	void reportResources();

	//Testing
	void memTest();

};

#endif