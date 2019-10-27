#include "simplePool.h"

void simplePool::calcAverage() {
	//Calculate Average Pool metrics
	average_span = 0.0f;
	average_distance = 0.0f;
	for (int i = 0; i < (int) this->solutions.size(); i++) {
		average_span += solutions[i]->cost_array[MAKESPAN];
		average_distance += solutions[i]->d_fromBest;
	}

	average_span = float(average_span) / this->solutions.size();
	average_distance = float(average_distance) / this->solutions.size();
}

void simplePool::calcDistances() {
	float max_distance = 0;
	for (int i = 0; i< (int) solutions.size(); i++) {
		solution *sol = this->solutions[i];
		sol->d_fromBest = solution::calcDistance(sol, best, DISTANCE_METRIC, false);
		sol->d_fromWorst = solution::calcDistance(sol, worst, DISTANCE_METRIC, false);
		//Update pool
		max_distance = std::max(max_distance, sol->d_fromBest);
	}

	//Calculate Weights

	for (int i = 0; i<(int) solutions.size(); i++) {
		solution *sol = this->solutions[i];
		calcWeight(sol);
	}

	//Calculate Averages
	calcAverage();
}

void simplePool::calcWeight(solution *sol) {
#if WEIGHT_METRIC == 0
	sol->weight = float(worst->cost_array[MAKESPAN] - sol->cost_array[MAKESPAN]) / (worst->cost_array[MAKESPAN] - best->cost_array[MAKESPAN]);
#elif WEIGHT_METRIC == 1
	//Find maximum distance 
	sol->weight = float(sol->d_fromBest) / float(max_distance);
#endif
}

int simplePool::update(solution* sol) {

	/*	The update method returns an integer that notes the insertion procedure
	0: The solution was not inserted, because it is either worst or already exists
	1: The solution was inserted but it is mediocre
	2: The solution was inserted and it is the new best
	3: The solution was inserted and it is the new worst
	*/ 

	//Init
	if (!solutions.size()) {
		solutions.push_back(sol);
		worst = sol;
		best = sol;
		sol->d_fromBest = 0;
		sol->d_fromWorst = 0;
		return 2;
	}


	int inserted = 0;

	//Check if sol is already in pool
	if (contains(sol)) {
		//printf("Solution exists in Pool \n");
		return inserted;
	}


	//Check if sol is better than the best
	if (solution::Objective_TS_Comparison(sol, best, false)) {
		//Update new best
		//printf("Candidate better than best \n");
		best = sol;

		if (isFull()) {
			int closest = findClosest(sol);
			printf("Removing Closest solution %d \n", closest);
			delete solutions[closest];
			vRemoveAt<solution*>(solutions, closest);
		}
		solutions.push_back(sol);

		this->findWorst();
		this->calcDistances(); //Recalculate Distances
		inserted = 2;
	}
	else if (solution::Objective_TS_Comparison(worst, sol, false)) {
		//printf("Candidate Worse than worst \n");
		if (isFull()) {
			//cout << "Full Worst" << endl;
			//Do Nothing
		}
		else {
			worst = sol;
			sol->d_fromWorst = 0;
			sol->d_fromBest = solution::calcDistance(sol, best, DISTANCE_METRIC, false);
			//sol->weight = sol->d_fromBest;
			solutions.push_back(sol);
			inserted = 3;
			this->calcDistances(); //Recalculate Distances
		}
	}
	else {
		//Calculate distances from best
		sol->d_fromBest = solution::calcDistance(sol, best, DISTANCE_METRIC, false);
		sol->d_fromWorst = solution::calcDistance(sol, worst, DISTANCE_METRIC, false);
		//sol->weight = sol->d_fromBest;


		//Find Closest
		int closest = findClosest(sol);
		//Compare Objectives

		if (isFull() && (closest >= 0)) {

			solution *sm = solutions[closest];
			float d1 = sol->d_fromBest;
			float d2 = sm->d_fromBest;

			//If the new solution is further away from the best than its closest one, we try save it
			//Otherwise don't save anything
			if (d1 > d2) {
				bool accept_flag = false;

				//If candidate is better then accept it immediately
				if (solution::Objective_TS_Comparison(sol, sm, false)) accept_flag = true;
				//Else accept with prob
				else if (randd() < 0.3f) accept_flag = true;

				if (accept_flag) {
					//printf("Removing Closest solution %d \n", closest);
					delete solutions[closest];
					vRemoveAt<solution*>(solutions, closest);
					solutions.push_back(sol);
					inserted = 1;
				}
			}
		}
		else {
			solutions.push_back(sol);
			inserted = 1;
		}

		this->findWorst();
		this->calcDistances();
	}

	//Before returning Check results
	//checkSolutions();

	return inserted;
}

solution* simplePool::select() {
	//Update fitness scores for all pool solutions
	float fitness_sum = 0.00001f;
	//Accumulate weights
	for (int i = 0; i<(int) solutions.size(); i++) {
		fitness_sum += solutions[i]->weight;
	}


	//Calculate fitness
	for (int i = 0; i< (int) solutions.size(); i++) {
		solution *sol = solutions[i];
		sol->fitness = sol->weight / fitness_sum;
	}

	//Generate probability
	double prob = randd();
	int id = rand() % solutions.size();

	//Roulette Selection
	for (int i = 0; i<(int) solutions.size(); i++) {
		prob -= solutions[i]->fitness;
		if (prob <= 0) { id = i; break; }
	}


	//printf("Selecting Solution %d with fitness %f \n", id, solutions[id]->fitness);

	// while (solutions[id] == best){
	// 	id = rand() % solutions.size();
	// }
	return solutions[id];
}

int simplePool::findClosest(solution *sol) {

	float minim = INF;
	float minim_cost = INF;
	int closest = -1;
	for (int i = 0; i < (int) solutions.size(); i++) {
		solution* cand = solutions[i];
		//if this is best skip
		if (cand == best) continue;
		float temp_d = solution::calcDistance(sol, cand, DISTANCE_METRIC, false);
		if ((temp_d < minim) && (cand->cost_array[MAKESPAN] <= minim_cost)) {
			minim = temp_d;
			minim_cost = cand->cost_array[MAKESPAN];
			closest = i;
		}
	}

	// if (closest == -1) {
	// 	printf("FIND CLOSEST FAILED\n");
	// }

	return closest;
};


//Tick function
void simplePool::tick() {
	//Update pool life
	life++;
	//Update solution lifes and remove old ones
	for (int i = 0; i < (int) solutions.size(); i++) {
		solution* sol = solutions[i];
		sol->life = std::min(sol->life + 1, lifespan);

		if (sol->life >= lifespan) {
			if (sol != best) {
				//printf("Deleting Old Solution\n");
				delete solutions[i];
				vRemoveAt<solution*>(solutions, i);
				changes += 5;
				findWorst();
				calcDistances();
			}
		}
	}

	//Calculate Pool Health
	//Old way
	//health = float(changes) / life;

	//New Way
	health = 0.0f;
	for (int i = 0; i < (int) solutions.size(); i++) {
		health += 1.0f - float(solutions[i]->life) / lifespan;
	}
	health /= solutions.size();
}