#ifndef SOLUTION_H
#define SOLUTION_H

#include <iostream>
#include <ostream>
#include <map>
#include <vector>
#include "PoolItemClass.h"
#include "StageClass.h"
#include "MachOpTable.h"
#include "ProcessStep.h"
#include "OperationClass.h"
#include "JobClass.h"
#include "DataClasses/ProblemDataset.h"
#include "Parameters.h"
#include "Graph.h"


//Cost enumeration
enum costs { MAKESPAN, TOTAL_FLOW_TIME, TOTAL_IDLE_TIME, TOTAL_ENERGY, TARDINESS, SLACK, DISTANCE, COST_NUM};

using namespace std;

//Class candidate 
class Candidate: public CObject {
public:
	operation* op;
	machine* mc;
	int pos;
	float span;
	int h_index;
	float fitness;

	~Candidate() { }
};


//Define DGRAPH
class DGraph : public Graph<operation> {
	/*
	This graph is a parent class of the main Graph class
	This extends the main one by providing path tracing functions
	specifically used in FJSSP
	*/

	void calcLateTimes();

};



//class ostream;
class solution : public CObject, public PoolItem //Don't remove the base class PFSS doens't work and I'm too lazy to fix this shit
{
public:

#if(PFSS)
	int *int_permutation;
	double *double_permutation;
	bool *intdouble_convertion;
	int **completion_time;

	int ranking;
	int dominated;
	int distance;
#endif
	//Costs
	float *cost_array;
	float *temp_cost_array;
	//Cost trend
	float *d_cost_array;
	float *temp_d_cost_array;

	//int cost;
	//int s_cost;
	//int t_cost;
	//int e_cost;
	//int tard_cost;
	//int slack;
	
	vector<job*> soljobs;
	vector<operation*> soloperations;
	vector<machine*> solmachines;

	//Keep reference of the problem dataset
	ProblemDataset *dataset;

    //New Machine Representation with Boolean table
	MachOpTable *mach_table;

	//vector<job*> jobshelp;
	vector<operation*> opshelp;
    vector<operation*> fixed_ops;
	
	vector<stage*> Stages;
	vector<process_step*> ProcSteps;

	//Index resources by name
	unordered_map<string, resource*> Resources;

	map<string, int> chovertime;

	//Solution Graphs
	Graph<operation>* FGraph; 
	Graph<operation>* BGraph;

    //Critical Path Related attributes
    vector<int3_tuple> criticalBlocks;
	
	solution();
	~solution();
	solution &operator=(const solution &rhs);
	bool operator==(const solution &rhs);
	friend ostream& operator<<(ostream &output, const solution &ss);

	//Saved Values
	int totalNumOp;


	//Basic setters/getters
	void setDataset(ProblemDataset *ds){
	    dataset = ds;
	}

    //Cost Functions
	void getCost(bool temp); //Calculates all costs
	void getCost(int costID, bool temp); //Calculate specific cost
	int calculate_makespan(int scenario, bool temp);
	int calculate_Tidle(int scenario, bool temp);
	int calculate_Tardiness(int scenario, bool temp);
	int calculate_SlackTime(int scenario, bool temp);
	int calculate_localSlack(int temp);
	int calculate_TFT(int scenario, bool temp);
	int calculate_Tenergy(int scenarion, bool temp);
	void reportCosts(bool temp);
	
	//Solution Construction
	bool greedyConstruction(bool test);
	bool aeapConstruction(int mode, vector<bl_pair> &blacklist, int objective);
    bool constructBaseSchedule();
    void freezeSchedule(long freezetime);
    void splitJobs(event* e);
	bool aeapConstruction_perOp();
	bool randomCandidateConstruction(vector<bl_pair> &blacklist);
	void resetSolution();
	void resetOperationUIDs();
	void fillMachTable();


	//Solution Comparison
	bool compareSolution(const solution* sol);
	void memTestSolution();
	void cleanUpSolution();
	void saveToFile(const char *filename);
	void savetoStream(ostringstream& outstream);
	
	//Static objective comparison
	static bool Objective_TS_Comparison(const solution* s1, const solution* s2, bool temp);
	static bool Objective_TS_Comparison(float* cost_array, float* mcost_array);

	//Time Manipulation
	bool scheduleAEAP(bool temp); //Minimalisic version
	bool scheduleAEAP(bool temp, bool override_status); //Force operation update status
	bool scheduleAEAP_BAK(bool temp, int cutoff_time, bool override_status);
	bool fixTimes();
	void replace_temp_values();
	void replace_real_values();
	void setDoneStatus(bool temp, bool status);
	void resetInheritanceCheck();
	void update_horizon();
	int changeovertime(machine *mc, operation *op1, operation *op2);

	//Validation
	void GetCounts();
	int getDoneOps();
	int getRemainingOps();
	bool find_conflicts(bool temp);
	bool testsolution();
	
	//RC Related
    void Setup_Resources();
	void Init_Resourse_Loads();
	void Calculate_Resourse_Loads();
	void Init_Resourse_Loads_mecutoff(int cutoff);
	void Update_Resourse_Loads(bool temp);
	void Update_Resourse_Loads_perOp(const operation *op, bool temp);
	void reportResources();
	void reportResourcesConsole();
	//Extra Job addition - THIS METHOD SHOULD BE REMOVED FROM THE FACE OF THIS WORLD
	//bool InsertOneAdditionalNew(job *jb);


	//Solution Manipulation Methods
	void insertOp(machine* mc, int position, operation* op);
	void insertOp_noGraphUpdate(machine* mc, int position, operation* op);
	void insertOpTable(machine* mc, int position, operation* op);
	void removeOp(machine* mc, int position);
	void removeOp_noGraphUpdate(machine* mc, int position);

    //Graph Methods
	void createGraphs();
	void insertOpToGraphs(operation* op);
	void insertOpToForwardGraph(operation* op);
	void insertOpToBackwardGraph(operation* op);
	
	void removeOpFromGraphs(operation* op);
	void removeOpFromForwardGraph(operation* op);
	void removeOpFromBackwardGraph(operation* op);
	

	//Distance Methods
	static int humDistance_single(solution* s1, solution* s2);
	static int humDistance_single_times(solution* s1, solution* s2);
	static int humDistance_self_single_times(solution* s1);
	static int humDistance_pairs(solution* s1, solution* s2);
	

	static float calcDistance(solution* s1, solution* s2, const int MODE, bool temp);
	void createDistanceMatrix();
	void findCriticalPath();
	void findCriticalPath_viaSlack();
	void findCriticalBlocks(int mode);
	int  numberOfCriticalOps();
};

#endif