#include "Parameters.h"
#include "CObject.h"
#include "OperationClass.h"
#include "JobClass.h"
#include "MachineClass.h"
#include "ProblemInterface.h"
#include "ProcessStep.h"
#include "LSSolutionClass.h"

using namespace std;


//Smart relocate assumes that the operations is already removed from machine k and is to be inserted to machine l
void localSearch::smart_relocate(operation *op, int k, int l, int i, int j, float *cost_ar, float &q_metric)
{
	//currentSol->replace_temp_values();
	Graph<operation> *gr = currentSol->FGraph;

	//printf("Before Smart Relocate Move Application \n");
	//cout << *currentSol;

	//Load pivot op
	machine *mck = currentSol->solmachines[k];
	machine *mcl = currentSol->solmachines[l];

	int target_mach_pos = j;
	machine *target_mc = mcl;

    /* KEEP THIS JUST FOR FUTURE REFERENCE - IT IS NOT FASTER THAN THE DUMMY ONE
    //Determine moving operations using the topological sort
    //gr->report_topological_sort();
    int id1 = gr->top_sort_index[op->UID];
    int id2 = gr->top_sort_index[target_mc->operations[target_mach_pos-1]->UID];
    int min_index = min(id1, id2);

    //Mark Moving operations
    for (int ii=0; ii<min_index; ii++)
        gr->top_sort[ii]->moving = false;
    for (int ii=min_index; ii<gr->size; ii++)
        gr->top_sort[ii]->moving = true;
    */

    //Insert op to solution
    currentSol->insertOp_noGraphUpdate(target_mc, target_mach_pos, op);


	//printf("After Smart Relocate Move Application \n");
	//cout << *currentSol;
	//printf("------------------------------------- \n");
	//printf("After Move Application Graph Forward report\n");
 	//gr->report();

    //printf("Excluding %d / %d operations from calculations \n", movingOps.size(), gr->NodeList.size());

	//Testing Phase
	//printf("Relocation Move\n");
	{
		if (!currentSol->scheduleAEAP(true, true))
            printf("smart relocate: Malakia sto sAEAP\n");

        currentSol->getCost(true);
		copy(currentSol->temp_cost_array, currentSol->temp_cost_array + COST_NUM, cost_ar);

		q_metric = applyTieBreaker();

		//Reverting Back
		currentSol->removeOp_noGraphUpdate(target_mc, target_mach_pos);

		op->setDoneStatus(false, true);
	}

}

//Temp relocate probes the whole move
bool localSearch::temp_relocate(operation *op, int k, int l, int i, int j, float *cost_ar, float &q_metric)
{
    bool feasible = true;

    //printf("Before Temp Relocate Move Application \n");
    //cout << *currentSol;

    //Load pivot op
    machine *mck = currentSol->solmachines[k];
    machine *mcl = currentSol->solmachines[l];

    int target_mach_pos = j;
    machine *target_mc = mcl;

    currentSol->removeOp_noGraphUpdate(mck, i);
    currentSol->insertOp_noGraphUpdate(target_mc, target_mach_pos, op);

    //printf("After Temp Relocate Move Application \n");
    //cout << *currentSol;
    //printf("------------------------------------- \n");
    //printf("After Move Application Graph Forward report\n");
    //gr->report();

    //Test final position
    if (op->mthesi != j){
        cout <<*currentSol;
        printf("Exei paixtei xontri malakia stin temp_relocate\n");
    }

    //Testing Phase
    if (currentSol->scheduleAEAP(true, false)) {
        currentSol->getCost(true);
        copy(currentSol->temp_cost_array, currentSol->temp_cost_array + COST_NUM, cost_ar);
        q_metric = applyTieBreaker();
    }
    else {
        //cout <<" Illegal Move" << endl;
        for (int ii = 0; ii < COST_NUM; ii++)
            cost_ar[ii] = INF;
        moveStats[INFEASIBLE]++;
        feasible = false;
        q_metric = -INF;

        //printf("Infeasible Sol \n");
        //cout << *currentSol;
		//printf("A\ns\nda\nsd\nas\nda\nsd\n");
    }



    //Reverting Back
    currentSol->removeOp_noGraphUpdate(target_mc, target_mach_pos);
    currentSol->insertOp_noGraphUpdate(mck, i, op);

    //printf("After Temp Relocate Move Restoration \n");
    //cout << *currentSol;

    op->setDoneStatus(false, true);

    return feasible;
}

// Core Exchange Operator
bool localSearch::smart_exchange(operation *opi, operation *opj, int old_pos_1, machine *mck, machine *mcl, float *cost_ar, float &q_metric, int min_index)
{

	/*	Smart Exchange assumes that opi has already been removed.
		It  1) removes the second operation
			2) applies the swap and calculates the new objectives
			3) undoes the swap and brings opj back to the solution + graph
	*/
	bool feasible = true;
	//currentSol->replace_temp_values();
	Graph<operation> *gr = currentSol->FGraph;
	//movingOps.clear();

	//Store already moving ops


	//cout << "SMART EXCHANGE INTRO PHASE REPLACED_TEMP SOLUTION" << endl;
	//cout << *currentSol;
	//currentSol->forwardGraph->report();

	int n = 0; //Loops
	int cutoff = 0;

	machine *target_mc = mcl;

	/*
	if (actual) {
	printf("Exchanging Op %d %d from machine %d with Op %d %d from machine %d \n",
	opi->jobID, opi->ID, k, opj->jobID, opj->ID, l);
	//printf("Graph Forward initial report\n");
	}
	*/

	//Do the move
	//Remove opj
	int old_pos_2 = opj->mthesi;
	currentSol->removeOp_noGraphUpdate(target_mc, old_pos_2);

	//Changing Machines to Ops
	//opi->changeMach(target_mc, old_pos_2);
	//opj->changeMach(mck, old_pos_1);
	//Insert Ops
	//target_mc->insertOp(opi, old_pos_2);
	//currentSol->insertOpToGraphs(opi);
    currentSol->insertOp_noGraphUpdate(target_mc, old_pos_2, opi);

	//mck->insertOp(opj, old_pos_1);
	//currentSol->insertOpToGraphs(opj);
    currentSol->insertOp_noGraphUpdate(mck, old_pos_1, opj);

	//cout << "Exchange after move Checks" << endl;
	//cout << *currentSol;
	//cout << "Solution Forward Graph" << endl;
	//currentSol->forwardGraph->report();


	//DFS both of them after the move
	bool cycleflag = false;
	//gr->dfs(opi->UID);
	//cycleflag |= gr->cycleflag;

	//gr->dfs(opj->UID);
	//cycleflag |= gr->cycleflag;

	//printf("Exchange Move\n");
	{
		//Testing Phase
		if (currentSol->scheduleAEAP(true, true)) {
			// cost1 = currentSol->calculate_makespan(0, true) - currentSol->cost;
			// cost2 = currentSol->calculate_TFT(0, true) - currentSol->s_cost;
			// cost3 = currentSol->calculate_Tidle(0, true) - currentSol->t_cost;
			// cost4 = currentSol->calculate_Tenergy(0, true) - currentSol->e_cost;
			currentSol->getCost(true);
			copy(currentSol->temp_cost_array, currentSol->temp_cost_array + COST_NUM, cost_ar);
			q_metric = applyTieBreaker();
		}
		else {
			// if (currentSol->scheduleAEAP(true, cutoff, false)){
			// 	printf("Possible false negative - Check solution and graph\n");
			// 	cout << *currentSol;
			// 	currentSol->forwardGraph->report();
			// 	assert(false);
			// }
			//15/09/2017 TEST PASSED.
			//cout <<" Illegal Move" << endl;
			for (int ii = 0; ii < COST_NUM; ii++)
				cost_ar[ii] = INF;
			moveStats[INFEASIBLE]++;
			feasible = false;
		}

		//cout << "PROBED EXCHANGE SOLUTION" << endl;
		//cout << *currentSol;

		//Reverting
		//Removing both ops
		currentSol->removeOp_noGraphUpdate(mck, old_pos_1);
		currentSol->removeOp_noGraphUpdate(target_mc, old_pos_2);


		//Changing Machines to Ops
		//opj->changeMach(target_mc, old_pos_2);

		//Insert Ops
		//target_mc->insertOp(opj, old_pos_2);
		//currentSol->insertOpToGraphs(opj);
        currentSol->insertOp_noGraphUpdate(target_mc,old_pos_2,opj);

		//Fix status
		opj->setDoneStatus(false, true);

		// Graph *gr = currentSol->createGraph(1);
		// if (!(*gr == *currentSol->forwardGraph)) {
		// 	cout << "EXCHANGE UNEQUAL GRAPH AFTER REVERT" << endl;
		// cout << "CURRENT SOLUTION" << endl;
		// cout << *currentSol;
		// 	cout << "Recalculated" << endl;
		// 	gr->report();
		// 	cout << "Inserted" << endl;
		// 	currentSol->forwardGraph->report();
		// 	assert(false);
		// 	int boom;
		// 	scanf("BULLSHIT %d", &boom);
		// }
		// delete gr;

	}
	return feasible;
}

// Core Exchange Operator
bool localSearch::temp_swap(operation *opi, operation *opj, machine *mck, machine *mcl, float *cost_ar, float &q_metric)
{

    /*	Temp Swap probes the swap between the two operations.
        It assumes in the case of a same machine swap, opi is before opj
    */
    bool feasible = true;


    //cout << "SMART EXCHANGE INTRO PHASE REPLACED_TEMP SOLUTION" << endl;
    //cout << *currentSol;
    //currentSol->forwardGraph->report();

    int n = 0; //Loops
    int cutoff = 0;

    machine *target_mc = mcl;

    /*
    if (actual) {
    printf("Exchanging Op %d %d from machine %d with Op %d %d from machine %d \n",
    opi->jobID, opi->ID, k, opj->jobID, opj->ID, l);
    //printf("Graph Forward initial report\n");
    }
    */

    //Do the move
    //Remove opi
    int old_pos_1 = opi->mthesi;
    currentSol->removeOp(mck, old_pos_1);

    //Remove opj
    int old_pos_2 = opj->mthesi;
    currentSol->removeOp(target_mc, old_pos_2);

    //Changing Machines to Ops
    opi->changeMach(target_mc, old_pos_2);
    opj->changeMach(mck, old_pos_1);
    //Insert Ops
    target_mc->insertOp(opi, old_pos_2);
    currentSol->insertOpToGraphs(opi);
    mck->insertOp(opj, old_pos_1);
    currentSol->insertOpToGraphs(opj);

    cout << "Solution after Swap" << endl;
    cout << *currentSol;
    //cout << "Solution Forward Graph" << endl;
    //currentSol->forwardGraph->report();

    //printf("Temp Swap Move\n");
    {
        //Testing Phase
        if (currentSol->scheduleAEAP(true, true)) {
            currentSol->getCost(true);
            copy(currentSol->temp_cost_array, currentSol->temp_cost_array + COST_NUM, cost_ar);
            q_metric = applyTieBreaker();
        }
        else {
            // if (currentSol->scheduleAEAP(true, cutoff, false)){
            // 	printf("Possible false negative - Check solution and graph\n");
            // 	cout << *currentSol;
            // 	currentSol->forwardGraph->report();
            // 	assert(false);
            // }
            //15/09/2017 TEST PASSED.
            //cout <<" Illegal Move" << endl;
            for (int ii = 0; ii < COST_NUM; ii++)
                cost_ar[ii] = INF;
            moveStats[INFEASIBLE]++;
            feasible = false;
        }

        //cout << "PROBED EXCHANGE SOLUTION" << endl;
        //cout << *currentSol;

        //Reverting
        //Removing both ops
        currentSol->removeOp(mck, old_pos_1);
        currentSol->removeOp(target_mc, old_pos_2);


        //Changing Machines to Ops
        opi->changeMach(mck, old_pos_1);
        opj->changeMach(target_mc, old_pos_2);

        //Insert Ops
        target_mc->insertOp(opj, old_pos_2);
        currentSol->insertOpToGraphs(opj);
        mck->insertOp(opi, old_pos_1);
        currentSol->insertOpToGraphs(opi);


        //Fix status
        opj->setDoneStatus(false, true);

        // Graph *gr = currentSol->createGraph(1);
        // if (!(*gr == *currentSol->forwardGraph)) {
        // 	cout << "EXCHANGE UNEQUAL GRAPH AFTER REVERT" << endl;
        // cout << "CURRENT SOLUTION" << endl;
        // cout << *currentSol;
        // 	cout << "Recalculated" << endl;
        // 	gr->report();
        // 	cout << "Inserted" << endl;
        // 	currentSol->forwardGraph->report();
        // 	assert(false);
        // 	int boom;
        // 	scanf("BULLSHIT %d", &boom);
        // }
        // delete gr;

    }
    return feasible;
}

void localSearch::relocate(int k, int l, int i, int j, bool actual, float *cost_ar)
{
	//currentSol->replace_temp_values();
	//currentSol->setDoneStatus(false, true);

	//Graph<operation> *gr = currentSol->FGraph;

	//Load pivot op
	machine *mck = currentSol->solmachines[k];
	machine *mcl = currentSol->solmachines[l];
	operation *opi = mck->operations[i];

	int target_mach_pos = j;
    machine *target_mc = mcl;

	//printf("Moving Operation %d from machine %d pos %d to machine %d pos %d\n", opi->UID, k, i, l, j);
	//cout << "Relocate Start Solution" << endl;
	//cout << *currentSol;

	// if (!actual){
	// 	cout << "Relocating Op: " << opi->jobID << " " << opi->ID << endl;
	// 	cout << "Found Node: " << opi_node->JobID << " " << opi_node->OpID << endl;
	// 	printf("Applying relocation move %d %d %d %d \n", k, l, i, target_mach_pos);
	// 	cout << *currentSol;
	// 	printf("Graph Forward initial report\n");
 	//  gr->report();
	// }

	//Save tabu moves before doing the move
	{
		//Save tabu moves first Part
		//Save destroyed arcs related to the removed operation
		operation *op1 = mck->operations[i - 1];
		operation *op2 = mck->operations[i];
        (this->*addMove)(op1, op2, mck, 0);

		op1 = mck->operations[i];
		op2 = mck->operations[i + 1];
        (this->*addMove)(op1, op2, mck, 0);
    }

	//Insertion Phase
	currentSol->removeOp(mck, i);
    currentSol->insertOp(target_mc, target_mach_pos, opi);

	{
		//Save destroyed arcs second part (save after removal)
		//Save arcs related to the second operation
		operation *op1 = target_mc->operations[target_mach_pos - 1];
		operation *op2 = target_mc->operations[target_mach_pos + 1];
        (this->*addMove)(op1, op2, target_mc, 0);
    }

	//printf("Excluding %d / %d operations from calculations \n", movingOps.size(), gr->NodeList.size());

    //Application Phase
	{
		if (!currentSol->scheduleAEAP(false, true)) {
			// for (int i = 0; i<movingOps.size(); i++) {
			// 	operation *op = movingOps[i];
			// 	printf("Moving Operation %d %d\n", op->jobID, op->ID);
			// }
			cout << "EXEI PAIXTEI MALAKIA" << endl;
			cout << *currentSol;
			cout << "Solution Forward Graph" << endl;
			currentSol->FGraph->report();
			//assert(false);
		};

		currentSol->getCost(false);
        currentSol->findCriticalPath_viaSlack(); //Recalculate critical Path
        currentSol->findCriticalBlocks(1); //Update critical Blocks


		//cout << "RELOCATION APPLICATION PHASE NEW SOLUTION" << endl;
		//cout << *currentSol;

		copy(currentSol->d_cost_array, currentSol->d_cost_array + COST_NUM, cost_ar);

		//Mark moved operations
		opi->move_freq++;

		//Clean Tabulist
        (this->*clearTabu)();

        //cout << *currentSol;
		//reportMoves(); //Report Tabulist moves after move application
	}
}

void localSearch::relocate_perturb(int k, int l, int i, int j, bool actual, float *cost_ar)
{
	currentSol->replace_temp_values();
	//currentSol->setDoneStatus(false, true);

	Graph<operation> *gr = currentSol->FGraph;

	//Load pivot op
	machine *mck = currentSol->solmachines[k];
	machine *mcl = currentSol->solmachines[l];
	operation *opi = mck->operations[i];

	int target_mach_pos = j;

	machine *target_mc = mcl;

	//printf("Moving Operation %d from machine %d pos %d\n to machine %d pos %d\n", opi->UID, k, i, l, j);
	//cout << "Relocate Start Solution" << endl;
	//cout << *currentSol;

	// if (!actual){
	// 	cout << "Relocating Op: " << opi->jobID << " " << opi->ID << endl;
	// 	cout << "Found Node: " << opi_node->JobID << " " << opi_node->OpID << endl;
	// 	printf("Applying relocation move %d %d %d %d \n", k, l, i, target_mach_pos);
	// 	cout << *currentSol;
	// 	printf("Graph Forward initial report\n");
	//  gr->report();
	// }

	//Save tabu moves before doing the move
	{
		//Save tabu moves first Part
		//Save destroyed arcs related to the removed operation
		operation *op1 = mck->operations[i - 1];
		operation *op2 = mck->operations[i];
        (this->*addMove)(op1,op2,mck,0);

		op1 = mck->operations[i];
		op2 = mck->operations[i + 1];
		(this->*addMove)(op1,op2,mck,0);
    }

	//Save the created arc from the removal of the operation
	{
		operation* op1 = mck->operations[i - 1];
		operation *op2 = mck->operations[i + 1];
        (this->*addMove)(op1,op2,mck,0);
    }

	//Insertion Phase
	currentSol->removeOp(mck, i);
	currentSol->insertOp(target_mc, target_mach_pos, opi);

    {
		//Save destroyed arcs second part (save after removal)
		//Save arcs related to the second operation
		operation *op1 = target_mc->operations[target_mach_pos - 1];
		operation *op2 = target_mc->operations[target_mach_pos + 1];
        (this->*addMove)(op1, op2, target_mc,0);
    }

	{
		//Save created arcs second part
		operation *op1 = target_mc->operations[target_mach_pos - 1];
        (this->*addMove)(op1, opi, target_mc, 0);

		op1 = target_mc->operations[target_mach_pos + 1];
        (this->*addMove)(opi, op1, target_mc, 0);
    }

	//printf("Excluding %d / %d operations from calculations \n", movingOps.size(), gr->NodeList.size());
    //Application Phase

    {
		if (!currentSol->scheduleAEAP(false, true)) {
			// for (int i = 0; i<movingOps.size(); i++) {
			// 	operation *op = movingOps[i];
			// 	printf("Moving Operation %d %d\n", op->jobID, op->ID);
			// }
			cout << "EXEI PAIXTEI MALAKIA" << endl;
			cout << *currentSol;
			cout << "Solution Forward Graph" << endl;
			currentSol->FGraph->report();
			assert(false);
		};
		currentSol->getCost(false);

		//cout << "RELOCATION APPLICATION PHASE NEW SOLUTION" << endl;
		//cout << *currentSol;

		copy(currentSol->d_cost_array, currentSol->d_cost_array + COST_NUM, cost_ar);

		//Mark moved operations
		opi->move_freq++;

		//Clean Tabulist
        (this->*clearTabu)();
		//cout << *currentSol;
		//reportMoves(); //Report Tabulist moves after move application
	}

	return;
}


void localSearch::exchange(int k, int l, int i, int j, bool actual, float *cost_ar)
{
	
	currentSol->replace_temp_values();
	Graph<operation> *gr = currentSol->FGraph;

	/*
	if (actual) {
		cout << "EXCHANGE INTRO PHASE REPLACED_TEMP SOLUTION" << endl;
		cout << *currentSol;
	 	//currentSol->DGraph->report();
	}
	*/
	
	

	//cout << "Start Solution" << endl;
	//cout << *currentSol;

	int n = 0; //Loops
	int cutoff = 0;

	machine *mck = currentSol->solmachines[k];
	machine *mcl = currentSol->solmachines[l];

	machine *target_mc = mcl;
	
	if (k == l) {
		target_mc = mck;
	}

	//j will be always greater than i
	operation *opi = mck->operations[i];
	operation *opj = target_mc->operations[j];

	//if (actual)
	//	printf("Swapping %d %d %d %d - UIDs %d %d \n", k, l, i, j, opi->UID, opj->UID);

	if ((j < i) && (k == l)) {
		//printf("Input i %d j %d \n", i, j);
		//printf("Swapping INputs Necessary\n");
		//assert(false);
		
		//Swap the inputs, should work
		operation *t = opj;
		opj = opi;
		opi = t;
		int t_i = i;
		i = j;
		j = t_i;
		
	}
	
	{
		//save the tabu moves before applying the move
		//Save moves
		//Save deleted arcs first
		operation *op1 = mck->operations[i - 1];
		operation *op2 = mck->operations[i];
		tabu *t1 = new tabu(op1->UID, op2->UID, mck->ID, 1);
		tabumlist->push_back(t1);

		op1 = mck->operations[i];
		op2 = mck->operations[i + 1];
		t1 = new tabu(op1->UID, op2->UID, mck->ID, 1);
		tabumlist->push_back(t1);


		if (!((k == l) && (j - i == 1))) {
			op1 = target_mc->operations[j - 1];
			op2 = target_mc->operations[j];
			t1 = new tabu(op1->UID, op2->UID, target_mc->ID, 1);
			tabumlist->push_back(t1);
		}
		

		op1 = target_mc->operations[j];
		op2 = target_mc->operations[j + 1];
		t1 = new tabu(op1->UID, op2->UID, target_mc->ID, 1);
		tabumlist->push_back(t1);
	}
	

	//Do the move
	//Removing both ops
	currentSol->removeOp(target_mc, j);
	currentSol->removeOp(mck, i);
 
	//Changing Machines to Ops
 	opi->changeMach(target_mc, j);
	opj->changeMach(mck, i);
	//Insert Ops
	mck->insertOp(opj, i);
	currentSol->insertOpToGraphs(opj);
	target_mc->insertOp(opi, j);
	currentSol->insertOpToGraphs(opi);
	

	//printf("Exchange Move\n");
	{
		//Application Phase
		if (!currentSol->scheduleAEAP(false, true)){
			cout << "EXEI PAIXTEI MALAKIA STO EXCHANGE" << endl;
			cout << *currentSol;
			cout << "Solution Forward Graph" << endl;
			currentSol->FGraph->report();
			assert(false);
		}

		/*cout << "Exchange after move application" << endl;
		cout << *currentSol;
		cout << "Solution Forward Graph" << endl;
		currentSol->forwardGraph->report();*/

		//Get Move Costs
		currentSol->getCost(false);
		
		//Copy Move Costs to ref array
		copy(currentSol->d_cost_array, currentSol->d_cost_array + COST_NUM, cost_ar);
		
		//cout << "Final Solution" << endl;
		//cout << *currentSol;
		
		//Mark moved operations
		opi->move_freq++;
		opj->move_freq++;
		
		//reportMoves();
		//CleanUp
		//Clean Tabulist
		(this->*clearTabu)();
	}
}

void localSearch::exchange_perturb(int k, int l, int i, int j, bool actual, float *cost_ar)
{

	currentSol->replace_temp_values();
	Graph<operation> *gr = currentSol->FGraph;

	/*
	if (actual) {
	cout << "EXCHANGE INTRO PHASE REPLACED_TEMP SOLUTION" << endl;
	cout << *currentSol;
	//currentSol->DGraph->report();
	}
	*/



	//cout << "Start Solution" << endl;
	//cout << *currentSol;

	int n = 0; //Loops
	int cutoff = 0;

	machine *mck = currentSol->solmachines[k];
	machine *mcl = currentSol->solmachines[l];

	machine *target_mc = mcl;

	if (k == l) {
		target_mc = mck;
	}

	//j will be always greater than i
	operation *opi = mck->operations[i];
	operation *opj = target_mc->operations[j];

	//if (actual)
	//	printf("Swapping %d %d %d %d - UIDs %d %d \n", k, l, i, j, opi->UID, opj->UID);

	if ((j < i) && (k == l)) {
		printf("Input i %d j %d \n", i, j);
		printf("Swapping INputs Necessary\n");
		assert(false);
		/*
		//Swap the inputs, should work
		operation *t = opj;
		opj = opi;
		opi = t;
		int t_i = i;
		i = j;
		j = t_i;
		*/
	}

	{
		//save the tabu moves before applying the move
		//Save moves
		//Save deleted arcs first
		operation *op1 = mck->operations[i - 1];
		operation *op2 = mck->operations[i];
		tabu *t1 = new tabu(op1->UID, op2->UID, mck->ID, 1);
		tabumlist->push_back(t1);

		op1 = mck->operations[i];
		op2 = mck->operations[i + 1];
		t1 = new tabu(op1->UID, op2->UID, mck->ID, 1);
		tabumlist->push_back(t1);


		if (!((k == l) && (j - i == 1))) {
			op1 = target_mc->operations[j - 1];
			op2 = target_mc->operations[j];
			t1 = new tabu(op1->UID, op2->UID, target_mc->ID, 1);
			tabumlist->push_back(t1);
		}


		op1 = target_mc->operations[j];
		op2 = target_mc->operations[j + 1];
		t1 = new tabu(op1->UID, op2->UID, target_mc->ID, 1);
		tabumlist->push_back(t1);
	}


	//Do the move
	//Removing both ops
	currentSol->removeOp(target_mc, j);
	currentSol->removeOp(mck, i);

	//Changing Machines to Ops
	opi->changeMach(target_mc, j);
	opj->changeMach(mck, i);
	//Insert Ops
	mck->insertOp(opj, i);
	currentSol->insertOpToGraphs(opj);
	target_mc->insertOp(opi, j);
	currentSol->insertOpToGraphs(opi);


	{
		//save created arcs after applying the move
		//Save moves
		//Save created arcs first
		operation *op2 = mck->operations[i + 1];
		tabu *t1 = new tabu(opj->UID, op2->UID, mck->ID, 1);
		tabumlist->push_back(t1);

		op2 = mck->operations[i - 1];
		t1 = new tabu(op2->UID, opj->UID, mck->ID, 1);
		tabumlist->push_back(t1);

		op2 = target_mc->operations[j + 1];
		t1 = new tabu(opi->UID, op2->UID, target_mc->ID, 1);
		tabumlist->push_back(t1);

		if (!((k == l) && (j - i == 1))) {
			op2 = target_mc->operations[j - 1];
			t1 = new tabu(op2->UID, opi->UID, target_mc->ID, 1);
			tabumlist->push_back(t1);
		}
	}

	//printf("Exchange Move\n");
	{
		//Application Phase
		if (!currentSol->scheduleAEAP(false, true)) {
			cout << "EXEI PAIXTEI MALAKIA STO EXCHANGE" << endl;
			cout << *currentSol;
			cout << "Solution Forward Graph" << endl;
			currentSol->FGraph->report();
			assert(false);
		}

		/*cout << "Exchange after move application" << endl;
		cout << *currentSol;
		cout << "Solution Forward Graph" << endl;
		currentSol->forwardGraph->report();*/

		//Get Move Costs
		currentSol->getCost(false);

		//Copy Move Costs to ref array
		copy(currentSol->d_cost_array, currentSol->d_cost_array + COST_NUM, cost_ar);

		//cout << "Final Solution" << endl;
		//cout << *currentSol;

		//Mark moved operations
		opi->move_freq++;
		opj->move_freq++;

		//reportMoves();
		//CleanUp
		//Clean Tabulist
        (this->*clearTabu)();
	}
}



bool localSearch::ls_perturb_smart_relocate(int &ke, int &le, int &ie, int &je, int mode, bool firstbest, solution *s2)
{
	Graph<operation> *gr_f = currentSol->FGraph;
	Graph<operation> *gr_b = currentSol->BGraph;
	bool vrika = false;
	float cost_ar[COST_NUM];
	float minim_ar[COST_NUM];
	for (int i = 0; i < COST_NUM; i++) {
		cost_ar[i] = INF;
		minim_ar[i] = INF;
	}

	float q_metric;
	float best_q_metric = -INF;

	//findCritical();

	//printf("LS_Smart Relocate\n");
	//cout << *currentSol;
	//for (int i = 0; i < tabumlist.size(); i++)
	//	tabumlist[i]->report();
	//printf("-----------------\n");
	
	/*
	printf("ForGraph\n");
	gr->state = FORWARD;
	gr->report();
	printf("BackGraph\n");
	gr->state = BACKWARD;
	gr->report();
	*/


	int tabuMCounter = 0;
	int probCounter = 0;

	//MoveStatusCounters
	int imprCounter = 0;
	int worseCounter = 0;
	int innertieCounter = 0;
	int outertieCounter = 0;


	//Store Topological order map
	gr_f->topological_sort();

	//for (int ji = 0; ji < currentSol->soljobs.size(); ji++) {
	//	job *jc = currentSol->soljobs[ji];
	for (int ji = 0; ji < currentSol->solmachines.size(); ji++) {
		machine *mmc = currentSol->solmachines[ji];
		for (int jj = 1; jj < mmc->operations.size() - 1; jj++) {
			//Load one operation for testing
			//operation *op = jc->operations[jj];
			operation *op = mmc->operations[jj];

			//Store operations attributes for bringing it back later
			int old_mc_id = mmc->ID;
			int old_pos = op->mthesi;

			//Mark the two first broken arcs that are caused by removing op from solution
			bool tabu_move;
			bool tabu_move_p1 = false;


			// Search for created arcs
			tabu_move_p1 = findMoveinList(op->mc->operations[old_pos - 1],
				op->mc->operations[old_pos + 1],
				op->mc, 0);

			//Clear Operation Infeasibility and Moving Status
			for (int i = 0; i < currentSol->solmachines.size(); i++) {
				machine *mc = currentSol->solmachines[i];
				for (int j = 0; j < mc->operations.size() - 1; j++) {
					mc->operations[j]->infeasible = false;
					mc->operations[j]->moving = 0;
				}
			}

			//DFS on the right of the suc (if any)
			if (op->has_sucs()) {
				operation *suc = op->get_suc();
				gr_f->dfs(suc->UID);
				for (int i = 0; i<gr_f->visitedNodes_n; i++) {
					operation* movOp = gr_f->visitedNodes[i];
					//printf("Forward BlackListed Operation %d %d \n", movOp->jobID, movOp->ID);
					movOp->infeasible = true;
				}
			}

			//DFS on the left of the pred (if any)
			if (op->has_preds()) {
				operation *pred = op->get_pred();
				gr_b->dfs(pred->UID);
				//In this case we need to blacklist the machine predecessor of the visited nodes
				for (int i = 0; i<gr_b->visitedNodes_n; i++) {
					operation *vOp = gr_b->visitedNodes[i];
					//Check for dummy operations
					if (!vOp->valid) continue;
					operation *movOp = vOp->get_mach_pred();

					//printf("Backward BlackListed Operation %d %d \n", movOp->jobID, movOp->ID);
					movOp->infeasible = true;
				}
			}

			//Neglect the machine predecessor to prevent placement on the same position
			operation *imm_pred;
			if (op->has_mach_preds()) {
				imm_pred = op->get_mach_pred();
				imm_pred->infeasible = true;
			}

			//Remove Operation from solution
			//printf("Solution after op %d %d removal from machine %d pos %d \n",op->jobID, op->ID, op->mc->ID, op->mthesi);
			currentSol->removeOp(op->mc, op->mthesi);
			//cout << *currentSol;
			//printf("-------------------------\n");

			//Probing Loop
			for (int i = 0; i < currentSol->solmachines.size(); i++) {
				machine *mc = currentSol->solmachines[i];
				if (!op->getProcTime(mc)) continue;

				for (int j = 0; j<mc->operations.size() - 1; j++) {
					operation *cand = mc->operations[j];
					if (cand->infeasible) {
						if (op->check_sucRelation(cand)) {
							//printf("Cyclic Infeasibility breaking \n");
							break;
						}
						else {
							//printf("Infeasible movement after Candidate %d %d\n", cand->jobID, cand->ID);
							//continue; Is there any point to continue probing on positions after an infeasible candidate?
							break;
						}
					}
					probCounter++;

					//I can probe at this point
					//printf("Moving %d %d after operation %d %d %d \n", 
					//	op->jobID, op->ID, cand->jobID, cand->ID, cand->UID);
					smart_relocate(op, old_mc_id, mc->ID, old_pos, cand->mthesi + 1, cost_ar, q_metric);
					//printf("Costs %d %d %d %d \n", cost1, cost2, cost3, cost4);
					
					//bool ts_comparison = solution::Objective_TS_Comparison( (int*) (const int []) {cost1, cost2, cost3, cost4}, (int*) (const int[]) { minim1, minim2, minim3, minim4 });
					bool ts_comparison = solution::Objective_TS_Comparison(cost_ar, minim_ar);

					//printf("Neighbor Distance %d %d\n", cost_ar[DISTANCE], cost_ar[MAKESPAN]);

					if (ts_comparison) {
						imprCounter++;
						//Check if move is tabu
						bool tabu_move_p2 = true;
						//Search for created arcs
						tabu_move_p2 &= findMoveinList(mc->operations[j],
							op,
							mc, 0);

						tabu_move_p2 &= findMoveinList(op,
							mc->operations[j + 1],
							mc, 0);


						//Search for destroyed arcs
						/*if (j < mc->operations.size() - 2) {
						tabu_move_p2 = findMove(mc->operations[j],
						mc->operations[j + 1],
						mc, 0);
						}*/

                        tabu_move = tabu_move_p1 && tabu_move_p2;

                        /*if (tabu_move) {
                        printf("MOVE IS TABU \n");
                        printf("Removed operation %d %d \n", op->jobID, op->ID);
                        printf("Inserted to machine %d pos %d", mc->ID, cand->mthesi + 1);
                        cout << *currentSol;
                        }*/

                        /*if (tabu_move) {
                        printf("MOVE IS TABU\n");
                        printf("Removed operation %d %d\n", op->jobID, op->ID);
                        printf("Inserted to machine %d pos %d\n", mc->ID, cand->mthesi + 1);
                        }*/

                        if (!tabu_move) {
                            //printf("New Better Move\n");
                            copy(cost_ar, cost_ar + COST_NUM, minim_ar);
                            best_q_metric = q_metric;
                            ie = old_pos; je = j + 1; ke = old_mc_id; le = mc->ID;
                            //printf("Better Move %d %d %d %d Relax %d\n", ie, je, ke, le, relax);
                            vrika = true;
                        }
                        else tabuMCounter++;
					}
					else
						worseCounter++;
				}
			}

			//Bring Operation Back in place
			currentSol->insertOp(mmc, old_pos, op);
		}
	}

	moveStats[TABU] += tabuMCounter;

	//printf("Final Costs %d %d %d %d \n", minim1, minim2, minim3, minim4);
	//printf("LS Smart Relocate Improving %d Outer Ties %d Inner Ties %d Worse %d\n", imprCounter, outertieCounter, innertieCounter, worseCounter);

	//char test = 0;
	//scanf("%c", &test);
	//printf("Done\n");

	return vrika;
}

bool localSearch::ls_perturb_smart_exchange(int &ke, int &le, int &ie, int &je, int mode, bool firstbest, solution *s2)
{
	Graph<operation> *gr = currentSol->FGraph;
	bool vrika = false;
	float cost_ar[COST_NUM];
	float minim_ar[COST_NUM];
	for (int i = 0; i < COST_NUM; i++) {
		cost_ar[i] = INF;
		minim_ar[i] = INF;
	}

	float q_metric;
	float best_q_metric = -INF;

	//findCritical();
	//printf("LS_Smart Exchange\n");
	//cout << *currentSol;
	//printf("ForGraph\n");
	//currentSol->forwardGraph->report();
	//printf("BackGraph\n");
	//currentSol->backwardGraph->report();

	int tabuMCounter = 0;
	int probCounter = 0;

	//MoveStatusCounters
	int imprCounter = 0;
	int worseCounter = 0;
	int innertieCounter = 0;
	int outertieCounter = 0;

	//First machine loop
	for (int mi = 0; mi < currentSol->solmachines.size(); mi++) {
		machine *old_mc = currentSol->solmachines[mi];

		//First Operation Loop
		for (int oi = 1; oi < old_mc->operations.size() - 1; oi++) {
			operation *op1 = old_mc->operations[oi];

			//Second Machine loop
			for (int mj = mi; mj < currentSol->solmachines.size(); mj++) {
				machine *new_mc = currentSol->solmachines[mj];

				//Clear Operation Infeasibility and Moving Status
				for (int i = 0; i<currentSol->solmachines.size(); i++) {
					machine *mc = currentSol->solmachines[i];
					for (int j = 0; j<mc->operations.size() - 1; j++) {
						mc->operations[j]->infeasible = false;
						//mc->operations[j]->moving = 0;
					}
				}

				int alreadyMoving = 0;

				//Remove Op1 from solution
				int old_pos_1 = op1->mthesi;
				currentSol->removeOp(old_mc, old_pos_1);

				//Second Operation loop
				int start_pos = (mi == mj) ? oi : 1;
				for (int oj = start_pos; oj < new_mc->operations.size() - 1; oj++) {
					operation *cand = new_mc->operations[oj]; //Swap candidate

															  //Prune
					if (!op1->getProcTime(new_mc)) break;
					if (!cand->getProcTime(old_mc)) continue;

					//Do the swap
					//printf("Swap Machines %d %d Positions %d %d \n", mi, mj, old_pos_1, oj);
					//printf("Solution before probe\n");
					//cout << *currentSol;

					//Probe the move
					bool feasible = smart_exchange(op1, cand, old_pos_1, old_mc, new_mc, cost_ar, q_metric, 10000);
					probCounter++;
					//printf("Solution after probe\n");
					//cout << *currentSol;


					//bool ts_comparison = solution::Objective_TS_Comparison( (int*) (const int []) {cost1, cost2, cost3, cost4}, (int*) (const int[]) { minim1, minim2, minim3, minim4 });
					bool ts_comparison = solution::Objective_TS_Comparison(cost_ar, minim_ar);

					if (ts_comparison && feasible) {
						//if (feasible) {
						imprCounter++;
						//Check if move is tabu
						bool tabu_move = true;
						//Search for created arcs
						//TODO: RE-ADD THE PROPER TABU SEARCHES
						tabu_move &= findMoveinList(old_mc->operations[oi - 1],
							new_mc->operations[oj],
							old_mc, 1);

						tabu_move &= findMoveinList(new_mc->operations[oj],
							((mi == mj) && (oj == oi)) ? op1 : old_mc->operations[oi],
							old_mc, 1);


						if (!((mi == mj) && (oj == oi))) {
							tabu_move &= findMoveinList(new_mc->operations[oj - 1],
								op1,
								new_mc, 1);
						}

						tabu_move &= findMoveinList(op1,
							new_mc->operations[oj + 1],
							new_mc, 1);

						//Reset tabu status under probability if the move is improving
						if (tabu_move && (randd() < tabu_relaxation)) tabu_move = false;

						if (!tabu_move) {
							copy(cost_ar, cost_ar + COST_NUM, minim_ar);
							best_q_metric = q_metric;
							ie = old_pos_1; je = (mi == mj) ? cand->mthesi + 1 : cand->mthesi;
							ke = old_mc->ID; le = new_mc->ID;
							vrika = true;
							//printf("New Better Move\n");
							//printf("%d, %d, %d, %d\n", ke, le, ie, je);
						}
						else
							tabuMCounter++;

					}
					else
						worseCounter++;

					//else if (cost1==0 && cost2==0 && cost3 == 0)
					//printf("Tie\n");
				}
				currentSol->insertOp(old_mc, old_pos_1, op1);
			}
		}

	}


	moveStats[TABU] += tabuMCounter;

	//printf("SMART EXCHANGE Final Costs %d %d %d %d \n", minim1, minim2, minim3, minim4);
	//printf("Final Move %d %d %d %d \n", ke, le, ie, je);
	//printf("LS Smart Exchange Improving %d Outter Ties %d Inner Ties %d Worse %d\n", imprCounter, outertieCounter, innertieCounter, worseCounter);

	//char test = 0;
	//scanf("%c", &test);

	return vrika;
}

bool localSearch::ls_smart_exchange(int &ke, int &le, int &ie, int &je, int mode, bool firstbest, bool improve_only)
{
	Graph<operation> *gr = currentSol->FGraph;
	bool vrika = false;
	float cost_ar[COST_NUM];
	float minim_ar[COST_NUM];
	for (int i = 0; i < COST_NUM; i++) {
		cost_ar[i] = INF;
		minim_ar[i] = INF;
	}

	float q_metric;
	float best_q_metric = -INF;

	//findCritical();
	//printf("LS_Smart Exchange\n");
	//cout << *currentSol;
	//printf("ForGraph\n");
	//currentSol->forwardGraph->report();
	//printf("BackGraph\n");
	//currentSol->backwardGraph->report();

	int tabuMCounter = 0;
	int probCounter = 0;

	//MoveStatusCounters
	int imprCounter = 0;
	int worseCounter = 0;
	int innertieCounter = 0;
	int outertieCounter = 0;

	//First machine loop
	for (int mi = 0; mi < currentSol->solmachines.size(); mi++) {
		machine *old_mc = currentSol->solmachines[mi];

		//First Operation Loop
		for (int oi = 1; oi < old_mc->operations.size() - 1; oi++) {
			operation *op1 = old_mc->operations[oi];

			//Second Machine loop
			for (int mj = mi; mj < currentSol->solmachines.size(); mj++) {
				machine *new_mc = currentSol->solmachines[mj];

				//Clear Operation Infeasibility and Moving Status
				for (int i = 0; i<currentSol->solmachines.size(); i++) {
					machine *mc = currentSol->solmachines[i];
					for (int j = 0; j<mc->operations.size() - 1; j++) {
						mc->operations[j]->infeasible = false;
						mc->operations[j]->moving = 0;
					}
				}

				//Remove Op1 from solution
				int old_pos_1 = op1->mthesi;
				currentSol->removeOp(old_mc, old_pos_1);

				//Second Operation loop
				int start_pos = (mi == mj) ? oi : 1;
				for (int oj = start_pos; oj < new_mc->operations.size() - 1; oj++) {
					operation *cand = new_mc->operations[oj]; //Swap candidate

															  //Prune
					if (!op1->getProcTime(new_mc)) break;
					if (!cand->getProcTime(old_mc)) continue;

					//Do the swap
					//printf("Swap Machines %d %d Positions %d %d \n", mi, mj, old_pos_1, oj);
					//printf("Solution before probe\n");
					//cout << *currentSol;

					//Probe the move
					bool feasible = smart_exchange(op1, cand, old_pos_1, old_mc, new_mc, cost_ar, q_metric, 10000);
					probCounter++;
					//printf("Solution after probe\n");
					//cout << *currentSol;


					//bool ts_comparison = solution::Objective_TS_Comparison( (int*) (const int []) {cost1, cost2, cost3, cost4}, (int*) (const int[]) { minim1, minim2, minim3, minim4 });
					bool ts_comparison = solution::Objective_TS_Comparison(cost_ar, minim_ar);

					//Check for ties with the main solution
					if (cost_ar[MAKESPAN] == 0 && cost_ar[TOTAL_FLOW_TIME] == 0 && cost_ar[TOTAL_IDLE_TIME] == 0 && feasible) {
						outertieCounter++;
						if (q_metric > best_q_metric) {
							//printf("TIE CANDIDATE! New %d vs Current %d \n", q_metric, best_q_metric);
							//Accept tie if its is further than the current best neighbor
							copy(cost_ar, cost_ar + COST_NUM, minim_ar);
							best_q_metric = q_metric;
							ie = old_pos_1; je = (mi == mj) ? cand->mthesi + 1 : cand->mthesi;
							ke = old_mc->ID; le = new_mc->ID;
							vrika = true;
						}
					}
					//Check for ties with the current best move
					else if (feasible && cost_ar[MAKESPAN] - minim_ar[MAKESPAN] == 0 && cost_ar[TOTAL_FLOW_TIME] - minim_ar[TOTAL_FLOW_TIME] == 0 && cost_ar[TOTAL_IDLE_TIME] - minim_ar[TOTAL_IDLE_TIME] == 0) {
						innertieCounter++;
						if (q_metric > best_q_metric) {
							//printf("TIE CANDIDATE! New %d vs Current %d \n", q_metric, best_q_metric);
							//Accept tie if its is further than the current best neighbor
							copy(cost_ar, cost_ar + COST_NUM, minim_ar);
							best_q_metric = q_metric;
							ie = old_pos_1; je = (mi == mj) ? cand->mthesi + 1 : cand->mthesi;
							ke = old_mc->ID; le = new_mc->ID;
							vrika = true;
						}
					}
					else if (ts_comparison && feasible) {
						//if (feasible) {
						imprCounter++;
						//Check if move is tabu
						bool tabu_move = true;
						//Search for created arcs
						//TODO: RE-ADD THE PROPER TABU SEARCHES
						tabu_move &= (this->*findMove)(old_mc->operations[oi - 1],
							new_mc->operations[oj],
							old_mc, 1);

						tabu_move &= (this->*findMove)(new_mc->operations[oj],
							((mi == mj) && (oj == oi)) ? op1 : old_mc->operations[oi],
							old_mc, 1);


						if (!((mi == mj) && (oj == oi))) {
							tabu_move &= (this->*findMove)(new_mc->operations[oj - 1],
								op1,
								new_mc, 1);
						}

						tabu_move &= (this->*findMove)(op1,
							new_mc->operations[oj + 1],
							new_mc, 1);

						//Reset tabu status under probability if the move is improving
						if (tabu_move && (randd() < tabu_relaxation)) tabu_move = false;

						if (!tabu_move) {
							copy(cost_ar, cost_ar + COST_NUM, minim_ar);
							best_q_metric = q_metric;
							ie = old_pos_1; je = (mi == mj) ? cand->mthesi + 1 : cand->mthesi;
							ke = old_mc->ID; le = new_mc->ID;
							vrika = true;
							//printf("New Better Move\n");
							//printf("%d, %d, %d, %d\n", ke, le, ie, je);
						}
						else if (Objective_TS_Comparison_Asp())
						{
							copy(cost_ar, cost_ar + COST_NUM, minim_ar);
							best_q_metric = q_metric;
							ie = old_pos_1; je = (mi == mj) ? cand->mthesi + 1 : cand->mthesi;
							ke = old_mc->ID; le = new_mc->ID;
							moveStats[ASPIRATION]++;
							//cout << "Aspiration Condition! " << cost1 << endl;
							//In this case the move has to be done as is in order to set the new best
							//Bring Operation Back in place before returning
							//currentSol->insertOp(old_mc, old_pos_1, op1);
							//return true;

							//EDIT: Not sure if I have to return at this point,
							//maybe there is a better aspiration move that I'll miss if I leave
							//printf("[ASPIRATION] New Better Move\n");
							//printf("%d, %d, %d, %d\n", ke, le, ie, je);
						}
						else
							tabuMCounter++;

					}
					else
						worseCounter++;

					//else if (cost1==0 && cost2==0 && cost3 == 0)
					//printf("Tie\n");
				}
				currentSol->insertOp(old_mc, old_pos_1, op1);
			}
		}

	}


	moveStats[TABU] += tabuMCounter;

	//printf("SMART EXCHANGE Final Costs %d %d %d %d \n", minim1, minim2, minim3, minim4);
	//printf("Final Move %d %d %d %d \n", ke, le, ie, je);
	//printf("LS Smart Exchange Improving %d Outter Ties %d Inner Ties %d Worse %d\n", imprCounter, outertieCounter, innertieCounter, worseCounter);

	//char test = 0;
	//scanf("%c", &test);

	if (improve_only) {
		//Compare best found move with the currentSol costs
		if (solution::Objective_TS_Comparison(minim_ar, currentSol->cost_array))
			return true;

		return vrika;
	}
	else
		return vrika;
}

bool localSearch::ls_smart_relocate(int &ke, int &le, int &ie, int &je, int mode, bool firstbest, bool improve_only)
{
	Graph<operation> *gr_f = currentSol->FGraph;
	Graph<operation> *gr_b = currentSol->BGraph;
	bool vrika = false;
	float cost_ar[COST_NUM];
	float minim_ar[COST_NUM];
	for (int i = 0; i < COST_NUM; i++) {
		cost_ar[i] = INF;
		minim_ar[i] = INF;
	}

	float q_metric;
	float best_q_metric = -INF;

	int tabuMCounter = 0;
	int probCounter = 0;

	//MoveStatusCounters
	int imprCounter = 0;
	int worseCounter = 0;
	int innertieCounter = 0;
	int outertieCounter = 0;


	//for (int ji = 0; ji < currentSol->soljobs.size(); ji++) {
	//	job *jc = currentSol->soljobs[ji];
	for (int ji = 0; ji < currentSol->solmachines.size(); ji++) {
		machine *mmc = currentSol->solmachines[ji];
		for (int jj = 1; jj < mmc->operations.size() - 1; jj++) {
			//Load one operation for testing
			//operation *op = jc->operations[jj];
			operation *op = mmc->operations[jj];

			//Store operations attributes for bringing it back later
			int old_mc_id = mmc->ID;
			int old_pos = op->mthesi;

			//Mark the two first broken arcs that are caused by removing op from solution
			bool tabu_move;
			bool tabu_move_p1 = false;


			// Search for created arcs
			tabu_move_p1 = (this->*findMove)(op->mc->operations[old_pos - 1],
				op->mc->operations[old_pos + 1],
				op->mc, 0);

			//Clear Operation Infeasibility and Moving Status
            for (int i=0; i<currentSol->soloperations.size(); i++)
                currentSol->soloperations[i]->infeasible = false;

            //DFS on the right of the suc (if any)
			if (op->has_sucs()) {
				operation *suc = op->get_suc();
                gr_f->dfs(suc->UID);

                for (int i = 0; i<gr_f->visitedNodes_n; i++) {
					operation* movOp = gr_f->visitedNodes[i];
					//printf("Forward BlackListed Operation %d %d \n", movOp->jobID, movOp->ID);
					movOp->infeasible = true;
				}

			}

			//DFS on the left of the pred (if any)
			if (op->has_preds()) {
				operation *pred = op->get_pred();
                gr_b->dfs(pred->UID);
				//In this case we need to blacklist the machine predecessor of the visited nodes
				for (int i = 0; i<gr_b->visitedNodes_n; i++) {
					operation *vOp = gr_b->visitedNodes[i];
					//Check for dummy operations
					if (!vOp->valid) continue;
					operation *movOp = vOp->get_mach_pred();

					//printf("Backward BlackListed Operation %d %d \n", movOp->jobID, movOp->ID);
					movOp->infeasible = true;
				}
			}

			//Neglect the machine predecessor to prevent placement on the same position
			operation *imm_pred;
			if (op->has_mach_preds()) {
				imm_pred = op->get_mach_pred();
				imm_pred->infeasible = true;
			}

			//Remove Operation from solution
			//printf("Solution after op %d %d removal from machine %d pos %d \n",op->jobID, op->ID, op->mc->ID, op->mthesi);
			currentSol->removeOp(op->mc, op->mthesi);
			//cout << *currentSol;
			//printf("-------------------------\n");

			//Probing Loop
			for (int i = 0; i < currentSol->solmachines.size(); i++) {
				machine *mc = currentSol->solmachines[i];
				if (!op->getProcTime(mc)) continue;

				for (int j = 0; j<mc->operations.size() - 1; j++) {
					operation *cand = mc->operations[j];
					if (cand->infeasible) {
						if (op->check_sucRelation(cand)) {
							//printf("Cyclic Infeasibility breaking \n");
							break;
						} else
                            continue;
					}
					probCounter++;

					//I can probe at this point
					printf("Moving %d %d after operation %d %d %d \n",
						op->jobID, op->ID, cand->jobID, cand->ID, cand->UID);
					smart_relocate(op, old_mc_id, mc->ID, old_pos, cand->mthesi + 1, cost_ar, q_metric);
                    printf("Costs %4.1f %4.1f %4.1f %4.1f \n", cost_ar[MAKESPAN], cost_ar[TOTAL_FLOW_TIME], cost_ar[TOTAL_IDLE_TIME], cost_ar[DISTANCE]);

					//bool ts_comparison = solution::Objective_TS_Comparison( (int*) (const int []) {cost1, cost2, cost3, cost4}, (int*) (const int[]) { minim1, minim2, minim3, minim4 });
					bool ts_comparison = solution::Objective_TS_Comparison(cost_ar, minim_ar);

#if TIE_BREAKER > 0
					//Check for ties with the main solution
					if (cost_ar[MAKESPAN] == 0 && cost_ar[TOTAL_FLOW_TIME] == 0 && cost_ar[TOTAL_IDLE_TIME] == 0) {
						outertieCounter++;
						if (q_metric > best_q_metric) {
							printf("Outer TIE CANDIDATE! New %d vs Current %d \n", q_metric, best_q_metric);
							//Accept tie if its is further than the current best neighbor
							copy(cost_ar, cost_ar + COST_NUM, minim_ar);
							best_q_metric = q_metric;
							ie = old_pos; je = j + 1; ke = old_mc_id; le = mc->ID;
							vrika = true;
						}
					}
					//Check for ties with the current best move
					else if (cost_ar[MAKESPAN] - minim_ar[MAKESPAN] == 0 && cost_ar[TOTAL_FLOW_TIME] - minim_ar[TOTAL_FLOW_TIME] == 0 && cost_ar[TOTAL_IDLE_TIME] - minim_ar[TOTAL_IDLE_TIME] == 0) {
						innertieCounter++;
						if ((q_metric > best_q_metric)) {
							printf("Inner TIE CANDIDATE! New %d vs Current %d \n", q_metric, best_q_metric);
							//Accept tie if its is further than the current best neighbor
							copy(cost_ar, cost_ar + COST_NUM, minim_ar);
							best_q_metric = q_metric;
							ie = old_pos; je = j + 1; ke = old_mc_id; le = mc->ID;
							vrika = true;
						}
					}
					else if (ts_comparison) {
#else
					if (ts_comparison) {
#endif
						imprCounter++;
						//Check if move is tabu
						bool tabu_move_p2 = true;
                        //Search for created arcs
                        tabu_move_p2 &= (this->*findMove)(mc->operations[j],
                                                         op,
                                                         mc, 0);

                        tabu_move_p2 &= (this->*findMove)(op,
                                                         mc->operations[j + 1],
                                                         mc, 0);

                        printf("Searching for Arcs : [%d-%d], [%d-%d], [%d-%d]\n",
                               mmc->operations[old_pos - 1]->UID, mmc->operations[old_pos]->UID,
                               mc->operations[j]->UID, op->UID,
                               op->UID, mc->operations[j + 1]->UID);

						//Search for destroyed arcs
						/* if (j < mc->operations.size() - 2) {
						tabu_move_p2 = findMove(mc->operations[j],
						mc->operations[j + 1],
						mc, 0);
						} */

						tabu_move = tabu_move_p1 && tabu_move_p2;

						/*if (tabu_move) {
						printf("MOVE IS TABU \n");
						printf("Removed operation %d %d \n", op->jobID, op->ID);
						printf("Inserted to machine %d pos %d", mc->ID, cand->mthesi + 1);
						cout << *currentSol;
						}*/

						//Reset tabu status under probability if the move is improving
						bool relax = false;

						if (tabu_move && (randd() < tabu_relaxation)) {
							relax = true;
							tabu_move = false;
						}

                        /*if (tabu_move) {
						printf("MOVE IS TABU\n");
						printf("Removed operation %d %d\n", op->jobID, op->ID);
						printf("Inserted to machine %d pos %d\n", mc->ID, cand->mthesi + 1);
						}*/

						if (!tabu_move) {
							//printf("New Better Move\n");
							copy(cost_ar, cost_ar + COST_NUM, minim_ar);
							best_q_metric = q_metric;
							ie = old_pos; je = j + 1; ke = old_mc_id; le = mc->ID;
							//printf("Better Move %d %d %d %d Relax %d\n", ie, je, ke, le, relax);
							vrika = true;
						}
						else if (Objective_TS_Comparison_Asp())
						{
							copy(cost_ar, cost_ar + COST_NUM, minim_ar);
							best_q_metric = q_metric;
							ie = old_pos; je = j + 1; ke = old_mc_id; le = mc->ID;
							moveStats[ASPIRATION]++;
							//printf("Aspiration Move %d %d %d %d Relax %d\n", ie, je, ke, le, relax);
                        }
						else tabuMCounter++;
					}
					else
						worseCounter++;


                    char in_c;
                    printf("Press any key to continue...\n");
                    scanf( "%c", &in_c);
				}
			}

			//Bring Operation Back in place
			currentSol->insertOp(mmc, old_pos, op);
		}
	}

	moveStats[TABU] += tabuMCounter;
    moveStats[OUTER_TIES] += outertieCounter;
    moveStats[INNER_TIES] += innertieCounter;


	//printf("Final Costs %d %d %d %d \n", minim1, minim2, minim3, minim4);
	//printf("LS Smart Relocate Improving %d Outer Ties %d Inner Ties %d Worse %d\n", imprCounter, outertieCounter, innertieCounter, worseCounter);

	//char test = 0;
	//scanf("%c", &test);
	//printf("Done\n");

	if (improve_only) {
		//Compare best found move with the currentSol costs
		if (solution::Objective_TS_Comparison(minim_ar, currentSol->cost_array))
			return true;

		return vrika;
	}
	else
		return vrika;
}

bool localSearch::FindNeighbors(int &ke, int &le, int &ie, int &je, int mode, bool firstbest)
{
	bool vrika = false;
	float cost_ar[COST_NUM];
	float minim_ar[COST_NUM];
	for (int i = 0; i < COST_NUM; i++) {
		cost_ar[i] = INF;
		minim_ar[i] = INF;
	}
	
	findCritical();
	char test = 0;
	scanf("%c", &test);

	/*printf("Temp Solution before Move \n");
	cout << *currentSol;
	currentSol->forwardGraph->report();*/

	int n; //used in loops
	for (size_t k = 0, n = (int) currentSol->solmachines.size(); k < n; k++)
	{
		machine *mck = currentSol->solmachines[k];
		int mck_n = (int) mck->operations.size();
		if (mck_n <= 2) continue;

		int s1 = -1, s2 = -1;

		switch (mode)
		{
			case RELOCATE: s1 = 0; s2 = n; break;
			case EXCHANGE: s1 = k; s2 = n; break;
		}
		// //TWEAK FOR JSSP INSTANCES
		// s1 = k;
		// s2 = k + 1;

		for (int l = s1; l < s2; l++)
		{
			machine *mcl = currentSol->solmachines[l];
			int mcl_n = (int) mcl->operations.size();
			
			//Selecting machines from the same process process_stepID
			if (mck->process_stepID != mcl->process_stepID) continue;

			if ((mode == EXCHANGE) && (mcl_n <= 2)) continue;
			
			for (int i = 1; i < mck_n - 1; i++)
			{
				int s3 = -1, s4 = -1;
				switch (mode)
				{
					//Greg Fixes no reason to skip one position
					/*
					case 0: if (k == l) { s3 = 2;  s4 = mck->operations.size() - 1; }
					else { s3 = 1; s4 = mcl->operations.size() - 1; } break;
					case 1: if (k == l) { s3 = i + 2; s4 = mck->operations.size() - 1; }
					else { s3 = 1; s4 = mcl->operations.size() - 1; }break;
					*/
					case RELOCATE:
						s3 = 1;
						s4 = mcl_n - 1; //Refix after relocate fixes 
						break;
					case EXCHANGE:
						if (k == l) { s3 = i + 1;}
						else {s3 = 1;}
						s4 = mcl_n - 1;
						break;
				}

				for (int j = s3; j< s4; j++)
				{
					bool mpikee = false;
					//cout<<"Evaluate Neighbor: "<<mode<<" "<<k<<" "<<l<<" "<<i<<" "<<j<<endl;
					switch (mode)
					{
						case RELOCATE:
						{
							if ((k == l) && (i == j)) continue;
							operation *op = mck->operations[i];
							//Check if the operation can go to mcl
							if (!op->getProcTime(mcl)) continue;
							
							mpikee = true;
							relocate(k, l, i, j, false, cost_ar);
							
						} break;
						case EXCHANGE:
						{
							operation *op1 = mck->operations[i];
							//Check if op1 can go to mcl
							if (!op1->getProcTime(mcl)) continue;

							operation *op2 = mcl->operations[j];
							//Check if op2 can go to mck
							if (!op2->getProcTime(mck)) continue;
	
							mpikee = true;
							exchange(k, l, i, j, false, cost_ar);
							
						} break;
					}

					//printf("Temp Solution after Move \n");
					//cout << *currentSol;
					//currentSol->forwardGraph->report();

					//For now load cost arrays manually
					// In the future this should be automated throughout the code (cost arrays/costObjects everywhere)
					//bool ts_comparison = solution::Objective_TS_Comparison( (int*) (const int []) {cost1, cost2, cost3, cost4}, (int*) (const int[]) { minim1, minim2, minim3, minim4 });
					bool ts_comparison = solution::Objective_TS_Comparison(cost_ar, minim_ar);
					//printf("Current Move Costs: %d %d %d %d \n", cost1, cost2, cost3, cost4);
					//printf("Minim Move Costs: %d %d %d %d \n", minim1, minim2, minim3, minim4);
					//cout << "FindNeighbors Objective_TS_Comparison result: " << ts_comparison << endl;
					
					if (mpikee && ts_comparison)
					{
						bool tabu_move = false;
						int t_c = 0;
						//Search for the move
						switch (mode) {
							case RELOCATE:
								tabu_move = findRelocate(k, l, i, j);
								break;
							case EXCHANGE:
								tabu_move = findExchange(k, l, i, j);
								break;
						}

						//Reset tabu status under probability if the move is improving
						if (tabu_move && randd() > 0.95) tabu_move = false;
						
						if (tabu_move) moveStats[TABU]++;
						
						if (!tabu_move) {
							copy(cost_ar, cost_ar + COST_NUM, minim_ar);
							ie = i; je = j; ke = k; le = l;
							vrika = true;

							if (firstbest)
							{
								if (optimtype == 1) if (cost_ar[MAKESPAN] < 0) return true;
								if (optimtype == 2) if (cost_ar[TOTAL_FLOW_TIME] < 0) return true;
								if (optimtype == 3) if (cost_ar[TOTAL_IDLE_TIME] < 0) return true;
								if (optimtype == 4) if (cost_ar[MAKESPAN] < 0 || (cost_ar[MAKESPAN] <= 0 && cost_ar[TOTAL_FLOW_TIME] < 0) || (cost_ar[MAKESPAN] <= 0 && cost_ar[TOTAL_FLOW_TIME] <= 0 && cost_ar[TOTAL_IDLE_TIME] < 0)) return true;
								if (optimtype == 5) if (cost_ar[TOTAL_FLOW_TIME] < 0 || (cost_ar[TOTAL_FLOW_TIME] <= 0 && cost_ar[TOTAL_IDLE_TIME] < 0) || (cost_ar[TOTAL_FLOW_TIME] <= 0 && cost_ar[TOTAL_IDLE_TIME] <= 0 && cost_ar[MAKESPAN] < 0)) return true;
								if (optimtype == 6) if (cost_ar[TOTAL_IDLE_TIME] < 0 || (cost_ar[TOTAL_IDLE_TIME] <= 0 && cost_ar[MAKESPAN] < 0) || (cost_ar[TOTAL_IDLE_TIME] <= 0 && cost_ar[MAKESPAN] <= 0 && cost_ar[TOTAL_FLOW_TIME] < 0)) return true;
								if (optimtype == 7) if (cost_ar[MAKESPAN] < 0 || (cost_ar[MAKESPAN] <= 0 && cost_ar[TOTAL_IDLE_TIME] < 0) || (cost_ar[MAKESPAN] <= 0 && cost_ar[TOTAL_IDLE_TIME] <= 0 && cost_ar[TOTAL_FLOW_TIME] < 0)) return true;
								if (optimtype == 8) if (cost_ar[TOTAL_FLOW_TIME] < 0 || (cost_ar[TOTAL_FLOW_TIME] <= 0 && cost_ar[MAKESPAN] < 0) || (cost_ar[TOTAL_FLOW_TIME] <= 0 && cost_ar[MAKESPAN] <= 0 && cost_ar[TOTAL_IDLE_TIME] < 0)) return true;
								if (optimtype == 9) if (cost_ar[TOTAL_IDLE_TIME] < 0 || (cost_ar[TOTAL_IDLE_TIME] <= 0 && cost_ar[MAKESPAN] < 0) || (cost_ar[TOTAL_IDLE_TIME] <= 0 && cost_ar[TOTAL_FLOW_TIME] <= 0 && cost_ar[MAKESPAN] < 0)) return true;
								if (optimtype == 10) if (cost_ar[TOTAL_ENERGY] < 0) return true;
								if (optimtype == 11) if (cost_ar[TOTAL_ENERGY] < 0 || (cost_ar[TOTAL_ENERGY] <= 0 && cost_ar[MAKESPAN] < 0) || (cost_ar[TOTAL_ENERGY] <= 0 && cost_ar[MAKESPAN] <= 0 && cost_ar[TOTAL_FLOW_TIME] < 0)) return true;
								if (optimtype == 12) if (cost_ar[TOTAL_ENERGY] < 0 || (cost_ar[TOTAL_ENERGY] <= 0 && cost_ar[TOTAL_FLOW_TIME] < 0) || (cost_ar[TOTAL_ENERGY] <= 0 && cost_ar[TOTAL_FLOW_TIME] <= 0 && cost_ar[MAKESPAN] < 0)) return true;
							}
						}
						else if (Objective_TS_Comparison_Asp())
						{
							copy(cost_ar, cost_ar + COST_NUM, minim_ar);
							ie = i; je = j; ke = k; le = l;
							moveStats[ASPIRATION]++;
							//cout << "Aspiration Condition! " << cost1 << endl;
							//In this case the move has to be done as is in order to set the new best
							return true;
						}
					}
				}
			}
		}
	}
	return vrika;
}

bool localSearch::FindNeighborsJobs(int &ce, int &ie, int &ue, int mode, int segment_size, int displacement, solution *s2)
{
	//return false;

	//TODO: Redesign when specs are finilized 
	//cout << "FINDNEIGHBORJOBS START SOLUTION" << endl;
	//cout << *currentSol;

	float minim_ar[COST_NUM];
	float cost_ar[COST_NUM];
	for (int i = 0; i < COST_NUM; i++) {
		minim_ar[i] = INF;
		cost_ar[i] = INF;
	}

	int q_metric = -INF;
	int best_q_metric = -INF;

	bool vrika = false;
	int improvingCounter = 0;
	int outertieCounter = 0;
	int innertieCounter = 0;
	int worseCounter = 0;

	//This loop tries to find a range of operations within a job
	//where all operations are scheduled in different machines
	for (size_t c = 0; c < currentSol->soljobs.size(); c++)
	{
		job *jb = currentSol->soljobs[c];
		int job_n = jb->operations.size();
		for (size_t i = 0; i < job_n - 1; i++)
		{
			operation *op1 = jb->operations[i];
			//SEGMENT SIZE CAN REPLACE THIS FOR
			//for (int u = i + 2; u <= min(job_n, segment_size); u++)
			int u = i + segment_size;
			if (u > job_n) continue; //Not enough operations to perform anything
			
			{
				int g1 = i;
				//At this point the range bounds have been selected.
				//Range Check for machine feasibility follows
				int max_FWD_disp = 10;
				int max_BWD_disp = 10;
				for (g1 = i; g1<u; g1++)
				{
					operation *op2 = jb->operations[g1];
					max_FWD_disp = min(max_FWD_disp, (int) op2->mc->operations.size() - 2 - op2->mthesi);
					max_BWD_disp = min(max_BWD_disp, (int) op2->mthesi - 1);
					int g2 = g1;
					//		if(c==2&&i==0&&u==2) cout<<op2->machID<< " ";
					for (g2 = g1 + 1; g2<u; g2++)
					{
						operation *op3 = jb->operations[g2];
						if (op3->machID == op2->machID) break;
					}
					if (g2 != u) break;
				}
				if (g1 != u) {
					//Neglect the probed sequence
					CompoundRangeSelector->update(segment_size, false);
					//printf("Range %d - %d of job %d infeasible\n", i, u, c);
					continue;
				}
				//	if(c==2&&i==0&&u==2) {cout<<endl;cout<<endl;}

				//cout << *currentSol;
				//printf("Calculated Max FWD Displacement %d \n", max_FWD_disp);
				//printf("Calculated Max BWD Displacement %d \n", max_BWD_disp);

				for (int ii = 0; ii < COST_NUM; ii++)
					cost_ar[ii] = INF;
				
				bool tabu_move = true;
				switch (mode)
				{
					case 0: {
						if (!max_FWD_disp) continue;
						JobShiftbyOneFWD(cost_ar, c, i, u, false, tabu_move, q_metric); 
					}; break;
					
					case 1: {
						if (!max_BWD_disp) continue;
						JobShiftbyOneBWD(cost_ar, c, i, u, false, tabu_move, q_metric);
					}; break;
				};

				//bool ts_comparison = solution::Objective_TS_Comparison( (int*) (const int []) {cost1, cost2, cost3, cost4}, (int*) (const int[]) { minim1, minim2, minim3, minim4 });
				bool ts_comparison = solution::Objective_TS_Comparison(cost_ar, minim_ar);
				//printf("Current Move Costs: %d %d %d %d \n", cost1, cost2, cost3, cost4);
				//printf("Minim Move Costs: %d %d %d %d \n", minim1, minim2, minim3, minim4);
				//cout << "FindNeighborJobs Objective_TS_Comparison result: " << ts_comparison << endl;
				int tslist_n = (int) tabumlist->size();
				CompoundRangeSelector->update(u - i, ts_comparison);
				
				//Check for ties with the main solution
				if (cost_ar[MAKESPAN] == 0 && cost_ar[TOTAL_FLOW_TIME] == 0 && cost_ar[TOTAL_IDLE_TIME] == 0) {
					outertieCounter++;
					if ( q_metric > best_q_metric ) {
						//printf("Sol TIE CANDIDATE! New %d vs Current %d \n", q_metric, best_q_metric);
						//Accept tie if its is further than the current best neighbor
						copy(cost_ar, cost_ar + COST_NUM, minim_ar);
						best_q_metric = q_metric;
						ce = c; ie = i; ue = u;
						vrika = true;
					}
				}
				//Check for ties with the current best move
				else if (cost_ar[MAKESPAN] - minim_ar[MAKESPAN] == 0 && cost_ar[TOTAL_FLOW_TIME] - minim_ar[TOTAL_FLOW_TIME] == 0 && cost_ar[TOTAL_IDLE_TIME] - minim_ar[TOTAL_IDLE_TIME] == 0) {
					innertieCounter++;
					if (q_metric > best_q_metric) {
						//printf("Move TIE CANDIDATE! New %d vs Current %d \n", q_metric, best_q_metric);
						//Accept tie if its is further than the current best neighbor
						copy(cost_ar, cost_ar + COST_NUM, minim_ar);
						best_q_metric = q_metric;
						ce = c; ie = i; ue = u;
						vrika = true;
					}
				}
				else if (ts_comparison)
				{
					improvingCounter++;
					//printf("Moving Job %d Range %d-%d Moving Ops %d Costs %d %d %d %d\n", c, i, u, u - i, cost1, cost2, cost3, cost4);
					if (tabu_move) if (randd() < tabu_relaxation) tabu_move = false;
					if (!tabu_move)
					{
						//printf("Move Found\n");
						copy(cost_ar, cost_ar + COST_NUM, minim_ar);
						best_q_metric = q_metric;
						ce = c; ie = i; ue = u;
						vrika = true;
						//		cout<<ce<<" "<<ie<<" "<<ue<<" "<<cost1<<" "<<cost2<<" "<<cost3<<" "<<cost4<<endl;
						//return vrika;
					}
					else if (Objective_TS_Comparison_Asp())
					{
						//printf("Aspirational Move Found\n");
						copy(cost_ar, cost_ar + COST_NUM, minim_ar);
						best_q_metric = q_metric;
						ce = c; ie = i; ue = u;
						vrika = true;
						moveStats[ASPIRATION]++;
						//		cout<<ce<<" "<<ie<<" "<<ue<<" "<<cost1<<" "<<cost2<<" "<<cost3<<" "<<cost4<<endl;
						//return vrika;
						//	return vrika;
					}
					else {
						//printf("Move %d not accepted - TABU -  \n", Counter);
						moveStats[TABU]++;
					}
				}
				else
					worseCounter++;

			}
		}
	}
	
	/*printf("Improving Moves Histogram: ");
	for (int i = 0; i < 10; i++) 
		printf("%d:%4.3f ", i, CompoundRangeSelector->n_move_frac[i]);
	printf("\n");*/
	//printf("Total Improving Moves %d\n", improvingCounter);
	
	//if (!vrika)
		//printf("ASDASDASDASDASD");
	
	return vrika;
}

void localSearch::RevertShiftBWD(int jobid, int range_a, int range_b) {

	job *jb = currentSol->soljobs[jobid];

	for (int y2 = range_a; y2<range_b; y2++)
	{
		operation *op2 = jb->operations[y2];
		machine *mc = currentSol->solmachines[op2->machID];

		int g = op2->mthesi;
		
		//mc->operations->RemoveAt(g);
		//vRemoveAt<operation*>(mc->operations, g);
		currentSol->removeOp(mc, g);
		op2->changeMach(mc, g + 1);

		//mc->operations->InsertAt(g + 1, op2);
		//vInsertAt<operation*>(mc->operations, g + 1, op2);
		mc->insertOp(op2, g + 1);
		currentSol->insertOpToGraphs(op2);
		
		//Fix done status of operations
		op2->setDoneStatus(false, true);
	}
}

void localSearch::RevertShiftFWD(int jobid, int range_a, int range_b) {

	job *jb = currentSol->soljobs[jobid];

	for (int y2 = range_a; y2<range_b; y2++)
	{
		operation *op2 = jb->operations[y2];
		machine *mc = currentSol->solmachines[op2->machID];
		
		//Since I'm using the removeOp methods, I can safely use the mthesi
		//attributes to index the operations
		int g = op2->mthesi;
		
		currentSol->removeOp(mc, g);
		op2->changeMach(mc, g - 1);
		
		mc->insertOp(op2, g - 1);
		currentSol->insertOpToGraphs(op2);
		
		//Fix Done Status of operation
		op2->setDoneStatus(false, true);
	}

	//printf("Solution after FWD Revert\n");
	//cout << *currentSol;
}


//Make moves void and give values to cost variables
void localSearch::JobShiftbyOneBWD(float *cost_ar, int c, int i, int u, bool actual, bool &tabu_move, int &q_metric)
{
	/*			DESCRIPTION

	Within the selected range from FindNeighborJobs
	shift all operations of the selected job back by one

	*/

	if (actual && ((u - i) == 1)) {
		cout << "Entering JobShiftbyOneBWD Move" << endl;
		printf("Applying BWD %d %d %d\n", c, i, u);
	}

	currentSol->replace_temp_values();
	Graph<operation> *gr = currentSol->FGraph;
	//int move = INF; //cost variables are initiallized in finrneighborjobs

	job *jb = currentSol->soljobs[c];
	
	//Check the move first
	for (int y = i; y<u; y++)
	{
		operation *op1 = jb->operations[y];
		machine *mc = currentSol->solmachines[op1->machID];
		int k = op1->mthesi;
		
		if (k <= 1) return;
		else {
			operation *op3 = mc->operations[k - 1];
			if (op3->jobID == op1->jobID)
			{
				//cout << "Infeasible Move" << endl;
				return;
			}
		}
	}

	//Apply the move
	for (int y = i; y<u; y++)
	{
		operation *op1 = jb->operations[y];
		machine *mc = currentSol->solmachines[op1->machID];

		int k = op1->mthesi;

		//mc->operations->RemoveAt(k);
		//vRemoveAt<operation*>(mc->operations, k);
		currentSol->removeOp(mc, k);
		op1->changeMach(mc, k - 1);

		//mc->operations->InsertAt(k - 1, op1);
		//vInsertAt<operation*>(mc->operations, k-1, op1);
		mc->insertOp(op1, k - 1);
		currentSol->insertOpToGraphs(op1);
	}

	//Get movingOps AFTER applying the move (Using the current state)
	/*
    for (int y = i; y < u; y++)
	{
		operation *op1 = jb->operations[y];
		gr->dfs(op1->UID);
		int n = gr->visitedNodes_n;
		for (int i = 0; i < n; i++) {
			operation *movOp = gr->visitedNodes[i];
			if (!vContains<operation*>(movingOps, movOp)) movingOps.push_back(movOp);
		}
	}
	*/

	//Check if move is tabu (move has already been applied)
	bool temp_tabu = true;
	if (!actual) {
		for (int y = i; y < u; y++) {
			machine *mc = jb->operations[y]->mc;
			operation *op1, *op2, *op;
			op = jb->operations[y];
			
			//op1 = op;
			op2 = mc->operations[op->mthesi + 1];
			temp_tabu &= (this->*findMove)(op,
				op2, mc, 2);

			op1 = mc->operations[op->mthesi + 1];
			op2 = mc->operations[op->mthesi + 2];
			temp_tabu &= (this->*findMove)(op1,
				op2, mc, 2);

			op1 = mc->operations[op->mthesi - 1];
			//op2 = mc->operations[op->mthesi - 1];
			temp_tabu &= (this->*findMove)(op1,
				op, mc, 2);
		}
	}
	tabu_move = temp_tabu;


	//Check for cycles
	bool cycleflag = false;
	for (int y = i; y < u; y++)
	{
		operation *op1 = jb->operations[y];
		//Find movingOps
		gr->dfs(op1->UID);
		cycleflag |= gr->cycleflag;
	}

	//Mark movingOps
	currentSol->setDoneStatus(!actual, false);

    if (!actual)
	{
		//Testing Phase
		if (!cycleflag){
			if (currentSol->scheduleAEAP(true, false) && currentSol->find_conflicts(true))
			{
				//TABU SOLUTION COSTS
				currentSol->getCost(true);
				copy(currentSol->temp_cost_array, currentSol->temp_cost_array + COST_NUM, cost_ar);
				q_metric = applyTieBreaker();
			} else {
				//cout << "Infeasible Move" << endl;
				memset(cost_ar, INF, sizeof(int)*COST_NUM);
			}
		}
		//Revert Move 
		RevertShiftBWD(c, i, u);
	}
	else
	{
		//Permanent Application Phase
		if (!currentSol->scheduleAEAP(false, false)) {
			cout << *currentSol;
			cout << "Comp BWD Oups" << endl;
			printf("Parameters c %d i %d u %d\n", c, i, u);
		}
		
		currentSol->getCost(false);
		copy(currentSol->d_cost_array, currentSol->d_cost_array + COST_NUM, cost_ar);
		
		//replace_real_values();
		//Save tabu moves
		//Save destroyed arcs
		if (arc_policy.save_deleted) {
			for (int y = i; y < u; y++) {
				operation *op = jb->operations[y];
				machine *mc = op->mc;
				operation *op1, *op2;
				tabu *t1;

				op1 = mc->operations[op->mthesi - 1];
				op2 = mc->operations[op->mthesi + 1];
				t1 = new tabu(op1->UID, op2->UID, mc->ID, 2);
				tabumlist->push_back(t1);


				op1 = mc->operations[op->mthesi + 1];
				//op2 = mc->operations[op1->mthesi - 1];
				t1 = new tabu(op1->UID, op->UID, mc->ID, 2);
				tabumlist->push_back(t1);

				//op1 = op;
				op2 = mc->operations[op->mthesi + 2];
				t1 = new tabu(op->UID, op2->UID, mc->ID, 2);
				tabumlist->push_back(t1);
			}
		}
		
		//Save created arcs
		if (arc_policy.save_created) {
			for (int y = i; y < u; y++) {
				operation *op = jb->operations[y];
				machine *mc = op->mc;
				operation *op1, *op2;
				tabu *t1;

				op1 = mc->operations[op->mthesi - 1];
				t1 = new tabu(op1->UID, op->UID, mc->ID, 2);
				tabumlist->push_back(t1);

				op1 = mc->operations[op->mthesi + 1];
				op2 = mc->operations[op->mthesi + 2];
				t1 = new tabu(op1->UID, op2->UID, mc->ID, 2);
				tabumlist->push_back(t1);

				op1 = mc->operations[op->mthesi + 1];
				t1 = new tabu(op->UID, op1->UID, mc->ID, 2);
				tabumlist->push_back(t1);
			}
		}
		//Clean Tabulist
        (this->*clearTabu)();

    }
	return;
}

void localSearch::JobShiftbyOneFWD(float *cost_ar, int c, int i, int u, bool actual, bool &tabu_move, int &q_metric)
{
	/*			DESCRIPTION

	Within the selected range from FindNeighborJobs
	shift all operations of the selected job by one
	on their machines

	*/
	if (actual && ((u-i) == 1)){
		cout << "Entering JobShiftbyOneFWD Move" << endl;
		printf("Applying FWD %d %d %d\n", c, i, u);
	}
	currentSol->replace_temp_values();
	Graph<operation> *gr = currentSol->FGraph;
	gr->state = FORWARD;

	job *jb = currentSol->soljobs[c];
	
	//Check move before making it
	for (int y = i; y<u; y++)
	{
		operation *op1 = jb->operations[y];
		machine *mc = op1->mc;
		int k = op1->mthesi;

		//If to delete and return without the for
		//Revert back move if the operation is last on its machine. Return INF
		if (k == (int) mc->operations.size() - 2) return;
		else {
			operation *op3 = mc->operations[k + 1];
			//This check handles the case where the operation in the next machine position
			//belongs to the same job
			if (op3->jobID == op1->jobID) {
				//cout << "FWD Shift infeasible" << endl; 
				return; 
			}
		}
	}

	//Get movingOps before applying the move (Using the current state)
	/*
    for (int y = i; y < u; y++)
	{
		operation *op1 = jb->operations[y];
		//Find movingOps
		gr->dfs(op1->UID);
		int n = gr->visitedNodes_n;
		for (int i = 0; i < n; i++) {
			operation *movOp = gr->visitedNodes[i];
			if (!vContains<operation*>(movingOps, movOp)) movingOps.push_back(movOp);
		}
	}
	*/

	//Apply the Move
	for (int y = i; y<u; y++)
	{
		operation *op1 = jb->operations[y];
		machine *mc = op1->mc;
		int k = op1->mthesi;

		//mc->operations->RemoveAt(k);
		//vRemoveAt<operation*>(mc->operations, k);
		currentSol->removeOp(mc, k);
		op1->changeMach(mc, k + 1);
		//mc->operations->InsertAt(k + 1, op1);
		//vInsertAt<operation*>(mc->operations, k + 1, op1);
		mc->insertOp(op1, k + 1);
		currentSol->insertOpToGraphs(op1);
	}

	//printf("Compound FWD temp solution\n");
	//cout << *currentSol;

	//Check for cycles
	bool cycleflag = false;
	for (int y = i; y < u; y++)
	{
		operation *op1 = jb->operations[y];
		//Find movingOps
		gr->dfs(op1->UID);
		cycleflag |= gr->cycleflag;
	}

	//Mark movingOps
	currentSol->setDoneStatus(!actual, false);
	/*
    for (size_t ii = 0; ii < movingOps.size(); ii++) {
		operation *movOp = movingOps[ii];
		movOp->setDoneStatus(!actual, false);
	}
	*/

	//Check if move is tabu (move has already been applied)
	bool temp_tabu = true;
	if (!actual) {
		for (int y = i; y < u; y++) {
			machine *mc = jb->operations[y]->mc;
			operation *op1, *op2, *op;
			op = jb->operations[y];
			op1 = op;
			op2 = mc->operations[op->mthesi + 1];
			temp_tabu &= (this->*findMove)(op1,
				op2, mc, 2);

			//op1 = jb->operations[y];
			op2 = mc->operations[op->mthesi - 1];
			temp_tabu &= (this->*findMove)(op2,
				op, mc, 2);

			op1 = mc->operations[op->mthesi - 2];
			op2 = mc->operations[op->mthesi - 1];
			temp_tabu &= (this->*findMove)(op1,
				op2, mc, 2);
		}
	}
	tabu_move = temp_tabu;
	
	if (!actual)
	{
		//TESTING PHASE
		//Recalculating schedule and reverting the move back
		if (!cycleflag){
			currentSol->scheduleAEAP(true, false);
			if (!currentSol->find_conflicts(true)) return;
		
			currentSol->getCost(true);
			copy(currentSol->temp_cost_array, currentSol->temp_cost_array + COST_NUM, cost_ar);
			
			q_metric = applyTieBreaker();
			//printf("FWD QMetric %d\n", q_metric);
		} else {
			//Illegal Move probably
			//printf("Illegal Move %d %d %d\n", c, i, u);
			memset(cost_ar, INF, sizeof(int)*COST_NUM);
		}

		
		//Revert Move
		RevertShiftFWD(c, i, u);

		//cout << "Current Solution after move revert" << endl;
		//cout << *currentSol;
	}
	else
	{
		//APPLICATION PHASE
		//cout << "Applying JobShiftbyOneFWD Move" << endl;
		//Make move permanent
		//printf("Applying JobShiftbyOneFWD \n");
		if (!currentSol->scheduleAEAP(false, false)) cout << "Compound FWD Oups" << endl;

		currentSol->getCost(false);
		copy(currentSol->d_cost_array, currentSol->d_cost_array + COST_NUM, cost_ar);
		
		//Save tabu moves
		//Save destroyed arcs
		if (arc_policy.save_deleted) {
			for (int y = i; y < u; y++) {
				operation *op = jb->operations[y];
				machine *mc = op->mc;
				operation *op1, *op2;
				tabu *t1;
			
				op1 = mc->operations[op->mthesi - 2];
				t1 = new tabu(op1->UID, op->UID, mc->ID, 2);
				tabumlist->push_back(t1);

				//op1 = mc->operations[op1->mthesi - 2];
				op2 = mc->operations[op->mthesi - 1];
				t1 = new tabu(op->UID, op2->UID, mc->ID, 2);
				tabumlist->push_back(t1);

				op1 = mc->operations[op->mthesi - 1];
				op2 = mc->operations[op->mthesi + 1];
				t1 = new tabu(op1->UID, op2->UID, mc->ID, 2);
				tabumlist->push_back(t1);
			}
		}
		//Save created arcs
		if (arc_policy.save_created) {
			for (int y = i; y < u; y++) {
				operation *op = jb->operations[y];
				machine *mc = op->mc;
				operation *op1, *op2;
				tabu *t1;

				op1 = mc->operations[op->mthesi + 1];
				t1 = new tabu(op->UID, op1->UID, mc->ID, 2);
				tabumlist->push_back(t1);

				op1 = mc->operations[op->mthesi - 2];
				op2 = mc->operations[op->mthesi - 1];
				t1 = new tabu(op1->UID, op2->UID, mc->ID, 2);
				tabumlist->push_back(t1);

				op1 = mc->operations[op->mthesi - 1];
				t1 = new tabu(op1->UID, op->UID, mc->ID, 2);
				tabumlist->push_back(t1);
			}
		}
		//Clean Tabulist
        (this->*clearTabu)();
    }

	return;
}


int localSearch::RemoveAndReinsert(solution *sol)
{
	//		int param = 5;
	sol->opshelp.clear();
	sol->Init_Resourse_Loads();
	//		for (int t = 0; t<param; t++)
	//		{
	//int jnn   = rand()%jobs->GetSize();
	int index = -1;
	int max = 0;

	//Look for the latest job and store its index at index
	for (int g = 0; g<(int) sol->solmachines.size(); g++)
	{
		machine *mc = sol->solmachines[g];
		operation *op1 = mc->operations[(int) mc->operations.size() - 2];

#if(FLEX)
		if (max < op1->temp_start + op1->getProcTime())
		{
			max = op1->temp_start + op1->getProcTime();
			index = op1->jobID;
		}
#else
		if (max < get<0>(op1->start_times)_temp + op1->processing_time)
		{
			max = get<0>(op1->start_times)_temp + op1->processing_time;
			index = op1->jobID;
		}
#endif
	}

	// OVERRIDE LATE JOB WITH A RANDOM JOB
	index = rand() % sol->soljobs.size();

	job *jb = sol->soljobs[index];
	
	/*cout << "PERTURBATION START SOLUTION" << endl;
	cout << *sol;*/

	/*Graph *gb = sol->createGraph(1);
	if (!(*gb == *sol->forwardGraph)) {
		cout << "GRAPH UNEQUALITY BEFORE PERTURBATION" << endl;
		cout << "PERTURBATION START SOLUTION" << endl;
		cout << *sol;
		cout << "Recalculated" << endl;
		gb->report();
		cout << "Inserted" << endl;
		sol->forwardGraph->report();
		assert(false);
		int boom;
		scanf("BULLSHIT %d", &boom);
	}
	delete gb;*/

	//Lookin and removing the job operations from the machines
	for (int y = 0; y<(int) sol->solmachines.size(); y++)
	{
		machine *mc = sol->solmachines[y];
		for (int h = 1; h<(int) mc->operations.size() - 1; h++)
		{
			operation *op = mc->operations[h];
			if (op->jobID == jb->ID)
			{
				//op->done = false;
				//op->temp_done = false;
				
				op->start = op->a;
#if(FLEX)
				op->end = op->start + op->getProcTime() + op->cleanup_time + op->setup_time;
#else
				op->end = op->start + op->processing_time + op->cleanup_time + op->setup_time;
#endif
				op->temp_start = op->start;
				op->temp_end = op->end;
				//mc->operations->RemoveAt(h);
				//mc->operations.erase(mc->operations.begin() + h);
				//vRemoveAt<operation*>(mc->operations, h);
				//mc->removeOp(h);
				sol->removeOp(mc, h);
				
				//Make sure to force the temp status after removing
				//the op from the solution and the graph
				op->setDoneStatus(false, false); 
				h--;
			}
		}
	}
	
	//Removing the job now, so that insertoneadditionaljob can add it again later
	//sol->soljobs.erase(sol->soljobs.begin() + index);
	vRemoveAt<job*>(sol->soljobs, index);
	
	//sol->jobshelp.push_back(jb);
	for (size_t i = 0; i < jb->operations.size(); i++) 
		sol->opshelp.push_back(jb->operations[i]);
	
	sol->Calculate_Resourse_Loads();
	sol->scheduleAEAP(false, true);
	
	sol->greedyConstruction(false);
	//sol->InsertOneAdditionalNew(jb); //Trying to deprecate this
	
	vInsertAt<job*>(sol->soljobs, index, jb); //Add job back to its original place
	if (!sol->scheduleAEAP(false, true)) { //Recalculate Schedules
		cout << "MALAKIA SE SCHEDULEAEAP STO PERTURBATION" << endl;
		assert(false);
	};
	sol->testsolution(); //Do the final feasibility test
	sol->opshelp.clear();

	//Finalize
	sol->replace_temp_values();
	sol->getCost(false);
	
	//cout << "INSERTION PARTIAL FORWARD GRAPH" << endl;
	//sol->forwardGraph->report();

	//Insert removed operations back to graph
	//printf("REMOVING JOB %d Operations\n", jb->ID);
	for (size_t ii = 0; ii < jb->operations.size(); ii++) {
		operation * op = jb->operations[ii];
		sol->insertOpToGraphs(op);
	}

	/*Graph *g = sol->createGraph(1);
	if (!(*g == *sol->forwardGraph)) {
		cout << "RECHECK" << endl;
		*g == *sol->forwardGraph;
		cout << "PERTURBATION END SOLUTION" << endl;
		cout << *sol;
		cout << "GRAPH UNEQUALITY AFTER PERTURBATION" << endl;
		cout << "Recalculated" << endl;
		g->report();
		cout << "Inserted" << endl;
		sol->forwardGraph->report();
		assert(false);
		int boom;
		scanf("BULLSHIT %d", &boom);
	}
	delete g;*/

	/*cout << "PERTURBATION SOLUTION" << endl;
	cout << *sol;

	cout << "INSERTION FORWARD GRAPH" << endl;
	sol->forwardGraph->report();*/

	//Recalculate Graphs
	//delete sol->forwardGraph;
	//cout << "RECALCULATED FORWARD GRAPH" << endl;
	//Graph *g = sol->createGraph(1);
	//g->report();
	
	return 0;
}

// bool localSearch::Objective_TS_Comparison(int cost1, int cost2, int cost3, int cost4, int mcost1, int mcost2, int mcost3, int mcost4)
// {
// 	bool flag = false;
// 	if (optimtype == 1) if (cost1 < mcost1) flag = true;
// 	if (optimtype == 2) if (cost2 < mcost2) flag = true;
// 	if (optimtype == 3) if (cost3 < mcost3) flag = true;
// 	if (optimtype == 4) if (cost1 < mcost1 || (cost1 <= mcost1 && cost2 < mcost2) || (cost1 <= mcost1 && cost2 <= mcost2 && cost3 < mcost3)) flag = true;
// 	if (optimtype == 5) if (cost2 < mcost2 || (cost2 <= mcost2 && cost3 < mcost3) || (cost2 <= mcost2 && cost3 <= mcost3 && cost1 < mcost1)) flag = true;
// 	if (optimtype == 6) if (cost3 < mcost3 || (cost3 <= mcost3 && cost1 < mcost1) || (cost3 <= mcost3 && cost1 <= mcost1 && cost2 < mcost2)) flag = true;
// 	if (optimtype == 7) if (cost1 < mcost1 || (cost1 <= mcost1 && cost3 < mcost3) || (cost1 <= mcost1 && cost3 <= mcost3 && cost2 < mcost2)) flag = true;
// 	if (optimtype == 8) if (cost2 < mcost2 || (cost2 <= mcost2 && cost1 < mcost1) || (cost2 <= mcost2 && cost1 <= mcost1 && cost3 < mcost3)) flag = true;
// 	if (optimtype == 9) if (cost3 < mcost3 || (cost3 <= mcost3 && cost2 < mcost2) || (cost3 <= mcost3 && cost2 <= mcost2 && cost1 < mcost1)) flag = true;
// 	if (optimtype == 10) if (cost4 < mcost4) flag = true;
// 	if (optimtype == 11) if (cost4 < mcost4 || (cost4 <= mcost4 && cost1 < mcost1) || (cost4 <= mcost4 && cost1 <= mcost1 && cost2 < mcost2)) flag = true;
// 	if (optimtype == 12) if (cost4 < mcost4 || (cost4 <= mcost4 && cost2 < mcost2) || (cost4 <= mcost4 && cost2 <= mcost2 && cost1 < mcost1)) flag = true;
// 	return flag;
// }

// bool localSearch::Objective_TS_Comparison_Asp(solution *sol, int mcost1, int mcost2, int mcost3, int mcost4)
// {
// 	//This is probably wront because temp_cost_array includes the move cost so there is no reason to add it again
// 	bool flag = false;
// 	int cost1 = sol->temp_cost_array[MAKESPAN];
// 	int cost2 = sol->temp_cost_array[TOTAL_FLOW_TIME];
// 	int cost3 = sol->temp_cost_array[TOTAL_IDLE_TIME];
// 	int cost4 = sol->temp_cost_array[TOTAL_ENERGY];

// 	//localElite Costs
// 	int l_cost1 = localElite->cost_array[MAKESPAN];
// 	int l_cost2 = localElite->cost_array[TOTAL_FLOW_TIME];
// 	int l_cost3 = localElite->cost_array[TOTAL_IDLE_TIME];
// 	int l_cost4 = localElite->cost_array[TOTAL_ENERGY];

// 	if (optimtype == 1 ) if (cost1 + mcost1 < l_cost1)  flag = true;
// 	if (optimtype == 2 ) if (cost2 + mcost2 < l_cost2) flag = true;
// 	if (optimtype == 3 ) if (cost3 + mcost3 < l_cost3) flag = true;
// 	if (optimtype == 4 ) if (cost1 + mcost1 < l_cost1 || (cost1 + mcost1 <= l_cost1 && cost2 + mcost2 < l_cost2) || (cost1 + mcost1 <= l_cost1 && cost2 + mcost2 <= l_cost2 && cost3 + mcost3 < l_cost3)) flag = true;
// 	if (optimtype == 5 ) if (cost2 + mcost2 < l_cost2 || (cost2 + mcost2 <= l_cost2 && cost3 + mcost3 < l_cost3) || (cost2 + mcost2 <= l_cost2 && cost3 + mcost3 <= l_cost3 && cost1 + mcost1 < l_cost1)) flag = true;
// 	if (optimtype == 6 ) if (cost3 + mcost3 < l_cost3 || (cost3 + mcost3 <= l_cost3 && cost1 + mcost1 < l_cost1) || (cost3 + mcost3 <= l_cost3 && cost1 + mcost1 <= l_cost1 && cost2 + mcost2 < l_cost2)) flag = true;
// 	if (optimtype == 7 ) if (cost1 + mcost1 < l_cost1 || (cost1 + mcost1 <= l_cost1 && cost3 + mcost3 < l_cost3) || (cost1 + mcost1 <= l_cost1 && cost3 + mcost3 <= l_cost3 && cost2 + mcost2 < l_cost2)) flag = true;
// 	if (optimtype == 8 ) if (cost2 + mcost2 < l_cost2 || (cost2 + mcost2 <= l_cost2 && cost1 + mcost1 < l_cost1) || (cost2 + mcost2 <= l_cost2 && cost1 + mcost1 <= l_cost1 && cost3 + mcost3 < l_cost3)) flag = true;
// 	if (optimtype == 9 ) if (cost3 + mcost3 < l_cost3 || (cost3 + mcost3 <= l_cost3 && cost2 + mcost2 < l_cost2) || (cost3 + mcost3 <= l_cost3 && cost2 + mcost2 <= l_cost2 && cost1 + mcost1 < l_cost1)) flag = true;
// 	if (optimtype == 10) if (cost4 + mcost4 < l_cost4) flag = true;
// 	if (optimtype == 11) if (cost4 + mcost4 < l_cost4 || (cost4 + mcost4 <= l_cost4 && cost1 + mcost1 < l_cost1) || (cost4 + mcost4 <= l_cost4 && cost1 + mcost1 <= l_cost1 && cost2 + mcost2 < l_cost2)) flag = true;
// 	if (optimtype == 12) if (cost4 + mcost4 < l_cost4 || (cost4 + mcost4 <= l_cost4 && cost2 + mcost2 < l_cost2) || (cost4 + mcost4 <= l_cost4 && cost2 + mcost2 <= l_cost2 && cost1 + mcost1 < l_cost1)) flag = true;
// 	return flag;
// }

bool localSearch::Objective_TS_Comparison_Asp()
{
	//Compare temp sol with localElite
	float s1_costs[COST_NUM];
	float s2_costs[COST_NUM];

	for (int i = 0; i < COST_NUM; i++) {
		s1_costs[i] = currentSol->temp_cost_array[i];
		s2_costs[i] = localElite->cost_array[i];
		//s2_costs[i] = globals->elite->cost_array[i];
	}
	
	return solution::Objective_TS_Comparison(s1_costs, s2_costs);
}

bool localSearch::vns()
{
	printf("starting VNS\n");
	time_t ts_time = time(NULL);

	currentSol->replace_temp_values();
	if (!currentSol->testsolution()) return false;

	currentSol->find_conflicts(false);
	currentSol->getCost(false); 
	/*tabu_cost  = tabu_solution->cost;
	tabu_scost = tabu_solution->s_cost;
	tabu_tcost = tabu_solution->t_cost;
	tabu_ecost = tabu_solution->e_cost;*/

	//printf("Input Graph\n");
	//currentSol->forwardGraph->report();

	float cost_ar[COST_NUM];
	float mcost_ar[COST_NUM];
	float minim_ar[COST_NUM];
	for (int i = 0; i < COST_NUM; i++) {
		cost_ar[i] = INF;
		mcost_ar[i] = INF;
		minim_ar[i] = INF;
	}


	int count = 0;
	int gl_count = 0;
	int tcount = 1;
	bool improve = false;
	int ncount = 0;
	int ke, le, ue, ie, je, ce, final_pos;
	
	//Initialize_Neighborhood();

	//string mm;
	//int bcmax = INF;
	//for( map<string, neighbor>::iterator ii=Rel_Neighborhood.begin(); ii!=Rel_Neighborhood.end(); ++ii)
	//{
	//	cout << (*ii).first << " => " << (*ii).second << '\n';
	//	if( (*ii).second.cost1 < bcmax)
	//	{
	//		bcmax = (*ii).second.cost1;
	//		mm = (*ii).first;
	//	}
	//}
	//cout<<"Best: "<<mm<<" "<<bcmax<<endl;

	//ke=-1,le=-1,ie=-1;je=-1;ce=-1;
	//bool feas = MAP_FindNeighbors(ke,le,ie,je,0,false);
	//cout<<"Best: "<<feas<<" "<<"k "<<ke<<" l "<<l<e<" i "<<ie<<" j "<<je<<endl;

	//int ggg;
	//cin >> ggg;
	int q_metric; //Just need the reference
	float seldomness = 0.0f; //Seldomness metric
	bool mpike = false;
	int perturblim = 20; //Choose from 20, 50, 100
	int perturCount = 0;
	float perturb_str = 0.10f;
	int p_c = 0;
	int r_c = 0;

	int total_impr_moves = 0;

	/*
	int compound_range = 15;
	for (size_t i = 0; i < currentSol->soljobs.size(); i++) {
		job *jb = currentSol->soljobs[i];
		compound_range = min(compound_range, (int)jb->operations.size());
	}
	//Setup the CompoundMoveSelector
	for (size_t i = 2; i <= compound_range; i++) {
		CompoundRangeSelector->impr_moves[i] = 1;
		CompoundRangeSelector->moves[i] = 3;
		CompoundRangeSelector->move_frac[i] = 1.0f / 3;
		CompoundRangeSelector->n_move_frac[i] = 1.0f / (compound_range - 1);
	}
	*/
	int max_compound_range = 0;
	int compound_range;

	//Find max compound_range
	for (int i = 0; i < currentSol->soljobs.size(); i++) {
		job *jb = currentSol->soljobs[i];
		max_compound_range = max((int)jb->operations.size(), max_compound_range);
	}
	compound_range = max_compound_range;
	
	//Log File
	//ofstream myfile;
	//myfile.open("ts_log");
	int vns_state = 0;
	int mode;
	bool move = false;
	bool run = true;
	
	while (run)
	{
		if (vns_state == 0) vns_state++;

		/*

			Start by applying small neighborhoods
			if no better solution is found then move on smaller and smaller neighborhoods

			In our case this is:
			
			1. Compound Moves [Ranges : High->Low]
			2. Exchange
			3. Relocate

			If a new solution is found then we reset the neighborhoods
			if no solution is found then we proceed with the next neighborhoods

		*/

		//printf("Before Move Solution and Graph\n");
		//currentSol->forwardGraph->report();
		//cout << *currentSol;

		//scheduleAEAP(false,0);
		currentSol->getCost(false);
		improve = false;
		count++;
		gl_count++;
		copy(currentSol->cost_array, currentSol->cost_array + COST_NUM, cost_ar);

		ke = -1, le = -1, ie = -1; je = -1; ce = -1; ue = -1;
		bool tabu_move = false;
		move = false;

		//Helping Stuff
		int FWD_BWD = rand() % 2;
			
		switch (vns_state) {
			//Neighborhood 1 [COMPOUND MOVES with gradual decrease of the compound range]
			case 0: {
				mode = COMPOUND;
				move = critical_FindNeighborsJobs(ce, ie, ue, FWD_BWD, compound_range, 1, true);
			}; break;
				
			//Neighborhood 2 [Exchange Move]
			case 1: {
				mode = SMART_EXCHANGE;
				move = ls_smart_exchange(ke, le, ie, je, mode, options->ts_strategy, true);
			}; break;
				
			//Neighborhood 3 [Relocation Move]
			case 2: {
				mode = SMART_RELOCATE;
				move = ls_smart_relocate(ke, le, ie, je, mode, options->ts_strategy, true);
			}; break;
		}

			
		if (move) {
				
			//Apply the move
			//printf("Mode %d Move Found \n", mode);
			switch (mode)
			{
				case SMART_RELOCATE:
					relocate(ke, le, ie, je, true, mcost_ar); break;
				case SMART_EXCHANGE:
					exchange(ke, le, ie, je, true, mcost_ar); break;
				case COMPOUND:
				{
					switch (FWD_BWD)
					{
					case 0: JobShiftbyOneFWD(mcost_ar, ce, ie, ue, true, tabu_move, q_metric); break;
					case 1: JobShiftbyOneBWD(mcost_ar, ce, ie, ue, true, tabu_move, q_metric); break;
					};
				}; break;
			};
			//Store Stats
			moveStats[mode]++;

			tcount++;

			//print_soltrue();

			if (!currentSol->find_conflicts(false))
			{
				int ggg = 0;
				cout << "OOPPPS" << endl;
				cin >> ggg;
			}

			//printf("After Move Solution and Graph\n");
			//currentSol->forwardGraph->report();
			//cout << *currentSol;
			//assert(false);
			//cout << "Mode: " << mode << " M attr: " <<ke<< " " <<le<< " " <<ie<< " " <<je<< " " <<ce<< " " <<ue<<endl;
			for (int i = 0; i < COST_NUM; i++) {
				if (mcost_ar[i] != currentSol->cost_array[i] - cost_ar[i])
				{
					//print_soltrue();
					for (int j = 0; j<COST_NUM; j++)
						cout << mcost_ar[j] << " != " << currentSol->cost_array[j] - cost_ar[j] << endl;
					cout << "Serious Warning TS #1" << endl;
					return false;
				}
			}

			if (currentSol->compareSolution(localElite))
			{
				//tcount = 0;
				improve = true;
				//printf("TS Move Applied Iter %d New Local Bests %d %d %d\n", count,
				//	localElite->cost_array[MAKESPAN], localElite->cost_array[TOTAL_FLOW_TIME], localElite->cost_array[TOTAL_IDLE_TIME]);
				count = 0;
				perturb_str = 0.1f;
				vns_state = 0; //Reset to the first vns neighborhood
				//The solution has been tested a million times so far. This test is redundant
				//if (!currentSol->testsolution()) return false;
				*localElite = *currentSol;

				//On improvement promote the mode and equally neglect the rest moves
				total_impr_moves++;
			}
			else {
				//printf("TS Move Applied Iter %d Objective %d Improvement %d \n", count, currentSol->cost_array[MAKESPAN], improve);
				if ((mode == COMPOUND) && (compound_range > 2)) compound_range--;
				else {
					compound_range = max_compound_range;
					vns_state = (vns_state + 1) % 3;
				}
			}


		} else {
			if ((mode == COMPOUND) && (compound_range > 2)) compound_range--;
			else {
				compound_range = max_compound_range;
				vns_state = (vns_state + 1) % 3;
			}
		}


		//myfile << "Iteration " << count << " " << currentSol->cost_array[MAKESPAN] << " " << currentSol->cost_array[TOTAL_FLOW_TIME] << \
				//	" " << currentSol->cost_array[TOTAL_IDLE_TIME] << " " << seldomness << endl;

		//cout << "Temp Solution" << endl;
		//cout << *currentSol;

		if (vns_state == SMART_RELOCATE && !improve) {
			*currentSol = *localElite;
			
			moveStats[PERTURBATION]++;
			//Perturb(currentSol, 0.10f, 1); //Testing-Previous
			distant_sol_finder(perturb_str, 50, 4, 14);
			//Log perturbation
			//myfile << "Perturbation " << currentSol->cost_array[MAKESPAN] << " " << currentSol->cost_array[TOTAL_FLOW_TIME] << \
							" " << currentSol->cost_array[TOTAL_IDLE_TIME] << endl;

			//Check limit
			perturCount = 0;
			//Update perturb strength

			//perturb_str += 0.02f;
			perturb_str += 0.01f;

			//Cleanup Tabulist
            (this->*clearTabu)();
			//printf("Perturbation Applied Iter %d Costs %d %d %d\n", count,
			//	currentSol->cost_array[MAKESPAN], currentSol->cost_array[TOTAL_FLOW_TIME], currentSol->cost_array[TOTAL_IDLE_TIME]);
		}

		if (count > options->ts_iters) run = false;

		time(&ts_time);
		if (difftime(time(&ts_time), now) > options->ts_time_limit) {
			cout << "Time Limit Reached for LS" << endl;
			break;
		}
	}
	
	//Cleanup
	//myfile.close();
	return true;
}

bool localSearch::vns_ts()
{
	printf("starting VNS+TS\n");
	time_t ts_time = time(NULL);

	//printf("Input Graph\n");
	//currentSol->forwardGraph->report();

	//int ggg;
	//cin >> ggg;
	int q_metric; //Just need the reference
	float seldomness = 0.0f; //Seldomness metric
	bool mpike = false;
	int perturblim = 20; //Choose from 20, 50, 100
	int perturCount = 0;
	float perturb_str = 0.10f;
	int p_c = 0;
	int r_c = 0;

	int count = 0;
	int gl_count = 0;
	int total_impr_moves = 0;

	//Log File
	//ofstream myfile;
	//myfile.open("ts_log");
	bool improve = true;
	int vns_state = 0;
	int mode;
	bool move = false;
	bool run = true;

	int max_compound_range = 0;
	int compound_range;

    //Find max compound_range
	for (int i = 0; i < currentSol->soljobs.size(); i++) {
		job *jb = currentSol->soljobs[i];
		max_compound_range = max((int)jb->operations.size(), max_compound_range);
	}
	compound_range = max_compound_range;

	solution *h_sol = new solution();
	*h_sol = *this->currentSol;

	while (run)
	{
		if (vns_state == 0) vns_state++;

		/*

		Start by applying ts schemes using large neighborhoods
		if no better solution is found then move on smaller and smaller neighborhoods

		In our case this is:

		1. Compound Moves [Ranges : High->Low]
		2. Exchange
		3. Relocate

		If a new solution is found then we reset the neighborhoods
		if no solution is found then we proceed with the next neighborhoods

		*/

		//Setup lsObject with the working solution
		setup(h_sol);
		count++;
		gl_count++;
        switch (vns_state) {
			//Neighborhood 1 [CRIT_COMPOUND MOVES with gradual decrease of the compound range]
		case 0: {
			mode = CRIT_COMPOUND;
            tabu_search(vector<int>{CRIT_COMPOUND}, compound_range);
		}; break;

			//Neighborhood 2 [Critical Exchange Move]
		case 1: {
			mode = CRIT_SMART_EXCHANGE;
			tabu_search(vector<int>{CRIT_SMART_EXCHANGE}, compound_range);
		}; break;

			//Neighborhood 3 [Critical Relocation Move]
		case 2: {
			mode = CRIT_SMART_RELOCATE;
			tabu_search(vector<int>{CRIT_SMART_RELOCATE}, compound_range);
		}; break;

			//Neighborhood 4 [COMPOUND MOVES with gradual decrease of the compound range]
		case 3: {
			mode = COMPOUND;
			tabu_search(vector<int>{COMPOUND}, compound_range);
		}; break;

			//Neighborhood 5 [Exchange Move]
		case 4: {
			mode = SMART_EXCHANGE;
			tabu_search(vector<int>{SMART_EXCHANGE}, compound_range);
		}; break;

			//Neighborhood 6 [Relocation Move]
		case 5: {
			mode = SMART_RELOCATE;
			tabu_search(vector<int>{SMART_RELOCATE}, compound_range);
		}; break;
		}

		

		//Compare Solution derived from the called TS functions
		if (solution::Objective_TS_Comparison(localElite, h_sol, false)) {
			//tcount = 0;
			improve = true;
			printf("VNS+TS Iter %d - Mode %d -  New Local Bests %5.1f %5.1f %5.1f\n", gl_count, mode,
				localElite->cost_array[MAKESPAN], localElite->cost_array[TOTAL_FLOW_TIME], localElite->cost_array[TOTAL_IDLE_TIME]);
			count = 0;
			perturb_str = 0.1f;
			vns_state = 0; //Reset to the first vns neighborhood
						   //The solution has been tested a million times so far. This test is redundant
						   //if (!currentSol->testsolution()) return false;
			*h_sol = *localElite;

			//On improvement promote the mode and equally neglect the rest moves
			total_impr_moves++;
		}
		else {
			if ((mode == COMPOUND || mode == CRIT_COMPOUND) && (compound_range > 2)) compound_range--;
			else {
				compound_range = max_compound_range;
				vns_state = (vns_state + 1) % 6;
			}
		}


		//myfile << "Iteration " << count << " " << currentSol->cost_array[MAKESPAN] << " " << currentSol->cost_array[TOTAL_FLOW_TIME] << \
						//	" " << currentSol->cost_array[TOTAL_IDLE_TIME] << " " << seldomness << endl;

		//cout << "Temp Solution" << endl;
		//cout << *currentSol;

		if (vns_state == CRIT_SMART_RELOCATE && !improve) {
			*currentSol = *h_sol;
			moveStats[PERTURBATION]++;
			//Perturb(currentSol, 0.10f, 1); //Testing-Previous
			distant_sol_finder(perturb_str, 50, 4, 14);
			*h_sol = *currentSol; //Bring the results back

			//Log perturbation
			//myfile << "Perturbation " << currentSol->cost_array[MAKESPAN] << " " << currentSol->cost_array[TOTAL_FLOW_TIME] << \
										" " << currentSol->cost_array[TOTAL_IDLE_TIME] << endl;

			//Check limit
			perturCount = 0;
			//Update perturb strength

			//perturb_str += 0.02f;
			perturb_str += 0.01f;

			//Cleanup Tabulist
            (this->*clearTabu)();
			//printf("Perturbation Applied Iter %d Costs %d %d %d\n", count,
			//	currentSol->cost_array[MAKESPAN], currentSol->cost_array[TOTAL_FLOW_TIME], currentSol->cost_array[TOTAL_IDLE_TIME]);
		}

		if (count > 50) run = false;

		time(&ts_time);
		if (difftime(time(&ts_time), now) > options->ts_time_limit) {
			cout << "Time Limit Reached for LS" << endl;
			break;
		}
	}

	//Cleanup
	delete h_sol;
	//myfile.close();
	return true;
}


bool localSearch::tabu_search(vector<int> modes, int compound_range)
{
	//printf("starting TS\n");
	time_t ts_time = time(NULL);
    now = time(NULL); //Update now

	currentSol->replace_temp_values();
	if (!currentSol->testsolution()) return false;

	currentSol->find_conflicts(false);
	currentSol->getCost(false);

	//findCritical();
	/*tabu_cost  = tabu_solution->cost;
	tabu_scost = tabu_solution->s_cost;
	tabu_tcost = tabu_solution->t_cost;
	tabu_ecost = tabu_solution->e_cost;*/

	//printf("Input Graph\n");
	//currentSol->forwardGraph->report();

	float cost_ar[COST_NUM];
	float mcost_ar[COST_NUM];
	float minim_ar[COST_NUM];
	for (int i = 0; i < COST_NUM; i++) {
		cost_ar[i] = INF;
		mcost_ar[i] = INF;
		minim_ar[i] = INF;
	}

	count = 0;
	int tcount = 1;
	bool improve = false;
	int ncount = 0;
	int ke, le, ue, ie, je, ce, final_pos;
	bool move;

	//Initialize_Neighborhood();

	//string mm;
	//int bcmax = INF;
	//for( map<string, neighbor>::iterator ii=Rel_Neighborhood.begin(); ii!=Rel_Neighborhood.end(); ++ii)
	//{
	//	cout << (*ii).first << " => " << (*ii).second << '\n';
	//	if( (*ii).second.cost1 < bcmax)
	//	{
	//		bcmax = (*ii).second.cost1;
	//		mm = (*ii).first;
	//	}
	//}
	//cout<<"Best: "<<mm<<" "<<bcmax<<endl;

	//ke=-1,le=-1,ie=-1;je=-1;ce=-1;
	//bool feas = MAP_FindNeighbors(ke,le,ie,je,0,false);
	//cout<<"Best: "<<feas<<" "<<"k "<<ke<<" l "<<l<e<" i "<<ie<<" j "<<je<<endl;

	//int ggg;
	//cin >> ggg;
	int q_metric; //Just need the reference
	float seldomness = 0.0f; //Seldomness metric
	bool mpike = false;
	int perturblim = options->ts_perturb_limit; //Choose from 20, 50, 100
	//int perturblim = options->ts_iters/5; //Choose from 20, 50, 100
	int perturCount = 0;
    int exitCount = 0;
    bool localImprov = false;
    float perturb_str = 0.010f;
	int p_c = 0;
	int r_c = 0;

	int total_impr_moves = 0;

    //Backups of the tenure for dynamic adjustment
    int max_tenure = tabu_tenure;
    int min_tenure = 1;
	
	//Setup the TabuMoveSelector
	for (int i = 0; i < modes.size(); i++) {
        int m = modes[i];
        TabuMoveSelector->impr_moves[m] = 1;
		TabuMoveSelector->moves[m] = 3;
		TabuMoveSelector->move_frac[m] = 1.0f / 3;
		TabuMoveSelector->n_move_frac[m] = 1.0f / 2;
	}
	
	//int compound_range = 15;
	for (size_t i = 0; i < currentSol->soljobs.size(); i++) {
		job *jb = currentSol->soljobs[i];
		compound_range = min(compound_range, (int) jb->operations.size());
	}
	compound_range = max(compound_range, 2);

	//Setup the CompoundMoveSelector
	for (size_t i = 2; i <= compound_range; i++) {
		CompoundRangeSelector->impr_moves[i] = 1;
		CompoundRangeSelector->moves[i] = 3;
		CompoundRangeSelector->move_frac[i] = 1.0f/3;
		CompoundRangeSelector->n_move_frac[i] = 1.0f / (compound_range - 1);
	}

	//Log File
	//ofstream myfile, myfile_moves;
	//myfile.open("ts_log");
    //myfile_moves.open("ts_log_moves");

    bool run = true;
	while (run)
	{

        //printf("Before Move Solution and Graph\n");
		//currentSol->forwardGraph->report();
		//cout << *currentSol;
		
		//scheduleAEAP(false,0);
		currentSol->getCost(false);
		//Reset Minim Array
        for (int i = 0; i < COST_NUM; i++)
            minim_ar[i] = INF;

        //tcount = 1;
		improve = false;
        localImprov = false;
		copy(currentSol->cost_array, currentSol->cost_array + COST_NUM, cost_ar);

		ke = -1, le = -1, ie = -1; je = -1; ce = -1; ue = -1;
		final_pos = -1;
		move = false;
		bool tabu_move = false;
		int tabu_move_counter = 0;
		//int mode = 2 + rand() % 3; //Init randomly and adaption will follow
		int original_mode;
        int effective_mode;

        //Move Selection
		original_mode = TabuMoveSelector->selectMove();
        effective_mode = original_mode;
		
		//Explicitly select between the two compound modes randomly
		int FWD_BWD = rand() % 2;
		//Compound Moves are disabled
		compound_range = CompoundRangeSelector->selectMove();
        //FWD_BWD = 1;

        switch (original_mode) {
	   		case RELOCATE:
                move = ls_relocate(ke, le, ie, je, effective_mode, options->ts_strategy, NULL, minim_ar);
                break;
	   		case EXCHANGE:
	   			move = FindNeighbors(ke, le, ie, je, effective_mode, options->ts_strategy);
	   			break;
   			case CRIT_COMPOUND:
   				//move = FindNeighborsJobs(ce, ie, ue, FWD_BWD, compound_range, 1, NULL);
				move = critical_FindNeighborsJobs(ce, ie, ue, FWD_BWD, compound_range, 1, NULL);
   				break;
			case COMPOUND:
				move = FindNeighborsJobs(ce, ie, ue, FWD_BWD, compound_range, 1, NULL);
				break;
			case SMART_RELOCATE:
				move = ls_smart_relocate(ke, le, ie, je, effective_mode, options->ts_strategy, NULL);
				break;
			case SMART_EXCHANGE:
				move = ls_smart_exchange(ke, le, ie, je, effective_mode, options->ts_strategy, NULL);
				break;

            //My Critical Moves
            case CRIT_SMART_RELOCATE:
                move = critical_ls_smart_relocate(ke, le, ie, je, options->ts_strategy, NULL, minim_ar);
                break;
            case CRIT_SMART_EXCHANGE:
				move = critical_ls_smart_exchange(ke, le, ie, je, effective_mode, options->ts_strategy, NULL, minim_ar);
				break;

            //CRITICAL BLOCK MOVES AND THEIR COMBINATIONS
            case CRIT_BLOCK_UNION:
                move = critical_block_union(ke, le, ie, je, effective_mode, options->ts_strategy, NULL);
                break;
			case CRIT_BLOCK_SEQUENCE:
                move = critical_block_sequencing(ke, le, ie, je, options->ts_strategy, NULL, minim_ar);
                break;
            case CRIT_BLOCK_EXCHANGE:
                move = critical_block_exchange(ke, le, ie, je, options->ts_strategy, NULL, minim_ar);
                break;
	   	}


	   	//cout << "Move Result: " << move << endl;
		//cout << "Pre Move Solution "<<endl;
		//cout << *currentSol << endl
		//print_sol();

	   	//Store Stats
	   	moveStats[effective_mode]++;

        if (move) {

            switch (effective_mode) {
                case SMART_RELOCATE:
                case CRIT_SMART_RELOCATE:
                case CRIT_BLOCK_SEQUENCE:
                case CRIT_BLOCK_EXCHANGE:
                case RELOCATE:
                    relocate(ke, le, ie, je, true, mcost_ar);
                    break;

                case SMART_EXCHANGE:
                case CRIT_SMART_EXCHANGE:
                case EXCHANGE:
                    exchange(ke, le, ie, je, true, mcost_ar);
                    break;

                case CRIT_COMPOUND:
                case COMPOUND: {
                    switch (FWD_BWD) {
                        case 0:
                            JobShiftbyOneFWD(mcost_ar, ce, ie, ue, true, tabu_move, q_metric);
                            break;
                        case 1:
                            JobShiftbyOneBWD(mcost_ar, ce, ie, ue, true, tabu_move, q_metric);
                            break;
                    };
                };
                    break;
            };


            // if (mcost1 < 0)
            // 	printf("LS Iter %d Improving Move Cost %d Old Obj %d New Obj %d \n", count, mcost1, cost1, currentSol->cost_array[MAKESPAN]);
            //cout << "Applying Better Move with move costs: " << mcost1 << " " << mcost2 << " " << mcost3 << " " << mcost4 << endl;
            //Costs are being calculated by the end of the move application
            //!!!I think it is safe to remove it from here!!!

            //currentSol->FGraph->topological_sort();
            //currentSol->FGraph->report_topological_sort();

            //print_soltrue();

            if (!currentSol->find_conflicts(false)) {
                int ggg = 0;
                cout << "OOPPPS" << endl;
                cin >> ggg;
            }

            //printf("After Move Solution and Graph\n");
            //currentSol->forwardGraph->report();
            //cout << *currentSol;
            //assert(false);
            //cout << "Mode: " << mode << " M attr: " <<ke<< " " <<le<< " " <<ie<< " " <<je<< " " <<ce<< " " <<ue<<endl;
            for (int i = 0; i < COST_NUM; i++) {
                if (mcost_ar[i] != currentSol->cost_array[i] - cost_ar[i]) {
                    //print_soltrue();
                    for (int j = 0; j < COST_NUM; j++)
                        cout << mcost_ar[j] << " != " << currentSol->cost_array[j] - cost_ar[j] << endl;
                    cout << "Serious Warning TS #1" << endl;
                    return false;
                }
            }


            // if (mcost1==0 && mcost2==0 &&mcost3==0)
            // 	printf("Tie Accepted\n");

            //cout << "Move: " << mode << " " << mcost1 << " " << mcost2 << " " << mcost3 << " " << mcost4 << " Cur_Cost: " << currentSol->cost << " " << currentSol->s_cost << " " << currentSol->t_cost << " " << currentSol->e_cost << endl;
            //cout << "Tabu: " << tabu_cost << " " << tabu_scost << " " << tabu_tcost << " " << tabu_ecost << endl;
            if (currentSol->compareSolution(localElite)) {
                //tcount = 0;
                improve = true;
                exitCount = 0;
                perturCount = 0;
                perturb_str = 0.01f;
                *localElite = *currentSol;
                total_impr_moves++;
                printf("TS Move Applied Iter %5d New Local Best %4.1f %4.1f %4.1f\n", count,
                       localElite->cost_array[MAKESPAN], localElite->cost_array[TOTAL_FLOW_TIME], localElite->cost_array[TOTAL_IDLE_TIME]);

            } else if (solution::Objective_TS_Comparison(currentSol->cost_array, cost_ar)) {
                //printf("TS Move Applied Iter %5d Local Improvement %4.1f %4.1f %4.1f\n", count,
                //       currentSol->cost_array[MAKESPAN], currentSol->cost_array[TOTAL_FLOW_TIME], currentSol->cost_array[TOTAL_IDLE_TIME]);
                //Local Improvement
                localImprov = true;
                //exitCount++;
            } else {
                exitCount++;
				perturCount++;
				//printf("TS Move Applied Iter %d Objective %4.1f Improvement %d \n", count, currentSol->cost_array[MAKESPAN], improve);
			}

            //printf("TS Move Applied Iter %d Objective %4.1f Improvement %d \n", count, currentSol->cost_array[MAKESPAN], improve);
            tabu_progress.push_back(currentSol->cost_array[MAKESPAN]);

			TabuMoveSelector->update(original_mode, improve);

            /*
            if ((count>40) && (count < 50)) {
                //Recalculate and report critical path
                printf("Iteration %d CPath & Bloks report\n", count);
                cout <<*currentSol;
                if (count == 44)
                    printf("break\n");
                vector<int3_tuple> cBlocks = currentSol->findCriticalBlocks(1);

                if (count == 44){
                    printf("break break\n");
                    printf("Critical Path: ");
                    for (int i=0;i<currentSol->FGraph->criticalPath.size();i++)
                        printf("%d ",currentSol->FGraph->criticalPath[i]->UID);
                    printf("\n");
                }

                for (int i=0; i< (int) cBlocks.size(); i++){
                    printf("Block %d Machine %d Range: %d-%d\n",
                           i, get<0>(cBlocks[i]), get<1>(cBlocks[i]), get<2>(cBlocks[i]));

                    printf("Operations: ");
                    machine *m = currentSol->solmachines[get<0>(cBlocks[i])];
                    for (int j=get<1>(cBlocks[i]);j<get<2>(cBlocks[i]);j++)
                        printf("%d ",m->operations[j]->UID);
                    printf("\n");
                }

            }
             */

            //Update the range probabilities for the compound move
			//if (mode == COMPOUND) 

			//cout << "After Move Solution "<<endl;
			//cout << *currentSol << endl;
			
		} else {
			//printf("Mode %d No Move Found \n", effective_mode);
            failedMoves[effective_mode]++;
            exitCount++;
			perturCount++;
		}

		//myfile << "Iteration " << count << " " << currentSol->cost_array[MAKESPAN] << " " << currentSol->cost_array[TOTAL_FLOW_TIME] << \
			" " << currentSol->cost_array[TOTAL_IDLE_TIME] << " " << seldomness << " Move Mode: " << effective_mode <<  endl;

        //myfile_moves << "Move " << ke <<  " " << le << " " << ie << " " << je << " Original Mode: " << original_mode << \
            " Effective Mode: " << effective_mode <<  " " << currentSol->cost_array[MAKESPAN] << endl;
		
		//cout << "Temp Solution" << endl;
		//cout << *currentSol;
		

		//Perturbation Application
		if (false) {
		//if ((move && (perturCount > perturblim) && (!improve)) || !move ) {
		//if ( (perturCount > perturblim) && (!localImprov)) {
		//if (count > perturblim)
        //if (false) {
			//count = 0;
            printf("Perturbing\n");
            //printf("Perturbing with strength %f \n", perturb_str);
			*currentSol = *localElite;
			//Perturb(currentSol, perturb_str, 3); //Much more stable
			//Perturb(currentSol, 0.05f, 2); //This nasty thing shuffles the solution partially

            //cout << *currentSol;
            Perturb(currentSol, 0.10f); //Old Setting
            //cout << *currentSol;
			
			
			moveStats[PERTURBATION]++;
			//distant_sol_finder(0.1f, 100, 5, 14);

			//cout << *currentSol;
			//Log perturbation
			//myfile << "Perturbation " << count << " " << currentSol->cost_array[MAKESPAN] << " " << currentSol->cost_array[TOTAL_FLOW_TIME] << \
				" " << currentSol->cost_array[TOTAL_IDLE_TIME] << " Elite Makespan " << localElite->cost_array[MAKESPAN] << endl;

			//Check limit
			perturCount = 0;
			//Update perturb strength
			
			//perturb_str += 0.02f;
			perturb_str += 0.01f;
			
			//if (perturb_str > 0.2f) run = false;
			
			//perturb_str = clamp(perturb_str, 0.1f, 0.2f); //Used in other perturbation methods
			perturb_str = clamp(perturb_str, 0.01f, 0.2f); 

			//Cleanup Tabulist
			(this->*clearTabu)();
			printf("Done Pertubring\n");
			//printf("Perturbation Applied Iter %5d Costs %4.0f %4.0f %4.0f\n", count, \
				currentSol->cost_array[MAKESPAN], currentSol->cost_array[TOTAL_FLOW_TIME], currentSol->cost_array[TOTAL_IDLE_TIME]);
		}

#ifdef TABU_TENURE
        //Dynamic Tabu Tenure Update
        if (improve)
            tabu_tenure = 1;
        else if (localImprov && tabu_tenure > 30)
            tabu_tenure = max(min_tenure, tabu_tenure - 1);
        else if (tabu_tenure <= 30)
            tabu_tenure = min(max_tenure, tabu_tenure + 1);
#endif

        if (exitCount > options->ts_iters) run = false;

		time(&ts_time);
		if (difftime(time(&ts_time), now) > options->ts_time_limit) {
			cout << "Time Limit Reached for LS" << endl;
			break;
		}
        count++;

		//break; //TEST
	}
	
	/*
	printf("IMPROVING MOVES REPORT\n");
	printf("MOVE FRACTIONS : REL %f EXC %f COM %f\n", 
		TabuMoveSelector->n_move_frac[3], TabuMoveSelector->n_move_frac[4], TabuMoveSelector->n_move_frac[2]);
	printf("RANGE FRACTIONS :");
	for (int i = 0; i < 6; i++) 
		printf("%d %f ", i, CompoundRangeSelector->n_move_frac[i]);
	printf("\n");
	*/

	//Cleanup
	//myfile.close();
    //myfile_moves.close();

	return true;
}

bool localSearch::old_tabu_search()
{
	time_t ts_time = time(NULL);

	currentSol->replace_temp_values();
	if (!currentSol->testsolution()) return false;

	currentSol->find_conflicts(false);
	currentSol->getCost(false);
	/*tabu_cost  = tabu_solution->cost;
	tabu_scost = tabu_solution->s_cost;
	tabu_tcost = tabu_solution->t_cost;
	tabu_ecost = tabu_solution->e_cost;*/

	//printf("Input Graph\n");
	//currentSol->forwardGraph->report();

	float cost_ar[COST_NUM];
	float mcost_ar[COST_NUM];
	float minim_ar[COST_NUM];
	for (int i = 0; i < COST_NUM; i++) {
		cost_ar[i] = INF;
		mcost_ar[i] = INF;
		minim_ar[i] = INF;
	}


	int count = 0;
	int tcount = 1;
	bool improve = false;
	int ncount = 0;
	int ke, le, ue, ie, je, ce, final_pos;
	bool move;

	//Initialize_Neighborhood();

	//string mm;
	//int bcmax = INF;
	//for( map<string, neighbor>::iterator ii=Rel_Neighborhood.begin(); ii!=Rel_Neighborhood.end(); ++ii)
	//{
	//	cout << (*ii).first << " => " << (*ii).second << '\n';
	//	if( (*ii).second.cost1 < bcmax)
	//	{
	//		bcmax = (*ii).second.cost1;
	//		mm = (*ii).first;
	//	}
	//}
	//cout<<"Best: "<<mm<<" "<<bcmax<<endl;

	//ke=-1,le=-1,ie=-1;je=-1;ce=-1;
	//bool feas = MAP_FindNeighbors(ke,le,ie,je,0,false);
	//cout<<"Best: "<<feas<<" "<<"k "<<ke<<" l "<<l<e<" i "<<ie<<" j "<<je<<endl;

	//int ggg;
	//cin >> ggg;
	int q_metric; //Just need the reference
	float seldomness = 0.0f; //Seldomness metric
	bool mpike = false;
	int perturblim = 100; //Choose from 20, 50, 100
	int perturCount = 0;
	float perturb_str = 0.010f;
	int p_c = 0;
	int r_c = 0;

	int total_impr_moves = 0;

	//Setup the TabuMoveSelector
	for (int i = COMPOUND; i <= SMART_RELOCATE; i++) {
		TabuMoveSelector->impr_moves[i] = 1;
		TabuMoveSelector->moves[i] = 3;
		TabuMoveSelector->move_frac[i] = 1.0f / 3;
		TabuMoveSelector->n_move_frac[i] = 1.0f / 2;
	}

	int compound_range = 15;
	for (size_t i = 0; i < currentSol->soljobs.size(); i++) {
		job *jb = currentSol->soljobs[i];
		compound_range = min(compound_range, (int)jb->operations.size());
	}

	//Setup the CompoundMoveSelector
	for (size_t i = 2; i <= compound_range; i++) {
		CompoundRangeSelector->impr_moves[i] = 1;
		CompoundRangeSelector->moves[i] = 3;
		CompoundRangeSelector->move_frac[i] = 1.0f / 3;
		CompoundRangeSelector->n_move_frac[i] = 1.0f / (compound_range - 1);
	}

	//Log File
	//ofstream myfile;
	//myfile.open("ts_log");

	bool run = true;
	while (run)
	{
		//printf("Before Move Solution and Graph\n");
		//currentSol->forwardGraph->report();
		//cout << *currentSol;

		//scheduleAEAP(false,0);
		currentSol->getCost(false);
		//tcount = 1;
		improve = false;
		copy(currentSol->cost_array, currentSol->cost_array + COST_NUM, cost_ar);

		ke = -1, le = -1, ie = -1; je = -1; ce = -1; ue = -1;
		final_pos = -1;
		move = false;
		bool tabu_move = false;
		int tabu_move_counter = 0;
		int mode = 2 + rand() % 3; //Init randomly and adaption will follow
								   //Move Selection
		mode = TabuMoveSelector->selectMove();

		//Explicitly select between the two compound modes randomly
		int FWD_BWD = rand() % 2;
		compound_range = CompoundRangeSelector->selectMove();

		//if(mode > 1) mode = 0;
		//mode = 1; //force exchange
		//mode = 2;
		//FWD_BWD = 1;
		switch (mode) {
		case RELOCATE:
		case EXCHANGE:
			move = FindNeighbors(ke, le, ie, je, mode, options->ts_strategy);
			break;
		case COMPOUND:
			move = FindNeighborsJobs(ce, ie, ue, FWD_BWD, compound_range, 1, NULL);
			break;
		case SMART_RELOCATE:
			move = ls_smart_relocate(ke, le, ie, je, mode, options->ts_strategy, NULL);
			break;
		case SMART_EXCHANGE:
			move = ls_smart_exchange(ke, le, ie, je, mode, options->ts_strategy, NULL);
			break;
		}

		//cout << "Move Result: " << move << endl;
		//cout << "Pre Move Solution "<<endl;
		//cout << *currentSol << endl
		//print_sol();

		//Store Stats
		moveStats[mode]++;

		if (move)
		{
			//printf("Mode %d Move Found \n", mode);
			switch (mode)
			{
			case SMART_RELOCATE:
			case RELOCATE: relocate(ke, le, ie, je, true, mcost_ar); break;

			case SMART_EXCHANGE:
			case EXCHANGE: exchange(ke, le, ie, je, true, mcost_ar); break;
			case COMPOUND:
			{
				switch (FWD_BWD)
				{
				case 0: JobShiftbyOneFWD(mcost_ar, ce, ie, ue, true, tabu_move, q_metric); break;
				case 1: JobShiftbyOneBWD(mcost_ar, ce, ie, ue, true, tabu_move, q_metric); break;
				};
			}; break;
			};


			// if (mcost1 < 0)
			// 	printf("LS Iter %d Improving Move Cost %d Old Obj %d New Obj %d \n", count, mcost1, cost1, currentSol->cost_array[MAKESPAN]);
			//cout << "Applying Better Move with move costs: " << mcost1 << " " << mcost2 << " " << mcost3 << " " << mcost4 << endl;
			//Costs are being calculated by the end of the move application
			//!!!I think it is safe to remove it from here!!!

			//currentSol->getCost(false);

			tcount++;

			//print_soltrue();

			if (!currentSol->find_conflicts(false))
			{
				int ggg = 0;
				cout << "OOPPPS" << endl;
				cin >> ggg;
			}

			//printf("After Move Solution and Graph\n");
			//currentSol->forwardGraph->report();
			//cout << *currentSol;
			//assert(false);
			//cout << "Mode: " << mode << " M attr: " <<ke<< " " <<le<< " " <<ie<< " " <<je<< " " <<ce<< " " <<ue<<endl;
			for (int i = 0; i < COST_NUM; i++) {
				if (mcost_ar[i] != currentSol->cost_array[i] - cost_ar[i])
				{
					//print_soltrue();
					for (int j = 0; j<COST_NUM; j++)
						cout << mcost_ar[j] << " != " << currentSol->cost_array[j] - cost_ar[j] << endl;
					cout << "Serious Warning TS #1" << endl;
					return false;
				}
			}


			// if (mcost1==0 && mcost2==0 &&mcost3==0)
			// 	printf("Tie Accepted\n");

			//cout << "Move: " << mode << " " << mcost1 << " " << mcost2 << " " << mcost3 << " " << mcost4 << " Cur_Cost: " << currentSol->cost << " " << currentSol->s_cost << " " << currentSol->t_cost << " " << currentSol->e_cost << endl;
			//cout << "Tabu: " << tabu_cost << " " << tabu_scost << " " << tabu_tcost << " " << tabu_ecost << endl;
			if (currentSol->compareSolution(localElite))
			{
				//tcount = 0;
				improve = true;
				//printf("TS Move Applied Iter %d New Local Bests %d %d %d\n", count,
				//	localElite->cost_array[MAKESPAN], localElite->cost_array[TOTAL_FLOW_TIME], localElite->cost_array[TOTAL_IDLE_TIME]);
				count = 0;
				perturCount = 0;
				perturb_str = 0.01f;
				//The solution has been tested a million times so far. This test is redundant
				//if (!currentSol->testsolution()) return false;
				*localElite = *currentSol;

				//On improvement promote the mode and equally neglect the rest moves
				total_impr_moves++;
			}
			else {
				count++;
				perturCount++;
				//printf("TS Move Applied Iter %d Objective %d Improvement %d \n", count, currentSol->cost_array[MAKESPAN], improve);
			}

			TabuMoveSelector->update(mode, improve);

			//Update the range proobabilities for the compound move
			//if (mode == COMPOUND) 


			//cout << "After Move Solution "<<endl;
			//cout << *currentSol << endl;

		}
		else {
			//printf("Mode %d No Move Found \n", mode);
			count++;
			perturCount++;
		}

		//myfile << "Iteration " << count << " " << currentSol->cost_array[MAKESPAN] << " " << currentSol->cost_array[TOTAL_FLOW_TIME] << \
					" " << currentSol->cost_array[TOTAL_IDLE_TIME] << " " << seldomness << " Move Mode: " << mode <<  endl;

//cout << "Temp Solution" << endl;
//cout << *currentSol;


//if (count > options->ts_iters) run = false;

		//Perturbation Application
		//if (false) {
		//if ((move && (perturCount > perturblim) && (!improve)) || !move ) {
		if ((perturCount > perturblim) && (!improve)) {
			
			//Perturb(currentSol, 0.10f, 1); //Testing-Previous
			moveStats[PERTURBATION]++;
			distant_sol_finder(perturb_str, 50, 4, 14);

			//Log perturbation
			//myfile << "Perturbation " << count << " " << currentSol->cost_array[MAKESPAN] << " " << currentSol->cost_array[TOTAL_FLOW_TIME] << \
							" " << currentSol->cost_array[TOTAL_IDLE_TIME];
			//myfile << " Elite Makespan " << localElite->cost_array[MAKESPAN] << endl;

			//Check limit
			perturCount = 0;
			//Update perturb strength

			//perturb_str += 0.02f;
			perturb_str += 0.01f;

			//if (perturb_str > 0.2f) run = false;

			//perturb_str = clamp(perturb_str, 0.1f, 0.2f); //Used in other perturbation methods
			perturb_str = clamp(perturb_str, 0.01f, 0.2f);

			if (count > options->ts_iters) run = false;

			//Cleanup Tabulist
			(this->*clearTabu)();
			//printf("Perturbation Applied Iter %d Costs %d %d %d\n", count,
			//	currentSol->cost_array[MAKESPAN], currentSol->cost_array[TOTAL_FLOW_TIME], currentSol->cost_array[TOTAL_IDLE_TIME]);
		}
		
		time(&ts_time);
		if (difftime(time(&ts_time), now) > options->ts_time_limit) {
			cout << "Time Limit Reached for LS" << endl;
			break;
		}
	}
	printf("IMPROVING MOVES REPORT\n");
	printf("MOVE FRACTIONS : REL %f EXC %f COM %f\n",
		TabuMoveSelector->n_move_frac[3], TabuMoveSelector->n_move_frac[4], TabuMoveSelector->n_move_frac[2]);

	//Cleanup
	//myfile.close();
	return true;
}

void localSearch::Perturbation()
{
	//cout << "PERTURBING...  " << endl;
	moveStats[PERTURBATION]++;
	RemoveAndReinsert(currentSol);
}


void localSearch::Perturb(solution* sol, float strength) {
	//Strength parameter defines the percentage of operations that will be removed
	//The solution will be reconstructed without those operations
	//Then the operations will be reinserted back using the construction algorithm
	moveStats[PERTURBATION]++;

	//int perturb_mode = 1; //0: Delete the operations of just one job, 1: Use strength to determine what will be removed

	//cout << "Perturb Init Solution" << endl;
	//cout << *sol;

#if PERTURBATION_METHOD < 4
	//Local variables
	int jbID;
	job* jb;
	vector<bl_pair> blacklist = vector<bl_pair>();
#endif

#if PERTURBATION_METHOD == 0
	{
		//Local variables
		int jbID;
		job* jb;
		vector<bl_pair> blacklist = vector<bl_pair>();

		//Select one job at random
		//jbID = rand() % sol->soljobs.size();

		//Find critical job
		int max_end = 0;
		for (size_t i = 0; i<sol->soljobs.size(); i++) {
			jb = sol->soljobs[i];
			operation *op = jb->operations[jb->operations.size() - 1];
			if (op->end > max_end) {
				jbID = i;
				max_end = op->end;
			}
		}

		jb = sol->soljobs[jbID];

		for (size_t i = 0; i < jb->operations.size(); i++) {
			operation* op = jb->operations[i];
			op->start = op->a;
#if(FLEX)
			op->end = op->start + op->processing_time_UM[op->mc->ID] + op->cleanup_time + op->setup_time;
#else
			op->end = op->start + op->processing_time + op->cleanup_time + op->setup_time;
#endif
			op->temp_start = op->start;
			get<1>(op->end_times) = op->end;

			sol->removeOp(op->mc, op->mthesi); //Remove Operation from solution
			op->setDoneStatus(false, false);
			sol->opshelp.push_back(op); //Used for randomized Construction
			bl_pair p = make_pair(op->jobID, op->ID);
			blacklist.push_back(p);
		}
	}
#elif PERTURBATION_METHOD == 1
	{
		//Decide how many operations to remove
		int rNumOp = (int)(strength * sol->totalNumOp);
		rNumOp = clamp(rNumOp, 1, sol->totalNumOp);
		//printf("Removing %d/%d Operations from solution \n", rNumOp,sol->totalNumOp);
		sol->opshelp.clear();

		int t = 0;
		while (t<rNumOp)
		{
			//Select one job at random
			jbID = rand() % sol->soljobs.size();
			jb = sol->soljobs[jbID];

			int i = jb->operations.size() - 1;
			while ((t < rNumOp) && (i >= 0)) {
				operation* op = jb->operations[i];
				if (!op->isDone()) { i--; continue; }
				op->start = op->a;
#if(FLEX)
				op->end = op->start + op->processing_time_UM[op->mc->ID] + op->cleanup_time + op->setup_time;
#else
				op->end = op->start + op->processing_time + op->cleanup_time + op->setup_time;
#endif
				op->temp_start = op->start;
				op->temp_end = op->end;

				sol->removeOp(op->mc, op->mthesi); //Remove Operation from solution
				op->setDoneStatus(false, false);
				sol->opshelp.push_back(op); //Used for randomized Construction
				bl_pair p = make_pair(op->jobID, op->ID);
				blacklist.insert(blacklist.begin(), p);
				t++;
				i--;
				//break; //This break removes operations in breadth first mode
			}
		}
		//Schedule Stuff
		//sol->randomCandidateConstruction();
		//sol->greedyConstruction(false);

        /*
         * NOT NEEDED
		if (!sol->scheduleAEAP(false, 0, true)) { //Recalculate Schedules
			cout << "MALAKIA SE SCHEDULEAEAP STO NEO PERTURBATION" << endl;
			assert(false);
		};
         */

		sol->aeapConstruction(1, blacklist, MAKESPAN); //Looks like this is working out of the box
		
	}
#elif PERTURBATION_METHOD == 2
	{
		//Local variables
		int jbID;
		job* jb;
		vector<bl_pair> blacklist = vector<bl_pair>();

		//Decide how many operations to remove
        int rNumOp = max(1, (int) (strength * sol->totalNumOp));
		//printf("Removing %d/%d Operations from solution \n", rNumOp,sol->totalNumOp);
		sol->opshelp.clear();

		int t = 0;
		while (t < rNumOp)
		{
			//Select one job at random
			jbID = rand() % sol->soljobs.size();
			jb = sol->soljobs[jbID];

            int i = rand() % jb->operations.size();

            operation* op = jb->operations[i];
            if (!op->isDone())
                continue;

            if (op->fixed)
                continue;
            //printf("Removing Operation %d %d \n", op->jobID, op->ID);
            op->start = op->a;
#if(FLEX)
            op->end = op->start + op->getProcTime() + op->cleanup_time + op->setup_time;
#else
            op->end = op->start + op->processing_time + op->cleanup_time + op->setup_time;
#endif
            op->temp_start = op->start;
            op->temp_end = op->end;

            sol->removeOp(op->mc, op->mthesi); //Remove Operation from solution
            op->setDoneStatus(false, false);
            sol->opshelp.push_back(op); //Used for randomized Construction
            bl_pair p = make_pair(op->jobID, op->ID);
            blacklist.push_back(p);
            t++;
        }

        //Schedule Stuff
        //This does not work due to multiple operations removed and sAEAP cannot calculate schedules
        //sol->aeapConstruction(1, blacklist, MAKESPAN); //Looks like this is working out of the box
        sol->randomCandidateConstruction(blacklist);

		if (!sol->scheduleAEAP(false, true)) { //Recalculate Schedules
			cout << "MALAKIA SE SCHEDULEAEAP STO PERTURBATION 2" << endl;
			assert(false);
		};

        sol->getCost(false);

        //Calculate Critical Paths/Blocks/TopSort
        sol->findCriticalPath_viaSlack();
        sol->findCriticalBlocks(1);
        sol->FGraph->topological_sort();
    };

#elif PERTURBATION_METHOD == 3
	{
		//Local variables
		int jbID;
		job* jb;
		vector<bl_pair> blacklist = vector<bl_pair>();

		//This mode is using the move_freq property of the operations
		//It accumulates the operation move frequency per job and stores them in a list
		//Then the strength is used in order to determine the amount of jobs that will be removed
		vector<operation*> s_ops = vector<operation*>();

		for (size_t i = 0; i < sol->soljobs.size(); i++) {
			job *jb = sol->soljobs[i];
			for (size_t j = 0; j < jb->operations.size(); j++) {
				operation *cand_op = jb->operations[j];
				int pos = 0;
				for (size_t k = 0; k < s_ops.size(); k++) {
					if (cand_op->move_freq < s_ops[k]->move_freq) {
						pos = k;
						break;
					}
					else pos++;
				}
				vInsertAt<operation*>(s_ops, pos, cand_op);
			}
		}

		//Decide how many jobs will be removed
		size_t rNumOp = max(1, (int)(strength * s_ops.size()));
		//printf("Removing %d/%d Jobs from solution \n", rNumJob, sol->soljobs.size());
		sol->opshelp.clear();

		for (size_t i = 0; i < rNumOp; i++) {
			operation* op = s_ops[i];
			op->start = op->a;
#if(FLEX)
			op->end = op->start + op->processing_time_UM[op->mc->ID] + op->cleanup_time + op->setup_time;
#else
			op->end = op->start + op->processing_time + op->cleanup_time + op->setup_time;
#endif
			op->temp_start = op->start;
			get<1>(op->end_times) = op->end;

			sol->removeOp(op->mc, op->mthesi); //Remove Operation from solution
			op->setDoneStatus(false, false);
			bl_pair p = make_pair(op->jobID, op->ID);
			//printf("Blacklisting Operatioon %d %d\n", op->jobID, op->ID);
			blacklist.push_back(p);

		}


		if (!sol->scheduleAEAP(false, 0, true)) { //Recalculate Schedules
			cout << "MALAKIA SE SCHEDULEAEAP STO NEO PERTURBATION" << endl;
			assert(false);
		};

		sol->aeapConstruction(1, blacklist); //Looks like this is working out of the box

											 //Partially clear the tabulist
											 //for (size_t i = 0; i < rNumJob; i++)
											 //	clearTabulist(s_jobs[i]);

		vec_del<operation*>(s_ops);
	};
#elif PERTURBATION_METHOD == 4
    distant_sol_finder(strength);

#endif

	
#if PERTURBATION_METHOD < 4

    //Solution should be ok, storing the temp values
	sol->replace_temp_values();
	sol->Calculate_Resourse_Loads();
	sol->reportResources();

	//Finalize
	vec_del<bl_pair>(blacklist);
#endif
	

	//printf("Done Perturbing\n");
	//cout << "Solution after reconstruction" << endl;
	//cout << *sol;
	//Insert removed operations back to graph
	//printf("REMOVING JOB %d Operations\n", jb->ID);
}

float localSearch::distant_sol_finder(float div, int iters, int distance_metric, int new_optimtype)
{

	//Create temporary solution 
	solution *temp_sol = new solution();
    solution *temp_best_sol= new solution();
	//printf("Perturbing\n");
	//cout << *currentSol;


	//Reset Parameters
	//Keep only the distance metric for faster calculations
	int OLD_MAKESPAN_F = MAKESPAN_F;
	int OLD_TOTAL_FLOW_TIME_F = TOTAL_FLOW_TIME_F;
	int OLD_TOTAL_IDLE_TIME_F = TOTAL_IDLE_TIME_F;
	int OLD_TOTAL_ENERGY_F = TOTAL_ENERGY_F;
	int OLD_TARDINESS_F = TARDINESS_F;
	int OLD_SLACK_F = SLACK_F;
	int OLD_DISTANCE__F = DISTANCE_F;
	
	//Set new values
	MAKESPAN_F = 1;
	TOTAL_FLOW_TIME_F = 0;
	TOTAL_IDLE_TIME_F = 0;
	TOTAL_ENERGY_F = 0;
	TARDINESS_F = 0;
	SLACK_F = 0;
	DISTANCE_F = distance_metric;
	optimtype = new_optimtype;

	//Swap active tabulists
	vector<tabu*> *old_list = tabumlist;
	tabumlist = p_tabumlist;
    //Swap function pointers
    addMove = &localSearch::addMoveinList;
    findMove = &localSearch::findMoveinList;
    clearTabu = &localSearch::clearTabuList;

	time_t ts_time = time(NULL);
	currentSol->replace_temp_values();
	currentSol->getCost(false);
	
	*temp_sol = *currentSol; //copy the input solution
    *temp_best_sol = *currentSol;

	float cost_ar[COST_NUM];
	float mcost_ar[COST_NUM];
	float minim_ar[COST_NUM];
	for (int i = 0; i < COST_NUM; i++) {
		cost_ar[i] = INF;
		mcost_ar[i] = INF;
		minim_ar[i] = INF;
	}

	int counter = 0;
	int non_improv = 0;
	int ke, le, ie, je, ce, ue, q_metric;
	bool tabu_move;
    float start_dist = currentSol->cost_array[DISTANCE];
    float target_dist = start_dist * (1.0f - div);
    float objective = start_dist;
    float best_makespan = INF;

	//printf("Start Distance %4.1f ", objective);
    //printf("Start Distance %d - Targeting... %f \n", currentSol->cost_array[DISTANCE], target_dist);
	
	//Log File
	//ofstream myfile;
	//myfile.open("ts_log");

	while(true)
	{
		//printf("Before Move Solution and Graph\n");
		//currentSol->forwardGraph->report();
		//cout << *currentSol;

		//scheduleAEAP(false,0);
		currentSol->getCost(false);
		copy(currentSol->cost_array, currentSol->cost_array + COST_NUM, cost_ar);

		int mode = SMART_RELOCATE + rand() % 1;
		
		int move;
		switch (mode) {
		case SMART_RELOCATE:
			move = ls_perturb_smart_relocate(ke, le, ie, je, mode, options->ts_strategy, NULL);
			break;
		case SMART_EXCHANGE:
			move = ls_perturb_smart_exchange(ke, le, ie, je, mode, options->ts_strategy, NULL);
			break;
		}

		//cout << "Move Result: " << move << endl;
		//cout << "Pre Move Solution "<<endl;
		//cout << *currentSol << endl
		//print_sol();

		//Store Stats
		//moveStats[mode]++;

		if (move)
		{
			switch (mode)
			{
			case SMART_RELOCATE:
			case RELOCATE: relocate_perturb(ke, le, ie, je, true, mcost_ar); break;

			case SMART_EXCHANGE:
			case EXCHANGE: exchange_perturb(ke, le, ie, je, true, mcost_ar); break;
			};

			if (!currentSol->find_conflicts(false))
			{
				int ggg = 0;
				cout << "OOPPPS" << endl;
				cin >> ggg;
			}
			
			////Testing
			////cout << "Mode: " << mode << " M attr: " <<ke<< " " <<le<< " " <<ie<< " " <<je<< " " <<ce<< " " <<ue<<endl;
			//for (int i = 0; i < COST_NUM; i++) {
			if (mcost_ar[DISTANCE] != currentSol->cost_array[DISTANCE] - cost_ar[DISTANCE])
			{
				//print_soltrue();
				for (int j = 0; j<COST_NUM; j++)
					cout << mcost_ar[j] << " != " << currentSol->cost_array[j] - cost_ar[j] << endl;
				cout << "Serious Warning PERTURB TS #1" << endl;
				return false;
			}
			//}

            //Check if a solution with better makespan is found and store it
            int old_optimtype = optimtype;
            optimtype = 1;
            if (currentSol->compareSolution(temp_best_sol)){
                *temp_best_sol = *currentSol;
                best_makespan = currentSol->cost_array[MAKESPAN];
            }
            optimtype = old_optimtype;

            //Save solution if better
			if (currentSol->compareSolution(temp_sol)) {
				//printf("Perturbation It %d - New Best Distance %d \n", c, currentSol->cost_array[DISTANCE]);
				//printf("Perturbation Tabu List size %d\n", p_tabumlist->size());
				*temp_sol = *currentSol;
				non_improv = 0;
                objective = currentSol->cost_array[DISTANCE];
				//cout << *temp_sol;
				//printf("----REPORTS\n");
				//cout << *currentSol;
				//globals->op_freq_map->report(currentSol);
				//printf("----END REPORT\n");
				
				//Terminate if we're within the target distance
				if (temp_sol->cost_array[DISTANCE] <= target_dist) break;
			}
			else
				non_improv++;


			
			//cout << "Move: " << mode << " " << mcost1 << " " << mcost2 << " " << mcost3 << " " << mcost4 << " Cur_Cost: " << currentSol->cost << " " << currentSol->s_cost << " " << currentSol->t_cost << " " << currentSol->e_cost << endl;
			//cout << "Tabu: " << tabu_cost << " " << tabu_scost << " " << tabu_tcost << " " << tabu_ecost << endl;
			
		}
		else {
			//printf("Mode %d No Move Found \n", mode);
		}
		counter++;

		if (non_improv > iters)  break;
	}

	//printf("Final Measured Distance %d \n", temp_sol->cost_array[DISTANCE]);

	//At this point bring temp_sol as the currentSol and return
	*currentSol = *temp_sol;
	delete temp_sol;
    delete temp_best_sol;

	//Cleanup
	//Restore Parameters
	MAKESPAN_F = OLD_MAKESPAN_F;
	TOTAL_FLOW_TIME_F = OLD_TOTAL_FLOW_TIME_F;
	TOTAL_IDLE_TIME_F = OLD_TOTAL_IDLE_TIME_F;
	TOTAL_ENERGY_F = OLD_TOTAL_ENERGY_F;
	TARDINESS_F = OLD_TARDINESS_F;
	SLACK_F = OLD_SLACK_F;
	DISTANCE_F = OLD_DISTANCE__F;
	optimtype = 1;

	//Bring back the correct tabu list
	tabumlist = old_list;

    //Bring back function pointers
    addMove = &localSearch::addMoveinList;
    findMove = &localSearch::findMoveinList;
    clearTabu = &localSearch::clearTabuList;
	
	//Recalculate costs properly
	currentSol->getCost(false);

    //Final Report
    printf("Final Distance achieved %4.1f. Effectiveness %4.1f\n", objective, (start_dist - objective)/(start_dist - target_dist));
    printf("Best Makespan encountered %4.1f - Final Sol Makespan %4.1f\n", best_makespan, currentSol->cost_array[MAKESPAN]);
	return objective;
}


void localSearch::cleanUp() {
	//CleanUp Statistics
	for (int i=0;i<TABU_MOVE_COUNT;i++){
        moveStats[i] = 0;
        failedMoves[i] = 0;
    }

	//CleanupLists
    //Clear tabu list
    for (size_t i = 0; i < tabumlist->size(); i++)
        delete (*tabumlist)[i];

    for (size_t i = 0; i < p_tabumlist->size(); i++)
        delete (*p_tabumlist)[i];

    tabumlist->clear();
    p_tabumlist->clear();

    //Cleanup Structs
	TabuMoveSelector->clear();
	CompoundRangeSelector->clear();
    tabu_progress.clear();

}

void localSearch::deleteTabuList() {
    //Delete Everything
    for (size_t i = 0; i < tabumlist->size(); i++)
        delete (*tabumlist)[i];

    for (size_t i = 0; i < p_tabumlist->size(); i++)
        delete (*p_tabumlist)[i];

    tabumlist->clear();
    p_tabumlist->clear();

    //make sure to swap the vectors because the underlying capacity has not been affected from the clear
    vec_del<tabu*>(*tabumlist);
    vec_del<tabu*>(*p_tabumlist);

    //Finally Delete Pointers
    delete tabumlist;
    delete p_tabumlist;
}

void localSearch::clearTabuList() {
	//Clear tabu list
	while (tabumlist->size() > (size_t) options->ts_tenure) {
		delete (*tabumlist)[0];
		vRemoveAt<tabu*>((*tabumlist), 0);
	}
}

void localSearch::clearTabuMatrix() {
    /*
    //Clear tabu list
    for (int i=0;i<NumberOfMachines;i++)
        for (int j=0;j<NumberOfOperations;j++)
            for (int k=0;k<NumberOfOperations;k++)
                tabumatrix[i][j][k]  -= tabu_tenure;

    */
}

void localSearch::clearTabuList(job *jb) {
	//Partial clear of the tabu list
	//Any arcs that involve jb operations will be removed, all the rest will be kept
	//Clear tabu lists 
	size_t i = 0;
	while (i < tabumlist->size()) {
		tabu *mov = (*tabumlist)[i];
		if ((get<0>(mov->comb) == jb->ID) || (get<2>(mov->comb) == jb->ID)) {
			delete (*tabumlist)[i];
			vRemoveAt<tabu*>((*tabumlist), i);
		}
		else i++;
	}
}


void localSearch::report() {
	//Report Stats
	int total_moves = moveStats[RELOCATE] + moveStats[EXCHANGE] + moveStats[COMPOUND] + moveStats[SMART_RELOCATE] + moveStats[SMART_EXCHANGE]\
		+ moveStats[CRIT_SMART_RELOCATE] + moveStats[CRIT_SMART_EXCHANGE] + moveStats[CRIT_BLOCK_EXCHANGE] + moveStats[CRIT_BLOCK_SEQUENCE];
	printf("TS MOVE REPORT - TOTAL MOVES %d \n | RELOCATE %d/%d EXCHANGE %d/%d COMPOUND %d/%d "
                   "S_RELOCATE %d/%d | S_EXCHANGE %d/%d \n | "
                   "C_RELOCATE %d/%d | C_EXCHANGE %d/%d PERTURBATION %d \n | "
                   "C_BLOCK_SEQUENCE %d/%d | C_BLOCK_EXCHANGE %d/%d \n | "
                   "ASPIRATION %d | INFEASIBLE %d \n | BLACKLISTED MOVES : %d | TABU MOVES %d \n | "
                   "OUTERTIES %d | INNERTIES %d \n",

		total_moves, moveStats[RELOCATE] - failedMoves[RELOCATE], moveStats[RELOCATE],
                     moveStats[EXCHANGE] - failedMoves[EXCHANGE], moveStats[EXCHANGE],
                     moveStats[COMPOUND] - failedMoves[COMPOUND], moveStats[COMPOUND],
	                 moveStats[SMART_RELOCATE] - failedMoves[SMART_RELOCATE], moveStats[SMART_RELOCATE],
                     moveStats[SMART_EXCHANGE] - failedMoves[SMART_EXCHANGE], moveStats[SMART_EXCHANGE],
	                 moveStats[CRIT_SMART_RELOCATE] - failedMoves[CRIT_SMART_RELOCATE], moveStats[CRIT_SMART_RELOCATE],
                     moveStats[CRIT_SMART_EXCHANGE] - failedMoves[CRIT_SMART_EXCHANGE], moveStats[CRIT_SMART_EXCHANGE], moveStats[PERTURBATION],
                     moveStats[CRIT_BLOCK_SEQUENCE] - failedMoves[CRIT_BLOCK_SEQUENCE], moveStats[CRIT_BLOCK_SEQUENCE],
                     moveStats[CRIT_BLOCK_EXCHANGE] - failedMoves[CRIT_BLOCK_EXCHANGE], moveStats[CRIT_BLOCK_EXCHANGE],
        moveStats[ASPIRATION], moveStats[INFEASIBLE],
           (int) tabumlist->size(), moveStats[TABU], moveStats[OUTER_TIES], moveStats[INNER_TIES]);
}

void localSearch::reportMoves() {

    printf("---Rep Start---\n");
    for (size_t i = 0; i < tabumlist->size(); i++) {
		tabu *mov = (*tabumlist)[i];
		mov->report();
	}
    printf("---Rep Stop---\n");
}

//Tabulist Functions
bool localSearch::findMoveinList(operation *op1, operation *op2, machine *mc, int mode) {
    // //Calculate Hash
    // string h = tabu::hash_re(ie, je, ke, le, mode);
    // unordered_map<string, tabu*>::const_iterator got = tabulist.find(h);

    //  if (got == tabulist.end())	return false;
    //  return true;


    //Just use tabumlist
    for (size_t i=0;i<tabumlist->size();i++){
        tabu *mov = (*tabumlist)[i];
        //Strict Version
        //if (mov->op1->is_Equal(op1) && (mov->op1_pos == op1->mthesi) && mov->op2->is_Equal(op2) && mov->mc->isEqual(mc))
        //Relaxed Version [Stable]
        if (get<0>(mov->comb)==op1->UID && get<1>(mov->comb) == op2->UID && get<2>(mov->comb) == mc->ID)
        //Way more relaxed version
        //if (mov->op1->is_Equal(op1) && mov->op2->is_Equal(op2))
            return true;
    }

    return false;
}

void inline localSearch::addMoveinList(operation *op1, operation *op2, machine *mc, int mode) {
    tabu *t1 = new tabu(op1->UID, op2->UID, mc->ID, mode);
    tabumlist->push_back(t1);
}

bool localSearch::findRelocate(int k, int l, int i, int j){
	//This wrapper function tries to find relocate tabu moves
	bool found = true;
	machine *mck = currentSol->solmachines[k];
	machine *mcl = currentSol->solmachines[l];

	//Search for the arcs that are to be created
	/*
	if ((i < mck->operations.size() - 2)){
		found &= findMove(mck->operations[i - 1],
						  mck->operations[i + 1],
				  		  mck, 0);
	}


	found &= findMove(mcl->operations[j - 1],
					  mck->operations[i],
			  		  mcl, 0);


	if (j < mcl->operations.size() - 2){
		found &= findMove(mck->operations[i],
						  mcl->operations[j],
						  mcl, 0);
	}
	*/

	//Search for the arcs that are to be deleted
	found &= (this->*findMove)(mck->operations[i - 1],
				mck->operations[i],
 				mck, 0);

	if (i < (int) mck->operations.size() - 2){
 		found &= (this->*findMove)(mck->operations[i],
 				mck->operations[i + 1],
 				mck, 0);
 	}

 	if (j < (int) mcl->operations.size() - 2) {
 		found &= (this->*findMove)(mcl->operations[j-1],
			mcl->operations[j],
            mcl, 0);
    }

    return found;
}

bool localSearch::findExchange(int k, int l, int i, int j){
	//This wrapper function tries to find exchange tabu moves
	bool found = true;
	machine *mck = currentSol->solmachines[k];
	machine *mcl = currentSol->solmachines[l];

	//Search for the created arcs
	/*
	found &= findMove(mck->operations[i - 1],
					  mcl->operations[j],
			  		  mck, 1);R

	if ((i < mck->operations.size() - 2) && (j > i + 1)) {
		found &= findMove(mcl->operations[j],
						  mck->operations[i + 1],
				  		  mck, 1);
	}

	if (j > i + 1) {
		found &= findMove(mcl->operations[j - 1],
						  mck->operations[i],
				  		  mcl, 1);
	}

	if (j < mcl->operations.size() - 2) {
		found &= findMove(mck->operations[i],
						  mcl->operations[j + 1],
				  		  mcl, 1 );
	}
	*/

	//Search for the deleted arcs
	found &= (this->*findMove)(mck->operations[i - 1],
				mck->operations[i],
 				mck, 1);


  	if ((i < (int) mck->operations.size() - 2) && (j > i+1)) {
 		found &= (this->*findMove)(mck->operations[i],
 				mck->operations[i + 1],
                mck, 1);
 	}

  	if (j > i + 1) {
 		found &= (this->*findMove)(mcl->operations[j - 1],
 	  	        mcl->operations[j],
 				mcl, 1);
 	}

  	if (j < (int) mcl->operations.size() - 2) {
 		found &= (this->*findMove)(mcl->operations[j],
 				mcl->operations[j + 1],
 				mcl, 1);
 	}


	return found;
}

float localSearch::applyTieBreaker() {
	#if TIE_BREAKER == 0
		return -INF; //The default best_qmetric value is -INF
	#elif TIE_BREAKER == 1
		return solution::humDistance_single_times(currentSol, localElite);	
	#elif TIE_BREAKER == 2
		return currentSol->calculate_localSlack(true);
	#elif TIE_BREAKER == 3
		return currentSol->humDistance_slots(true);
	#elif TIE_BREAKER == 4
		return globals->op_freq_map->seldomness(currentSol, true);
    //Gia ta panhguria metric
    #elif TIE_BREAKER == 5
        return currentSol->numberOfCriticalOps();

	#endif
}