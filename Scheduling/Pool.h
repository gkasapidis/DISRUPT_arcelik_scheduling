#include "SolutionClass.h"

#ifndef POOL_H
#define POOL_H

class Pool {
public:
	vector<solution*> solutions;
	//Metrics
	solution* worst;
	solution* best;
	int worst_id;
	int best_id;
	float average_span; //Holds the average cost of all pool solutions
	float average_distance; //Holds the average distance from best of all pool solutions
	int max_distance; //Holds the current maxium distance from the best

	//Pool Stats
	int size;
	int lifespan;
	int life; //Holds the number of iterations that the pool exists
	int changes; //Holds the iterations on which the pool was updated
	float health;
	float strength;
	

	//basic methods
	Pool(int size, int lifespan);
	~Pool();
	
	//Virtual Functions
	virtual solution* select() { return NULL; };
	virtual int update(solution* sol) { return -1;};
	virtual void calcDistances() {};
	virtual void calcAverage() {};
	virtual void calcWeight(solution *sol) {};
	virtual void report(bool sol_stats) {};
	//Tick Function
	virtual void tick() {};

	//Standard Functions
	int Count();
	void findWorst();
	void findBest();
	bool contains(solution *sol);
	
	//Status Methods
	bool isFull();
	//Error Checking
	bool checkSolutions();
};

	
#endif // POOL_H
