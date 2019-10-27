#include "MachOpTable.h"


//Constructor
MachOpTable::MachOpTable() {
	is_temp = false;
	
	table = (MachOpTableEntry***) malloc(sizeof(MachOpTableEntry**) * MAX_MACHINES);
	for (int i=0;i<MAX_MACHINES;i++){
		MachOpTableEntry** mach_table = (MachOpTableEntry**) malloc(sizeof(MachOpTableEntry*) * MAX_OPERATIONS);
		for (int j=0;j<MAX_OPERATIONS;j++){
			MachOpTableEntry* row = (MachOpTableEntry*) calloc(MAX_OPERATIONS, sizeof(MachOpTableEntry));
			mach_table[j] = row;
		}
		table[i] = mach_table;
	}
}

//Deconstructor
MachOpTable::~MachOpTable() {

	for (int i=0;i<MAX_MACHINES;i++){
		MachOpTableEntry** mach_table = table[i];
		for (int j=0;j<MAX_OPERATIONS;j++)
			delete mach_table[j];
		delete mach_table;
	}
	delete table;
}

//Report
void MachOpTable::report() {
	for (int k=0;k<MAX_MACHINES;k++){
		MachOpTableEntry** mach_table = table[k];
		printf("Machine %d Table\n", k);
		for (int i=0;i<MAX_OPERATIONS;i++){
			MachOpTableEntry* row = mach_table[i];
			for (int j=0;j<MAX_OPERATIONS;j++){
				if (is_temp)
					printf("%d ", row[j].temp);
				else
					printf("%d ", row[j].perm);
			}
			printf("\n");
		}
	}
}

//Clear
void MachOpTable::clear() {
	for (int k=0;k<MAX_MACHINES;k++){
		MachOpTableEntry** mach_table = table[k];
		for (int i=0;i<MAX_OPERATIONS;i++){
			MachOpTableEntry* row = mach_table[i];
			memset(row, 0, sizeof(MachOpTableEntry) * MAX_OPERATIONS);
		}
	}
}

//Converters
void MachOpTable::perm_to_temp() {
	for (int k=0;k<MAX_MACHINES;k++){
		MachOpTableEntry** mach_table = table[k];
		for (int i=0;i<MAX_OPERATIONS;i++){
			MachOpTableEntry* row = mach_table[i];
			for (int j=0;j<MAX_OPERATIONS;j++)
				row[j].temp = row[j].perm;
		}
	}
}

//Getter
bool MachOpTable::get_val(int op1UID, int op2UID, int mcID) {
	if (is_temp)
		return table[mcID][op1UID][op2UID].temp;
	else
		return table[mcID][op1UID][op2UID].perm;
}
//Setter
void MachOpTable::set_val(int op1UID, int op2UID, int mcID) {
	if (is_temp)
		table[mcID][op1UID][op2UID].temp = 1;
	else
		table[mcID][op1UID][op2UID].perm = 1;
}