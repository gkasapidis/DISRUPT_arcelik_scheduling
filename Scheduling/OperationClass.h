#include <iostream>
#include <ostream>
#include "Parameters.h"
#include "MachineClass.h"
#include "CObject.h"
#include "Graph.h"

using namespace std;


#ifndef OPERATION_H
#define OPERATION_H


enum DFS_MODES {MARK_INFEASIBLE, MARK_MOVING, DFS_MODES_COUNT};

//Blacklist Pair
typedef pair<int, int> bl_pair;

class operation :public CObject, public Node
{

public:
	//Input
	int UID; //Unique ID used to identify the operation and index it on the graph as well
	int ID;
	int articlestepsequenceID;
	int opertypeID;
	int jobID;
	int actualprocess_stepID;
	int process_stepID;
	int prodphase;

	int setup_time;
	int cleanup_time;

#if(FLEX)
	int const_processing_time_UM; //Keep processing times that are not subject to parallel processors
    int *var_processing_time_UM; //Keep processing times that are subject to parallel processors
#else
	int processing_time;
#endif

	long a; //Release time as defined in JobID
    long b; //Due time as defined in JobID

	//Variables
	machine *mc;

	bool done;
	bool temp_done;
	long start; //or head
	long temp_start;
	long end;
	long temp_end;
	int machID;
	int temp_machID;
	int mthesi;
	int temp_mthesi;
	
	int tail;
	bool is_temp;
	bool inh_ok;
	int sca;
	int scb;

	float weight; //Adding this one for tardiness

	bool infeasible = false; //Used for ls moves
							 //Moving is converted to an int so that we can track tyhe times that the operation
							 //was marked as moving and be able to revert those passes
	int moving = 0; //Used for cutoff
					//CObArray *pred;

	int move_freq = 0; //Used for calculating operation movement frequencies during LS
	bool fixed = false; //Used for rescheduling
    int default_machID; //The default machine when the operation is fixed

    operation* pred[MAX_JOB_RELATIONS];
	//CObArray *suc;
	operation* suc[MAX_JOB_RELATIONS];

	//Keep track of the produced resource in a local level so that we can trace the time for produced quantities
	resource *prod_res;

	operation();
	~operation();
	operation(const operation &copyin);
	operation &operator=(const operation &rhs);
	bool operator==(const operation &rhs);
	bool operator!=(const operation &rhs);
	bool isEqual(operation *op);
	//Inheritance Queries
	bool has_preds();
	bool has_sucs();
	bool has_mach_sucs();
	bool has_mach_preds();
	bool is_suc(operation *op);
	bool is_pred(operation *op);
	bool is_mach_suc(operation *op);
	bool is_mach_pred(operation *op);
	operation* get_suc();
	operation* get_suc(int k);
	operation* get_pred();
	operation* get_pred(int k);

    operation* get_mach_suc();
	operation* get_mach_pred();

	bool check_sucRelation(operation *op);
	bool check_predRelation(operation *op);
	bool recursiveCheck(operation* next, int level);
	
	//Getters
	int getProcTime() const;
	int getProcTime(machine *mc) const;
	int getStartTime() const;
	int getEndTime() const;
    void resetProcessingTimes();
    void report();

    //Setters
	void setStartTime(int value);
	void setEndTime(int value);
    void setInfeasible();
    void setMoving();

	bool fixTimes(bool finalize);
	void changeMach(machine *mc, int mach_pos);
	void storeTemp();
	void storePerm();
	void resetDone();
	void setDone();
	void setDoneStatus(bool temp, bool status);
	bool isDone();

	//GraphNode Methods and Properties
	int *g_state;
	
	//Node methods
	bool isConnected(operation *node); //This should be called preferably
	bool f_isConnected(operation *node);
	bool b_isConnected(operation *node);

	bool connect(operation *node, bool temp, int type); //This should be called preferably
	bool f_connect(operation *node, bool temp, int type);
	bool b_connect(operation *node, bool temp, int type);
	
	bool disconnect(operation *node);
	
	void removeConnection(int i); //This should be called preferably
	void f_removeConnection(int i);
	void b_removeConnection(int i);

	//void visit(Graph<operation> *gr, int state, bool *stack, bool &cycleflag, operation ** &visitedNodes, int &visitedNodes_n);
	void visit(Graph<operation> *gr);

};

typedef pair<operation*, int> op_conn_pair;

#endif


