#include "Parameters.h"
#include "ProbabilisticMoveClass.h"
#include "SolutionClass.h"

#ifndef TABU_H
#define TABU_H

using namespace std;
enum move { RELOCATE, EXCHANGE, COMPOUND, SMART_RELOCATE, SMART_EXCHANGE,
    CRIT_COMPOUND, CRIT_SMART_RELOCATE, CRIT_SMART_EXCHANGE,
    CRIT_BLOCK_SEQUENCE, CRIT_BLOCK_EXCHANGE, CRIT_BLOCK_UNION,
    PERTURBATION, INFEASIBLE, ASPIRATION, TABU, OUTER_TIES, INNER_TIES, TABU_MOVE_COUNT };

class tabu_search_options: search_options{
public:
    int ts_iters;
    int ts_tenure;
    int ts_l_size;
    int ts_perturb_limit;
    bool ts_strategy;
    int ts_time_limit;

    tabu_search_options(){
        ts_iters = 10;
        ts_tenure = 2;
        ts_l_size = 2;
        ts_perturb_limit = 1;
        ts_strategy = false;
        ts_time_limit = 1;
    }
};


struct tabu_move_arc_policy {
	bool save_deleted;
	bool save_created;
};

class tabu
{
public:
	int3_tuple comb;
	int mode, life;
	string hash;

	tabu() {
		life = 0; //init life
		mode = 0;
	}

	//Constructor for relocate/exchange moves
	tabu(int op1_uid, int op2_uid, int mc_id, int mode) {
		comb = int3_tuple(op1_uid, op2_uid, mc_id);
		this->mode = mode;
		this->life = 0;
		//this->hash = tabu::hash_re(ie, je, ke, le, m);
	}

	//Redundant hash moves (for now I hope)
	//Relocation, Exchange Hash Function
	static string hash_re(int ie, int je, int ke, int le, int mode) {
		char *h = new char[100];
		sprintf(h, "#%d#%d#%d#%d#%d#", ie ,je, ke, le, mode);
		string hsh(h);
		delete[] h;
		return hsh;
	}

	//Compound Move Hash Function
	static string hash_c(int ie, int je, int ce, int mode) {
		char *h = new char[100];
		sprintf(h, "#%d#%d#%d#%d#", ie ,je, ce, mode);
		string hsh(h);
		delete[] h;
		return hsh;
	}

	void report() {
		printf("Tabu Arc from Op %d to Op %d on machine %d\n", 
			std::get<0>(comb), std::get<1>(comb), std::get<2>(comb));
	}

};

class localSearch
{
//Extend solution with the tabu search methods and stuff
public:

	solution *currentSol;
	solution *localElite;
	
	//Tabu List holds the moves applied so far
	vector<tabu*> *tabumlist;
	//Tabu List holds the moves applied so far within perturbation
	vector<tabu*> *p_tabumlist;
	
	//unordered_map<string, tabu*> tabulist; //Leave the hashtable for now
	
	//Search Options
	tabu_search_options *options;
	tabu_move_arc_policy arc_policy;

	//Statistics
	vector<int> moveStats;
    vector<int> failedMoves;

	//Specialized Structures for Adaptive selection parts of the algorithm
	ProbabilisticMove *TabuMoveSelector;
	ProbabilisticMove *CompoundRangeSelector;

	//LS Functions
    bool tabu_search(vector<int> modes, int compound_range); //Main Search function

	bool old_tabu_search(); //Main Search function
	bool vns(); //VNS Search Scheme
	bool vns_ts(); //VNS+TS Combo Search Scheme
	
	float distant_sol_finder(float div, int iters, int distance_metric, int new_optimtype); //Search function for finding perturbed solutions
	void Perturbation(); //localElite solution perturbation
	void Perturb(solution* sol, float strength); //Custom perturbation method with strength
	bool FindNeighbors(int &ke, int &le, int &ie, int &je, int mode, bool firstbest); //Old

    bool ls_relocate(int &ke, int &le, int &ie, int &je, int mode, bool firstbest, bool improving_only, float *minim_ar);
    bool ls_exchange(int &ke, int &le, int &ie, int &je, int mode, bool firstbest, bool improving_only);
    bool ls_smart_relocate(int &ke, int &le, int &ie, int &je, int mode, bool firstbest, bool improving_only);
	bool ls_smart_exchange(int &ke, int &le, int &ie, int &je, int mode, bool firstbest, bool improving_only);
	bool critical_ls_smart_relocate(int &ke, int &le, int &ie, int &je, bool firstbest, bool improving_only, float *minim_ar);
	bool critical_ls_smart_exchange(int &ke, int &le, int &ie, int &je, int mode, bool firstbest, bool improving_only, float* minim_ar);
    bool critical_block_sequencing(int &ke, int &le, int &ie, int &je, bool firstbest, bool improving_only, float *minim_ar);
	bool critical_block_exchange(int &ke, int &le, int &ie, int &je, bool firstbest, bool improving_only, float *minim_ar);
	bool critical_block_union(int &ke, int &le, int &ie, int &je, int &mode, bool firstbest, bool improving_only);

    //Stripped down versions that are not using tabu moves and ties (created for perturbation)
    bool ls_perturb_smart_relocate(int &ke, int &le, int &ie, int &je, int mode, bool firstbest, solution *s2);
	bool ls_perturb_smart_exchange(int &ke, int &le, int &ie, int &je, int mode, bool firstbest, solution *s2);
	bool FindNeighborsJobs(int &ce, int &ie, int &ue, int mode, int segment_size, int displacement, solution *s2);
	bool critical_FindNeighborsJobs(int &ce, int &ie, int &ue, int mode, int segment_size, int displacement, bool improve_only);
	void JobShiftbyOneBWD(float *cost_ar, int j, int i, int u, bool actual, bool &tabu_move, int &q_metric);
	void RevertShiftBWD(int jobid, int range_a, int range_b);
	void JobShiftbyOneFWD(float *cost_ar, int j, int i, int u, bool actual, bool &tabu_move, int &q_metric);
	void RevertShiftFWD(int jobid, int range_a, int range_b);

    void relocate(int k, int l, int i, int j, bool actual, float *cost_ar);
	//Same as the relocated function but it is adding the
    void relocate_perturb(int k, int l, int i, int j, bool actual, float *cost_ar);

    void smart_relocate(operation *op, int k, int l, int i, int j, float *cost_ar, float &q_metric);
    bool temp_relocate(operation *op, int k, int l, int i, int j, float *cost_ar, float &q_metric);
	bool smart_exchange(operation *op1, operation *op2, int pos, machine *mck, machine *mcl, float *cost_ar, float &q_metric, int min_index);
    bool temp_swap(operation *op1, operation *op2, machine *mck, machine *mcl, float *cost_ar, float &q_metric);
	void exchange(int k, int l, int i, int j, bool actual, float *cost_ar);
	void exchange_perturb(int k, int l, int i, int j, bool actual, float *cost_ar);
	int RemoveAndReinsert(solution* sol);

    void findCritical(); //Deprecated using function from the solution object
		
	//Tabulist methods
	//Used in all non perturbation LS methods
    bool findRelocate(int k, int l, int i, int j);
	bool findExchange(int k, int l, int i, int j);
    //Core Move searching functions
	bool findMoveinMatrix(operation *op1, operation *op2, machine *mc, int mode);
	bool findMoveinList(operation *op1, operation *op2, machine *mc, int mode);
    //Function pointer for swapping between findMove functions
    bool (localSearch::*findMove)(operation *op1, operation *op2, machine *mc, int mode) = nullptr;

    //Core Move insertion functions
    void inline addMoveinMatrix(operation *op1, operation *op2, machine *mc, int mode);
    void inline addMoveinList(operation *op1, operation *op2, machine *mc, int mode);
    //Function pointer for swapping between addMove functions
    void (localSearch::*addMove)(operation *op1, operation *op2, machine *mc, int mode) = nullptr;



    void deleteTabuMatrix(); //Completely delete the structure
    void deleteTabuList(); //Completely delete the structure

	void clearTabuMatrix(); //Clear tabu matrix based on tenure
    void clearTabuList(); //Clear based on tenure
    void (localSearch::*clearTabu)() = nullptr;

    void clearTabuList(job *jb);//Partial Clear function
	void cleanUp();
	void report();

	//TieBreaker Selector
	float applyTieBreaker();

    //Report Functions
    void reportMoves();

	//Costs
	//bool Objective_TS_Comparison(solution *sol, int mcost1, int mcost2, int mcost3, int mcost4);
	bool Objective_TS_Comparison_Asp();

	//Temp Stuff
	bool **infeasibility_matrix;
	int count;
    vector<float> tabu_progress;
	


	//Trivial constructor
	localSearch() {
		currentSol = new solution();
		localElite = new solution();
		moveStats = vector<int>(TABU_MOVE_COUNT);
        failedMoves = vector<int>(TABU_MOVE_COUNT);
		//tabulist = unordered_map<string, tabu*>();
		tabumlist = new vector<tabu*>();
		p_tabumlist = new vector<tabu*>();
		count = 0;

        for (int i=0;i<TABU_MOVE_COUNT;i++) moveStats[i] = 0;

		//Init Structs
		TabuMoveSelector = new ProbabilisticMove(TABU_MOVE_COUNT);
		CompoundRangeSelector = new ProbabilisticMove(20); //Set range to 20 operations hopefully it won't damage anything

		//Setup Infeasibility Matrix
		bool *buf = new bool[MAX_OPERATIONS*MAX_OPERATIONS];
		infeasibility_matrix = new bool*[MAX_OPERATIONS];
		for (int i = 0; i < MAX_OPERATIONS; i++) 
			infeasibility_matrix[i] = buf + i*MAX_OPERATIONS;


		//Setup Default Arc policy
		arc_policy.save_created = false;
		arc_policy.save_deleted = true;

        //Setup Temp Variables
        tabu_progress = vector<float>();
	}

	void setup(solution *input){
		//Delete solutions
		delete currentSol;
		delete localElite;

		//Setup starting solutions
		currentSol = new solution();
		localElite = new solution();

		*currentSol = *input;
		*localElite = *input;

		if (currentSol->soloperations.size() > MAX_OPERATIONS) {
			printf("Increase the MAX Operations limit\n");
			assert("false");
		}

		//cout << *currentSol;
		//currentSol->FGraph->report();

		//Setup optimizes the Object
		//Reserv Space for tabulist
		tabumlist->reserve((unsigned long) 4*options->ts_tenure);

        //Set Default functions
        addMove = &localSearch::addMoveinList;
        findMove = &localSearch::findMoveinList;
        clearTabu = &localSearch::clearTabuList;


		//Reset Stats
		cleanUp();
	}

	//Locally define a deconstructor
	~localSearch() {
		delete currentSol;
		delete localElite;
		//Delete Temp stuff
		vec_del<int>(moveStats);
        vec_del<int>(failedMoves);

        delete[] &infeasibility_matrix[0][0];
        delete[] infeasibility_matrix;

        //Delete Options
        if (options != nullptr)
            delete options;


        //Delete Structs
		delete TabuMoveSelector;
		delete CompoundRangeSelector;

        deleteTabuList();
	}


};

#endif