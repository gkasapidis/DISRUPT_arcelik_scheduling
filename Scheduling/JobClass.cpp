#include "CObject.h"
#include "OperationClass.h"
#include "JobClass.h"
#include "Parameters.h"


//Job Class Methods

job::job() {
	ID = -1;
	actualID = -1;
	quantity = 0;
	lotSize = 1;
	lotFactor = 1;
	orderID = -1;
	rel = planhorstart;
	due = planhorend;
	weight = 1.0;
	saID = -1;
	done = false;
	operations = vector<operation*>();
};

job::~job()
{
	operations.clear();
	vec_del<operation*>(operations);
}

job::job(const job &copyin) {
	ID = copyin.ID;
	actualID = copyin.actualID;
	orderID = copyin.orderID;
	done = copyin.done;
	quantity = copyin.quantity;
	lotSize = copyin.lotSize;
	lotFactor = copyin.lotFactor;
	rel = copyin.rel;
	due = copyin.due;
	saID = copyin.saID;

	for (int i = 0; i<operations.size(); i++) delete operations[i];
	operations.clear();

	for (int i = 0, i_end = copyin.operations.size(); i<i_end; ++i)
	{
		operation *oporig = copyin.operations[i];
		operation *opdest = new operation();
		*opdest = *oporig;
		operations.push_back(opdest);
	}
}

bool job::operator==(const job &rhs){
	if ((this->ID != rhs.ID) && (this->due != rhs.due)) return false;
	return true;
}

bool job::operator!=(const job &rhs){
	if (*this == rhs) return false;
	return true;
}

job& job::operator=(const job &rhs) {

	this->due = rhs.due;
	this->rel = rhs.rel;
	this->weight = rhs.weight;
	this->done = rhs.done;
	this->ID = rhs.ID;
	this->actualID = rhs.actualID;
	this->quantity = rhs.quantity;
	this->lotSize = rhs.lotSize;
	this->lotFactor = rhs.lotFactor;
	this->orderID = rhs.orderID;

	//this->move_freq = rhs.move_freq; Try not to copy it over so that every time
	//the solution is copied this counter resets to zero

	for (int i = 0; i<this->operations.size(); i++) delete this->operations[i];
	this->operations.clear();


	return *this;
}


void job::fixTimes() {
	for (int i = 0; i < (int) this->operations.size(); i++) {
		operation* op = this->operations[i];
		if (op->mc != NULL) op->fixTimes(true); //Finalize fixes
	}
}

ostream& operator<<(ostream &output, const job &jb) {
	output << jb.ID << " " << jb.orderID <<
           " " << jb.quantity << " " << jb.rel << " " << jb.due <<
           " " << jb.lotFactor << " ";

    if (!jb.operations.empty())
        output << jb.operations[0]->process_stepID << endl;

    return output;
}

