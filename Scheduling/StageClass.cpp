#include <iostream>
#include <ostream>
#include "Parameters.h"
#include "StageClass.h"

using namespace std;

//Stage Class Methods
stage::stage() {
	ID = -1;
	psteps = vector<process_step*>();
}

stage::~stage() {
	for (int i = 0; i < (int) this->psteps.size(); i++) delete psteps[i];
	psteps.clear();
	vec_del<process_step*>(psteps);
}


