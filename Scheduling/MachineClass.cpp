#include <iostream>
#include <ostream>
#include "MachineUnav.h"
#include "MachineClass.h"
#include "OperationClass.h"
#include "DataClasses/ProductionLine.h"

using namespace std;

//Machine Class Methods
machine::machine() {
	ID = -1;
	actualID = -1;
	process_stepID = -1;
	actualprocess_stepID = -1;
	parallel_processors = new int[1 + MACHINETYPECOUNT];
    embeddedMachineIDs = vector<int>();

	for (int i=0;i<MACHINETYPECOUNT;i++) parallel_processors[i] = 0;
	//typeID = -1;
	line = nullptr;
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
	op1->temp_mthesi = 0;
	op2->mthesi = 1;
	op2->temp_mthesi = 1;
	
	//op2->mthesi = 1;
	op1->ID = -1;
	op2->ID = -1;
	op1->jobID = -1;
	op2->jobID = -2;
	
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

	for (int i = 0; i < NumberOfArticles; i++) {
		setup_t[i] = 0;
		processing_t[i] = 0;
		cleanup_t[i] = 0;
		capacity[i] = 0;
	}

	#if(RESOURCE_CONSTRAINTS)
	res_consum_setup = new double[NumberOfResources];
	res_consum_processing = new double[NumberOfResources];
	res_consum_cleanup = new double[NumberOfResources];

	//Init Array of lists that will host the resource blocks
	resources = vector<resource*>();

	for (int i = 0; i < NumberOfResources; i++) {
		res_consum_setup[i] = 0;
		res_consum_processing[i] = 0;
		res_consum_cleanup[i] = 0;
		//Init all resource blocks
		resources.push_back(new resource());
	}

	mach_res_load_t = new double*[NumberOfResources];
	for (int r = 0; r < NumberOfResources; r++) {
		mach_res_load_t[r] = new double[planhorend];
	}

	for (int r = 0; r < NumberOfResources; r++) {
		for (int t = 0; t < planhorend; t++) {
			mach_res_load_t[r][t] = 0;
		}
	}
	#endif

	#if(MACHINE_AVAILABILITIES)
	//unavailability = new CObArray();
	unavailability = vector<mach_unav*>();

	#endif

}



#if(MACHINE_AVAILABILITIES)
bool machine::checkAvail(operation* op) {
	int mc_setup = op->setup_time;
	int mc_cleanup = op->cleanup_time;

	for (int u = 0; u < unavailability.size(); u++)
	{
		mach_unav *muv = unavailability[u];
		if (get<1>(op->end_times) <= muv->start_unav) continue;
		if ((get<1>(op->end_times) > muv->start_unav && get<1>(op->end_times) <= muv->end_unav) || (op->temp_start >= muv->start_unav && op->temp_start <= muv->end_unav) || (muv->start_unav >= op->temp_start && muv->end_unav <= get<1>(op->end_times)))
		{
			op->temp_start = muv->end_unav;
			get<1>(op->end_times) = op->temp_start + mc_setup + op->processing_time_UM[ID] + mc_cleanup;//mc->speed*op->processing_time
			continue;
		}
	}

	return true;
}
#endif


machine::~machine()
{
	operations.clear(); vector<operation*>().swap(operations);
	delete[] parallel_processors;
	delete[] setup_t;
	delete[] processing_t;
	delete[] cleanup_t;
    delete[] capacity;
    embeddedMachineIDs.clear();

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

	//Unavailabilities are stored in the corresponding line so there is no reason to delete it here
    unavailability.clear();
    vec_del<mach_unav*>(unavailability);
}

machine::machine(const machine &copyin) {
	ID = copyin.ID;
	actualID = copyin.actualID;
	process_stepID = copyin.process_stepID;
	actualprocess_stepID = copyin.actualprocess_stepID;
	//typeID = copyin.typeID;
	line = copyin.line;
    setup_time = copyin.setup_time;
	cleanup_time = copyin.cleanup_time;
	cleanup_frequency = copyin.cleanup_frequency;
	priority = copyin.priority;
	weight = copyin.weight;
	speed = copyin.speed;
	for (int i=0;i<MACHINETYPECOUNT;i++)
    	parallel_processors[i] = copyin.parallel_processors[i];

	for (int i = 0; i< (int) operations.size(); i++) delete operations[i];
	operations.clear();

	for (int i=0;i<copyin.embeddedMachineIDs.size();i++)
	    embeddedMachineIDs.push_back(copyin.embeddedMachineIDs[i]);


	for (int i = 0, i_end = (int) copyin.operations.size(); i<i_end; ++i)
	{
		operation *oporig = copyin.operations[i];
		operation *opdest = new operation();
		*opdest = *oporig;
		operations.push_back(opdest);
	}

	for (int i = 0; i<NumberOfArticles; i++) {
		setup_t[i] = copyin.setup_t[i];
		processing_t[i] = copyin.processing_t[i];
		cleanup_t[i] = copyin.cleanup_t[i];
		capacity[i] = copyin.capacity[i];
	}

#if(RESOURCE_CONSTRAINTS)
	for (int i = 0; i<NumberOfResources; i++) {
		res_consum_setup[i] = copyin.res_consum_setup[i];
		res_consum_processing[i] = copyin.res_consum_processing[i];
		res_consum_cleanup[i] = copyin.res_consum_cleanup[i];
	}
#endif

	unavailability.clear();
	for (int i = 0; i<copyin.unavailability.size(); ++i)
	{
		mach_unav *mcuv_orig = copyin.unavailability[i];
		unavailability.push_back(mcuv_orig);
	}
}

machine& machine::operator=(const machine &rhs) {

	this->ID = rhs.ID;
	this->actualID = rhs.actualID;
	this->process_stepID = rhs.process_stepID;
	this->actualprocess_stepID = rhs.actualprocess_stepID;
	//this->typeID = rhs.typeID;
	this->line = rhs.line;
	this->setup_time = rhs.setup_time;
	this->cleanup_time = rhs.cleanup_time;
	this->cleanup_frequency = rhs.cleanup_frequency;
	this->priority = rhs.priority;
	this->weight = rhs.weight;
	this->speed = rhs.speed;
	for (int i=0;i<MACHINETYPECOUNT;i++)
		parallel_processors[i] = rhs.parallel_processors[i];

	//This is probably handled from the solution operator=
	/*
	for (int i = 0; i< (int) this->operations.size(); i++) delete this->operations[i];
	this->operations.clear();

	for (int i = 0, i_end = (int)rhs.operations.size(); i<i_end; ++i)
	{
		operation *oporig = rhs.operations[i];
		operation *opdest = new operation();
		*opdest = *oporig;
		this->operations.push_back(opdest);
	}
	*/

    for (int i=0; i<rhs.embeddedMachineIDs.size(); i++)
        embeddedMachineIDs.push_back(rhs.embeddedMachineIDs[i]);

	for (int i = 0; i<NumberOfArticles; i++) {
		this->setup_t[i] = rhs.setup_t[i];
		this->processing_t[i] = rhs.processing_t[i];
		this->cleanup_t[i] = rhs.cleanup_t[i];
		this->capacity[i] = rhs.capacity[i];
	}

#if(RESOURCE_CONSTRAINTS)
	for (int i = 0; i < NumberOfResources; i++) {
		delete resources[i];
	}
	resources.clear();

	for (int i = 0; i<NumberOfResources; i++) {
		this->res_consum_setup[i] = rhs.res_consum_setup[i];
		this->res_consum_processing[i] = rhs.res_consum_processing[i];
		this->res_consum_cleanup[i] = rhs.res_consum_cleanup[i];

		//Copy resources
		resource * res = rhs.resources[i];
		resource *new_res = new resource();

		*new_res = *res;
		this->resources.push_back(new_res);
	}
#endif

	unavailability.clear();
	for (int i = 0; i<rhs.unavailability.size(); ++i)
	{
		mach_unav *mcuv_orig = rhs.unavailability[i];
		unavailability.push_back(mcuv_orig);
	}

	return *this;
}

bool machine::operator==(const machine &mc){
	//Step 1 check ID
	if (this->ID != mc.ID) return false;
	//Step 2 check process ID
	if (this->process_stepID != mc.process_stepID) return false;
	//Step 3 check operations
	if (this->operations.size() != mc.operations.size()) return false;
	//Step 4 check operations individually
	for (int i=0; i<this->operations.size(); i++){
		operation *op = this->operations[i];
		operation *rh_op = mc.operations[i];
		if (*op != *rh_op) return false;
	}
	return true;
}

bool machine::operator!=(const machine &mc){
	if (*this == mc) return false;
	return true;
}


ostream& operator<<(ostream &output, const machine &mc) {
	char s[500]; //Output placeholder
#if(RESOURCE_CONSTRAINTS)
	sprintf(s, "Machine ID: %d ACTUAL ID: %d RESOURCE_0 '%2f / %2f' \n", mc.ID, mc.actualID, mc.res_consum_processing[0]);
#else
	sprintf(s, "Machine ID: %d ACTUAL ID: %d LINE ID %d PROCESS STEP: %d \n", mc.ID, mc.actualID, mc.line->ID, mc.process_stepID);
#endif
	output << s;
	
#ifdef DEBUG
	sprintf(s, "\t %-5s | %-5s | %-4s | %-6s | %-5s | %-5s | %-5s | %-5s | %-4s | %-7s | %-10s | %-10s | %-10s | %-12s | \n",
		         "JobID", "OpID","UID", "weight", "a",  "b", "Start", "End", "PT", "Status (T-P)", "Position", "Address", "HasPreds", "PredAddress");
#else
	sprintf(s, "\t %-5s | %-5s | %-4s | %-6s | %-6s | %-10s | %-10s | %-10s | %-10s | %-10s | %-6s | %-7s | %-10s | %-10s |\n",
		         "JobID", "OpID","UID", "Fixed", "Process", "a",  "b", "Start", "Tail", "End", "PT", "Status (T-P)", "Position", "HasPreds");
#endif
	output << s;
	for (int h = 1; h < (int) mc.operations.size()-1; h++)
	{
		operation *op = mc.operations[h];

#ifdef DEBUG
	#if(FLEX)
			//output << op->ID << "\t" << op->jobID << "\t" << op->a << "\t" << op->b << "\t" << op->start << "\t" << op->end << "\t" << op->processing_time_UM[mc.ID] << endl;
			sprintf(s, "\t %5d | %5d | %4d | %6.3f | %5d | %5d | %5d | %5d | %4d | %3d   -  %3d | %10d | %10ld | %10d | %12ld | \n", op->jobID, op->ID, op->UID, op->weight, op->a, op->b, ((op->done) ? op->start : op->temp_start), ((op->done) ? op->end : op->temp_end), op->processing_time_UM[op->process_stepID], op->temp_done, op->done, op->mthesi, op,
																				((op->pred[0] != NULL )? 1:0), (op->pred[0] != NULL) ? op->pred[0] : 0);
	#else
			//TODO: Fix the output for non Flex Problems
			sprintf(s, "\t %5d | %5d | %4d | %5d | %5d | %5d | %5d | %10d |\n", op->ID, op->jobID, op->a, op->b, op->start, op->end, op->processing_time);
			
	#endif
#else
	#if(FLEX)
            //output << op->ID << "\t" << op->jobID << "\t" << op->a << "\t" << op->b << "\t" << op->start << "\t" << op->end << "\t" << op->processing_time_UM[mc.ID] << endl;
			sprintf(s, "\t %5d | %5d | %4d | %6d | %7d | %10d | %10d | %10d | %10d | %10d | %6d | %3d   -  %3d | %10d | %10d | \n",
					op->jobID, op->ID, op->UID, op->fixed, op->process_stepID, op->a, op->b, ((op->done) ? op->start : op->temp_start), op->tail, ((op->done) ? op->end : op->temp_end),
			 op->getProcTime(), op->temp_done, op->done, op->mthesi, ((op->pred[0] !=NULL )? 1:0));
	#else
			//TODO: Fix the output for non Flex Problems
			sprintf(s, "\t %5d | %5d | %4d | %5d | %5d | %5d | %5d | %10d |\n", op->ID, op->jobID, op->a, op->b, op->start, op->end, op->processing_time);
	#endif	
	
#endif
	output << s;
}
	return output;
}

int machine::containsOP(operation* op) {
	for (int i = 1; i < (int) operations.size() - 1; i++) {
		operation *cand = operations[i];
		if (cand->isEqual(op)) return i;
	}
	return 0;
}

bool machine::fixTimes() {
	for (int i = 1; i < (int) this->operations.size()-1; i++) {
		operation *op = this->operations[i];
		if (op->done) continue;
		if (!op->fixTimes(true)) return false; //Fix time in op and finalize
	}
	return true;
}

void machine::setDoneStatus(bool temp, bool status) {
	for (int i = 1; i <(int) this->operations.size() - 1; i++) {
		operation *op = this->operations[i];
        op->setDoneStatus(temp, status); //Reset Done Status
	}
}

void machine::insertOp(operation* op, int index) {
	
	//Updating positions of the following ops
	for (int i = index; i < (int) this->operations.size(); i++) {
		operation *cop = this->operations[i];
		cop->mthesi += 1;
		cop->temp_mthesi += 1;
	}

	//Inserting the operation in the list
	//this->operations->InsertAt(index, op);
	//this->operations.insert(operations.begin() + index, op);
	vInsertAt<operation*>(operations, index, op);
}


void machine::removeOp(int index) {
	
	//Unassign machine from removed op
	operation *rop = this->operations[index];
	rop->mc = NULL;
	rop->mthesi = -1;
	rop->temp_mthesi = -1;
	rop->is_temp = true;
	rop->done = false;
	rop->temp_done = false;
	
	//this->operations->RemoveAt(index);
	vRemoveAt<operation*>(this->operations, index);
	//Fix positions of the following ops
	for (int i = index; i < (int) this->operations.size(); i++) {
		operation *cop = this->operations[i];
		cop->mthesi -= 1;
		cop->temp_mthesi -= 1;
	}
	
}

void machine::reportResources() {
#if(RESOURCE_CONSTRAINTS)
	char filename[200];

	for (int i = 0; i < this->resources.size(); i++) {
		resource *res = this->resources[i];
		sprintf(filename, "MachineID_%d_ResID_%d_consumption.txt", this->ID, res->ID);
		res->export_to_txt(filename);
	}

#endif
}

void machine::memTest() {
	cout << "Iterating in Machine Operations" << endl;
		for (int j = 1; j < (int) this->operations.size()-1; j++) {
			operation *op = this->operations[j];
			cout << "Machine Position: " << op->mthesi << " Op Job: " << op->jobID << " Op ID " << op->ID << " Mem Address " << op << " Op MC Accress" << op->mc << endl;
		}
}

bool machine::isEqual(machine *mc) {
	//THis function is a stripped down version of the == operator
	//It is used just to identify that we're talking about the same machine
	//Same ID, same process Step

	//Step 1 check ID
	if (this->ID != mc->ID) return false;
	//Step 2 check process ID
	if (this->process_stepID != mc->process_stepID) return false;

	return true;
}

void machine::applyUnavWindow(mach_unav *w){
    unavailability.push_back(w);
}