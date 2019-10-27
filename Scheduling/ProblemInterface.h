#include "CObject.h"
#include "SolutionClass.h"
#include "LSSolutionClass.h"

#ifndef PGLOBALS_H
#define PGLOBALS_H

class costObject {
public:
	int iter;		//Iteration of Capturing
	int costs[COST_NUM]; //Array for storing all costs
	
	//Method for loading attributes from an existing solution
	costObject(solution* s, int it);
};



class ProblemGlobals {
public:
	//CObArray *RCL
	//RCL is used by the construction routines and the insert job function in the solution class
	//I should create it and destroy it there instead of keeping it global
	vector<operation*> RCL;
	//Elite is needed from the problem and the lsObject so it has to be global as well
	solution *elite;
	//Elite Evolution vectors;
	vector<costObject*> elite_prog;

	ProblemGlobals();
};

//Problem Globals
extern ProblemGlobals* globals;

#endif