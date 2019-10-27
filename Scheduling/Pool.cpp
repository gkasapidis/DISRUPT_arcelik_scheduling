#include "Pool.h"
#include "Parameters.h"

using namespace std;


Pool::Pool(int size, int span){
	this->size = size;
	this->lifespan = span;
	this->life = 1;
	this->changes = 1;
	this->health = 1.0f;
	this->strength = 0.0f; 
	this->average_span = 0.0f;
	this->average_distance = 0.0f;
	this->max_distance = 0;
}

Pool::~Pool() {
	cout << "Deconstructing Solution Pool" << endl;
	//Cleanup
	while (solutions.size()) {
		delete solutions[0];
		vRemoveAt<solution*>(solutions, 0);
	}
	vec_del<solution*>(solutions);
}

bool Pool::isFull(){
	if (this->solutions.size() >= this->size) return true;
	return false;
}

int Pool::Count() {
	return this->solutions.size();
}

void Pool::findBest() {
	solution *w = this->solutions[0];
	best_id = 0;

	for (int i = 1; i<this->solutions.size(); i++) {
		solution *sol = this->solutions[i];
		if (solution::Objective_TS_Comparison(sol, w, false)) {
			w = sol; //Update best solution
			best_id = i;
		}
	}

	//Set new best
	best = w;
}

void Pool::findWorst() {
	solution *w = this->solutions[0];
	worst_id = 0;

	for (int i = 1; i<this->solutions.size(); i++) {
		solution *sol = this->solutions[i];
		if (solution::Objective_TS_Comparison(w, sol, false)) {
			w = sol; //Update worst solution
			worst_id = i;
		}
	}

	//Set new worst
	//printf("New Worst solution: %d \n", w->cost_array[MAKESPAN]);
	worst = w;
}


bool Pool::checkSolutions() {
	for (int i = 0; i < this->solutions.size(); i++) {
		solution *sol = solutions[i];
		if (sol->solmachines.size() == 0){
			printf("Exei paixtei malakia sto update \n");
			assert(false);
		}
	}

	return true;
}


bool Pool::contains(solution* sol) {
	for (int i = 0; i < solutions.size(); i++){
		solution *pool_sol = solutions[i];
		//Preliminary test
		if ((abs(pool_sol->cost_array[MAKESPAN] - sol->cost_array[MAKESPAN]) <= TOL_FEA) &&
			(abs(pool_sol->cost_array[TOTAL_FLOW_TIME] - sol->cost_array[TOTAL_FLOW_TIME]) <= TOL_FEA) &&
			(abs(pool_sol->cost_array[TOTAL_IDLE_TIME] - sol->cost_array[TOTAL_IDLE_TIME]) <= TOL_FEA)) {
			//printf("Alarm - Possible Equal Sol\n");
			//cout << "Sol 1 " << endl;
			//cout << *pool_sol;
			//cout << "Sol 2 " << endl;
 			//cout << *sol;
			if (*pool_sol == *sol) return true;
		}
	}
	return false;
}