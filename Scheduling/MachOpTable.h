#include "Parameters.h"


#ifndef MACHOPTABLE_H
#define MACHOPTABLE_H

struct MachOpTableEntry
{
	bool temp;
	bool perm;
};


class MachOpTable
{
public:
	bool is_temp;

	MachOpTableEntry ***table;

	MachOpTable();
	~MachOpTable();
	
	//Methods
	void temp_to_perm();
	void perm_to_temp();
	bool get_val(int op1UID, int op2UID, int mcID);
	void set_val(int op1UID, int op2UID, int mcID);
	void clear();
	void report();

};


#endif