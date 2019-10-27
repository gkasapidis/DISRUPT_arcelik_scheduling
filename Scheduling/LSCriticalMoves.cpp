#include "LSSolutionClass.h"


bool localSearch::critical_ls_smart_exchange(int &ke, int &le, int &ie, int &je, int mode, bool firstbest, bool improve_only, float* minim_ar)
{
	Graph<operation> *gr_f = currentSol->FGraph;
	Graph<operation> *gr_b = currentSol->BGraph;
	bool vrika = false;
	float cost_ar[COST_NUM];
	for (int i = 0; i < COST_NUM; i++)
		cost_ar[i] = INF;

	float q_metric;
	float best_q_metric = -INF;
    currentSol->findCriticalPath_viaSlack();
	vector<operation*> criticalOps = currentSol->FGraph->criticalPath;

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

	copy(cost_ar, cost_ar + COST_NUM, minim_ar);

	//cout << *currentSol;
	//Fill Infeasibility matrix
	//Init
	memset(&infeasibility_matrix[0][0], 0, sizeof(bool)*MAX_OPERATIONS*MAX_OPERATIONS);
	for (int i = 0; i < currentSol->soloperations.size(); i++) {
		operation *op = currentSol->soloperations[i];
		if (!op->valid) continue;

		//Step A Neglect machine operations after the job successor
		operation *suc, *pred;
		suc = op->get_suc();
		pred = op->get_pred();
		while (true)
		{
			if (suc == NULL) break;
			machine *mc = suc->mc;

			if ((mc->ID == op->mc->ID)) {
				infeasibility_matrix[suc->UID][op->UID] = true;
				infeasibility_matrix[op->UID][suc->UID] = true;
			}
			for (int j = suc->mthesi + 1; j < mc->operations.size() - 1; j++) {
				operation *mop = mc->operations[j];
				infeasibility_matrix[mop->UID][op->UID] = true;
				infeasibility_matrix[op->UID][mop->UID] = true;
			}
			//Advance
			suc = suc->get_suc();
		}

		//Step B Neglect machin operations before the job predecessor
		while (true)
		{
			if (pred == NULL) break;
			machine *mc = pred->mc;

			if ((mc->ID == op->mc->ID)) {
				infeasibility_matrix[pred->UID][op->UID] = true;
				infeasibility_matrix[op->UID][pred->UID] = true;
			}

			for (int j = 1; j < pred->mthesi; j++) {
				operation *mop = mc->operations[j];
				infeasibility_matrix[mop->UID][op->UID] = true;
				infeasibility_matrix[op->UID][mop->UID] = true;
			}
			//Advance
			pred = pred->get_pred();
		}

	}

	/*
	//Make sure to allow swaps with same job operations
	for (int i = 0; i < currentSol->soloperations.size() - 1; i++) {
	operation *op1 = currentSol->soloperations[i];
	if (!op1->valid) continue;
	for (int j = i + 1; j < currentSol->soloperations.size(); j++) {
	operation *op2 = currentSol->soloperations[j];
	if (!op2->valid) continue;
	if (!(op1->jobID == op2->jobID)) continue;
	infeasibility_matrix[op1->UID][op2->UID] = false;
	infeasibility_matrix[op2->UID][op1->UID] = false;
	}
	}
	*/



	//Critical Ops Loop
	for (int oi = 0; oi<criticalOps.size(); oi++) {
		operation *op1 = criticalOps[oi];
		machine *op1_mc = op1->mc;
		int mj = op1_mc->ID;
		int op1_thesi = op1->mthesi;


		//Remove Op1 from solution
		//int old_pos_1 = op1->mthesi;
		currentSol->removeOp(op1_mc, op1_thesi);

		//Iterate in normal ops
		//Machine loop
		for (int mi = 0; mi < currentSol->solmachines.size(); mi++) {
			machine *new_mc = currentSol->solmachines[mi];

			//First Operation Loop
			for (int ooj = 1; ooj < new_mc->operations.size() - 1; ooj++) {
				operation *cand = new_mc->operations[ooj];
				//int oj = cand->mthesi;
				if (cand->UID == op1->UID) continue;

				//Prune
				if (infeasibility_matrix[cand->UID][op1->UID]) continue;
				if (!op1->getProcTime(new_mc)) continue;
				if (!cand->getProcTime(op1_mc)) continue;

				//if ((mj == mi) && (oi > oj)) continue; Not sure what this does

				//Do the swap
				//printf("Swap Machines %d %d Positions %d %d Ops %d %d \n", mi, mj, op1_thesi, ooj, op1->UID, cand->UID);
				//printf("Solution before probe\n");
				//cout << *currentSol;

				//Probe the move
				bool feasible = smart_exchange(op1, cand, op1_thesi, op1_mc, new_mc, cost_ar, q_metric, 10000);
				probCounter++;

				/*
				if (!feasible) {
				printf("Infeasible swap. Further Investigate\n");
				printf("Swap Machines %d %d Positions %d %d Ops %d %d \n", mi, mj, op1_thesi, ooj, op1->UID, cand->UID);
				cout << *currentSol;
				}
				*/
				//printf("Solution after probe\n");
				//cout << *currentSol;

				bool ts_comparison = solution::Objective_TS_Comparison(cost_ar, minim_ar);
                bool save_sol = false;
				//Check for ties with the main solution
#if (TIE_BREAKER > 0)
				if (cost_ar[MAKESPAN] == 0 && cost_ar[TOTAL_FLOW_TIME] == 0 && cost_ar[TOTAL_IDLE_TIME] == 0 && feasible) {
					outertieCounter++;
					if (q_metric > best_q_metric) {
						//printf("TIE CANDIDATE! New %d vs Current %d \n", q_metric, best_q_metric);
						//Accept tie if its is further than the current best neighbor
                        save_sol = true;
                        copy(cost_ar, cost_ar + COST_NUM, minim_ar);
                        best_q_metric = q_metric;
                        ie = op1_thesi;
                        if (mi != mj)
                            je = cand->mthesi;
                        else {
                            if (ooj < op1_thesi) je = cand->mthesi;
                            else if (ooj >= op1_thesi) je = cand->mthesi + 1;
                        }
                        ke = op1_mc->ID; le = new_mc->ID;
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
                        ie = op1_thesi;
                        if (mi != mj)
                            je = cand->mthesi;
                        else {
                            if (ooj < op1_thesi) je = cand->mthesi;
                            else if (ooj >= op1_thesi) je = cand->mthesi + 1;
                        }
                        ke = op1_mc->ID; le = new_mc->ID;
                        vrika = true;
					}
				}
				else if (ts_comparison && feasible) {
#else
				if (ts_comparison && feasible) {
#endif
					//if (feasible) {
					imprCounter++;
					//Check if move is tabu
					bool tabu_move = true;
					//Search for created arcs
					//TODO: RE-ADD THE PROPER TABU SEARCHES

					if (mi != mj || ((mj == mi) && ((ooj - op1_thesi >= 1) || (op1_thesi - ooj >= 2)))) {
						//4 Arcs generic case
						//old_mc
						tabu_move &= (this->*findMove)(op1_mc->operations[op1_thesi - 1],
							new_mc->operations[ooj],
							op1_mc, 1);

						tabu_move &= (this->*findMove)(new_mc->operations[ooj],
							op1_mc->operations[op1_thesi],
							op1_mc, 1);

						//new_mc
						tabu_move &= (this->*findMove)(op1,
							new_mc->operations[ooj + 1],
							new_mc, 1);

						tabu_move &= (this->*findMove)(new_mc->operations[ooj - 1],
							op1,
							new_mc, 1);
					}
					//Same machine sequential swaps
					else {
						if (op1_thesi - ooj == 1) {
							//Cand is above op1
							tabu_move &= (this->*findMove)(op1,
								new_mc->operations[ooj],
								op1_mc, 1);

							tabu_move &= (this->*findMove)(new_mc->operations[ooj - 1],
								op1,
								op1_mc, 1);

							tabu_move &= (this->*findMove)(new_mc->operations[ooj],
								op1_mc->operations[op1_thesi],
								op1_mc, 1);
						}
						else if (ooj == op1_thesi) {
							//Op1 is above cand
							tabu_move &= (this->*findMove)(op1_mc->operations[op1_thesi - 1],
								cand,
								op1_mc, 1);

							tabu_move &= (this->*findMove)(cand,
								op1,
								op1_mc, 1);

							tabu_move &= (this->*findMove)(op1,
								new_mc->operations[ooj + 1],
								new_mc, 1);
						}
						else {
							printf("Unresolved Case\n");
							assert(false);
						}
					}

					//Reset tabu status under probability if the move is improving
					if (tabu_move && (randd() < tabu_relaxation)) tabu_move = false;

					if (!tabu_move) {
						copy(cost_ar, cost_ar + COST_NUM, minim_ar);
						best_q_metric = q_metric;
						ie = op1_thesi;
						if (mi != mj)
							je = cand->mthesi;
						else {
							if (ooj < op1_thesi) je = cand->mthesi;
							else if (ooj >= op1_thesi) je = cand->mthesi + 1;
						}
						ke = op1_mc->ID; le = new_mc->ID;
						vrika = true;
						//printf("New Better Move\n");
						//printf("%d, %d, %d, %d, UIDs %d %d\n", ke, le, ie, je, op1->UID, cand->UID);
					}
					else if (Objective_TS_Comparison_Asp())
					{
						copy(cost_ar, cost_ar + COST_NUM, minim_ar);
						best_q_metric = q_metric;
						ie = op1_thesi;
						if (mi != mj)
							je = cand->mthesi;
						else {
							if (ooj < op1_thesi) je = cand->mthesi;
							else if (ooj >= op1_thesi) je = cand->mthesi + 1;
						}
						ke = op1_mc->ID; le = new_mc->ID;
						moveStats[ASPIRATION]++;
						//cout << "Aspiration Condition! " << cost1 << endl;
						//printf("New Better Asp Move\n");
						//printf("%d, %d, %d, %d, UIDs %d %d\n", ke, le, ie, je, op1->UID, cand->UID);
					}
					else
						tabuMCounter++;

				}
				else
					worseCounter++;

				//else if (cost1==0 && cost2==0 && cost3 == 0)
				//printf("Tie\n");
				}

			}
		currentSol->insertOp(op1_mc, op1_thesi, op1);
		}


	moveStats[TABU] += tabuMCounter;

	//printf("SMART EXCHANGE Final Costs %d %d %d %d \n", minim1, minim2, minim3, minim4);
	//printf("Final Move %d %d %d %d \n", ke, le, ie, je);
	//printf("LS Smart Exchange Improving %d Outter Ties %d Inner Ties %d Worse %d\n", imprCounter, outertieCounter, innertieCounter, worseCounter);

	//char test = 0;
	//scanf("%c", &test);
	//Cleanup
	criticalOps.clear();
	vec_del<operation*>(criticalOps);

	if (improve_only) {
		//Compare best found move with the currentSol costs
		if (solution::Objective_TS_Comparison(minim_ar, currentSol->cost_array))
			return true;

		return vrika;
	}

	return vrika;
}

bool localSearch::critical_ls_smart_relocate(int & ke, int & le, int & ie, int & je, bool firstbest, bool improving_only, float *minim_ar)
{
	Graph<operation> *gr_f = currentSol->FGraph;
	Graph<operation> *gr_b = currentSol->BGraph;
	bool vrika = false;

    float cost_ar[COST_NUM];
    for (int i = 0; i < COST_NUM; i++)
        cost_ar[i] = INF;

    float q_metric;
	float best_q_metric = -INF;

    //currentSol->findCriticalPath_viaSlack(); Path should be already updated
	vector<operation*> criticalOps = currentSol->FGraph->criticalPath;
    //printf("LS_Smart Relocate\n");
	//cout << *currentSol;
	//for (int i = 0; i < tabulist.size(); i++)
	//	tabumlist[i]->report();
	//printf("-----------------\n");


	int tabuMCounter = 0;
	int probCounter = 0;

	//MoveStatusCounters
	int imprCounter = 0;
	int worseCounter = 0;
	int innertieCounter = 0;
	int outertieCounter = 0;

	/*
	//Store Topological order map
	gr_f->topological_sort();
	int *top_sort = new int[gr_f->size];
	for (int i = 0; i < gr_f->size; i++) {
	operation *op = gr_f->top_sort[i];
	top_sort[op->UID] = i;
	}
	//Report TSort


	printf("TSort ");
	for (int i = 0; i < gr_f->top_sort.size(); i++) {
	operation *cand = gr_f->top_sort[i];
	printf("%d ", cand->UID);
	}
	printf("\n");
	*/


	//for (int ji = 0; ji < currentSol->soljobs.size(); ji++) {
	//	job *jc = currentSol->soljobs[ji];
	//for (int ji = 0; ji < currentSol->solmachines.size(); ji++) {
	for (int ji = 0; ji < criticalOps.size(); ji++) {

		//for (int jj = 1; jj < mmc->operations.size() - 1; jj++) {
		{
			//Load one operation for testing
			//operation *op = jc->operations[jj];
			operation *op = criticalOps[ji];
			machine *mmc = op->mc;

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
			for (int i = 0; i < currentSol->solmachines.size(); i++) {
				machine *mc = currentSol->solmachines[i];
				for (int j = 0; j < mc->operations.size() - 1; j++) {
					mc->operations[j]->infeasible = false;
					//mc->operations[j]->moving = 0;
				}
			}

			//DFS on the right of the suc (if any)
			if (op->has_sucs()) {
#if(COMPLEX_PRECEDENCE_CONSTRAINTS)
                for (int kk=0;kk<MAX_JOB_RELATIONS;kk++){
                    operation *suc = op->get_suc(kk);
                    if (suc == nullptr) break;
                    gr_f->dfs(suc->UID);
                    for (int i = 0; i < gr_f->visitedNodes_n; i++) {
                        operation* movOp = gr_f->visitedNodes[i];
                        //printf("Forward BlackListed Operation %d %d \n", movOp->jobID, movOp->ID);
                        movOp->infeasible = true;
                    }
                }
#else
                operation *suc = op->get_suc();
				gr_f->dfs(suc->UID);
				for (int i = 0; i < gr_f->visitedNodes_n; i++) {
					operation* movOp = gr_f->visitedNodes[i];
					//printf("Forward BlackListed Operation %d %d \n", movOp->jobID, movOp->ID);
					movOp->infeasible = true;
				}
#endif
            }

			//DFS on the left of the pred (if any)
			if (op->has_preds()) {
#if(COMPLEX_PRECEDENCE_CONSTRAINTS)
                for (int kk=0;kk<MAX_JOB_RELATIONS;kk++){
                    operation *pred = op->get_pred(kk);
                    if (pred == nullptr) break;
                    gr_b->dfs(pred->UID);
                    //In this case we need to blacklist the machine predecessor of the visited nodes
                    for (int i = 0; i < gr_b->visitedNodes_n; i++) {
                        operation *vOp = gr_b->visitedNodes[i];
                        //Check for dummy operations
                        if (!vOp->valid) continue;
                        operation *movOp = vOp->get_mach_pred();
                        //printf("Backward BlackListed Operation %d %d \n", movOp->jobID, movOp->ID);
                        movOp->infeasible = true;
                    }
                }

#else
                operation *pred = op->get_pred();
				gr_b->dfs(pred->UID);
				//In this case we need to blacklist the machine predecessor of the visited nodes
				for (int i = 0; i < gr_b->visitedNodes_n; i++) {
					operation *vOp = gr_b->visitedNodes[i];
					//Check for dummy operations
					if (!vOp->valid) continue;
					operation *movOp = vOp->get_mach_pred();
                    //printf("Backward BlackListed Operation %d %d \n", movOp->jobID, movOp->ID);
					movOp->infeasible = true;
				}
#endif




            }

			//Neglect the machine predecessor to prevent placement on the same position
			operation *imm_pred = op->get_mach_pred();
			if (imm_pred != NULL)
				imm_pred->infeasible = true;

			//Remove Operation from solution
			//printf("Solution after op %d %d removal from machine %d pos %d \n",op->jobID, op->ID, op->mc->ID, op->mthesi);
			currentSol->removeOp(op->mc, op->mthesi);
			//cout << *currentSol;
			//printf("-------------------------\n");

			//Probing Loop
			for (int i = 0; i < currentSol->solmachines.size(); i++) {
				machine *mc = currentSol->solmachines[i];
				if (!op->getProcTime(mc)) continue;

				for (int j = 0; j < mc->operations.size() - 1; j++) {
					operation *cand = mc->operations[j];
                    if (cand->infeasible) {
                        if (op->check_sucRelation(cand)) {
                            //printf("Cyclic Infeasibility breaking \n");
                            break;
                        } else
                            continue;
                    }
                    probCounter++;

					//Set Identify moving ops and set done status
					//Find min index
					//int min_index = min(top_sort[op->UID], top_sort[cand->UID]);
					//cout << *currentSol;

					//I can probe at this point
					//printf("Moving %d %d after operation %d %d %d \n", 
					//	op->jobID, op->ID, cand->jobID, cand->ID, cand->UID);
					smart_relocate(op, old_mc_id, mc->ID, old_pos, cand->mthesi + 1, cost_ar, q_metric);
					//printf("Costs %d %d %d %d \n", cost1, cost2, cost3, cost4);


					//bool ts_comparison = solution::Objective_TS_Comparison( (int*) (const int []) {cost1, cost2, cost3, cost4}, (int*) (const int[]) { minim1, minim2, minim3, minim4 });
					bool ts_comparison = solution::Objective_TS_Comparison(cost_ar, minim_ar);

					//Check for ties with the main solution
#if (TIE_BREAKER > 0)
					if (cost_ar[MAKESPAN] - currentSol->cost_array[MAKESPAN] == 0 && cost_ar[TOTAL_FLOW_TIME] - currentSol->cost_array[TOTAL_FLOW_TIME] == 0 && cost_ar[TOTAL_IDLE_TIME] - currentSol->cost_array[TOTAL_IDLE_TIME] == 0) {
						outertieCounter++;
						if (q_metric > best_q_metric) {
							//printf("Outer TIE CANDIDATE! New %d vs Current %d \n", q_metric, best_q_metric);
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
							//printf("Inner TIE CANDIDATE! New %d vs Current %d \n", q_metric, best_q_metric);
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

						//if (true) {

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
							//cout << "Aspiration Condition! " << cost1 << endl;
							//In this case the move has to be done as is in order to set the new best
							//Bring Operation Back in place before returning
							//currentSol->insertOp(currentSol->solmachines[old_mc_id], old_pos, op);
							//return true;

							//EDIT: Not sure if I have to return at this point,
							//maybe there is a better aspiration move that I'll miss if I leave
						}
						else
                            tabuMCounter++;
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
	//Cleanup
	criticalOps.clear();
	//delete top_sort;
	vec_del<operation*>(criticalOps);

    if (improving_only) {
		//Compare best found move with the currentSol costs

		if (solution::Objective_TS_Comparison(minim_ar, currentSol->cost_array))
			return true;
		else
			return false;

	}
	else {
		return vrika;
	}
}

bool localSearch::ls_relocate(int & ke, int & le, int & ie, int & je, int mode, bool firstbest, bool improving_only, float *minim_ar) {

	Graph<operation> *gr_f = currentSol->FGraph;
	Graph<operation> *gr_b = currentSol->BGraph;
	bool vrika = false;
	float cost_ar[COST_NUM];
	for (int i = 0; i < COST_NUM; i++)
		cost_ar[i] = INF;

	float q_metric;
	float best_q_metric = -INF;
	bool feasible;
	bool save_sol;
	//Counters
	int imprCounter = 0;
	int worseCounter = 0;
	int tabuMCounter = 0;
	int outertieCounter = 0;
	int innertieCounter = 0;

    //printf("Start Sol\n");
    //cout << *currentSol;
    for (int op_id = 0; op_id < currentSol->soloperations.size(); op_id++) {
        operation *opi = currentSol->soloperations[op_id];
        int i = opi->mthesi;
        if (opi->jobID < 0) continue; //dummy machine ops
        if (opi->fixed) continue; //Do not work on fixed ops
        machine *mc_i = opi->mc;
        process_step *opi_ps = currentSol->ProcSteps[opi->process_stepID];

        for (int l = 0; l < opi_ps->machines.size(); l++) {
            machine *mc_j = opi_ps->machines[l];

            int mc_j_lim = (int) mc_j->operations.size();
            if (mc_i->ID==mc_j->ID) mc_j_lim -= 1;

            for (int j = 1; j < mc_j_lim; j++) {

                //Pruning
                if (j==i && (mc_i->ID == mc_j->ID)) continue;
				if (opi->getProcTime(mc_j) == 0) continue;

				operation *op_j, *op_k; //Generic Defs
                if ((mc_i->ID == mc_j->ID) && (j>i)) {
                    op_j = mc_i->operations[j];
                    op_k = mc_i->operations[j + 1];
                } else {
                    op_j = mc_j->operations[j - 1];
                    op_k = mc_j->operations[j];
                }

                //Prune easy cases
                if (opi->check_sucRelation(op_j) || opi->check_predRelation(op_k)){
                    //printf("Easy Pruning\n");
                    continue;
                }

                //Rescheduling don't allow moves before fixed ops
                if (op_k->fixed) continue;

                //Do the relocation
				//printf("Moving [%d,%d] of %d after operation [%d,%d] of %d \n",
                //	   opi->jobID, opi->ID, mc_i->ID, cand->jobID, cand->ID, mc_j->ID);

                feasible = temp_relocate(opi, mc_i->ID, mc_j->ID, i, j, cost_ar, q_metric);
                if (!feasible) {
                    //printf("Infeasible\n");
                    //cout << *currentSol;
                }

                //printf("Costs %4.1f %4.1f %4.1f %4.1f \n", cost_ar[MAKESPAN], cost_ar[TOTAL_FLOW_TIME], cost_ar[TOTAL_IDLE_TIME], cost_ar[DISTANCE]);
                //Prove moves
                //Evaluate move right here

                bool ts_comparison = solution::Objective_TS_Comparison(cost_ar, minim_ar);

                //Stop if the solution is not feasible
                if (!feasible) {
                    //printf("Infeasible Move : Moving [%d,%d] of %d after operation [%d,%d] of %d \n",
                    //       opi->jobID, opi->ID, mc_i->ID, cand->jobID, cand->ID, mc_j->ID);
                    worseCounter++;
                    continue;
                } else{
                    /*
                    printf("Probing Move : Moving [%d,%d] of %d after operation [%d,%d] of %d \n",
                           opi->jobID, opi->ID, mc_i->ID, cand->jobID, cand->ID, mc_j->ID);
                    printf("Costs %4.1f \n", cost_ar[MAKESPAN]);
                     */
                }

                save_sol = false;

#if TIE_BREAKER > 0
                //Check for ties with the current best move
                if (cost_ar[MAKESPAN] - minim_ar[MAKESPAN] == 0 && cost_ar[TOTAL_FLOW_TIME] - minim_ar[TOTAL_FLOW_TIME] == 0 && cost_ar[TOTAL_IDLE_TIME] - minim_ar[TOTAL_IDLE_TIME] == 0) {
                    innertieCounter++;
                    if ((q_metric < best_q_metric)) {
                        //printf("Inner TIE CANDIDATE! New %7.6f vs Current %7.6f \n", q_metric, best_q_metric);
                        //Accept tie if its is further than the current best neighbor
                        save_sol = true;
                    }
                }
                else if (ts_comparison) {
#else
                if (ts_comparison) {
#endif
                    //if (feasible) {
                    imprCounter++;
                    //Check if move is tabu
                    bool tabu_move = true;

                    //Search for created arcs
                    if ((mc_i->ID == mc_j->ID) && (j>i)) {
                        //Normal mode
                        tabu_move &= (this->*findMove)(mc_i->operations[i - 1],
                                                       mc_i->operations[i + 1],
                                                       mc_i, 1);

                        tabu_move &= (this->*findMove)(mc_i->operations[j],
                                                       mc_i->operations[i],
                                                       mc_i, 1);

                        tabu_move &= (this->*findMove)(mc_i->operations[i],
                                                       mc_i->operations[j + 1],
                                                       mc_i, 1);


                        /*printf("Searching for Arcs : [%d-%d], [%d-%d], [%d-%d]\n",
                               mc_i->operations[i - 1]->UID, mc_i->operations[i + 1]->UID,
                               mc_i->operations[j]->UID, mc_i->operations[i]->UID,
                               mc_i->operations[i]->UID, mc_i->operations[j + 1]->UID);*/

                    } else {
                        tabu_move &= (this->*findMove)(mc_i->operations[i - 1],
                                                       mc_i->operations[i + 1],
                                                       mc_i, 1);

                        tabu_move &= (this->*findMove)(mc_i->operations[i],
                                                       mc_j->operations[j],
                                                       mc_j, 1);

                        tabu_move &= (this->*findMove)(mc_j->operations[j - 1],
                                                       mc_i->operations[i],
                                                       mc_j, 1);

                        /*
                        printf("Searching for Arcs : [%d-%d], [%d-%d], [%d-%d]\n",
                               mc_i->operations[i - 1]->UID, mc_i->operations[i + 1]->UID,
                               mc_i->operations[i]->UID, mc_j->operations[j]->UID,
                               mc_j->operations[j - 1]->UID, mc_i->operations[i]->UID);
                        */

                    }

                    //Reset tabu status under probability if the move is improving
                    if (tabu_move && (randd() < tabu_relaxation)) tabu_move = false;

                    if (!tabu_move) {
                        save_sol = true;
                        //printf("Improving Move\n");
                    }
                    else if (Objective_TS_Comparison_Asp())
                    {
                        save_sol = true;
                        moveStats[ASPIRATION]++;
                        //printf("Aspiration Move\n");
                    }
                    else
                        tabuMCounter++;
                }
                else
                    worseCounter++;

                if (save_sol) {
                    copy(cost_ar, cost_ar + COST_NUM, minim_ar);
                    best_q_metric = q_metric;
                    ie = i;
                    je = j;
                    ke = mc_i->ID;
                    le = mc_j->ID;
                    vrika = true;
                    //printf("New Move: %d, %d, %d, %d\n", ke, le, ie, je);


                    if (firstbest){
                        //Return immediately if an improving solution has been found
                        if (solution::Objective_TS_Comparison(cost_ar, currentSol->cost_array))
                            return true;
                    }
                }

                /*char in_c;
                printf("Press any key to continue...\n");
                scanf( "%c", &in_c);*/

            }
        }
    }

    //Update Stats
    moveStats[TABU] += tabuMCounter;
    moveStats[OUTER_TIES] += outertieCounter;
    moveStats[INNER_TIES] += innertieCounter;

	return vrika;
}

bool localSearch::critical_FindNeighborsJobs(int &ce, int &ie, int &ue, int mode, int segment_size, int displacement, bool improve_only)
{
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

    //Load critical blocks
    vector<int3_tuple> cJobRanges = currentSol->criticalBlocks;

    //This loop tries to find a range of operations within a job
    //where all operations are scheduled in different machines
    for (size_t jr = 0; jr < cJobRanges.size(); jr++)
    {
        int c = std::get<0>(cJobRanges[jr]);
        int r_start = std::get<1>(cJobRanges[jr]);
        int r_end = std::get<2>(cJobRanges[jr]);
        job *jb = currentSol->soljobs[c];
        int job_n = (int) jb->operations.size();

        for (int i = r_start; i < r_end; i++)
        {
            operation *op1 = jb->operations[i];
            //SEGMENT SIZE CAN REPLACE THIS FOR
            int u = min(i + segment_size, i + r_end - r_start);
            if (u > job_n) continue; //Not enough operations to perform anything
            if ((u - i) == 1) continue;
            //for (int u = i + 2; u <= min(r_end + 1,  min(r_end + 1, job_n); u++)
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

                bool ts_comparison = solution::Objective_TS_Comparison(cost_ar, minim_ar);
                //printf("Current Move Costs: %d %d %d %d \n", cost1, cost2, cost3, cost4);
                //printf("Minim Move Costs: %d %d %d %d \n", minim1, minim2, minim3, minim4);
                //cout << "FindNeighborJobs Objective_TS_Comparison result: " << ts_comparison << endl;
                int tslist_n = (int)tabumlist->size();
                CompoundRangeSelector->update(u - i, ts_comparison);

                //Check for ties with the main solution
                if (cost_ar[MAKESPAN] == 0 && cost_ar[TOTAL_FLOW_TIME] == 0 && cost_ar[TOTAL_IDLE_TIME] == 0) {
                    outertieCounter++;
                    if (q_metric > best_q_metric) {
                        printf("Sol TIE CANDIDATE! New %d vs Current %d \n", q_metric, best_q_metric);
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
                        printf("Move TIE CANDIDATE! New %d vs Current %d \n", q_metric, best_q_metric);
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

    if (improve_only) {
        //Compare best found move with the currentSol costs
        if (solution::Objective_TS_Comparison(minim_ar, currentSol->cost_array))
            return true;

        return vrika;
    }
    else
        return vrika;
}

void localSearch::findCritical(){
	//This function aims to locate critical operations of a solution so that
	//we can indentify them and experiment with moving just those ones
	//since these operations will be the only candidates whose move
	//MAY decrease the makespan
	currentSol->findCriticalPath();
	printf("Looking for Critical Operations\n");
	//cout << *currentSol;
	//currentSol->saveToFile("test_sol");
	vector<vector<operation*>> criticalOps = vector<vector<operation*>>(NumberOfMachines);

	//At first capture the last finishing operations
	int max_end = (int) currentSol->cost_array[MAKESPAN];
	operation *main_critical_op = NULL;
	for (size_t i=0; i<currentSol->soljobs.size(); i++){
		job *jb = currentSol->soljobs[i];
		operation *op = jb->operations[jb->operations.size() - 1];
		if (op->end == max_end){
			if ((main_critical_op == NULL) || (randd() > 0.5f))
				main_critical_op = op;
		}
	}

	if (main_critical_op == NULL)
		printf("pws sto poutso ksefuges\n");

	//Now we need to dfs to the backward graph and ALL visited nodes are critical operations
	Graph<operation> *gr = currentSol->BGraph;
	gr->dfs(main_critical_op->UID);

	//Check visited
	int critical_op_counter = 0;
	for (int j=0; j<gr->visitedNodes_n; j++){
		operation* movOp = gr->visitedNodes[j];
		if (!movOp->valid) continue;
		criticalOps[movOp->mc->ID].push_back(movOp);
		critical_op_counter++;
	}

	//Report the critical operations found for now
	printf("Critical Operations %d / %d \n", critical_op_counter, currentSol->totalNumOp);

	//Report Critical Operations per machine
	for (int i = 0; i < NumberOfMachines; i++) {
		printf("Machine %d Critical Ops:\n", i);
		for (int j = 0; j < criticalOps[i].size(); j++) {
			printf("Op %d %d - Pos %d \n", criticalOps[i][j]->jobID, criticalOps[i][j]->ID, criticalOps[i][j]->mthesi);
		}
	}


	//Step B
	//Organize Critical Operations per machine
	//Sort the arrays using their machine position and locate any critical blocks

	return;
}


//New Critical Neighborhoods
bool localSearch::critical_block_sequencing(int &ke, int &le, int &ie, int &je, bool firstbest, bool improve_only, float *minim_ar){

    //Init
    bool vrika = false;
    float cost_ar[COST_NUM];
    for (int i = 0; i < COST_NUM; i++)
        cost_ar[i] = INF;

    float best_q_metric = INF;
    float q_metric;
    bool feasible;
    //Counters
    int worseCounter = 0;
    int imprCounter = 0;
    int tabuMCounter = 0;


    //Load Critical Blocks
    //currentSol->findCriticalPath_viaSlack();
    vector<int3_tuple> cBlocks = currentSol->criticalBlocks;
    vector<int3_tuple> moves = vector<int3_tuple>();

    for (auto block :cBlocks) {

        int mc_id = get<0>(block);
        int start = get<1>(block);
        int end = get<2>(block);
        int block_size = end - start;


        //Move related stuff
        int op1_pos, op2_pos;
        machine *mc;
        operation *op1, *op2;

        switch (block_size) {
            //Block of only 2 operations, just swap them
            case 2: {
                int3_tuple move = int3_tuple(start, end - 1, mc_id);
                moves.push_back(move);
                break;
            }
            case 1: {
                printf("ERROR ERROR ERROR MALAKIA RANGE\n");
                return -1;
            }
            default: {
                //printf("Intermediate Stuff should happen here\n");


                //Step A - move intermediate ops to the corners of the block
                //Probe all intermediate operations before the first operation of the block
                //Probe all intermediate operations after the last operation of the block
                mc = currentSol->solmachines[mc_id];
                for (int i=start+1; i<end-1; i++) {
                    int3_tuple move = int3_tuple(i, start, mc_id);
                    //int3_tuple move = int3_tuple(start, i, mc_id); //WRONG
                    moves.push_back(move);

                    move = int3_tuple(i, end - 1, mc_id);
                    moves.push_back(move);
                }

                //Step Ba - move first block operation to all other available internal positions
                for (int i=start+2; i<end-1; i++) {
                    int3_tuple move = int3_tuple(start, i, mc_id);
                    moves.push_back(move);

                }
                //Step Bb - move last block operation to all other available internal positions
                for (int i=end-3; i>start; i--) {
                    int3_tuple move = int3_tuple(end - 1, i, mc_id);
                    moves.push_back(move);
                }


                break;
            }
        }

    }

    //Iterate in neighborhood moves
    //Neighborhood is small so there should be not too many candidates
    for(auto move: moves){

        int op1_pos = get<0>(move);
        int op2_pos = get<1>(move);
        machine *mc = currentSol->solmachines[get<2>(move)];

        operation *op1 = mc->operations[op1_pos];
        operation *op2 = mc->operations[op2_pos];


        //printf("Neighbor [%d-%d-%d] temp Sol\n",op1_pos,op2_pos, mc->ID);
        feasible = temp_relocate(op1, mc->ID, mc->ID, op1_pos, op2_pos, cost_ar, q_metric);





        //Prove moves
        //Evaluate move right here
        bool ts_comparison = solution::Objective_TS_Comparison(cost_ar, minim_ar);

        if (ts_comparison && feasible) {

            //if (feasible) {
            imprCounter++;
            //Check if move is tabu
            bool tabu_move = true;
            //Search for created arcs
            //TODO: RE-ADD THE PROPER TABU SEARCHES

            if (op2_pos < op1_pos) {
                tabu_move &= (this->*findMove)(mc->operations[op1_pos - 1],
                                      mc->operations[op1_pos + 1],
                                      mc, 1);

                tabu_move &= (this->*findMove)(mc->operations[op1_pos],
                                      mc->operations[op2_pos],
                                      mc, 1);

                tabu_move &= (this->*findMove)(mc->operations[op2_pos - 1],
                                      mc->operations[op1_pos],
                                      mc, 1);

                /*
                printf("Searching for Arcs : [%d-%d], [%d-%d], [%d-%d]\n",
                       mc->operations[op1_pos - 1]->UID, mc->operations[op1_pos + 1]->UID,
                       mc->operations[op1_pos]->UID, mc->operations[op2_pos]->UID,
                       mc->operations[op2_pos - 1]->UID, mc->operations[op1_pos]->UID);
                   */

            } else {
                //Normal mode
                tabu_move &= (this->*findMove)(mc->operations[op1_pos - 1],
                                      mc->operations[op1_pos + 1],
                                      mc, 1);

                tabu_move &= (this->*findMove)(mc->operations[op2_pos],
                                      mc->operations[op1_pos],
                                      mc, 1);

                tabu_move &= (this->*findMove)(mc->operations[op1_pos],
                                      mc->operations[op2_pos + 1],
                                      mc, 1);

                /*
                printf("Searching for Arcs : [%d-%d], [%d-%d], [%d-%d]\n",
                       mc->operations[op1_pos - 1]->UID, mc->operations[op1_pos + 1]->UID,
                       mc->operations[op2_pos]->UID, mc->operations[op1_pos]->UID,
                       mc->operations[op1_pos]->UID, mc->operations[op2_pos + 1]->UID);
                */
            }



            //Reset tabu status under probability if the move is improving
            if (tabu_move && (randd() < tabu_relaxation)) tabu_move = false;

            if (!tabu_move) {
                copy(cost_ar, cost_ar + COST_NUM, minim_ar);
                best_q_metric = q_metric;
                ie = op1_pos;
                je = op2_pos;
                ke = mc->ID;
                le = mc->ID;
                vrika = true;
                //printf("New Better Move\n");
                //printf("%d, %d, %d, %d, UIDs %d %d\n", ke, le, ie, je, op1->UID, cand->UID);
            }
            else if (Objective_TS_Comparison_Asp())
            {
                copy(cost_ar, cost_ar + COST_NUM, minim_ar);
                best_q_metric = q_metric;
                ie = op1_pos;
                je = op2_pos;
                ke = mc->ID;
                le = mc->ID;
                moveStats[ASPIRATION]++;
                vrika = true;
                //cout << "Aspiration Condition! " << cost1 << endl;
                //printf("New Better Asp Move\n");
                //printf("%d, %d, %d, %d, UIDs %d %d\n", ke, le, ie, je, op1->UID, cand->UID);
            }
            else
                tabuMCounter++;

        }
        else
            worseCounter++;
    }

    //if (tabuMCounter == moves.size()) printf("All Moves tabu fucking do something\n");


    //Cleanup
    moves.clear();
    cBlocks.clear();
    vec_del<int3_tuple>(moves);
    vec_del<int3_tuple>(cBlocks);

    moveStats[TABU] += tabuMCounter;
    return vrika;
}

bool localSearch::critical_block_exchange(int &ke, int &le, int &ie, int &je, bool firstbest, bool improve_only, float *minim_ar){
    //Init
    bool vrika = false;
    float cost_ar[COST_NUM];
    for (int i = 0; i < COST_NUM; i++)
        cost_ar[i] = INF;

    float best_q_metric = INF;
    float q_metric;
    bool feasible;
    //Counters
    int worseCounter = 0;
    int imprCounter = 0;
    int tabuMCounter = 0;

    //Load Critical Path
    //vector<operation*> cPath = currentSol->findCriticalPath();
    //currentSol->findCriticalPath_viaSlack(); Path is already updated
    vector<operation*> cPath = currentSol->FGraph->criticalPath;

    //Calculate Topological Sort
    currentSol->FGraph->topological_sort();

    //Store Topological order map
    int graph_size =currentSol->FGraph->size;
    int *old_top_order = new int[graph_size];
    int *visiting_order = new int[graph_size];

    for (int i = 0; i < graph_size; i++) {
        visiting_order[i] = currentSol->FGraph->top_sort[graph_size - 1 - i]->UID;
        old_top_order[i] = currentSol->FGraph->top_sort[i]->UID;
    }


	/*
    currentSol->FGraph->report_topological_sort();
    printf("\nGraph Topological Indices \n");
    for (int i = 0; i < currentSol->soloperations.size(); i++) {
        operation *op = currentSol->soloperations[i];
        //if (op->valid)
        printf("%d ", currentSol->FGraph->top_sort_index[op->UID]);
    }
    printf("\n");
    */


    //Arrays
    vector<int4_tuple> moves = vector<int4_tuple>();

    //printf("Start Probing\n");

    for (int i=0;i<cPath.size();i++) {
        operation* c_op = cPath[i];
        int old_top_index = currentSol->FGraph->top_sort_index[c_op->UID];

        //Try to move operation to all of its available machines
        for (int mj = 0;mj<NumberOfMachines;mj++){
            if (c_op->getProcTime(currentSol->solmachines[mj]) == 0) continue;
            if (mj == c_op->mc->ID) continue;

            //Start Calculations
            //printf("Trying to move operation %d to machine %d\n", c_op->UID, mj);
            machine *mc = currentSol->solmachines[mj];
            int min_pos = 1;
            int max_pos = (int) mc->operations.size() - 1;

            for (int j=0; j < mc->operations.size() - 1; j++) {
                operation *op = mc->operations[j];
                int op_top_index = currentSol->FGraph->top_sort_index[op->UID];
                //Check index in topological order
                if (old_top_index > op_top_index)
                    min_pos = op->mthesi + 1;
                else if (old_top_index < op_top_index)
                    max_pos = min(max_pos, op->mthesi);

            }

            //printf( "Final Insert Position Range %d - %d\n", min_pos, max_pos );

            //printf( "Check new topological order\n" );

            //printf( "Store the move\n" );

            if (max_pos< min_pos)
                printf("Block Exchange ERROR ERROR ERROR\n");
            if (min_pos > mc->operations.size() || max_pos > mc->operations.size())
                printf("ERROR ERROR ERROR #1\n");

            if (max_pos != min_pos)
                printf("More than one moves yay\n");

            for (int k=min_pos; k<=max_pos; k++){
                int4_tuple move = int4_tuple(c_op->mc->ID, c_op->mthesi, mj, k);
                moves.push_back(move);
            }
        }
    }

    //Probe Moves
    //Iterate in neighborhood moves
    //Neighborhood is small so there should be not too many candidates
    for(auto move: moves){

        int op1_mc_id = get<0>(move);
        int op1_pos = get<1>(move);
        int op2_mc_id = get<2>(move);
        int op2_pos = get<3>(move);

        machine *op1_mc = currentSol->solmachines[op1_mc_id];
        machine *op2_mc = currentSol->solmachines[op2_mc_id];

        operation *op1 = op1_mc->operations[op1_pos];
        operation *op2 = op2_mc->operations[op2_pos];

        //printf("Neighbor [%d-%d-%d-%d] temp Sol\n",op1_pos, op2_pos, op1_mc->ID, op2_mc->ID);
        feasible = temp_relocate(op1, op1_mc_id, op2_mc_id, op1_pos, op2_pos, cost_ar, q_metric);

        //Prove moves
        //Evaluate move right here
        bool ts_comparison = solution::Objective_TS_Comparison(cost_ar, minim_ar);

        if (ts_comparison && feasible) {

            //if (feasible) {
            imprCounter++;
            //Check if move is tabu
            bool tabu_move = true;

            //Search for created arcs

            //Smaller case

            //Relocation always to a different machine will always create 3 arcs
            //old_mc
            tabu_move &= (this->*findMove)(op1_mc->operations[op1_pos - 1],
                                  op1_mc->operations[op1_pos + 1],
                                  op1_mc, 1);

            //new_mc
            tabu_move &= (this->*findMove)(op2_mc->operations[op2_pos - 1],
                                  op1,
                                  op2_mc, 1);

            tabu_move &= (this->*findMove)(op1,
                                  op2_mc->operations[op2_pos],
                                  op2_mc, 1);


            /*
            printf("Searching for Arcs : [%d-%d], [%d-%d], [%d-%d]\n",
                   op1_mc->operations[op1_pos - 1]->UID, op1_mc->operations[op1_pos + 1]->UID,
                   op2_mc->operations[op2_pos - 1]->UID, op1->UID,
                   op1->UID, op2_mc->operations[op2_pos]->UID);
            */

            //Reset tabu status under probability if the move is improving
            if (tabu_move && (randd() < tabu_relaxation)) tabu_move = false;

            if (!tabu_move) {
                copy(cost_ar, cost_ar + COST_NUM, minim_ar);
                best_q_metric = q_metric;
                ie = op1_pos;
                je = op2_pos;
                ke = op1_mc_id;
                le = op2_mc_id;
                vrika = true;
                //printf("New Better Move\n");
                //printf("%d, %d, %d, %d, UIDs %d %d\n", ke, le, ie, je, op1->UID, cand->UID);
            }
            else if (Objective_TS_Comparison_Asp())
            {
                copy(cost_ar, cost_ar + COST_NUM, minim_ar);
                best_q_metric = q_metric;
                ie = op1_pos;
                je = op2_pos;
                ke = op1_mc_id;
                le = op2_mc_id;
                vrika = true;
                moveStats[ASPIRATION]++;
                //cout << "Aspiration Condition! " << cost1 << endl;
                //printf("New Better Asp Move\n");
                //printf("%d, %d, %d, %d, UIDs %d %d\n", ke, le, ie, je, op1->UID, cand->UID);
            }
            else
                tabuMCounter++;

        }
        else
            worseCounter++;
    }

    //Compare topological sort
    currentSol->FGraph->topological_sort();

    //Compare Topological sorts
    for (int i=0; i < currentSol->FGraph->size; i++){
        if (currentSol->FGraph->top_sort[i]->UID != old_top_order[i]){
            printf("Topological Order MisMatch\n");
            currentSol->FGraph->topological_sort();
            break;
        }
    }

    //Cleanup
    moves.clear();
    vec_del<int4_tuple>(moves);
    delete[] old_top_order;
    delete[] visiting_order;

    //printf("Done\n");
    moveStats[TABU] += tabuMCounter;

    return vrika;
}

bool localSearch::critical_block_union(int &ke, int &le, int &ie, int &je, int &mode, bool firstbest, bool improve_only){
    float minim_ar[COST_NUM];
    for (int i = 0; i < COST_NUM; i++)
        minim_ar[i] = INF;

    bool move = false;
    bool move1;
    bool move2;

    //Search for blockSequence
    //printf("Critical Blocks Union Move. Searching...\n");
    //printf("Block Sequence move...\n");
    //cout << *currentSol;
    move1 = critical_block_sequencing(ke, le, ie, je, firstbest, improve_only, minim_ar);
    if (move1) mode = CRIT_BLOCK_SEQUENCE;
    move |= move1;
    //printf("Block Exchange move...\n");
    //cout << *currentSol;

    move2 = critical_block_exchange(ke, le, ie, je, firstbest, improve_only, minim_ar);
    if (move2) mode = CRIT_BLOCK_EXCHANGE;
    move |= move2;

    //move3 = critical_ls_smart_relocate(ke, le, ie, je, firstbest, improve_only, minim_ar);
    //if (move3) mode = CRIT_SMART_RELOCATE;
    //move |= move3;

    //printf("Move1 %d Move2 %d Move3 %d Move %d\n",move1,move2,move3,move);
    return move;
}