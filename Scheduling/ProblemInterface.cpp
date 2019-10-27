#include "ProblemInterface.h"
#include "SolutionClass.h"

ProblemGlobals::ProblemGlobals(){
	 
	 this->RCL = vector<operation*>();
	 this->elite = new solution();
	 this->elite_prog = vector<costObject*>();
	 //The frequency map has to be initiallized in a next step
 };

 costObject::costObject(solution* s, int it){
	 this->iter = it; //Update iteration
	 for (int i=0;i<COST_NUM;i++)
		costs[i] = s->cost_array[i];
 }