#pragma once
#include "Pool.h"

//Scatter Search Pool
class SSPool : public Pool {

public:
	//Custom Variables
	int learning_period;
	vector<int> removeCandidates;

	//Constructor
	SSPool(int size, int lifespan, int lp) :Pool(size, lifespan) {
		learning_period = lp;
		removeCandidates = vector<int>();
	};

    ~SSPool() {
        removeCandidates.clear();
    }

    //Overloaded Methods
    void findWorst();
    void findBest();
    bool isFull();

	//Custom Methods
    void removeSolutions();
	void tick();
	void report();
	int update(solution* sol, int m, int* parents);
};