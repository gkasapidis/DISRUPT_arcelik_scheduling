#include <ctime>
#include <cassert>
#include <iostream>
#include <ostream>
#include <fstream>
#include <string>
#include <cstring>
#include <cmath>       /* fabs */
#include <cstdlib>
#include <unistd.h>
#include <cstdio>
#include <vector>
#include <deque>
#include <cstdint>
#include <algorithm>
#include <random>
#include <unordered_map>
#include "console_colors.h"
#include <tuple>
#include <unordered_set>
#include "hash_tuple.h"
#include <ctime>

//Define Problem Class
#define JSS     1
#define FLEX    1
#define PFSS    0

//Define Operational Realties
#define CHANGEOVERTIMES         1

#define IDENTICAL_MACHINES      0
#define UNIFORM_MACHINES        0
#define UNRELATED_MACHINES      1 

#define RESOURCE_CONSTRAINTS    0
#define MACHINE_AVAILABILITIES  0
#define LINE_CONSTRAINTS		0
#define COMPLEX_PRECEDENCE_CONSTRAINTS 0

//Due Date FLAGS
#define REL_DUE_DATES_DATASET   0 //If this flag is set, due dates are read from the dataset
#define REL_DUE_DATES_CUSTOM    0 //If this flag is set, due dates are generated randomly in readFlexible
#define HARD_DUE_DATES			0 //If this flag is set, the algorithm quits when due dates are violated
								  //only warnings are issued otherwise	


#define DYNAMIC_TENURE 0 		//Algorithm adaptively modifies the tabu list size if enabled
#define PERTURBATION_METHOD 2   //0: Random job all ops, 1: Strength based job prioritized random ops
								//2: Strength based random ops, random reconstruction, 3: Strength based, neglected ops (move_freq)
                                //4: LS Perturb (does nothing for now)
#define DISTANCE_METRIC	1 		//Distance metric sets the mode to be used for calculating distances between solutions
						  		//0: Humming Single, 1: Humming Pairs, 2: Distance Matrices, 3: Distance from common ops in freq map
						  		//4: Seldomness, 5: Distance from ref set
#define WEIGHT_METRIC 1 		//Weight metric sets the way solution weights are going to be selected.
								//0: Based on solution costs, 1: Based on solution distances
#define TIE_BREAKER 0 			//0: Nothing, 1: humDistanceSingleTimes, 2: LocalSlack/CriticalOperations
                                //3: humDistanceSlots, 4: Seldomness, 5: NumberOfCriticalOps


//Define Estimates for Allocation [DEPRECATED]
#define MAX_OPERATIONS 2000
#define MAX_MACHINES 200
#define MAX_JOB_RELATIONS 3


//GLOBAL ENUMS

#ifndef GLOBAL_ENUMS
#define GLOBAL_ENUMS

//enum MachineTypes{mSMT = 1, mSMT_SPEC, mTHT, mMANUAL_INSERTION,
//    mFLUX, mPOTA_SOLTEC, mMASKING, mSAKI, mSAKI_OP,
//    mARVISION, mICT, mFT, mTAMIR, mPACKAGING, MACHINETYPECOUNT};


//Warning: This enum follows the DB definitions for the machine types. DO NOT CHANGE if the DB has not been updated
enum MachineTypes{mTHT = 1, mPOTA_SOLTEC, mREPAIR, mSMT, mFLUX,
    mMASKING, mPACKAGING, mICT, mARVISION, mFT, mSAKI, mMANUAL_INSERTION, MACHINETYPECOUNT};


enum execMachineTypes{exmSMT = 1, exmTHT, exmMANUAL_INSERTION,
    exmFLUX, exmPOTA_SOLTEC, exmMASKING, exmSAKI,
    exmARVISION, exmICT, exmFT, exmREPAIR, exmPACKAGING, exMACHINETYPECOUNT};


enum BoardSide{SIDE_DEFAULT = 0, SIDE_TOP, SIDE_BOTTOM, SIDE_COUNT};

#endif
//*************************************************************************************************************
//Database connection parameters, global variables & Related Functions


#ifndef PARAMETER_H
#define PARAMETER_H

int strlen(char s[]);
void reverse(char s[]);
void itoa(int n, char s[]);
double randd();

//int min(int a, int b);
//int max(int a, int b);

#endif

//*************************************************************************************************************
//*************************************************************************************************************
// Optimization tolerances
//const double TOL_INT = 1.E-5;		// Integrality tolerance
//const double TOL_FEA = 1.E-6;		// Feasibility tolerance
//const double TOL_OPT = 1.E-3;		// Optimality  tolerance
//
//const double PI = 3.14159265359;  //Pi 
//const double TINY = 1.E-10;		    // Tiny number
//const int    INF = 1000000000;		// Infinity

//#define INF 100000000000
#define INF 2147483647
#define TOL_INT 1.E-5
#define TOL_FEA 1.E-6
#define TOL_OPT 1.E-3
#define SEED	4987924

#ifndef PARAM_FUNCS
#define PARAM_FUNCS

inline double my_round(const double val )
{
    int prec = 5;

    double nval = val;
    double prec_fac;
    while (prec >= 0){
        prec_fac = pow(10, prec);
        nval = nval * prec_fac;

        if( val < 0 )
            nval = ceil(nval - 0.5);
        else
            nval = floor(nval + 0.5);
        nval /= prec_fac;
        prec--;
    }

    return nval;
}

template <typename T>
inline T clamp(T a, T b, T c){
	return std::max(b, std::min(a, c));
}

template <typename T>
inline void vec_del(std::vector<T> a) {
	std::vector<T>().swap(a);
};

template <typename T>
inline void vRemoveAt(std::vector<T> &a, int index) {
	a.erase(a.begin() + index);
};

template <typename T>
inline void vInsertAt(std::vector<T> &a, int index, T &val) {
	a.insert(a.begin() + index, val);
};

template <typename T>
inline bool vContains(std::vector<T> &a, T &val) {
	return (std::find(a.begin(), a.end(), val) != a.end());
};

#endif

#ifndef PARAM_STRUCTS
#define PARAM_STRUCTS

//Main Common structs/classes
//Solver Options
class search_options {
public:
    int ms_iters;
    bool strategy;
    int time_limit;

    //Default Constructor
    search_options(){
        ms_iters = -100;
        strategy = false;
        time_limit = 1;
    }

    search_options(int a, bool b, int c){
        ms_iters = a;
        strategy = b;
        time_limit = c;
    }
};

#endif


//Common Typedefs
typedef std::pair<int, int> int_pair;
typedef std::tuple<int, int, int> int3_tuple;
typedef std::tuple<int, int, int, int> int4_tuple;
typedef std::tuple<int, int, int, int, int> int5_tuple;

extern std::string benchfile;


//Global parameters defined from main arguments
extern int jobID;
extern int eventCount;
extern int* eventIDs;
extern int executionMode;
extern int scheduleCount;
extern int alternativeActionID;
extern std::string reschedulingTime;
extern std::string productionStartTime;
extern std::string productionEndTime;
extern struct tm PlanningHorizonStart_Minus;
extern struct tm PlanningHorizonStart;
extern struct tm ReschedulingTime;
extern struct tm PlanningHorizonEnd;
extern int scopetype;
extern int seed;

extern int optimtype;

extern int procID;
extern int OLDexecutionID;

extern time_t now;
extern double time_elapsed;

extern long planhorstart;
extern long planhorend;
extern int reopt_time;

extern int NumberOfOrders;
extern int NumberOfJobs;
extern int NumberOfMachines;
extern int NumberOfOperations;
extern int NumberOfProcSteps;
extern int NumberOfResources;
extern int NumberOfArticles;

//DISRUPT extra parameters
extern int LotSize;
extern int BlackBoxSize;

//Algorithm Parameters
extern int restart_iterations; //Number of restarts (Greedy Randomized Construction Heuristic - Tabu Search) (must be >= 1)
extern int tabu_iterations; //Number of internal tabu search iterations without observing improvement (must be >=1)
extern int tabu_tenure; //Tabu Move Life/Search History (must be between 12 and 60)
extern int tabu_list_size; //Tabu List Size (must be between 12 and 60)
extern float tabu_relaxation; //Relaxation Parameter for Tabu Status

//Pool Settings
extern int pool_size;
extern int pool_lifespan;


//Cost Calculation Flags
extern int MAKESPAN_F;
extern int TOTAL_FLOW_TIME_F;
extern int TOTAL_IDLE_TIME_F;
extern int TOTAL_ENERGY_F;
extern int TARDINESS_F;
extern int SLACK_F;
extern int DISTANCE_F;




#ifdef _WIN32
	#pragma warning( disable: 4996 4010 4018 )
#elif __linux__
	#define PRETTY_PRINT
    #pragma GCC diagnostic ignored "-Wunused-but-set-variable"
    #pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

