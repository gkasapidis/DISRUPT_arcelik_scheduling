#pragma once

#include "Pool.h"

class simplePool : public Pool {

	public:
		simplePool(int size, int lifespan):Pool(size, lifespan) {

		};
		
		//Custom Methods
		void tick();
		int update(solution* sol);
		void updateWeights();
		void calcDistances();
		void calcWeight(solution *sol);
		solution* select();
		int findClosest(solution *sol);

		void calcAverage();

};
