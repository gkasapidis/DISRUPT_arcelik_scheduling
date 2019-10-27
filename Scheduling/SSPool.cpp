#include "SSPool.h"

int SSPool::update(solution *sol, int m, int* parents) {
	/*	The update method returns an integer that notes the insertion procedure
	-1: The solution was not inserted, because it is either worst or already exists
	0: An exact copy of the solution is already in the pool
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



	bool remove = false;
	int inserted = -1;

	//Check if sol is already in pool
	if (contains(sol)) {
		//printf("Solution exists in Pool \n");
		return 0;
	}

	//Try to update main solution pool
	solution *remove_cand;
	int remove_cand_id;



	bool look_for_weak_cand = false;
	//Case 1, Better than best always insert
	if (sol->compareSolution(best)) {
		//printf("Better than Best\n");

		if (life < learning_period) {
			remove_cand = worst;
			remove_cand_id = worst_id;
		}
		else
			look_for_weak_cand = true;
		inserted = 2;
		if (isFull()) remove = true;
	}
	//Case 2, Worst than worst solution, always delete
	else if (worst->compareSolution(sol)) {
		//printf("Worse than worst\n");
		look_for_weak_cand = false;
		if (isFull()) {
			inserted = 0;
		}
		else {
			worst = sol;
			inserted = 3;
		}
	}
	else {
		//printf("Intermediate Sol\n");
		if (isFull()) {
			look_for_weak_cand = true;
			remove = true;
		}

		inserted = 1;
	}

	if (look_for_weak_cand)
	{
		remove_cand = worst;
		remove_cand_id = worst_id;
		float remove_cand_solv = (float) worst->suc_hits / worst->hits;

		for (int i = 0; i < size; i++) {
			float solv = (float)solutions[i]->suc_hits / solutions[i]->hits;

			//Option 1: Use Distance and makespan
			//if ((solutions[i]->cost_array[DISTANCE] > sol->cost_array[DISTANCE]) &&
			//	(solutions[i]->cost_array[MAKESPAN] > sol->cost_array[MAKESPAN]) &&
			//	(solutions[i]->cost_array[DISTANCE] < remove_cand->cost_array[DISTANCE]))

			//Option 2: use Solvency ratio and makespan
			if ((solv < remove_cand_solv) &&  (solutions[i]->cost_array[MAKESPAN] > remove_cand->cost_array[MAKESPAN])
                    && (find(removeCandidates.begin(), removeCandidates.end(), i) == removeCandidates.end()) )
			{
				remove_cand = solutions[i];
				remove_cand_id = i;
				remove_cand_solv = solv;
			}
		}
	}

	//Insertions/Removals
	if (inserted > 0) {
		solutions.push_back(sol);
		//Recompensate for parents
		for (int i = 0; i < m; i++) 
			solutions[parents[i]]->suc_hits++;
	}
	
	if (remove) {
		//delete remove_cand;
        //solutions.erase(solutions.begin() + remove_cand_id);
        //Instead of delete store the solution id in the removeCandidate list
        removeCandidates.push_back(remove_cand_id);
    }


	//Recalculate Best/Worst
	findBest();
	findWorst();
	return inserted;
}


///Overload
void SSPool::findWorst() {
    int index = 0;
    solution *w = solutions[0];
    worst_id = 0;
    while (find(removeCandidates.begin(), removeCandidates.end(), index) != removeCandidates.end()){
        w = this->solutions[index];
        worst_id = index;
        index++;
    }

    if (w == nullptr)
        printf("SSPool ERROR ERROR ERROR\n");

    for (int i = 0; i< min((int) solutions.size(), size); i++) {
        solution *sol = this->solutions[i];
        if (find(removeCandidates.begin(), removeCandidates.end(), i) != removeCandidates.end())
            continue;
        if (solution::Objective_TS_Comparison(w, sol, false)) {
            w = sol; //Update worst solution
            worst_id = i;
        }
    }

	//Set new worst
    //printf("New Worst solution: %d \n", w->cost_array[MAKESPAN]);
    worst = w;
}

void SSPool::findBest() {
    int index = 0;
    solution *w = solutions[0];
    best_id = 0;
    while (find(removeCandidates.begin(), removeCandidates.end(), index) != removeCandidates.end()){
        w = solutions[index];
        best_id = index;
        index++;
    }

    for (int i = 0; i<min((int) solutions.size(), size); i++) {
        solution *sol = this->solutions[i];
        if (find(removeCandidates.begin(),removeCandidates.end(), i) != removeCandidates.end())
            continue;
        if (solution::Objective_TS_Comparison(sol, w, false)) {
            w = sol; //Update best solution
            best_id = i;
        }
    }

    //Set new best
    best = w;
}

void SSPool::report() {
	//Pool Report
	printf("POOL REPORT ---\n");
	printf("\t%ld Solutions in Pool | Best %4.1f | Worst %4.1f \n",
		solutions.size(), best->cost_array[MAKESPAN], worst->cost_array[MAKESPAN]);

	{
		for (int i = 0; i < (int) solutions.size(); i++) {
			const char *b_string, *w_string, *e_string;

#ifdef PRETTY_PRINT	
			b_string = BOLDGREEN;
			w_string = BOLDRED;
			e_string = RESET;
#else
			b_string = "+++ ";
			w_string = "--- ";
			e_string = "";
#endif

			if (solutions[i] == best) {
				printf("%s", b_string);
			}
			else if (solutions[i] == worst) {
				printf("%s", w_string);
			}

			float solv = (float)solutions[i]->suc_hits / solutions[i]->hits;
			printf("\t Solution %2d Solvency %4.3f M: %5.1f TFT %5.1f TIT %5.1f Seldomness %5.3f",
				i, solv, solutions[i]->cost_array[MAKESPAN], solutions[i]->cost_array[TOTAL_FLOW_TIME], solutions[i]->cost_array[TOTAL_IDLE_TIME],
				solutions[i]->cost_array[DISTANCE]);

			printf("%s\n", e_string);
			//cout << *solutions[i];
		}
	}
}

void SSPool::removeSolutions(){
    //Sort rCandidate list by the higher index
    std::sort(removeCandidates.begin(), removeCandidates.end(), std::greater<int>());
    while(removeCandidates.size()){
        int index = removeCandidates[0];
        solution *cand = solutions[index];
        delete cand;
        vRemoveAt(solutions, index);
        removeCandidates.erase(removeCandidates.begin());
    }
    //Update worst, best
    findBest();
    findWorst();
}

bool SSPool::isFull(){
    if (((int) solutions.size() - (int) removeCandidates.size()) >= size) return true;
    return false;
}

void SSPool::tick() {
	//Progress Generation
	life++;
    //Clear RemoveCandidates
    removeCandidates.clear();

	//At this point the generation has been created and evaluated
	//Recalculate seldomness metric based on the updated freq map
	for (int i = 0; i < size; i++) {
		float val = solution::calcDistance(solutions[i], NULL, 4, false);
		solutions[i]->cost_array[DISTANCE] = val;
	}
}