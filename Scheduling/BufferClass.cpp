#include <iostream>
#include <ostream>
#include "BufferClass.h"
#include "OperationClass.h"
#include "Parameters.h"

using namespace std;

//Machine Class Methods
buffer::buffer() {
	ID = -1;
	actualID = -1;
	process_stepID = -1;
	actualprocess_stepID = -1;
	setup_time = 0;
	cleanup_time = 0;
	cleanup_frequency = 0;
	priority = 0;
	weight = 1;
	speed = 1;

	operations = vector<operation*>();

	operation *op1 = new operation();
	operation *op2 = new operation();

	op1->mthesi = 0;
	op2->mthesi = 1;
	op1->ID = -1;
	op2->ID = -1;
	op1->jobID = -1;
	op2->jobID = -1;
	op1->start = 0;
	op1->end = 0;
	op2->start = planhorend;
	op2->end = planhorend;

	operations.push_back(op1);
	operations.push_back(op2);

	setup_t = new int[NumberOfArticles];
	processing_t = new int[NumberOfArticles];
	cleanup_t = new int[NumberOfArticles];

	capacity = new double[NumberOfArticles];

	for (int i = 0; i<NumberOfArticles; i++) {
		setup_t[i] = 0;
		processing_t[i] = 0;
		cleanup_t[i] = 0;
		capacity[i] = 0;
	}

}


buffer::~buffer()
{
	operation *opfirst = operations[0];
	delete opfirst;
	vRemoveAt<operation*>(operations, 0);
	operation *oplast = operations[operations.size() - 1];
	delete oplast;
	operations.clear(); vector<operation*>().swap(operations);

	delete[] setup_t;
	delete[] processing_t;
	delete[] cleanup_t;

	delete[] capacity;

#if(RESOURCE_CONSTRAINTS)
	delete[] res_consum_setup;
	delete[] res_consum_processing;
	delete[] res_consum_cleanup;

	//Delete Resources
	for (int r = 0; r < NumberOfResources; r++) {
		delete[] mach_res_load_t[r];
		delete resources[r];
	}
	
	this->resources.clear();
	vector<resource*>().swap(resources);
	delete[] mach_res_load_t;
#endif

#if(MACHINE_AVAILABILITIES)
	for (int i = 0; i < unavailability.size(); i++) delete unavailability[i];
	unavailability.clear();
	vector<mach_unav*>().swap(unavailability);
#endif
}