#include <sstream>
#include "Parameters.h"
#include "ListObject.h"
#include "OperationClass.h"
#include "MachineClass.h"
#include "ProblemInterface.h"
#include "Resource.h"

using namespace std;

//TEMP STUFF
int *sAEAP_mach_positions = new int[MAX_MACHINES];


//Solution Class Methods
solution::solution()
{
	//cost = planhorend;
	//s_cost = planhorend;
	//t_cost = planhorend;
	//e_cost = planhorend;

	//Init cost arrays
	cost_array = new float[COST_NUM];
	temp_cost_array = new float[COST_NUM];
	d_cost_array = new float[COST_NUM];
	temp_d_cost_array = new float[COST_NUM];
	
	for (int i = 0; i < COST_NUM; i++) {
		cost_array[i] = INF;
		temp_cost_array[i] = INF;
		d_cost_array[i] = INF;
		temp_d_cost_array[i] = INF;
	}
	
    soljobs = vector<job*>();
	solmachines = vector<machine*>();
	soloperations = vector<operation*>();
	//solorders = vector<ProductionOrder*>();
	
	//jobshelp = vector<job*>();
	opshelp = vector<operation*>();
	Stages = vector<stage*>();
	ProcSteps = vector<process_step*>();
	Resources = unordered_map<string, resource*>();

	//Saved values for quick access
	totalNumOp = 0;

	//Init Graphs
	FGraph = new Graph<operation>(MAX_OPERATIONS);
	BGraph = new Graph<operation>(MAX_OPERATIONS);


#if(PFSS)
	int_permutation = new int[NumberOfJobs];
	double_permutation = new double[NumberOfJobs];
	intdouble_convertion = new bool[NumberOfJobs];

	completion_time = new int*[NumberOfMachines];
	for (int m = 0; m<NumberOfMachines; m++) {
		completion_time[m] = new int[NumberOfJobs];
	}
	for (int m = 0; m<NumberOfMachines; m++) {
		for (int j = 0; j<NumberOfJobs; j++) {
			completion_time[m][j] = 0;
		}
	}
#endif
}

solution::~solution()
{
	//Delete cost arrays

	//unroll
	delete[] cost_array;
	delete[] temp_cost_array;
	delete[] d_cost_array;
	delete[] temp_d_cost_array;
	
	for (size_t i = 0; i<soloperations.size(); i++) delete soloperations[i];
	soloperations.clear(); vector<operation*>().swap(soloperations); //Free Memory of vector;

	for (size_t i = 0; i<soljobs.size(); i++) delete soljobs[i];
	soljobs.clear(); vector<job*>().swap(soljobs); //Free Memory of vector;

	for (size_t i = 0; i<solmachines.size(); i++) delete solmachines[i];
	solmachines.clear(); vector<machine*>().swap(solmachines);

	//Cleanup Resources
	for (auto rc : Resources)
	    delete rc.second;
    Resources.clear();

	for (size_t i = 0; i < ProcSteps.size(); i++) delete ProcSteps[i];
	ProcSteps.clear(); vec_del<process_step*>(ProcSteps);

	for (size_t i = 0; i < Stages.size(); i++) delete Stages[i];
	Stages.clear(); vector<stage*>().swap(Stages);
	
	opshelp.clear(); vec_del<operation*>(opshelp);

	criticalBlocks.clear();	vec_del<int3_tuple>(criticalBlocks);


	//Delete Graphs
	delete FGraph;
	delete BGraph;
	
#if(PFSS)
	delete[] int_permutation;
	delete[] double_permutation;
	delete[] intdouble_convertion;
	for (int m = 0; m < NumberOfMachines; m++) { delete[] completion_time[m]; }
	delete[] completion_time;
#endif
}


solution& solution::operator=(const solution &rhs) {
	
	//This Operator is exactly the same with the solution assignment operator
	//The only difference is the assignment of the additional baseSolution


	//copy the dataset reference
	dataset = rhs.dataset;

	//cout << "Calling TabuSolution Custom Assignment Operator" << endl;
    for (int i = 0; i < COST_NUM; i++) {
		this->cost_array[i] = rhs.cost_array[i];
		this->d_cost_array[i] = rhs.d_cost_array[i];
		this->temp_cost_array[i] = rhs.temp_cost_array[i];
		this->temp_d_cost_array[i] = rhs.temp_d_cost_array[i];
	}
	
    this->weight = rhs.weight;
	this->fitness = rhs.fitness;
	this->totalNumOp = rhs.totalNumOp;
	// this->cost = rhs.cost;
	// this->s_cost = rhs.s_cost;
	// this->t_cost = rhs.t_cost;
	// this->e_cost = rhs.e_cost;
	// this->tard_cost = rhs.tard_cost;
	// this->slack = rhs.slack;

	for (int i = 0; i< (int) soloperations.size(); i++)
		delete this->soloperations[i];
	soloperations.clear();

	for (int i = 0; i<soljobs.size(); i++) delete this->soljobs[i];
	this->soljobs.clear();

	//Init operation list
	for (int i = 0; i < rhs.soloperations.size(); i++) {
		operation *op = new operation();
		*op = *rhs.soloperations[i];
		soloperations.push_back(op);
	}

	//Create jobs and assign predecessors/successors 
	for (int y = 0; y<rhs.soljobs.size(); y++) {
		job *jb = rhs.soljobs[y];
		job *jb_this = new job();
		*jb_this = *jb;

		for (int i = 0, i_end = jb->operations.size(); i<i_end; ++i)
		{
			operation *oporig = jb->operations[i];
			jb_this->operations.push_back(soloperations[oporig->UID]);
		}

		//Fixing Precedence and Succession stuff
		//Fist delete all preds
		for (int i = 0; i < jb_this->operations.size(); i++) {
			operation *op = jb_this->operations[i];
			//Delete Preds

			for (int j = 0; j < MAX_JOB_RELATIONS; j++) {
				op->pred[j] = NULL;
				op->suc[j] = NULL;
			}
		}

		//Now add the proper preds back
		for (int i = 0; i < jb_this->operations.size(); i++) {
			operation *op = jb_this->operations[i];
			operation *oporig =jb->operations[i];
			//Add proper preds
			for (int j = 0; j < MAX_JOB_RELATIONS; j++) {
				operation *predorig = oporig->pred[j];
				if (predorig == NULL) 
					break;
				//Assuming that preds are operations from the same job
				operation *newpred = soloperations[predorig->UID];
				op->pred[j] = newpred;
			}

			//Add proper sucs
			for (int j = 0; j < MAX_JOB_RELATIONS; j++) {
				operation *sucorig = oporig->suc[j];
				if (sucorig == NULL) 
					break;
				
				//Assuming that preds are operations from the same job
				operation *newsuc = soloperations[sucorig->UID];
				op->suc[j] = newsuc;
			}
		}

		soljobs.push_back(jb_this);
	}

	for (auto rc: Resources)
	    delete rc.second;
    Resources.clear();

	//Copy solution resources
	for (auto orig_entry : rhs.Resources){
	    resource *rc_orig = orig_entry.second;
	    resource *rc_new = new resource();
	    *rc_new = *rc_orig;
	    Resources[rc_new->name] = rc_new;
	}

	for (int i = 0; i<this->solmachines.size(); i++) delete this->solmachines[i];
	this->solmachines.clear();

	for (int i = 0; i<this->ProcSteps.size(); i++) delete this->ProcSteps[i];
	this->ProcSteps.clear();



	//Copy machines
    for (int i = 0; i < (int) rhs.solmachines.size(); i++) {
        //For each machine in the original psoring, create a copy
        //and save it in ps, finally add the ps to the procsteps
        machine *mcorig = rhs.solmachines[i];
        machine *mc = new machine();
        *mc = *mcorig;

        //Manually handle dummy ops
        delete mc->operations[0];
        delete mc->operations[1];
        mc->operations[0] = soloperations[mcorig->operations[0]->UID];
        mc->operations[1] = soloperations[mcorig->operations[mcorig->operations.size() - 1]->UID];

        //Now add them manually
        for (int k = 1; k < (int) mcorig->operations.size() - 1; k++) {
            operation *oporig = mcorig->operations[k];
            operation *op = soloperations[oporig->UID];
            op->mc = mc;

            //operation *pos = mc->operations[k];
            //cout << "Position Address: " << pos << " Pointer Address: " << (&pos) << endl;
            vInsertAt<operation*>(mc->operations, k, op);
        }

        this->solmachines.push_back(mc);
    }

    //Populate the process steps of the new solution

    for (int i = 0; i < rhs.ProcSteps.size(); i++) {
        process_step *psorig = rhs.ProcSteps[i];
        process_step *ps = new process_step();

        //Setup proper parameters for the new ps
        ps->ID = psorig->ID;
        ps->actualID = psorig->actualID;
        ps->processID = psorig->processID;


        //Copy the machines
        for (int j=0; j < (int) psorig->machines.size(); j++)
            ps->machines.push_back(solmachines[psorig->machines[j]->ID]);

        this->ProcSteps.push_back(ps);
	}
	
	//Delete existing Graphs
	delete FGraph;
	delete BGraph;
	//Reinit Graphs
	FGraph = new Graph<operation>((int)rhs.totalNumOp + 2 * rhs.solmachines.size());
	BGraph = new Graph<operation>((int)rhs.totalNumOp + 2 * rhs.solmachines.size());
	//copy_graph(this->DGraph, rhs.DGraph);

	//Set Function Pointers
	//FGraph->fptr = &operation::setMoving;
	//BGraph->fptr = &operation::setMoving;

	//Copy Forward Graph
	//Copy Graph Manually
	FGraph->size = rhs.FGraph->size;
	FGraph->state = rhs.FGraph->state;

	for (int g = 0; g < 2; g++) {
		Graph<operation> *gr, *or_g;
		
		switch (g)
		{
		case 0:
			gr = FGraph;
			or_g = rhs.FGraph;
			break;
		case 1:
			gr = BGraph;
			or_g = rhs.BGraph;
			break;
		}

		gr->size = or_g->size;
		gr->state = or_g->state;

		for (int i = 0; i < gr->size; i++) {
			adj_list_elem<operation> orig_struct = or_g->adj_list[i];
			adj_list_elem<operation> new_struct = gr->adj_list[i];

			//Load Op
			new_struct.op = soloperations[orig_struct.op->UID];

			//Load t_connection
			new_struct.t_connection = orig_struct.t_connection;

			//Load p_connections
			for (int j = 0; j < MAX_JOB_RELATIONS; j++) {
				new_struct.p_connections[j] = orig_struct.p_connections[j];
			}

			gr->adj_list[i] = new_struct;
		}

        //printf("Top_Sort size %d %d\n", gr->top_sort.size(), or_g->top_sort.size());
        //Copy t_sort
        for (int i=0;i<or_g->top_sort.size();i++)
            gr->top_sort.push_back(or_g->top_sort[i]);

        for (int i=0; i<gr->size; i++)
            gr->top_sort_index[i] = or_g->top_sort_index[i];

	}

	//Copy Critical Path of FGraph
	FGraph->criticalPath.clear();
	for (int i=0;i<rhs.FGraph->criticalPath.size();i++)
		FGraph->criticalPath.push_back(soloperations[rhs.FGraph->criticalPath[i]->UID]);

	//Copy Critical Path of BGraph (Should be empty)
	BGraph->criticalPath.clear();
	for (int i=0;i<rhs.BGraph->criticalPath.size();i++)
		BGraph->criticalPath.push_back(soloperations[rhs.BGraph->criticalPath[i]->UID]);

	//Copy Critical Blocks
	criticalBlocks.clear();
	for (int i=0;i<rhs.criticalBlocks.size();i++)
		criticalBlocks.push_back(int3_tuple(rhs.criticalBlocks[i]));


#if(PFSS)
	this->ranking = rhs.ranking;
	this->dominated = rhs.dominated;
	this->distance = rhs.distance;

	for (int j = 0; j<NumberOfJobs; j++) {
		this->int_permutation[j] = rhs.int_permutation[j];
		this->double_permutation[j] = rhs.double_permutation[j];
		this->intdouble_convertion[j] = rhs.intdouble_convertion[j];
	}
	for (int m = 0; m<NumberOfMachines; m++) {
		for (int j = 0; j<NumberOfJobs; j++) {
			this->completion_time[m][j] = rhs.completion_time[m][j];
		}
	}
#endif

	return *this;

}

void copy_graph(Graph<operation> *out, Graph<operation> *in) {
	
}

bool solution::operator==(const solution &rhs) {
	//printf("Solution equality operator triggered\n");
	//Checking solutions for equality
	//Step 1 check number of jobs
	if (this->soljobs.size() != rhs.soljobs.size()) return false;
	//Step 2 check number of machines
	if (this->solmachines.size() != rhs.solmachines.size()) return false;
	//Step 3 check number of operations
	if (this->totalNumOp != rhs.totalNumOp) return false;

	//Step 4 Check costs
	for (int i=0; i<COST_NUM - 1; i++) {
		if (this->cost_array[i] != rhs.cost_array[i]) return false;
		//if (this->temp_cost_array[i] != rhs.temp_cost_array[i]) return false;
		//if (this->d_cost_array[i] != rhs.d_cost_array[i]) return false;
		//if (this->temp_d_cost_array[i] != rhs.temp_d_cost_array[i]) return false;
	}
	
	//Step 5 check jobs
	for (int i=0; i<this->soljobs.size(); i++) {
		job *jb = this->soljobs[i];
		job *rh_jb = rhs.soljobs[i];
		if (*jb != *rh_jb) return false;
	}

	//Step 6 check machines
	for (int i=0; i<this->solmachines.size(); i++){
		machine *mc = this->solmachines[i];
		machine *rh_mc = rhs.solmachines[i];
		if (*mc != *rh_mc) return false;
	}


	return true;
}

ostream& operator<<(ostream &output, const solution &ss) {
	// output << "Cmax:  " << ss.cost << endl;
	// output << "TFT:   " << ss.s_cost << endl;
	// output << "Tidle: " << ss.t_cost << endl;
	// output << "Tardiness: " << ss.tard_cost << endl;
	// output << "SlackTime: " << ss.slack << endl;
	// output << "Energy: " << ss.e_cost << endl;
	output << "Cmax:  " << ss.cost_array[MAKESPAN] << endl;
	output << "TFT:   " << ss.cost_array[TOTAL_FLOW_TIME] << endl;
	output << "Tidle: " << ss.cost_array[TOTAL_IDLE_TIME] << endl;
	output << "Tardiness: " << ss.cost_array[TARDINESS] << endl;
	output << "SlackTime: " << ss.cost_array[SLACK] << endl;
	output << "Energy: " << ss.cost_array[TOTAL_ENERGY] << endl;
	char s[200]; //Output placeholder

#if(JSS)
	output << "-Jobs-" << endl;
	for (int g = 0; g<(int) ss.soljobs.size(); g++)
	{
		job *jb = ss.soljobs[g];
		int sum = 0;
		for (int h = 1; h<(int) jb->operations.size(); h++)
		{
			operation *op1 = jb->operations[h - 1];
			operation *op2 = jb->operations[h];
			sum += op2->start - op1->end;
		}
		sprintf(s, "Job: %10d ID: %5d OrderID: %d Quantity %d Op.No %5d Idle Time %5d \n", jb->actualID, jb->ID, jb->orderID, jb->quantity, (int) jb->operations.size(), sum);
		output << s;
	}
	output << "-Machines-" << endl;
	for (int y = 0; y < (int) ss.solmachines.size(); y++){
	    if (ss.solmachines[y]->operations.size() > 2)
            output << *ss.solmachines[y];
	}

#endif

#if(PFSS)
	cout << "Permutation: " << endl;
	for (int j = 0; j<NumberOfJobs; j++) {
		cout << ss.int_permutation[j] << " ";
	}
	cout << endl;
	cout << "Completion times" << endl;;
	for (int m = 0; m<NumberOfMachines; m++) {
		cout << "M#" << m << ": ";
		for (int j = 0; j<NumberOfJobs; j++) {
			cout << ss.completion_time[m][j] << " ";
		}
		cout << endl;
	}
#endif
	return output;
}


void solution::memTestSolution() {

	cout << "Iterating in Job Operations" << endl;
	for (int i = 0; i < (int) soljobs.size(); i++) {
		job *jb = soljobs[i];
		for (int j = 0; j < (int) jb->operations.size(); j++) {
			operation *op = jb->operations[j];
			cout << "Op Job: " << op->jobID << " Op ID " << op->ID << " Mem Address " << op << endl;
		}
		
	}
	
	for (int i = 0; i < (int) this->solmachines.size(); i++) {
		machine *mc = this->solmachines[i];
		cout << "Machine Address " << mc << endl;
		mc->memTest();
	}

}

void solution::cleanUpSolution() {
	
	//Initialize Jobhelp structure
	opshelp.clear();
	fixed_ops.clear();


	//Find and store all fixed operations
    for (int i=0;i<(int) solmachines.size();i++){
        machine *mc = solmachines[i];
        for (int j =1;j<mc->operations.size()-1;j++){
            operation *op =mc->operations[j];
            if (op->fixed)
                fixed_ops.push_back(op);
        }
    }

    //Cleanup current solution
	for (int g = 0; g<(int) soljobs.size(); g++)
	{
		job *jb = soljobs[g];
		for (int y = 0; y<(int) jb->operations.size(); y++)
		{
			operation *op = jb->operations[y];

			//Set Job release/due dates in the operation
			//op->a = (int) dataset->orderIDMap.at(jb->orderID)->releaseTime;
			op->a = jb->rel;
			//op->b = (int) dataset->orderIDMap.at(jb->orderID)->dueTime; //Values are wrong
			op->b = planhorend;

			if (op->fixed) continue; //Do not reset op if its fixed //Rescheduling

			op->setDoneStatus(false, false);
            op->start = op->a;
			//op->end = op->start + op->processing_time + op->cleanup_time + op->setup_time;
			op->end = op->start + op->cleanup_time + op->setup_time;
			op->temp_start = op->start;
			op->temp_end = op->end;
			
			//reset machine assignments
			op->mc = NULL;
			op->mthesi = -1;
			op->temp_mthesi = -1;
			op->machID = -1;
			op->temp_machID = -1;

			//test.push_back(op);
			opshelp.push_back(op);


        }
    }

    for (int y = 0; y<(int) this->solmachines.size(); y++)
    {
        machine *mc = this->solmachines[y];
        while(mc->operations.size()>2) {
            mc->operations.erase(mc->operations.begin() + 1);
        }
    }

	//Initiallize data structures for all resources
	Init_Resourse_Loads();

	//Remove All machine connections from the Graph
	
}

void solution::GetCounts() {
	totalNumOp = 0;

    //Count Operations
    for (int i = 0; i < (int) soljobs.size(); i++) {
        job *jb = soljobs[i];
        totalNumOp += (int) jb->operations.size();
    }
}

bool solution::fixTimes() {
	//Fix Times for every machine in solmachines
	int counter = 0;
	int lim = 50;
	while (counter < lim) {
		//While all machines have reported fixed times keep iterating
		bool stat = true;
		for (int i = 0; i < (int) this->solmachines.size(); i++) {
			machine  *mc = this->solmachines[i];
			bool temp_stat = mc->fixTimes();
			stat = stat && temp_stat;
		}

		//If all have reported true finalize
		if (stat) break;
		counter += 1;

	}
	if (counter == lim) {
		return false;
	}
	//cout << "Fix Times Iterations: " << counter << endl;
	return true;
}

int solution::getRemainingOps() {
	int jobs = 0;
	for (int i = 0; i < (int) this->solmachines.size(); i++) {
		machine *mc = this->solmachines[i];
		for (int j = 1; j < (int) mc->operations.size()-1; j++)
			jobs += 1;
	}
	return this->totalNumOp - jobs;
}

int solution::getDoneOps() {
	int jobs = 0;
	for (int i = 0; i < (int) soljobs.size(); i++) {
		job *jb = soljobs[i];
		for (int j = 0; j < (int) jb->operations.size(); j++) {
			operation *op = jb->operations[j];
			if (op->done) jobs += 1;
		}
	}
	return jobs;
}


void solution::setDoneStatus(bool temp, bool status) {
	//Fix Done Status for every machine in solmachines
	for (int i = 0; i < (int) this->solmachines.size(); i++) {
		machine  *mc = this->solmachines[i];
		mc->setDoneStatus(temp, status);
	}
}

void solution::resetInheritanceCheck() {
	//Reset Inheritance check for all scheduled operations
	//in machines
	for (int i = 0; i < (int) this->solmachines.size(); i++) {
		machine  *mc = this->solmachines[i];
		for (int j = 1; j < (int) mc->operations.size()-1; j++) {
			operation * op = mc->operations[j];
			op->inh_ok = NULL; //Reset status completely
		}
	}
}


void solution::getCost(bool  temp) {
	for (int i=0;i<COST_NUM;i++) getCost(i, temp);
	return;
}

void solution::getCost(int costID, bool temp) {
	float val = INF;
	int temp_slot = (int)temp;
	switch(costID){
		case MAKESPAN:
			if (MAKESPAN_F)
				val = (float) calculate_makespan(0, temp);
			break;
		case TOTAL_FLOW_TIME:
			if (TOTAL_FLOW_TIME_F)
				val = (float)calculate_TFT(0, temp);
			break;
		case TOTAL_IDLE_TIME:
			if (TOTAL_IDLE_TIME_F)
				val = (float) calculate_Tidle(0, temp);
			break;
		case TOTAL_ENERGY:
			if (TOTAL_ENERGY_F)
				val = (float) calculate_Tenergy(0, temp);
			break;
		case TARDINESS:
			if (TARDINESS_F)
				val = (float) calculate_Tardiness(0, temp);
			break;
		case SLACK:
			if (SLACK_F)
				val = (float) calculate_SlackTime(0, temp);
			break;
		/*For now the distance cost is directly pointing to the common slot distance
		//In this special case, no second sulutions is needed and therefore no more parameters
		are required in this getCost method. In the future, if we ever need to compare with reference
		solutions using this fuction, we'll have to reimplement this with additional parameters
		*/
		case DISTANCE:
			if (DISTANCE_F)
				val = solution::calcDistance(this, NULL, DISTANCE_F, temp);
			break;
	}

	switch (temp) {
	case true:
		temp_d_cost_array[costID] = val - temp_cost_array[costID];
		temp_cost_array[costID] = val;
		break;
	case false:
		d_cost_array[costID] = val - cost_array[costID];
		cost_array[costID] = val;
	}
	return;
}

int solution::calculate_makespan(int scenario, bool temp) {
	long cmax = 0;
    long start = 0;
	for (int g = 0; g < (int) this->solmachines.size(); g++)
	{
		machine *mc = this->solmachines[g];
		if ((int)mc->operations.size() == 2) continue;
		operation *op_init = mc->operations[1];
		operation *op = mc->operations[(int)mc->operations.size() - 2];
		if (temp) {
			cmax = max(cmax, op->temp_end);
			start = max(start, op_init->temp_start);
		}
		else {
            cmax = max(cmax, op->end);
            start = max(start, op_init->start);
        }
	}
	int output = 0;
	if (scenario == 0) output = cmax;
	if (scenario == 1) output = cmax - start;

	return output;
}


int solution::calculate_localSlack(int temp) {
	/*
		This function at first does a backward pass
		and calculates the late start and finish times
		of each operation. Then the local Slack is calculated
		as the abs diff of the early start - late start of each op
		The total sum of those diffs for all operations is returned
	*/
	int lSlack = 0;
	Graph<operation> *gr = FGraph;
	vector<operation*> last_ops;

	//cout << *this;

	//TODO: REWRITE THE FUNCTION - STUFF MISSING

	//Accumulate slacks
	for (int i = 0; i < soljobs.size(); i++) {
		job *jb = soljobs[i];
		for (int j = 0; j < jb->operations.size(); j++) {
			operation *op = jb->operations[j];
			int diff;
			if (temp)
				diff = op->tail - op->temp_end;
			else
				diff = op->tail - op->end;
			
			//Just count the critical operations
			if (diff == 0) lSlack++;
			//Sum all the slacks
			//lSlack += diff;
		}
	}

	return lSlack;
}

int solution::calculate_Tenergy(int scenario, bool temp)
{
	int e_proc = 0;
	int e_idle = 0;
	int e_indirect = 0;
#if(RESOURCE_CONSTRAINTS)
	if (scenario == 0)//Total proc + idle energy consumption (considering machine unavailabilities)
	{
		for (int y = 0; y < (int) solmachines.size(); y++)
		{
			machine *mc = solmachines[y];
			for (int k = 1; k < (int) mc->operations.size() - 1; k++)
			{
				operation *op = mc->operations[k];
				if (temp) {
					for (int r = 0; r < NumberOfResources; r++) {
						e_proc += int(op->processing_time_UM[mc->ID] * mc->res_consum_processing[r]);
						//e_proc += (get<1>(op->end_times) - op->temp_start)*mc->res_consum_processing[r];
					}
				}
				else {
					for (int r = 0; r < NumberOfResources; r++) {
						e_proc += int(op->processing_time_UM[mc->ID] * mc->res_consum_processing[r]);
						//e_proc += (op->end - op->start)*mc->res_consum_processing[r];
					}
				}
			}
		}
		for (int y = 0; y<(int) solmachines.size(); y++)
		{
			machine *mc = solmachines[y];
			bool mpike = false;
			int startt = 0;
			int endt = 0;
			for (int k = 2; k<(int) mc->operations.size() - 1; k++)
			{
				mpike = true;
				operation *op1 = mc->operations[k - 1];
				operation *op2 = mc->operations[k];
				if (temp) {
					for (int r = 0; r < NumberOfResources; r++) {
						e_idle += int((op2->temp_start - op1->temp_end) * mc->res_consum_processing[r] / 2);
					}
					if (k == 2)                           startt = get<0>(op1->start_times)_temp;
					if (k == (int) mc->operations.size() - 2) endt = op2->temp_end;
				}
				else {
					for (int r = 0; r < NumberOfResources; r++) {
						e_idle += int((op2->start - op1->end) * mc->res_consum_processing[r] / 2);
					}
					if (k == 2)                           startt = get<0>(op1->start_times);
					if (k == (int) mc->operations.size() - 2) endt = op2->end;
				}
			}
			if (mpike) {

#if(MACHINE_AVAILABILITIES)
				for (int u = 0; u<mc->(int) unavailability.size(); u++)
				{
					mach_unav *muv = mc->unavailability[u];
					if (muv->start_unav >= startt && muv->end_unav <= endt) {
						for (int r = 0; r < NumberOfResources; r++) {
							e_idle -= (muv->end_unav - muv->start_unav)*mc->res_consum_processing[r];
						}
						continue;
					}
					if (muv->start_unav > endt) break;
				}
#endif
			}
		}
	}
#endif
	if (temp){
		e_indirect = temp_cost_array[MAKESPAN] * 3;	
	} else{
		e_indirect = cost_array[MAKESPAN] * 3;	
	}
	

	int val = (e_idle + e_proc + e_indirect) / 1000;
	return val;
}

int solution::calculate_Tidle(int scenario, bool temp)
{
	int tidle = 0;
	if (scenario == 0) //Total non productive time (considering machine unavailabilities) + weights
	{
		for (int y = 0; y<(int) this->solmachines.size(); y++)
		{
			machine *mc = this->solmachines[y];
			bool mpike = false;
			int startt = 0;
			int endt = 0;
			
			for (int k = 2; k<(int) mc->operations.size() - 1; k++)
			{
				/* REDUNDANT
				if (temp) {
					startt = mc->operations[2]->temp_start;
					endt = mc->operations[(int) mc->operations.size() - 2]->temp_end;
				}
				else {
					startt = mc->operations[2]->start;
					endt = mc->operations[(int) mc->operations.size() - 2]->end;
				}
				*/

				mpike = true;
				operation *op1 = mc->operations[k - 1];
				operation *op2 = mc->operations[k];
				if (temp)
					tidle += op2->temp_start - op1->temp_end;
				else
					tidle += op2->start - op1->end;
			}

            if (mpike) {
                for (int u = 0; u<(int) mc->unavailability.size(); u++)
                {
                    mach_unav *muv = mc->unavailability[u];
                    if (muv->start_unav >= startt && muv->end_unav <= endt) {
                        tidle -= mc->weight*(muv->end_unav - muv->start_unav);
                        continue;
                    }
                    if (muv->start_unav > endt) break;
                }
            }


		}
	}
	if (scenario == 1)//Total non productive time (considering machine unavailabilities)
	{
		for (int y = 0; y<(int) this->solmachines.size(); y++)
		{
			machine *mc = this->solmachines[y];
			bool mpike = false;
			/* REDUNDANT
			
			int startt = 0;
			int endt = 0;
			
			if (temp) {
				startt = mc->operations[2]->temp_start;	
				endt = mc->operations[mc->operations.size() - 2]->temp_end;
			} 
			else{
				startt = mc->operations[2]->start;
				endt = mc->operations[mc->operations.size() - 2]->end;
			} 
			
			*/


			for (int k = 2; k<(int) mc->operations.size() - 1; k++)
			{
				mpike = true;
				operation *op1 = mc->operations[k - 1];
				operation *op2 = mc->operations[k];
				if (temp) tidle += op2->temp_start - op1->temp_end;
				else tidle += op2->start - op1->end;
			}

			if (mpike) {

		#if(MACHINE_AVAILABILITIES)
			for (int u = 0; u<(int) mc->unavailability.size(); u++)
			{
				mach_unav *muv = mc->unavailability[u];
				if (muv->start_unav >= startt && muv->end_unav <= endt) {
					tidle -= (muv->end_unav - muv->start_unav);
					continue;
				}
				if (muv->start_unav > endt) break;
			}
		#endif
			}
		}
	}

	if (scenario == 2)//Measure idle time between the operations of each production order
	{
		for (int g = 0; g>(int) soljobs.size(); g++)
		{
			job *jb = soljobs[g];
			if ((int) jb->operations.size() > 1)
			{
				for (int h = 1; h<(int) jb->operations.size(); h++)
				{
					operation *op1 = jb->operations[h - 1];
					operation *op2 = jb->operations[h];
					if (temp) {
						tidle += op2->temp_start - op1->temp_end;
					}
					else {
						tidle += op2->start - op1->end;
					}
				}
			}
		}
	}
	
	return tidle;
}

int solution::calculate_TFT(int scenario, bool temp)
{
	//Flow time is calculated as the difference of the end completion time from a job
	//from its release time

	int TFT = 0;
	for (int g = 0; g<(int) this->soljobs.size(); g++)
	{
		job *jb = this->soljobs[g];
        if (jb->operations.size() == 0)
            continue;

        operation *op = jb->operations[(int) jb->operations.size() - 1];
		if (temp) {
			if (scenario == 0) TFT += op->temp_end;
			else if (scenario == 1) TFT += op->temp_end - op->a;
		}
		else {
			if (scenario == 0) TFT += op->end;
			else if (scenario == 1) TFT += op->end - op->a;
		}
	}
	
	return TFT;
}

int solution::calculate_Tardiness(int scenario, bool temp)
{
	//Tardiness is calculated by the difference of the end time of an operation 
	//from its due date
	int tard = 0;
#if(REL_DUE_DATES_DATASET || REL_DUE_DATES_CUSTOM || HARD_DUE_DATES	)
		
	for (int g = 0; g<(int) this->soljobs.size(); g++)
	{
		job *jb = this->soljobs[g];

		for (int i =0;i<(int) jb->operations.size();i++){
			operation *op = jb->operations[i];
			
			if (temp) {
				tard += op->weight * max(0, get<1>(op->end_times) - op->b);
			}
			else {
				tard += op->weight * max(0, op->end - op->b);
			}
		}
	}

#endif
	return tard;
}

int solution::calculate_SlackTime(int scenario, bool temp)
{
	//Needs Implementation
	int slack = 0;
	
	return slack;
}

void solution::replace_temp_values()//OK
{
	for (int g = 0; g<(int) soloperations.size(); g++)
	{
		operation *op = soloperations[g];
		op->temp_start = op->start;
		op->temp_end = op->end;
		op->temp_machID = op->machID;
		op->temp_mthesi = op->mthesi;
		op->temp_done = op->done;
	}

	//Replace Costs
	for (int i = 0; i < COST_NUM; i++)
	{
		this->temp_cost_array[i] = this->cost_array[i];
		this->temp_d_cost_array[i] = this->d_cost_array[i];
	}
	
	return;
}

void solution::replace_real_values()//OK
{
	for (int g = 0; g<(int) this->soljobs.size(); g++)
	{
		job *jb = this->soljobs[g];
		for (int h = 0; h<(int) jb->operations.size(); h++)
		{
			operation *op = jb->operations[h];
			op->start = op->temp_start;
			op->end = op->temp_end;
			op->machID = op->temp_machID;
			op->mthesi = op->temp_mthesi;
			op->done = op->temp_done;
		}
	}

	//Replace Costs
	for (int i =0;i <COST_NUM;i++)
	{
		this->cost_array[i] = this->temp_cost_array[i];
		this->d_cost_array[i] = this->temp_d_cost_array[i];
	}

	return;
}

#if 0
bool solution::scheduleAEAP(bool temp)
{
	/* NEW SAEAP - WORK ONLY ONCE ON OPERATIONS AND KEEP TRACK OF THE PROGRESS SO FAR

	--DESCRIPTION--
	scheduleAEAP routine resets the current solution, and actually recalculates all the operation start & end times.
	It seems that it doesn't change the position of the operations, it does reschedule them to the positions that were
	assigned during the construction phase.

	Routine runs until all operations have been assigned to a machine (check is done using an opcounter)
	*/

	//Init temp var
	for (int i = 0; i < MAX_MACHINES; i++)
		sAEAP_mach_positions[i] = 1;

	//Initiallize start and end times for all operations

	int actualopcounter = 0;
	int opcounter = 0;
    int back_opcounter = 0;
	int skipped_ops = 0;

	int machs_n = this->solmachines.size();
	int jobs_n = this->soljobs.size();

	for (int g = 0; g < machs_n; g++)
	{
		machine *mc = this->solmachines[g];
		int mc_n = (int)mc->operations.size();
		for (int h = 1; h < mc_n - 1; h++)
		{
			operation *op = mc->operations[h];
			//cout << "Schedule AEAP cleanup: OP ID: " << op->ID << " Job ID " << op->jobID << " Address " << op << endl;

			if (temp) {
				op->temp_done = false;
				op->temp_start = op->a;
			}
			else {
				op->done = false;
				op->start = op->a;
			}
		}
	}
	//printf("Skipped %d Operations \n", skipped_ops);


	/*
	Since the scheduleaeap calls are followed almost always by testing routines
	I'm replacing the actualopcounter to match the current scheduled operations number
	so that I can call this one at all times. I'm also removing any counter checks from here
	*/

	actualopcounter = 0;
	for (int g = 0; g < machs_n; g++) {
		machine *mc = solmachines[g];
		int mc_n = (int) mc->operations.size();
		actualopcounter += mc_n - 2;
	}

	//Initiallize data structures for all resources
	//Init_Resourse_Loads_mecutoff(0);
	Init_Resourse_Loads();
	//	print_sol();
	//Schedule everything as early as possible
	int iter = 0;
	int iter_lim = 4;
	while (true)
	{
		int minim = INF;
		int start = 0;
		int end = 0;

		int start_t = 0;
		int end_t = 0;

        back_opcounter = opcounter;
		for (int g = 0; g < machs_n; g++)
		{
			machine *mc = this->solmachines[g];
			operation *op0 = mc->operations[0];
            process_step *ps = ProcSteps[mc->process_stepID];

			if (temp) {
				op0->temp_start = 0;
				op0->temp_end = 0;
			}
			else {
				op0->start = 0;
				op0->end = 0;
			}

			int mc_n = (int)mc->operations.size();
			for (int u = sAEAP_mach_positions[g]; u < mc_n - 1; u++)
			{
				operation *opi = mc->operations[u - 1];
				operation *opj = mc->operations[u];

				//cout << "Schedule AEAP checks: OP ID: " << opj->ID << " Job ID " << opj->jobID << " Address " << opj << endl;
				if (temp) {
					if (opj->temp_done) continue;
					start = max(opi->temp_end, opj->a);
				}
				else {
					if (opj->done) continue;
					start = max(opi->end, opj->a);
				}


				//Greg: I think this check has to be done after the pred_end time has been accumulated as well
				//if (index != -1 && start > minim) break;

				if (opj->has_preds())
				{
					operation *opj_pred = opj->pred[0];
					if (temp) {
						if (!opj_pred->temp_done) break;
						start = max(start, opj_pred->temp_end);
					}
					else {
						if (!opj_pred->done) break;
						start = max(start, opj_pred->end);
					}


				}

				//if (index != -1 && start > minim) break;

				//Calculate new temp_start and end_tmep
				int mc_setup = opj->setup_time;
#if(CHANGEOVERTIMES)
				mc_setup += changeovertime(mc->ID, opi->opertypeID, opj->opertypeID);
#endif

				int mc_cleanup = opj->cleanup_time;

#if(FLEX)
				if (temp)
					end = start + mc_setup + opj->processing_time_UM[opj->temp_machID] + mc_cleanup;
				else
					end = start + mc_setup + opj->processing_time_UM[opj->machID] + mc_cleanup;
#else
				if (temp) 	end = start + mc_setup + opj->processing_time + mc_cleanup;
				else		end = start + mc_setup + opj->processing_time + mc_cleanup;
#endif

				while (true) {

					start_t = start;
					end_t = end;

#if(MACHINE_AVAILABILITIES)
					//Check if machine is available and update start and end temp times
					for (int c = 0; c<mc->(int)unavailability.size(); c++)
					{
						mach_unav *muv = mc->unavailability[c];
						if (muv->end_unav < start) continue;
						if ((end > muv->start_unav && end <= muv->end_unav) || (start >= muv->start_unav && start <= muv->end_unav) || (muv->start_unav >= start && muv->end_unav <= end))
						{
							start = muv->end_unav;

							//end   = start + mc_setup + opj->processing_time + mc_cleanup;

							if (temp) end = start + mc_setup + opj->processing_time_UM[opj->temp_machID] + mc_cleanup;
							else
								end = start + mc_setup + opj->processing_time_UM[opj->machID] + mc_cleanup;
							continue;
						}
						if (end <= muv->start_unav) break;
					}
#endif

					//if (index != -1 && start > minim) break;

#if(RESOURCE_CONSTRAINTS)
#if(FLEX)
					int duration = mc_setup + opj->processing_time_UM[mc->ID] + mc_cleanup;
#else
					int duration = mc_setup + opj->processing_time + mc_cleanup;
#endif
					int max_start = start;
					int t = 0;

					//NEW BLOCKS WAY
					//cout << " Trying to fit operation " << opj->jobID << " " << opj->ID << endl;
					for (int r = 0; r < (int)Resources.size(); ++r) {
						resource *rc = Resources[r];
						max_start = max(max_start, rc->find_block(start, end, mc->res_consum_processing[r]));

						//if (max_start > start) {
						//	cout << "FInd Block Did Something Naughty :O " << endl;
						//}
					}

					//OLD WAY
					//while (t >= 0)
					//{
					//	int y = 0;
					//	for (y = 0; y < duration; y++)
					//	{
					//		//Check all resources
					//		int r = 0;
					//		for (r = 0; r < Resources.size(); ++r)
					//		{	
					//			
					//			resource *rc = Resources[r];
					//			if (mc->res_consum_processing[r] == 0) continue;
					//			if ((rc->res_load_t[max_start + y + t] + mc->res_consum_processing[r]) > rc->max_level) break;
					//			if ((ps->proc_res_load_t[r][max_start + y + t] + mc->res_consum_processing[r]) > ps->resalarm[r]) break;
					//			
					//		}

					//		if (r != Resources.size()) break;
					//	}
					//	if (y == duration) break;
					//	else t += y + 1;

					//	if (index != -1 && (max_start + t) > minim) break;
					//}
					//Update start and end times
					start = max_start + t;

					if (index != -1 && start > minim) break;

#if(FLEX)
					if (temp) end = start + mc_setup + opj->processing_time_UM[opj->temp_machID] + mc_cleanup;
					else      end = start + mc_setup + opj->processing_time_UM[opj->machID] + mc_cleanup;
#else
					if (temp) end = start + mc_setup + opj->processing_time + mc_cleanup;
					else      end = start + mc_setup + opj->processing_time + mc_cleanup;
#endif

#endif

					if (!MACHINE_AVAILABILITIES) break;
					if (!RESOURCE_CONSTRAINTS) break;

					if (start_t == start && end_t == end) break;

					//if (index != -1 && start > minim) break;
				}

				//Schedule the Op Right away
				//printf("scheduleAEAP | Operation: %d %d Scheduled - OpCounter %d \n", op->jobID, op->ID, opcounter);
				switch (temp) {
				case 0:
					opj->done = true;
					opj->machID = mc->ID;
					opj->mthesi = u;
					opj->end = end;
					opj->start = start;
					break;
				case 1:
					opj->temp_done = true;
					opj->temp_machID = mc->ID;
					opj->temp_mthesi = u;
					opj->temp_end = end;
					opj->temp_start = start;
					break;
				}

				opcounter++;
				opj->setDoneStatus(temp, true);
				sAEAP_mach_positions[g]++;

				//Update resource loads

				//Global Update
				//Update_Resourse_Loads(temp);
				//Per Op Update
				Update_Resourse_Loads_perOp(opj, temp);
			}
		}

        if (back_opcounter == opcounter) iter++;
        else iter = 0;

        if (iter > iter_lim){
            //Eite malakia eite infeasible solution
			//printf("Kapoia malakia exei paixtei me ta moving/done status\n");
            //this->FGraph->report_topological_sort();
            //cout << *this;
            return false;
        }

        if (opcounter == actualopcounter) break;

    }
	if (temp) {
		//cout << "SCHEDULEAEAP FINISHED SOLUTION" << endl;
		//cout << *this;
		//cout << "MemTest after scheduleAEAP" << endl;
		//this->memTestSolution();
		return true;
	}
	else
	{
		if (!find_conflicts(temp)) return false;
		else return true;
	}
}
#endif


//DISRUPT CHANGEOVER TIME
int solution::changeovertime(machine *mc, operation *op1, operation *op2){


	//Setup times are imposed only on the SMT & THT lines for now.
	//Judging from the data. Each changeover costs 15 minutes, so this is the cost that we should imply
	//if there is a productID mismatch
	if (mc->line->type->productionLineTypeID <= 2) {
        if (op1->jobID == -1){
			//In the case of the first operation on the machine, we should compare against the initial setup of the
			//line/machine. For now I am directly imposing the 15 minutes of setup because we have no such info.
			return 900;
		}

		if (dataset->orderIDMap.at(soljobs[op1->jobID]->orderID)->productID != dataset->orderIDMap.at(soljobs[op2->jobID]->orderID)->productID)
			return 900;
	}

	return 0;
}


bool solution::scheduleAEAP(bool temp, bool override_status)
{
	/* NEW SAEAP - WORK ONLY ONCE ON OPERATIONS AND KEEP TRACK OF THE PROGRESS SO FAR

	--DESCRIPTION--
	scheduleAEAP routine resets the current solution, and actually recalculates all the operation start & end times.
	It seems that it doesn't change the position of the operations, it does reschedule them to the positions that were
	assigned during the construction phase.

	Routine runs until all operations have been assigned to a machine (check is done using an opcounter)
	*/

	//Init temp var
	for (int i = 0; i < MAX_MACHINES; i++)
		sAEAP_mach_positions[i] = 1;

	//Initiallize start and end times for all operations

	int actualopcounter = 0;
	int opcounter = 0;
	int back_opcounter = 0;
	int skipped_ops = 0;

	int machs_n = (int) this->solmachines.size();
	int jobs_n = (int) this->soljobs.size();


	/*
	if (override_status) {
		for (int i=0;i<soloperations.size();i++)
			soloperations[i]->moving = false;
		for (int i=0; i<machs_n; i++){
			for (int j=1; j<solmachines[i]->operations.size()-1; j++){
				operation *op = solmachines[i]->operations[j];
				op->moving = true;
			}
		}
	}
	*/
	//printf("Skipped %d Operations \n", skipped_ops);

	/*
	Since the scheduleaeap calls are followed almost always by testing routines
	I'm replacing the actualopcounter to match the current scheduled operations number
	so that I can call this one at all times. I'm also removing any counter checks from here
	*/
	/*
	actualopcounter = 0;
	//printf("Moving Ops: ");
	for (int i=0;i<soloperations.size();i++){
		operation *op = soloperations[i];
		if (op->moving && op->valid) {
			//printf("%d ", op->UID);
			actualopcounter++;
			if (temp) op->temp_done = false;
			else op->done = false;
		}
	}
	//printf("\n");
	*/


	//NO MOVING OPS PREDICTION CODE
	actualopcounter = 0;
	for (int i=0;i<solmachines.size();i++){
        for (int j=1;j<solmachines[i]->operations.size()-1;j++){
            operation *op = solmachines[i]->operations[j];
            if (op->valid && op->fixed)
                op->setDoneStatus(temp, true);
            if (op->valid && !op->fixed){
				op->setDoneStatus(temp, false);
                actualopcounter++;
            }
        }
    }

	//Initiallize data structures for all resources
	//Init_Resourse_Loads_mecutoff(0);
	Init_Resourse_Loads();
	//	print_sol();
	//Schedule everything as early as possible
	int iter = 0;
	int iter_lim = 4;
	while (true)
	{
		int minim = INF;
		long start = 0;
		long end = 0;

        long start_t = 0;
        long end_t = 0;

		back_opcounter = opcounter;

		for (int g = 0; g < machs_n; g++)
		{
			machine *mc = this->solmachines[g];
			operation *op0 = mc->operations[0];
            process_step *ps = ProcSteps[mc->process_stepID];

			if (temp) {
				op0->temp_start = 0;
				op0->temp_end = 0;
			}
			else {
				op0->start = 0;
				op0->end = 0;
			}
			
			int mc_n = (int) mc->operations.size();
			for (int u = sAEAP_mach_positions[g]; u < mc_n - 1; u++)
			{
				operation *opi = mc->operations[u - 1];
				operation *opj = mc->operations[u];


				if (temp){
					if (opj->temp_done || opj->fixed) {
						sAEAP_mach_positions[g]++;
						continue;
					}
				}
				else {
					if (opj->done || opj->fixed) {
						sAEAP_mach_positions[g]++;
						continue;
					}
				}

				//cout << "Schedule AEAP checks: OP ID: " << opj->ID << " Job ID " << opj->jobID << " Address " << opj << endl;

				if (temp)
					start = max(opi->temp_end, opj->a);
				else
					start = max(opi->end, opj->a);

				//Greg: I think this check has to be done after the pred_end time has been accumulated as well
				//if (index != -1 && start > minim) break;

				if (opj->has_preds())
				{
#if(COMPLEX_PRECEDENCE_CONSTRAINTS)
                    bool break_flag = false;
                    for (int kk=0; kk<MAX_JOB_RELATIONS; kk++){
                        operation *opj_pred = opj->pred[kk];
                        if (opj_pred == nullptr) continue;
                        if (temp) {
                            if (!opj_pred->temp_done){
                                break_flag = true;
                                break;
                            }
                            start = max(start, opj_pred->temp_end);
                        }
                        else {
                            if (!opj_pred->done) {
                                break_flag = true;
                                break;
                            }
                            start = max(start, opj_pred->end);
                        }
                    }
                    if (break_flag) break;
#else
                    operation *opj_pred = opj->pred[0];
                    job *opj_pred_jb = soljobs[opj->jobID];
					if (temp) {

						if (!opj_pred->temp_done) break;
                        long early_pred_start  = opj_pred->temp_start +  (int) (opj_pred_jb->lotFactor * opj_pred->getProcTime());
						start = max(start, early_pred_start);
					}
					else {
						if (!opj_pred->done) break;
                        long early_pred_start  = opj_pred->start +  (int) (opj_pred_jb->lotFactor * opj_pred->getProcTime());
						start = max(start, early_pred_start);
					}
#endif
                }

				//if (index != -1 && start > minim) break;

				//Calculate new temp_start and end_temp
				int mc_setup = opj->setup_time;

#if(CHANGEOVERTIMES)
				mc_setup += changeovertime(mc, opi, opj);
#endif
                start += mc_setup; //Increase start time so that the gant chart is as clean as possible

                int mc_cleanup = opj->cleanup_time;

#if(FLEX)
                operation *opj_pred = opj->pred[0];
                job *opj_pred_jb = soljobs[opj->jobID];
                //End calculated from machine predecessor
                long proper_end = start + opj->getProcTime() + mc_cleanup;

                if (temp) {
                    //Earliest end time is the end time that can be possibly achieved once the last blackbox
                    //leaves the predecessor and is processed at this stage
                    long earliest_end = proper_end;
                    if (opj_pred != nullptr)
				        earliest_end = opj_pred->temp_end + (int) (opj_pred_jb->lotFactor * opj_pred->getProcTime());
                    //make sure that op cannot end before the predecessor has finished processing the last mini batch of boards
                    end = max(proper_end, earliest_end);
				} else {
                    long earliest_end = proper_end;
                    if (opj_pred != nullptr)
                        earliest_end = opj_pred->end + (int) (opj_pred_jb->lotFactor * opj_pred->getProcTime());
                    //make sure that op cannot end before the predecessor
                    end = max(proper_end, earliest_end);
                }

#else
				if (temp) 	end = start + mc_setup + opj->processing_time + mc_cleanup;
				else		end = start + mc_setup + opj->processing_time + mc_cleanup;
#endif


				//MACHINE UNAVAILABILITIES

				//Check if machine is available and update start and end temp times
                for (int c = 0; c < mc->unavailability.size(); c++)
                {
                    mach_unav *muv = mc->unavailability[c];
                    if (start > muv->end_unav) continue;

                    //Check if the muv
                    //This check covers all three cases of the unav window compared to [start, end]
                    //At all cases the operation is shifted to the end of the unavailability window
                    if ((end > muv->start_unav && end <= muv->end_unav) || (start >= muv->start_unav && start <= muv->end_unav) || (muv->start_unav >= start && muv->end_unav <= end))
                    {
                        int diff = muv->end_unav - start;
                        start += diff;
                        end += diff;
                        continue;
                    }
                    if (end <= muv->start_unav) break;
                }


                //CHECK RESOURCE AVAILABILITY

                //Step A: Calculate the consumption/production rate
                job *opj_job = soljobs[opj->jobID];
                ProductionOrder *po = dataset->orderIDMap.at(opj_job->orderID);
                BOMEntry *be = dataset->BOM.at(po->productName);
                double coeffs[2];
                double rc_usage_rate = (float) opj_job->lotSize / opj->getProcTime();
                bool inserted_consumption = false;
                bool consumpt_status = false;
                resource *rc_req, *rc;

                //Step A: Add consumption (if any)
                //Find time period to accomodate the required resource consumption
                if (be->code_type != 0 && be->requirement != "NONE"){
                    //Make sure that the required quantity is there
                    rc_req = Resources[be->requirement];

                    //if (rc_req->name == "VGY112Y")
                    //    printf("break\n");

                    if (rc_req->end_q + 1 < opj_job->lotSize){
                        //printf("Not Enough Resource quantity for operation %d, Order ID %d, Product %s.Remaining Quantity %d of Resource %s\n",
                        //        opj->UID, po->orderID, po->productName.c_str(), rc_req->end_q, rc_req->name.c_str());
                        break;
                    }


                    //Add consumption
                    coeffs[0] = -rc_usage_rate;
                    coeffs[1] = 0.0f;


                    //if (rc_req->name == "TA5111Y" && rc_req->start_q == 1104 && rc_req->end_q == 1100)
                    //    printf("break");
                    //if (rc_req->name == "UK6111Y" && opj_job->lotSize == 182 && rc_req->start_q == 0 && rc_req->prod_q==784);
                    //    printf("break");

                    double res_start, res_end;
                    resource * rc_req_bak = new resource();
                    *rc_req_bak = *rc_req;
                    consumpt_status = rc_req->insert_consumption(opj_job->lotSize, 50, start, coeffs,
                            res_start, res_end);

                    if (!consumpt_status){
                        return false; //Return infeasible schedule. Hopefully this happens only during local search
                        printf("Problem with Consumption Insertion\n");
                        rc_req_bak->export_to_txt("test_resource.txt");
                        rc_req_bak->insert_consumption(opj_job->lotSize, 50, start, coeffs,
                                                       res_start, res_end);
                    }

                    delete rc_req_bak;

                    inserted_consumption = true;
                    //Save new start/end times
                    start = (long) res_start;
                    end = (long) res_end;
                }

                //Step B: Add production
                rc = Resources.at(po->productName);
                coeffs[0] = rc_usage_rate;
                coeffs[1] = 0.0f;

                //if (rc->name == "VGY111Y")
                //    printf("break\n");

                //Add production to the generic resources and on the operations as well
                if (inserted_consumption){
                    consumpt_status = rc->insert_production_over_consumption(opj_job->lotSize, coeffs, rc_req);
                    opj->prod_res->insert_production_over_consumption(opj_job->lotSize, coeffs, rc_req);
                } else {
                    rc->insert_production(opj_job->lotSize, (float) start, (float) end, coeffs);
                    opj->prod_res->insert_production(opj_job->lotSize, (float) start, (float) end, coeffs);
                }


#if(RESOURCE_CONSTRAINTS)
#if(FLEX)
					int duration = mc_setup + opj->processing_time_UM[mc->ID] + mc_cleanup;
#else
					int duration = mc_setup + opj->processing_time + mc_cleanup;
#endif
					int max_start = start;
					int t = 0;

					//NEW BLOCKS WAY
					//cout << " Trying to fit operation " << opj->jobID << " " << opj->ID << endl;
					for (int r = 0; r < (int)Resources.size(); ++r) {
						resource *rc = Resources[r];
						max_start = max(max_start, rc->find_block(start, end, mc->res_consum_processing[r]));

						//if (max_start > start) {
						//	cout << "FInd Block Did Something Naughty :O " << endl;
						//}
					}

					//Update start and end times
					start = max_start + t;

					if (index != -1 && start > minim) break;

#if(FLEX)
					if (temp) end = start + mc_setup + opj->processing_time_UM[opj->temp_machID] + mc_cleanup;
					else      end = start + mc_setup + opj->processing_time_UM[opj->machID] + mc_cleanup;
#else
					if (temp) end = start + mc_setup + opj->processing_time + mc_cleanup;
					else      end = start + mc_setup + opj->processing_time + mc_cleanup;
#endif

#endif
                //Schedule the Op Right away
				//printf("scheduleAEAP | Operation: %d %d Scheduled - OpCounter %d \n", op->jobID, op->ID, opcounter);
				switch (temp) {
				case 0:
					opj->done = true;
					opj->machID = mc->ID;
					opj->mthesi = u;
					opj->end = end;
					opj->start = start;
					break;
				case 1:
					opj->temp_done = true;
					opj->temp_machID = mc->ID;
					opj->temp_mthesi = u;
					opj->temp_end = end;
					opj->temp_start = start;
					break;
				}

				if (opj->fixed)
				    printf("MALAKIES");
				
				opcounter++;
				opj->setDoneStatus(temp, true);
				sAEAP_mach_positions[g]++;
				
				//Update resource loads

				//Global Update
				//Update_Resourse_Loads(temp);
            }
		}
		
		if (back_opcounter == opcounter)
		    iter++;
		else
		    iter = 0;
		
		if (iter > iter_lim){
			//printf("Kapoia malakia exei paixtei me ta moving/done status\n");
			//this->FGraph->report_topological_sort();
			//cout << *this;
			//printf("Cannot calculate schedule - Probably Infeasible\n");
            return false;
		}
		
		if (opcounter == actualopcounter) break;
	}
	if (temp) {
		//cout << "SCHEDULEAEAP FINISHED SOLUTION" << endl;
		//cout << *this;
		//cout << "MemTest after scheduleAEAP" << endl;
		//this->memTestSolution();
		return true;
	}
	else
	{
		if (!find_conflicts(temp)) {
			printf("Malakia sth findConflicts\n");
			return false;
		}
		else return true;
	}
}


bool solution::find_conflicts(bool temp)
{
	int tot_op1 = 0;
	int temp_slot = (int)temp;
	for (int g = 0; g<(int) this->soljobs.size(); g++)
	{
		job *jb = this->soljobs[g];
		for (int h = 0; h<(int) jb->operations.size(); h++)
		{
			operation *op1 = jb->operations[h];
			switch (temp) {
			case 0:
				if (op1->done) tot_op1++;
				break;
			case 1:
				if (op1->temp_done) tot_op1++;
				break;
			}

		}
	}


	//TODO Delete
	//for(int y=0;y<machines->GetSize();y++){
	//	machine *mc=(machine*)(*machines)[y];
	//	if(mc->ID != y){
	//		cout<<"MC_ID Check point!"<<endl;
	//	}
	//}
	//for(int p=0;p<ProcSteps.size();p++){
	//	process_step *ps = ProcSteps[p];
	//	for(int m=0;m<ps->mach_avail.size();m++){
	//		machine *mc = ps->mach_avail[m];
	//		if(mc->operations.size()==2) continue;
	//			
	//		for(int y=0; y<mc->operations.size(); y++){
	//			operation *op = mc->operations[y];
	//			if(temp){
	//				get<1>(op->machID) = mc->ID;
	//				op->temp_mthesi = y;
	//			}
	//			else{
	//				op->machID = mc->ID;
	//				op->mthesi = y;
	//			}
	//		}
	//	}
	//}

	int tot_op2 = 0;
	for (int y = 0; y<(int) this->solmachines.size(); y++)
	{
		machine *mc = this->solmachines[y];
		tot_op2 += (int) mc->operations.size() - 2;
		for (int k = 1; k<(int) mc->operations.size() - 1; k++)
		{
			operation *op = mc->operations[k];

			int op_mthesi;
			int op_machID;
			if (temp) {
				op_mthesi = op->temp_mthesi;
				op_machID = op->temp_machID;
			}
			else {
				op_mthesi = op->mthesi;
				op_machID = op->machID;
			}

			if ((op_machID != y) || (op_mthesi != k)) {
				cout << op->machID << " " << y << " " << op->mthesi << " " << k << endl;
				cout << op->ID << endl;
				printf("Find Conflicts Warning #1 Temp Status %d\n", temp_slot);
				return false;
			}
			//	if(mc->process_stepID != op->process_stepID){
			//	cout<<mc->process_stepID<<" "<<op->process_stepID<<endl;
			//	cout<<"Find Conflicts Warning #2"<<endl;
			//	return false;
			//}
		}
	}

	//Check if all operations appaer in the solution
	if (tot_op1 != tot_op2) {
		cout << tot_op1 << " " << tot_op2 << endl;
		cout << "Find Conflicts Warning #3" << endl;
		cout << *this;
		this->FGraph->report();
		//	return false;
	}

	//Check start and end times
	for (int y = 0; y<(int) this->solmachines.size(); y++)
	{
		machine *mc = this->solmachines[y];
		for (int k = 1; k < (int) mc->operations.size(); k++)
		{
			operation *op1 = mc->operations[k];
			if (temp)
			{
				if (op1->temp_start < op1->a){
					cout << "Find Conflicts Warning #3.1" << endl;
					return false;	
				} 
				//if(op1->temp_end   > op1->b) return false;
				if (op1->has_preds())
				{
					operation *op1_pred = op1->pred[0];
					if (op1_pred->temp_end > op1->temp_start){
						cout << "Find Conflicts Warning #3.2" << endl;
						return false;
					}
				}
				for (int l = k + 1; l < (int) mc->operations.size() - 1; l++)
				{
					operation *op2 = mc->operations[l];
					if (op1->temp_start > op2->temp_start || op1->temp_end < op1->temp_start || op2->temp_end < op2->temp_start || op1->temp_end > op2->temp_start){
						printf("Start Temps op1 %d %d : %d, op2 %d %d : %d \n", op1->jobID, op1->ID, op1->temp_start, op2->jobID, op2->ID, op2->temp_start);
						printf("End Temps op1 %d End Temp op2 %d \n", op1->temp_end, op2->temp_end);
						cout << "Find Conflicts Warning #3.3" << endl;
						return false;	
					}
				}
			}
			else
			{
				if (op1->start < op1->a) {
					cout << "Find Conflicts Warning #4" << endl;
					return false;
				}
				if (op1->end > op1->b) {
					#if(HARD_DUE_DATES)
						cout << "JobID: " << op1->jobID << " OpID: " << op1->ID << endl;
						cout << "Start_Time: " << get<0>(op1->start_times) << " Rel_Time: " << op1->a << endl;
						cout << "End_Time: " << op1->end << " Due_Time: " << op1->b << endl;
						cout << "Find Conflicts Warning #5" << endl;
						return false;
					#endif
					//getchar();
					
				}
				if (op1->has_preds())
				{
                    operation *op1_pred = op1->pred[0];
                    job* op1_pred_jb = soljobs[op1_pred->jobID];
                    if (op1_pred->start + (int) (op1_pred_jb->lotFactor * op1_pred->getProcTime()) > op1->start) {
                        cout << "Find Conflict Warning #6" << endl;
                        return false;
                    }
                }
				for (int l = k + 1; l < (int) mc->operations.size(); l++)
				{
					operation *op2 = mc->operations[l];
					if (op1->start > op2->start || op1->end < op1->start || op2->end < op2->start || op1->end > op2->start) {
						cout << "MC Size: " << (int) mc->operations.size() - 2 << endl;
						cout << *mc;
						cout << "Thesi k: " << k << " " << op1->ID << " Thesi l: " << l << " " << op2->ID << endl;
						cout << op1->start << " > " << op2->start << endl;
						cout << op1->end << " < " << op1->start << endl;
						cout << op2->end << " < " << op2->start << endl;
						cout << op1->end << " > " << op2->start << endl;
						cout << "Find Conflicts Warning #7" << endl;
						return false;
					}
				}
			}
		}
	}

#if(MACHINE_AVAILABILITIES)

	//Check machine availabilities
	for (int y = 0; y<(int) solmachines.size(); y++)
	{
		machine *mc = solmachines[y];
		for (int c = 0; c<(int) mc->unavailability.size(); c++)
		{
			mach_unav *muv = mc->unavailability[c];
			for (int k = 1; k < (int) mc->operations.size(); k++)
			{
				operation *op = mc->operations[k];
				if (temp)
				{
					if (get<1>(op->end_times) <= muv->start_unav) continue;
					if (op->temp_start >= muv->end_unav) continue;
					if (get<1>(op->end_times) > muv->start_unav && get<1>(op->end_times) <= muv->end_unav) return false;
					if (op->temp_start >= muv->start_unav && op->temp_start < muv->end_unav) return false;
					if (op->temp_start >= muv->start_unav && op->temp_start < muv->end_unav) return false;
					if (muv->start_unav >= op->temp_start && muv->end_unav <= get<1>(op->end_times)) return false;
				}
				else
				{
					if (op->end <= muv->start_unav) continue;
					if (op->start >= muv->end_unav) continue;
					if (op->end > muv->start_unav && op->end <= muv->end_unav) {
						cout << "Find Conflicts Warning #8" << endl;
						return false;
					}
					if (op->start >= muv->start_unav && op->start < muv->end_unav) {
						cout << "Find Conflicts Warning #9" << endl;
						return false;
					}
					if (op->start >= muv->start_unav && op->start < muv->end_unav) {
						cout << "Find Conflicts Warning #10" << endl;
						return false;
					}
					if (muv->start_unav >= op->start && muv->end_unav <= op->end) {
						cout << "Find Conflicts Warning #11" << endl;
						return false;
					}
				}
			}
		}
	}
#endif

	//Check rule
	for (int g = 0; g<(int) this->soljobs.size(); g++)
	{
		job *jb = this->soljobs[g];
		for (int h = 0; h<(int) jb->operations.size(); h++)
		{
            operation *op2 = jb->operations[h];
            bool op2_done_status;

            switch (temp) {
                case 0:
                    op2_done_status = op2->done;
                    break;
                case 1:
                    op2_done_status = op2->temp_done;
                    break;
            }

            for (int kk=0; kk<MAX_JOB_RELATIONS; kk++) {
                operation *op1 = op2->pred[kk];
                if (op1== nullptr) continue;
                bool op1_done_status;
                int op1_machID;

                switch (temp) {
                    case 0:
                        op1_done_status = op1->done;
                        op1_machID = op1->machID;
                        break;
                    case 1:
                        op1_done_status = op1->temp_done;
                        op1_machID = op1->temp_machID;
                        break;
                }

                if (op1_done_status != op2_done_status) {
                    cout << "Find Conflicts Warning #11.1" << endl;
                    return false;
                }

                if (!op1_done_status || !op1_done_status)
                    return false;

                machine *mc = this->solmachines[op1_machID];
#if(FLEX)
                int rhs = 0;

                if (temp)
                    rhs = op1->temp_start + (int) (jb->lotFactor * op1->getProcTime()) + op1->setup_time + op1->cleanup_time;
                else
                    rhs = op1->start + (int) (jb->lotFactor * op1->getProcTime()) + op1->setup_time + op1->cleanup_time;
#else
				if (temp)
                    rhs = op1->temp_start + (int) (jb->lotFactor * op1->getProcTime) + op1->setup_time + op1->cleanup_time;
                else
                    rhs = op1->start + (int) (jb->lotFactor * op1->getProcTime) + op1->setup_time + op1->cleanup_time;
#endif


                //NOTE: At this point changeovertimes have probably been accounted for in the start time of op1
//#if(CHANGEOVERTIMES)
//				rhs += changeovertime(mc, op1->get_mach_pred(), op1);
//#else

				if (temp) {
                    if (op2->temp_start < rhs) {
                        cout << "Find Conflicts Warning #11.2" << endl;
                        cout << op2->temp_start << " " << rhs << endl;
                        return false;
                    }
                }
                else {
                    if (op2->start < rhs) {
                        cout << "Find Conflicts Warning #11.2" << endl;
						cout << op2->start << " " << rhs << endl;
                        return false;
                    }
                }
            }
        }
	}

#if(RESOURCE_CONSTRAINTS)
	//Check resource constraints - feasibility checks
	for (int p = 0; p<(int) ProcSteps.size(); p++) {
		process_step *ps = ProcSteps[p];

		for (int m = 0; m<(int) ps->mach_avail.size(); m++) {
			machine *mc = ps->mach_avail[m];
			if ((int) mc->operations.size() == 2) continue;

			for (int r = 0; r<NumberOfResources; r++) {
				resource *rc = Resources[r];

				//New Way
				for (int i = 0; i < (int) c->blocks.size(); i++) {
					res_block *block = rc->blocks[i];

					//Check if resource limit is violated
					if (block->res_load > rc->max_level) {
						cout << "Test Solution Error #10" << endl;
						return false;
					}
				}
				//Here I need to add the loads on the process step

				//Check if the machine load is violated per time block
				//Load machine resource
				resource * mach_res = mc->resources[r];
				for (int i = 0; i < (int) mach_res->blocks.size(); i++) {
					res_block *block = mach_res->blocks[i];
					if (block->res_load > mc->res_consum_processing[r]) {
						cout << "Test Solution Error #12" << endl;
						return false;
					}

					//Calculation Check
					//This check looks wrong because, there could be cases where the machine remains idle.

					//if (block->res_load != mc->res_consum_processing[r]) {
					//	cout << "Test Solution Error #13 " << "Block Load: " << block->res_load << "Machine Processing Load: " << mc->res_consum_processing[r] << endl;
					//	return false;
					//}
				}
			}
		}
	}

	//Old Code Block
	//Check resource constraints - feasibility checks
	//for (int p = 0; p<ProcSteps.size(); p++) {
	//	process_step *ps = ProcSteps[p];

	//	for (int m = 0; m<ps->mach_avail.size(); m++) {
	//		machine *mc = ps->mach_avail[m];
	//		if (mc->operations.size() == 2) continue;

	//		for (int r = 0; r<NumberOfResources; r++) {
	//			resource *rc = Resources[r];
	//			for (int t = 0; t<planhorend; t++) {//TODO need to break down the different phases setup, processing and cleanup time
	//				if (rc->res_load_t[t] > rc->max_level) {
	//					cout << "Find Conflicts Warning #14" << endl;
	//					return false;
	//				}
	//				if (ps->proc_res_load_t[r][t] > ps->resalarm[r]) {
	//					cout << "Find Conflicts Warning #15" << endl;
	//					return false;
	//				}
	//				if (mc->mach_res_load_t[r][t] > mc->res_consum_processing[r]) {
	//					cout << "Find Conflicts Warning #16" << endl;
	//					return false;
	//				}
	//			}
	//		}
	//	}
	//}
#endif
	return true;
}

bool solution::testsolution()
{
	//If a process_step is repeated in the articlestepsequence this check must be inactive
	//for(int y=0;y<machines->GetSize();y++)
	//{
	//	machine *mc=(machine*)(*machines)[y];
	//	for(int k=1; k<mc->operations.size(); k++)
	//	{
	//		operation *op1=mc->operations[k];
	//		for(int l = k+1; l<mc->operations.size()-1; l++)
	//		{
	//			operation *op2=mc->operations[l];
	//			if(op1->jobID == op2->jobID) return false;
	//		}
	//	}
	//}

	//Check if all operations are done=true
	int tot_op1 = 0;
	for (int g = 0; g<(int) this->soljobs.size(); g++)
	{
		job *jb = this->soljobs[g];
		tot_op1 += (int) jb->operations.size();
		for (int h = 0; h<(int) jb->operations.size(); h++)
		{
			operation *op1 = jb->operations[h];
			if (!op1->done) {
				cout << "Test Solution Error #1" << endl;
				cout << *this;
				cout.flush();
				return false;
			}
		}
	}

	int tot_op2 = 0;
	for (int y = 0; y<(int) this->solmachines.size(); y++)
	{
		machine *mc = this->solmachines[y];
		tot_op2 += (int) mc->operations.size() - 2;
		for (int k = 1; k<(int) mc->operations.size() - 1; k++)
		{
			operation *op = mc->operations[k];
			if (op->machID != y || op->mthesi != k) {
				printf("Test Solution Error #2. Operation %d %d not in machine %d != %d or not in position %d != %d \n", 
					op->jobID, op->ID, op->machID, y, op->mthesi, k);
				//cout << "Test Solution Error #2" << "Operationendl;
				return false;
			}
			//Check if all operations are assigned to process_step machines
			if (mc->process_stepID != op->process_stepID) {
				cout << "Test Solution Error #3" << endl;
				return false;
			}
		}
	}

	//Check if all operations appear in the solution
	if (tot_op1 != tot_op2) {
		cout << "Test Solution Error #4 ";
		cout << tot_op1 << " " << tot_op2 << endl;
		return false;
	}

	//Check start and end times
	for (int y = 0; y<(int) this->solmachines.size(); y++)
	{
		machine *mc = this->solmachines[y];
		for (int k = 1; k < (int) mc->operations.size(); k++)
		{
			operation *op1 = mc->operations[k];
			//Basic schedule tests I
			if (op1->start < op1->a) {
				cout << "Test Solution Error #5" << endl;
				return false;
			}
			if (op1->end > op1->b) {
				#if (HARD_DUE_DATES)
				cout << "Test Solution - Due date Warning #5a" << endl;
					return false;
				#endif
			}
			if (op1->a > op1->start) {
				cout << "Test Solution - Release time Warning #5c" << endl;
				return false;
			}
			#if(FLEX)
				if (k != 0 && k != (int) mc->operations.size() - 1 && op1->getProcTime() == 0) {
					cout << "Test Solution - ERROR #5b" << endl;
					return false;
				}
			#else
				if (k != 0 && k != (int) mc->operations.size() - 1 && op1->processing_time == 0) {
					cout << "Test Solution - ERROR #5b" << endl;
					return false;
				}
#endif
			if (op1->has_preds())
			{
				operation *op1_pred = op1->pred[0];
				job* op1_pred_jb = soljobs[op1_pred->jobID];
				if (op1_pred->start + (int) (op1_pred_jb->lotFactor * op1_pred->getProcTime()) > op1->start) {
					cout << "Test Solution Error #6" << endl;
					return false;
				}
			}

			for (int l = k + 1; l < (int) mc->operations.size() - 1; l++)
			{
				operation *op2 = mc->operations[l];
				//Basic schedule tests II
				if (op1->start > op2->start || op1->end < op1->start || op2->end < op2->start || op1->end > op2->start) {
					cout << op1->start << " > " << op2->start << endl;
					cout << op1->end << " < " << op1->start << endl;
					cout << op2->end << " < " << op2->start << endl;
					cout << op1->end << " > " << op2->start << endl;
					cout << "Test Solution Error #7" << endl;
					return false;
				}
			}
		}
	}

#if(MACHINE_AVAILABILITIES)
	//Check machine availabilities
	for (int y = 0; y<(int) solmachines.size(); y++)
	{
		machine *mc = solmachines[y];
		for (int c = 0; c<(int) mc->unavailability.size(); c++)
		{
			mach_unav *muv = mc->unavailability[c];
			for (int k = 1; k < (int) mc->operations.size(); k++)
			{
				bool infeas = false;
				operation *op = mc->operations[k];
				if (op->end <= muv->start_unav) continue;
				if (op->start >= muv->end_unav) continue;
				if (op->end > muv->start_unav && op->end <= muv->end_unav) infeas = true;
				if (op->start >= muv->start_unav && op->start < muv->end_unav) infeas = true;
				if (op->start >= muv->start_unav && op->start < muv->end_unav) infeas = true;
				if (muv->start_unav >= op->start && muv->end_unav <= op->end) infeas = true;
				if (infeas) {
					cout << "Test Solution Error #8" << endl;
					return false;
				}
			}
		}
	}
#endif

    //NOTE: I think this code was created for the CBC stuff It is safe to remove it
    /*
	//Check schedule
	for (int g = 0; g<(int) this->soljobs.size(); g++)
	{
		job *jb = this->soljobs[g];
		for (int h = 0; h<(int) jb->operations.size(); h++)
		{
            operation *op2 = jb->operations[h];
            for (int k=0;k<MAX_JOB_RELATIONS;k++){
                operation *op1 = op2->pred[k];
                if (op1 == nullptr) continue;

                machine *mc = this->solmachines[op1->machID];
    #if(FLEX)
                int rhs = op1->start + op1->processing_time_UM[op1->process_stepID] + op1->setup_time + op1->cleanup_time;
    #else
                int rhs = op1->start + op1->processing_time + op1->setup_time + op1->cleanup_time;
    #endif

    #if(CHANGEOVERTIMES)
                rhs += changeovertime(mc->ID, op_pred->opertypeID, op1->opertypeID);
    #endif
                if (op2->start < rhs && op1->done && op2->done) {
                    cout << "Test Solution Error #9" << endl;
                    return false;
                }
            }
		}
	}
    */

#if(RESOURCE_CONSTRAINTS)

	//Check resource constraints - feasibility checks
	for (int p = 0; p<(int) ProcSteps.size(); p++) {
		process_step *ps = ProcSteps[p];

		for (int m = 0; m<(int) ps->mach_avail.size(); m++) {
			machine *mc = ps->mach_avail[m];
			if ((int) mc->operations.size() == 2) continue;

			for (int r = 0; r<NumberOfResources; r++) {
				resource *rc = Resources[r];
				
				//Old Way
				//for (int t = 0; t<planhorend; t++) {//TODO need to break down the different phases setup, processing and cleanup time
				//	if (rc->res_load_t[t] > rc->max_level) {
				//		cout << "Test Solution Error #10" << endl;
				//		return false;
				//	}
				//	if (ps->proc_res_load_t[r][t] > ps->resalarm[r]) {
				//		cout << "Test Solution Error #11" << endl;
				//		return false;
				//	}
				//	if (mc->mach_res_load_t[r][t] > mc->res_consum_processing[r]) {
				//		cout << "Test Solution Error #12" << endl;
				//		return false;
				//	}
				//}

				//New Way
				for (int i = 0; i < (int) rc->blocks.size(); i++) {
					res_block *block = rc->blocks[i];

					//Check if resource limit is violated
					if (block->res_load > rc->max_level) {
						cout << "Test Solution Error #10" << endl;
						return false;
					}
				}
				//Here I need to add the loads on the process step

				//Check if the machine load is violated per time block
				//Load machine resource
				resource * mach_res = mc->resources[r];
				for (int i = 0; i < (int) mach_res->blocks.size(); i++) {
					res_block *block = mach_res->blocks[i];
					if (block->res_load > mc->res_consum_processing[r]) {
						cout << "Test Solution Error #12" << endl;
						return false;
					}

					//Calculation Check
					//This check looks wrong because, there could be cases where the machine remains idle.
					
					//if (block->res_load != mc->res_consum_processing[r]) {
					//	cout << "Test Solution Error #13 " << "Block Load: " << block->res_load << "Machine Processing Load: " << mc->res_consum_processing[r] << endl;
					//	return false;
					//}
				}
			}
		}
	}

	////Calculation checks I
	//for (int p = 0; p<ProcSteps.size(); p++) {
	//	process_step *ps = ProcSteps[p];

	//	for (int m = 0; m<ps->mach_avail.size(); m++) {
	//		machine *mc = ps->mach_avail[m];
	//		if (mc->operations.size() == 2) continue;

	//		for (int y = 1; y<mc->operations.size() - 1; y++) {
	//			operation *opi = mc->operations[y];
	//			operation *opj = mc->operations[y + 1];
	//			for (int r = 0; r<NumberOfResources; r++) {
	//				for (int t = opi->start; t<opi->end; t++) {
	//					if (mc->mach_res_load_t[r][t] != mc->res_consum_processing[r]) {
	//						cout << "Test Solution Error #13" << endl;
	//						return false;
	//					}
	//				}
	//				
	//				for (int t = opi->end + 1; t<opj->start; t++) {
	//					if (mc->mach_res_load_t[r][t] != 0) {
	//						cout << "Test Solution Error #14" << endl;
	//						return false;
	//					}
	//				}
	//			}
	//		}
	//	}
	//}

	//With the new way the following checks probably pass since the arrays are never populated and they are always 0.

	//Calculation checks II
	for (int p = 0; p<(int) ProcSteps.size(); p++) {
		process_step *ps = ProcSteps[p];
		for (int r = 0; r<NumberOfResources; r++) {
			resource *rc = Resources[r];
			for (int t = 0; t<planhorend; t++) {//TODO need to break down the different phases setup, processing and cleanup time
				double sum = 0;
				for (int m = 0; m<(int) ps->mach_avail.size(); m++) {
					machine *mc = ps->mach_avail[m];
					if ((int) mc->operations.size() == 2) continue;

					sum += mc->mach_res_load_t[r][t];
				}
				if (ps->proc_res_load_t[r][t] != sum) {
					cout << "Test Solution Error #15" << endl;
					return false;
				}
			}
		}
	}

	//Calculation checks III
	for (int r = 0; r<NumberOfResources; r++) {
		resource *rc = Resources[r];
		for (int t = 0; t<planhorend; t++) {//TODO need to break down the different phases setup, processing and cleanup time
			double sum = 0;
			for (int m = 0; m< (int) this->solmachines.size(); m++) {
				machine *mc = this->solmachines[m];
				if ((int) mc->operations.size() == 2) continue;
				sum += mc->mach_res_load_t[r][t];
			}
			if (rc->res_load_t[t] != sum) {
				cout << "Test Solution Error #16" << endl;
				return false;
			}
		}
	}
#endif
	return true;
}

bool solution::compareSolution(const solution* sol) {
	return Objective_TS_Comparison(this, sol, false);

	////Current sol costs
	//int makespan_diff = cost_array[MAKESPAN] - sol->cost_array[MAKESPAN];
	//int tft_diff =  cost_array[TOTAL_FLOW_TIME] - sol->cost_array[TOTAL_FLOW_TIME];
	//int tidle_diff = cost_array[TOTAL_IDLE_TIME] - sol->cost_array[TOTAL_IDLE_TIME];
	//int energy_diff = cost_array[TOTAL_ENERGY] - sol->cost_array[TOTAL_ENERGY];
	//
	//switch(optimtype){
	//	case 1:
	//		if (makespan_diff < 0) flag = true;
	//		break;
	//	case 2:
	//		if (tft_diff < 0) flag = true;
	//		break;
	//	case 3:
	//		if (tidle_diff < 0) flag = true;
	//		break;
	//	case 4:
	//		if (makespan_diff < 0 || (tft_diff <= 0 && tidle_diff < 0) || (makespan_diff <= 0 && tft_diff <= 0 && tidle_diff < 0))         flag = true;
	//		break;
	//	case 5:
	//		if (makespan_diff < 0 || (tft_diff <= 0 && tidle_diff < 0) || (tft_diff <= 0 && tidle_diff <= 0 && makespan_diff < 0)) flag = true;
	//		break;
	//	case 6:
	//		if (tidle_diff < 0 || (tidle_diff <= 0 && makespan_diff < 0) || (tidle_diff <= 0 && makespan_diff <= 0 && tft_diff < 0))    flag = true;
	//		break;
	//	case 7:
	//		if (makespan_diff < 0 || (makespan_diff <= 0 && tidle_diff < 0) || (makespan_diff <= 0 && tidle_diff <= 0 && tft_diff < 0))        flag = true;
	//		break;
	//	case 8:
	//		if (tft_diff < 0 || (tft_diff <= 0 && makespan_diff < 0) || (tft_diff <= 0 && makespan_diff <= 0 && tidle_diff < 0))    flag = true;
	//		break;
	//	case 9:
	//		if (tidle_diff < 0 || (tidle_diff <= 0 && tft_diff < 0) || (tidle_diff <= 0 && tft_diff <= 0 && makespan_diff < 0))    flag = true;
	//		break;
	//	case 10:
	//		if (optimtype == 10) if (energy_diff < 0) flag = true;
	//		break;
	//	case 11:
	//		if (energy_diff < 0 || (energy_diff <= 0 && makespan_diff < 0) || (energy_diff <= 0 && makespan_diff <= 0 && tft_diff < 0))    flag = true;
	//		break;
	//	case 12:
	//		if (energy_diff < 0 || (energy_diff <= 0 && tft_diff < 0) || (energy_diff <= 0 && tft_diff <= 0 && makespan_diff < 0))    flag = true;
	//		break;
	//	default:
	//		break;
	//}
	//
	//return flag;
}

//RESOURCE CONSTRAINTS STUFF
void solution::Setup_Resources(){
    //Initialize Resources for all the associated components related to the job/operations
    for (int i=0; i<soljobs.size(); i++){
        job *jb = soljobs[i];
        ProductionOrder *po = dataset->orderIDMap.at(jb->orderID);

        //Check if resource is already initialized
        if (Resources.find(po->productName) != Resources.end())
            continue;

        //Generate Resource for associated product
        resource *rc = new resource();
        rc->name = po->productName;
        Resources[rc->name] = rc;

        //Also generate resource for the BOM requirement of the order (if any)
        BOMEntry *productBOM = dataset->BOM.at(po->productName);
        rc = new resource();
        rc->name = productBOM->requirement;
        Resources[rc->name]= rc;
    }

    //Add any extra resources that exist in the Production Inventory table
    for (auto e : dataset->productInventory) {
        //Check if resource is already initialized
        if (Resources.find(e.first) != Resources.end())
            continue;

        //Generate Resource for associated product
        resource *rc = new resource();
        rc->name = e.first;
        Resources[rc->name] = rc;
    }

    //Setup resources per operation
    for (int i=0;i<soljobs.size();i++){
        job *jb = soljobs[i];
        ProductionOrder *po = dataset->orderIDMap.at(jb->orderID);
        for (int j=0; j < jb->operations.size(); j++){
            operation *op = jb->operations[j];

            //The operation resource probably doesn't need initialization since it should
            //be already initialized

            //Set correct name to the operation-tied resource
            op->prod_res->name = po->productName;
        }
    }
}


void solution::Init_Resourse_Loads() {
    //Clear generic resources
    for (auto rc : Resources){
        rc.second->clear();
    }

    //Clear per operations resources
    for (auto op : soloperations){
        op->prod_res->clear();
    }

    //Add product inventory (if any)
    for (auto rc : Resources){
        //Search for inventory
        if (dataset->productInventory.find(rc.second->name) != dataset->productInventory.end()){
            //Setup Coeffs
            double coeffs[2];
            coeffs[0] = 0.0f;
            coeffs[1] = 0.0f;

            coeffs[1] = dataset->productInventory.at(rc.second->name);

            rc.second->insert_production((int) coeffs[1], 0, planhorend, coeffs);
        }
    }

#if(RESOURCE_CONSTRAINTS)

	//Remove all resource blocks
	for (int r = 0; r<(int) Resources.size(); r++) {
		resource *res = Resources[r];
		
		res_block *block; //init a reference pointer
		for (int i = 0; i < (int) res->blocks.size(); i++) {
			res_block *block = res->blocks[i];
			delete block;
		}
		res->blocks.clear();
		//Add the new block here
		res->blocks.push_back(new res_block(0, planhorend, 0, res->max_level, res->ID));
	}

	//Remove all resource blocks from machines
	for (int m = 0; m < NumberOfMachines; m++) {
		machine* mc = solmachines[m];
		for (int r = 0; r<(int) Resources.size(); r++) {
			resource *res = mc->resources[r];
			
			for (int i = 0; i < (int) res->blocks.size(); i++) {
				res_block *block = res->blocks[i];
				delete block;
			}
			res->blocks.clear();

			//Add the new block here
			//Note: Since there are no max levels per machine resource we're fixing this to 0 everytime a machine resource 
			//is initiallized
			res->blocks.push_back(new res_block(0, planhorend, 0, 0, res->ID));
		}
	}
	

#endif
}


//ORIGINAL INIT_RESOURCE_LOADS
//void solution::Init_Resourse_Loads()
//{
//	//Initialize to zero the resource consumption vectors (central and per process step)
//#if(RESOURCE_CONSTRAINTS)
//
//	for (int r = 0; r<Resources.size(); r++) {
//		for (int t = 0; t<planhorend; t++) {
//			(Resources[r])->res_load_t[t] = 0;
//		}
//	}
//
//	for (int p = 0; p<ProcSteps.size(); p++) {
//		for (int r = 0; r<NumberOfResources; r++) {
//			for (int t = 0; t<planhorend; t++) {
//				(ProcSteps[p])->proc_res_load_t[r][t] = 0;
//			}
//		}
//	}
//
//	for (int m = 0; m<solmachines.size(); m++) {
//		for (int r = 0; r<NumberOfResources; r++) {
//			for (int t = 0; t<planhorend; t++) {
//				(solmachines[m])->mach_res_load_t[r][t] = 0;
//			}
//		}
//	}
//#endif
//	return;
//}

void solution::Update_Resourse_Loads_perOp(const operation *op, bool temp) {
	machine *mc;
	int temp_slot = (int)temp;
	int op_start = 0;
	int op_end = 0;
	if (temp){
        mc = solmachines[op->temp_machID];
        op_start = op->temp_start;
        op_end = op->temp_end;
	} else {
        mc = solmachines[op->machID];
        op_start = op->start;
        op_end = op->end;
	}


#if(RESOURCE_CONSTRAINTS)
	//cout << "Updating Resources for Op: " << op->jobID << " " << op->ID << " on Machine: " << mc->ID << endl;
	//Actual Resource Update if operation is not done yet
	for (int r = 0; r<NumberOfResources; r++) {
		resource *rc = Resources[r];
		resource *mc_rc = mc->resources[r];
		if (temp) {
			//Add to resource
			rc->insert_block(op->temp_start, get<1>(op->end_times), mc->res_consum_processing[r]);
			//Add to machine
			mc_rc->insert_block(op->temp_start, get<1>(op->end_times), mc->res_consum_processing[r]);
		}
		else {
			//Add to resource
			rc->insert_block(op->start, op->end, mc->res_consum_processing[r]);
			//Add to machine
			mc_rc->insert_block(op->start, op->end, mc->res_consum_processing[r]);
		}

		//Print Out resource
		//if (r == 0) cout << *rc;
	}

#endif
}


void solution::Update_Resourse_Loads(bool temp) {
#if(RESOURCE_CONSTRAINTS)
	//Originally this was set to initiallize everytime,
	//I sense that initialize is not needed at all because I need to update existing consumptions
	//int this method and not reset everything
	Init_Resourse_Loads();
	
	//Calculate resource loads
	for (int p = 0; p<(int) ProcSteps.size(); p++) {
		process_step *ps = ProcSteps[p];
		//Select Machine
		for (int m = 0; m<ps->(int) mach_avail.size(); m++) {
			machine *mc = ps->mach_avail[m];
			if ((int) mc->operations.size() == 2) continue;
			//Select Machine Operation
			for (int y = 0; y<(int) mc->operations.size(); y++) {
				operation *op = mc->operations[y];
				//If not done continue
				if (temp) {
					//get<1>(op->machID) = mc->ID;
					//op->temp_mthesi = y;
					if (!op->temp_done) continue;
				}
				else {
					//op->machID = mc->ID;
					//op->mthesi = y;
					if (!op->done) continue;
				}
				//cout << "Updating Resources for Op: " << op->jobID << " " << op->ID << " on Machine: " << mc->ID << endl;
				//Actual Resource Update if operation is not done yet
				for (int r = 0; r<NumberOfResources; r++) {
					resource *rc = Resources[r];
					resource *mc_rc = mc->resources[r];
					if (temp) {
						//Add to resource
						rc->insert_block(op->temp_start, get<1>(op->end_times), mc->res_consum_processing[r]);
						//Add to machine
						mc_rc->insert_block(op->temp_start, get<1>(op->end_times), mc->res_consum_processing[r]);
					}
					else {
						//Add to resource
						rc->insert_block(op->start, op->end, mc->res_consum_processing[r]);
						//Add to machine
						mc_rc->insert_block(op->start, op->end, mc->res_consum_processing[r]);
					}
						
					//Print Out resource
					//if (r==0) cout << *rc;
				}
			}
		}
	}
#endif
}



//ORIGINAL UPDATE_RESOURCE_LOADS
//void solution::Update_Resourse_Loads(bool temp)
//{
//#if(RESOURCE_CONSTRAINTS)
//	Init_Resourse_Loads();
//
//	//Calculate resource loads
//	for (int p = 0; p<ProcSteps.size(); p++) {
//		process_step *ps = ProcSteps[p];
//		//Select Machine
//		for (int m = 0; m<ps->mach_avail.size(); m++) {
//			machine *mc = ps->mach_avail[m];
//			if (mc->operations.size() == 2) continue;
//			//Select Machine Operation
//			for (int y = 0; y<mc->operations.size(); y++) {
//				operation *op = mc->operations[y];
//				//If not done continue
//				if (temp) {
//					//get<1>(op->machID) = mc->ID;
//					//op->temp_mthesi = y;
//					if (!op->temp_done) continue;
//				}
//				else {
//					//op->machID = mc->ID;
//					//op->mthesi = y;
//					if (!op->done) continue;
//				}
//				
//				//Actual Resource Update
//				for (int r = 0; r<NumberOfResources; r++) {
//					resource *rc = Resources[r];
//					if (temp) {
//						for (int t = op->temp_start; t<get<1>(op->end_times); t++) {//TODO need to break down the different phases setup, processing and cleanup time
//							mc->mach_res_load_t[r][t] += mc->res_consum_processing[r];
//							ps->proc_res_load_t[r][t] += mc->res_consum_processing[r];
//							rc->res_load_t[t] += mc->res_consum_processing[r];
//						}
//					}
//					else {
//						for (int t = op->start; t<op->end; t++) {//TODO need to break down the different phases setup, processing and cleanup time
//							mc->mach_res_load_t[r][t] += mc->res_consum_processing[r];
//							ps->proc_res_load_t[r][t] += mc->res_consum_processing[r];
//							rc->res_load_t[t] += mc->res_consum_processing[r];
//						}
//					}
//				}
//			}
//		}
//	}
//#endif
//	return;
//}

void solution::Calculate_Resourse_Loads()
{
#if(RESOURCE_CONSTRAINTS)
	Init_Resourse_Loads();

	//Calculate resource loads
	for (int p = 0; p<(int) ProcSteps.size(); p++) {
		process_step *ps = ProcSteps[p];

		for (int m = 0; m<(int) ps->mach_avail.size(); m++) {
			machine *mc = ps->mach_avail[m];
			if ((int) mc->operations.size() == 2) continue;

			for (int y = 1; y<(int) mc->operations.size() - 1; y++) {
				operation *op = mc->operations[y];
				//cout<<op->ID<<" "<<op->start<<" "<<op->end<<" "<<op->machID<<endl;
				for (int r = 0; r<NumberOfResources; r++) {
					resource *rc = Resources[r];
					rc->insert_block(op->start, op->end, mc->res_consum_processing[r]);
					mc->resources[r]->insert_block(op->start, op->end, mc->res_consum_processing[r]);
					//cout << *rc;
				}
			}
		}
	}
#endif
	return;
}

//void solution::Calculate_Resourse_Loads()
//{
//#if(RESOURCE_CONSTRAINTS)
//	Init_Resourse_Loads();
//
//	//Calculate resource loads
//	for (int p = 0; p<ProcSteps.size(); p++) {
//		process_step *ps = ProcSteps[p];
//
//		for (int m = 0; m<ps->mach_avail.size(); m++) {
//			machine *mc = ps->mach_avail[m];
//			if (mc->operations.size() == 2) continue;
//
//			for (int y = 1; y<mc->operations.size() - 1; y++) {
//				operation *op = mc->operations[y];
//				//cout<<op->ID<<" "<<op->start<<" "<<op->end<<" "<<op->machID<<endl;
//				for (int r = 0; r<NumberOfResources; r++) {
//					resource *rc = Resources[r];
//					for (int t = op->start; t<op->end; t++) {//TODO need to break down the different phases setup, processing and cleanup time
//						mc->mach_res_load_t[r][t] += mc->res_consum_processing[r];
//						ps->proc_res_load_t[r][t] += mc->res_consum_processing[r];
//						rc->res_load_t[t] += mc->res_consum_processing[r];
//						if (rc->res_load_t[t] > rc->max_level) {
//							cout << "RC Infeasibility #1" << endl;
//						}
//						if (ps->proc_res_load_t[r][t] > ps->resalarm[r]) {
//							cout << "RC Infeasibility #2" << endl;
//						}
//					}
//				}
//			}
//		}
//	}
//#endif
//	return;
//}

void solution::Init_Resourse_Loads_mecutoff(int cutoff)
{
	//Initialize to zero the resource consumption vectors (central and per process step)
#if(RESOURCE_CONSTRAINTS)

	for (int r = 0; r<(int) Resources.size(); r++) {
		for (int t = cutoff; t<planhorend; t++) {
			(Resources[r])->res_load_t[t] = 0;
		}
	}
	for (int p = 0; p<(int) ProcSteps.size(); p++) {
		for (int r = 0; r<NumberOfResources; r++) {
			for (int t = cutoff; t<planhorend; t++) {
				(ProcSteps[p])->proc_res_load_t[r][t] = 0;
			}
		}
	}
	for (int m = 0; m<(int) this->solmachines.size(); m++) {
		for (int r = 0; r<NumberOfResources; r++) {
			for (int t = cutoff; t<planhorend; t++) {
				(this->solmachines[m])->mach_res_load_t[r][t] = 0;
			}
		}
	}
#endif
	return;
}


void solution::reportResourcesConsole() {
#if(RESOURCE_CONSTRAINTS)
	cout << "--Global Resource Consumption--" << endl;
	//Report Global Consumptions
	int rcCount = 1;
	for (int i = 0; i < rcCount; i++) {
		resource *res = this->Resources[i];
		cout << *res;
	}

	cout << "--Per Machine Resource Consumption--" << endl;
	//Report Consumptions per Machine
	for (int j = 0; j < (int) this->solmachines.size(); j++) {
		machine *mc = this->solmachines[j];
		for (int k = 0; k < rcCount; k++) {
			resource *res = mc->resources[k];
			cout << *res;
		}
	}
#endif
}

void solution::reportResources() {

	char filename[200];

	//Report Global Consumptions
	for (auto e : Resources){
        resource *res = e.second;
	    sprintf(filename, "ResID_%s_consumption.txt", res->name.c_str());
        res->export_to_txt(filename);
	}
}

void solution::savetoStream(ostringstream& output){
    output << "Costs: ";
    for (int i=0;i<COST_NUM;i++)
        output << cost_array[i] << " ";
    output << endl;

    //Report jobs
    output << "Jobs " << NumberOfJobs << endl;
	for (int i=0; i<NumberOfJobs; i++) {
        job *jb = soljobs[i];
        output << *jb;
    }

    //Report machines
    output << "Machines " << NumberOfMachines << endl;
    for (int i = 0; i < NumberOfMachines; i++) {
        machine *mc = solmachines[i];
        output << mc->ID << " " << (int) mc->operations.size() - 2 <<
               " " <<  mc->line->ID << " " << mc->line->Name << endl; // Machine ID & number of Ops in machine

        for (int j = 1; j < (int) mc->operations.size()-1; j++) {
            operation *op = mc->operations[j];
            job *jb = soljobs[op->jobID];
            output << op->UID << " " << jb->orderID << " " << op->jobID << " " << op->ID << " " << op->start << " " << op->end << endl;

        }
    }
}

//void solution::saveToFile(const char* filename) {
//	ofstream myfile;
//	myfile.open(filename, std::ios_base::app);
//	//I need a custom output for easier parsing
//	//First of all report the costs because I can't see shit from the output file
//	myfile << "Costs: ";
//	for (int i=0;i<COST_NUM;i++)
//		myfile << cost_array[i] << " ";
//
//	myfile << endl;
//	myfile << NumberOfJobs << " " << NumberOfMachines << endl;
//
//	for (int i = 0; i < NumberOfMachines; i++) {
//		machine *mc = solmachines[i];
//		myfile << mc->ID << " " << (int) mc->operations.size() - 2 << " " << mc->line->productionLineName << endl; // Machine ID & number of Ops in machine
//
//		for (int j = 1; j < (int) mc->operations.size()-1; j++) {
//			operation *op = mc->operations[j];
//			job *jb = soljobs[op->jobID];
//            myfile << op->UID << " " << jb->order->orderID << " " << op->jobID << " " << op->ID << " " << op->start << " " << op->end << endl;
//		}
//    }
//
//	myfile.close();
//}


void solution::update_horizon() {
	int newhorizonplan = 0;

	for (int i = 0; i < (int) soljobs.size(); i++) {
		job *jb = soljobs[i];

		for (int j = 0; j < (int) jb->operations.size(); j++) {
			operation *op = jb->operations[j];

			//Iterate in available processing times
			int maxproctime = 0;
			for (int k = 0; k < NumberOfMachines; k++) {
				#if(FLEX)
					maxproctime = max(maxproctime, op->getProcTime(solmachines[k]));
				#else
					maxproctime = max(maxproctime, op->processing_time);
				#endif
			}

			newhorizonplan += maxproctime;
		
		}
	}
	//Update planhorizon
	planhorend = newhorizonplan;
}


//Graph construction
void solution::createGraphs() {
	//This method returns a graph out of the current solution

	/*
		Mode controls the connection scheme
		Mode 0 is the backwards scheme, each operation is connected with its predecessors
		Mode 1 is the forwards scheme, each operation is connected with its successors

		Currently:
		Mode 0, is used for detecting cycle dependencies
		Mode 1, is used for calculating the affected operations after a modification
	*/

	//Create Graph
	Graph<operation> *g_f = new Graph<operation>( (int) this->totalNumOp + 2 * this->solmachines.size() );
	Graph<operation> *g_b = new Graph<operation>( (int) this->totalNumOp + 2 * this->solmachines.size() );
	
	//Populate nodes
	g_f->size = (int) this->totalNumOp + 2 * this->solmachines.size();
	g_b->size = (int) this->totalNumOp + 2 * this->solmachines.size();
	
	//Reserve Stuff
	//g->visitedNodes.reserve(this->totalNumOp);
	int op_counter = 0;

	//Init all nodes positions to null
	for (int i = 0; i < (int) this->soljobs.size(); i++) {
		job *jb = this->soljobs[i];
		//vector<Node*> *ar = &(g->Nodes)[i];
		
		//Add Nodes without Connections
		for (int j = 0; j < (int) jb->operations.size(); j++) {
			operation *op = jb->operations[j];
			op->valid = true;
			
			//Forward
			struct adj_list_elem<operation> node_f;
			for (int k=0;k <MAX_JOB_RELATIONS; k++)
				node_f.p_connections[k] = -MAX_JOB_RELATIONS;

            for (int k=0;k <MAX_JOB_RELATIONS; k++){
                if (op->suc[k] != NULL)
                    node_f.p_connections[k] = op->suc[k]->UID;
            }

			node_f.t_connection = -MAX_JOB_RELATIONS;
			node_f.op = op;

			g_f->adj_list[op->UID] = node_f;

            //Backward
			struct adj_list_elem<operation> node_b;
			for (int k = 0; k <MAX_JOB_RELATIONS; k++)
				node_b.p_connections[k] = -MAX_JOB_RELATIONS;

            for (int k = 0; k <MAX_JOB_RELATIONS; k++){
                if (op->pred[k] != NULL)
                    node_b.p_connections[k] = op->pred[k]->UID;
            }

			node_b.t_connection = -MAX_JOB_RELATIONS;
			node_b.op = op;

			g_b->adj_list[op->UID] = node_b;


			op_counter++;
		}

	}

	//Add dummy machine nodes
	for (int i=0; i<this->solmachines.size(); i++) {
		machine *mc = this->solmachines[i];
		int mc_ops = mc->operations.size();
		
		//Forward
		//First Op
		struct adj_list_elem<operation> node_f1;
		for (int k = 0; k <MAX_JOB_RELATIONS; k++)
			node_f1.p_connections[k] = -MAX_JOB_RELATIONS;
		node_f1.t_connection = -MAX_JOB_RELATIONS;
		node_f1.op = mc->operations[0];
		
		g_f->adj_list[node_f1.op->UID] = node_f1;

		//Last Op
		struct adj_list_elem<operation> node_f2;
		for (int k = 0; k <MAX_JOB_RELATIONS; k++)
			node_f2.p_connections[k] = -MAX_JOB_RELATIONS;
		node_f2.t_connection = -MAX_JOB_RELATIONS;
		node_f2.op = mc->operations[mc_ops - 1];
		
		g_f->adj_list[node_f2.op->UID] = node_f2;


        //Backward
		//First Op
		struct adj_list_elem<operation> node_b1;
		for (int k = 0; k <MAX_JOB_RELATIONS; k++)
			node_b1.p_connections[k] = -MAX_JOB_RELATIONS;
		node_b1.t_connection = -MAX_JOB_RELATIONS;
		node_b1.op = mc->operations[0];
		
		g_b->adj_list[node_b1.op->UID] = node_b1;

        //Last Op
		struct adj_list_elem<operation> node_b2;
		for (int k = 0; k <MAX_JOB_RELATIONS; k++)
			node_b2.p_connections[k] = -MAX_JOB_RELATIONS;
		node_b2.t_connection = -MAX_JOB_RELATIONS;
		node_b2.op = mc->operations[mc_ops - 1];
		
		g_b->adj_list[node_b2.op->UID] = node_b2;
		
	
		op_counter += 2;
	}


	//Insert Edges
	/*
	for (int i = 0; i < g_f->size; i++) {
		adj_list_elem<operation> node_f = g_f->adj_list[i];
		adj_list_elem<operation> node_b = g_b->adj_list[i];
		//Add perm edges
		for (int j = 0; j < MAX_JOB_RELATIONS; j++) {
			if (node_f.p_connections[j] == NULL) continue;
			else g_f->edges.insert(make_tuple(node_f.op->UID, node_f.p_connections[j]->UID, 0));

			if (node_b.p_connections[j] == NULL) continue;
			else g_b->edges.insert(make_tuple(node_b.op->UID, node_b.p_connections[j]->UID, 0));
		}
	}
	*/

	//Insert dummy node temp edges
	for (int i = 0; i < this->solmachines.size(); i++) {
		machine *mc = this->solmachines[i];
		int mc_ops = mc->operations.size();
		operation *op1 = mc->operations[0];
		operation *op2 = mc->operations[mc_ops - 1];

		//g_f->edges.insert(make_tuple(op1->UID, op2->UID, 1));
		g_f->adj_list[op1->UID].t_connection = op2->UID;
		//g_b->edges.insert(make_tuple(op2->UID, op1->UID, 1));
		g_b->adj_list[op2->UID].t_connection = op1->UID;
	}

	//FGraph->fptr = &operation::setMoving;
	//BGraph->fptr = &operation::setMoving;

	FGraph = g_f;
	BGraph = g_b;
}

void solution::resetSolution(){
	//Initiallize Jobhelp structure
	opshelp.clear();

	//Cleanup current solution
	for (int g = 0; g<soljobs.size(); g++)
	{
		job *jb = soljobs[g];
		//jobshelp.push_back(jb);
		for (int y = 0; y<(int) jb->operations.size(); y++)
		{
			operation *op = jb->operations[y];
			op->done = false;
			op->temp_done = false;
			op->start = op->a;
			//op->end = op->start + op->processing_time + op->cleanup_time + op->setup_time;
			op->end = op->start + op->cleanup_time + op->setup_time;
			op->temp_start = op->start;
			op->temp_end = op->end;

			opshelp.push_back(op);
		}
		for (uint y = 0; y<solmachines.size(); y++)
		{
			machine *mc = solmachines[y];
			while ((int) mc->operations.size() > 2) mc->operations.erase(mc->operations.begin() + 1);
		}
	}

	//Initiallize data structures for all resources
	Init_Resourse_Loads();
}

bool solution::greedyConstruction(bool test){

		/*
			This is a copy of the basic construction function
		*/

		//CleanUp Solution
		cleanUpSolution();
		Init_Resourse_Loads();

		//Construction
		int RCL_Size = max(3, rand() % 8);//Note: This can be defined outside as user defined parameter
		RCL_Size = 5;
		//cout << "Greedy Construction Method" << endl;
		//forwardGraph->report();

		//Construct solution
		int iter = 0;
		while (true)
		{
			//Find all noncompleted operations whose op_pred has been completed
			globals->RCL.clear();
			int gndex = -1;
			for (int y = 0; y < (int) opshelp.size(); y++)
			{
				operation *op = opshelp[y];

				//If op is completed continue to another op
				if (op->temp_done) continue;

				//If op_pred is not yet completed then break and move to another job;
				
				if (op->has_preds()) {
					operation *op_pred = op->pred[0];
					if (!op_pred->temp_done) continue;
				}
					
				globals->RCL.push_back(op);
				if ((int) globals->RCL.size() > RCL_Size)
				{
					int maxb = 0;
					int opb_thesi = -1;
					for (int r = 0; r<globals->RCL.size(); r++)
					{
						operation *opb = globals->RCL[r];
						if (opb->b > maxb) {
							maxb = opb->b;
							opb_thesi = r;
						}
					}
					//globals->RCL->RemoveAt(opb_thesi);
					globals->RCL.erase(globals->RCL.begin() + opb_thesi);
					if ((int) globals->RCL.size() > RCL_Size) {
						cout << "RCL Size warning!!!" << endl;
					}
				}
			}
			
			//Choose randomly one op
			if (globals->RCL.size() == 0) break;
			if (globals->RCL.size() > 1) gndex = rand() % globals->RCL.size();
			else gndex = 0;

			operation *op = globals->RCL[gndex];
            process_step *ps = ProcSteps[op->process_stepID];
			//cout << "Selecting ProcessStep: " << op->process_stepID << endl;

			bool mpike = false;
			int thesi = -1;
			int mindex = -1;
			int minim = INF;
			int op_start_temp = 0;
			int op_end_temp = 0;

			int start_t = 0;
			int end_t = 0;

			//For all available machines
			for (int m = 0; m < ps->machines.size(); m++)
			{
				machine *mc = ps->machines[m];
				op->mc = mc;//Set operation Machine temporarily

				if (op->getProcTime() == 0) continue;
				
				//For all insertion positions
				//Find best position and start time
				for (int y = 1; y<mc->operations.size(); y++)
				{
					operation *op1 = mc->operations[y - 1];
					operation *op2 = mc->operations[y];

					//Calculate earliest possible start time for op (if op is placed between op1 and op2)
					op->temp_start = op->a;

					//Check end time of op_pred
					if (op->ID != 0) {
						operation *op_pred = op->pred[0];
						if (op->temp_start < op_pred->end) op->temp_start = op_pred->end;
					}

					//Check end time of op1
					if (op->temp_start < op1->end) op->temp_start = op1->end;

					//Check if machine is available and (if not) find the earliest time that becomes available
					//Assumption: no preemption is allowed

					int mc_setup = op->setup_time;

#if(CHANGEOVERTIMES)
					mc_setup += changeovertime(mc, op1, op);
#endif

					int mc_cleanup = op->cleanup_time;
					op->temp_end = op->temp_start + mc_setup + op->getProcTime() + mc_cleanup;// mc->speed*op->processing_time

					while (true)
					{
						start_t = op->temp_start;
						end_t = op->temp_end;

#if(MACHINE_AVAILABILITIES)
						//Replace with one simple check for the machine
						mc->checkAvail(op);
						
#endif

#if(RESOURCE_CONSTRAINTS)
						int duration = mc_setup + op->processing_time_UM[mc->ID] + mc_cleanup;//mc->speed*op->processing_time
						int max_start = op->temp_start;
						int t = 0;
						while (t >= 0)
						{
							int y = 0;
							for (y = 0; y < duration; y++)
							{
								//Check all resources
								int r = 0;
								for (r = 0; r < p_solution->Resources.size(); ++r)
								{
									resource *rc = p_solution->Resources[r];
									if (mc->res_consum_processing[r] == 0) continue;
									if ((rc->res_load_t[max_start + y + t] - mc->mach_res_load_t[r][max_start + y + t] + mc->res_consum_processing[r]) > rc->max_level) break;
									if ((ps->proc_res_load_t[r][max_start + y + t] - mc->mach_res_load_t[r][max_start + y + t] + mc->res_consum_processing[r]) > ps->resalarm[r]) break;
								}
								if (r != p_solution->Resources.size()) break;
							}
							if (y == duration) break;
							else t += y + 1;
						}
						//Update start and end times
						op->temp_start = max_start + t;
						get<1>(op->end_times) = op->temp_start + mc_setup + op->getProcTime() + mc_cleanup;//mc->speed*op->processing_time
#endif

						if (!MACHINE_AVAILABILITIES) break;
						if (!RESOURCE_CONSTRAINTS) break;

						if (start_t == op->temp_start && end_t == op->temp_end) break;
					}

					//Check start_time of op2
					if (op->temp_end > op2->start)
					{
						//Calculate new start and end times for op2
						op2->temp_start = op->temp_end;
						if (op2->a > op2->temp_start) op2->temp_start = op2->a;

						mc_setup = op2->setup_time;
#if(CHANGEOVERTIMES)
						mc_setup += changeovertime(mc, op, op2);
#endif

						mc_cleanup = op2->cleanup_time;
						op2->temp_end = op2->temp_start + mc_setup + op2->getProcTime() + mc_cleanup;//mc->speed*op2->processing_time

						while (true)
						{
							start_t = op2->temp_start;
							end_t = op2->temp_end;

#if(MACHINE_AVAILABILITIES)
							//Check if machine is available and update start and end times
							mc->checkAvail(op2);
#endif

#if(RESOURCE_CONSTRAINTS)
							int duration = mc_setup + op2->getProcTime() + mc_cleanup;//mc->speed*op2->processing_time
							int max_start = op2->temp_start;
							int t = 0;
							while (t >= 0)
							{
								int y = 0;
								for (y = 0; y < duration; y++)
								{
									//Check all resources
									int r = 0;
									for (r = 0; r < p_solution->Resources.size(); ++r)
									{
										resource *rc = p_solution->Resources[r];
										if (mc->res_consum_processing[r] == 0) continue;
										if ((rc->res_load_t[max_start + y + t] - mc->mach_res_load_t[r][max_start + y + t] + mc->res_consum_processing[r]) > rc->max_level) break;
										if ((ps->proc_res_load_t[r][max_start + y + t] - mc->mach_res_load_t[r][max_start + y + t] + mc->res_consum_processing[r]) > ps->resalarm[r]) break;
									}
									if (r != p_solution->Resources.size()) break;
								}
								if (y == duration) break;
								else t += y + 1;
							}
							//Update start and end times
							op2->temp_start = max_start + t;
							op2->temp_end = op2->temp_start + mc_setup + op->processing_time_UM[mc->ID] + mc_cleanup;
#endif

							#if(!MACHINE_AVAILABILITIES)
								break;
							#endif
							#if(!RESOURCE_CONSTRAINTS)
								break;
							#endif

							if (start_t == op2->temp_start && end_t == op2->temp_end) break;
						}

						//Check succesor first
						if (op2->has_sucs())
						{
							operation *op2_suc = op2->suc[0];
							if (op2_suc->start < op2->temp_end) continue;
						}

						//Check due date
						if (op2->temp_end > op2->b) continue;

						//Check all remaining operations
						bool flag = false;
						int v = y + 1;
						for (v = y + 1; v<mc->operations.size(); v++)
						{
							operation *opi = mc->operations[v - 1];
							operation *opj = mc->operations[v];

							//Calculate new temp_start and end_tmep
							opj->temp_start = opi->temp_end;
							if (opj->a > opj->temp_start) opj->temp_start = opj->a;

#if(UNRELATED_MACHINES)
							mc_setup = opj->setup_time;
#if(CHANGEOVERTIMES)
							mc_setup += changeovertime(mc, opi, opj);
#endif

							mc_cleanup = opj->cleanup_time;
							opj->temp_end = opj->temp_start + mc_setup + opj->getProcTime() + mc_cleanup; //mc->speed*opj->processing_time

							while (true) {

								start_t = opj->temp_start;
								end_t = opj->temp_end;

#if(MACHINE_AVAILABILITIES)
								//Check if machine is available and update start and end temp times
								mc->checkAvail(op);
#endif

#if(RESOURCE_CONSTRAINTS)
								int duration = mc_setup + opj->getProcTime() + mc_cleanup;//mc->speed*opj->processing_time
								int max_start = opj->temp_start;
								int t = 0;
								while (t >= 0)
								{
									int y = 0;
									for (y = 0; y < duration; y++)
									{
										//Check all resources
										int r = 0;
										for (r = 0; r < p_solution->Resources.size(); ++r)
										{
											resource *rc = p_solution->Resources[r];
											if (mc->res_consum_processing[r] == 0) continue;
											if ((rc->res_load_t[max_start + y + t] - mc->mach_res_load_t[r][max_start + y + t] + mc->res_consum_processing[r]) > rc->max_level) break;
											if ((ps->proc_res_load_t[r][max_start + y + t] - mc->mach_res_load_t[r][max_start + y + t] + mc->res_consum_processing[r]) > ps->resalarm[r]) break;
										}
										if (r != p_solution->Resources.size()) break;
									}
									if (y == duration) break;
									else t += y + 1;
								}
								//Update start and end times
								opj->temp_start = max_start + t;
								opj->temp_end = opj->temp_start + mc_setup + opj->getProcTime() + mc_cleanup; //mc->speed*opj->processing_time
#endif

								if (!MACHINE_AVAILABILITIES) break;
								if (!RESOURCE_CONSTRAINTS) break;

								if (start_t == opj->temp_start && end_t == opj->temp_end) break;
							}
#endif

							if (opj->temp_start > opj->start)
							{
								//Check successor
								if (opj->has_sucs())
								{
									operation *opj_suc = opj->suc[0];
									if (opj_suc->start < opj->temp_end) break;
								}
								//Check due date
								if (opj->temp_end > opj->b) break;
							}
							else flag = true;
						}
						if (v != mc->operations.size() - 1 && !flag) continue; //Here reset the op->mc to null if necessary

						if (op->temp_end < minim)
						{
							mpike = true;
							thesi = y;
							mindex = m;
							op_start_temp = op->temp_start;
							op_end_temp = op->temp_end;
							minim = op->temp_end;
						}
					}
					if (op->temp_end < minim)
					{
						mpike = false;
						thesi = y;
						mindex = m;
						op_start_temp = op->temp_start;
						op_end_temp = op->temp_end;
						minim = op->temp_end;
					}
				}
			}

			if (thesi == -1) break;

			machine *mctest = ps->machines[mindex];
			if (thesi <= 0 || thesi >= (int) mctest->operations.size())
			{
				cout << "Construction Error" << endl;
				cout << mctest->operations.size() << " " << thesi << endl;
				return false;
			}

			//Schedule op at mc
			machine *mc = ps->machines[mindex];
			//printf("Scheduling Operation %d %d Machine %d Pos %d ...", op->jobID, op->ID, mc->ID, thesi);
			//mc->operations->InsertAt(thesi, op);

			
			//mc->operations.insert(mc->operations.begin() + thesi, op);
			//op->mc = mc;
			//op->machID = mc->ID;
			//get<1>(op->machID) = mc->ID;
			//op->mthesi = thesi;
			//op->temp_mthesi = thesi;
			op->changeMach(mc, thesi);
			vInsertAt<operation*>(mc->operations, thesi, op);

			op->start = op_start_temp;
			op->end = op_end_temp;
			op->temp_start = op->start;
			op->temp_end = op->end;
			//op->temp_done = true;
			//op->done = true;
			

			//insertOp(mc, thesi, op); //Insert to Solution and Machine
			op->setDoneStatus(false, true); //Fix Status
			//cout <<"Operation " << op->jobID << " " << op->ID << " Scheduled" << endl;
			//cout << *this;
			insertOpToGraphs(op);
			//forwardGraph->report();
			
			//Insert operation to graphs
			
			//this->insertOpToGraph(op, 0);
			//this->insertOpToGraph(op, 1);
			//printf("Done. \n");
			
			for (int v = thesi; v<mc->operations.size() - 1; v++)
			{
				operation *opi = mc->operations[v];
				operation *opj = mc->operations[v + 1];

				//Update positions
				opi->mthesi = v;
				opi->temp_mthesi = v;
				opj->mthesi = v + 1;
				opj->temp_mthesi = v + 1;
			}
			

			//Update succesors on the same machine
			if (mpike)
			{
				for (int v = thesi; v<mc->operations.size() - 1; v++)
				{
					operation *opi = mc->operations[v];
					operation *opj = mc->operations[v + 1];

					//Calculate new temp_start and end_tmep
					opj->temp_start = opi->end;
					if (opj->a > opj->temp_start) opj->temp_start = opj->a;

#if(UNRELATED_MACHINES)
					int mc_setup = opj->setup_time;
#if(CHANGEOVERTIMES)
					mc_setup += changeovertime(mc, opi, opj);
#endif

					int mc_cleanup = opj->cleanup_time;
					opj->temp_end = opj->temp_start + mc_setup + opj->getProcTime() + mc_cleanup;//mc->speed*opj->processing_time

					while (true) {

						start_t = opj->temp_start;
						end_t = opj->temp_end;

#if(MACHINE_AVAILABILITIES)
						//Check if machine is available and update start and end temp times
						mc->checkAvail(op);
#endif 

#if(RESOURCE_CONSTRAINTS)
						int duration = mc_setup + opj->getProcTime() + mc_cleanup;//mc->speed*opj->processing_time
						int max_start = opj->temp_start;
						int t = 0;
						while (t >= 0)
						{
							int y = 0;
							for (y = 0; y < duration; y++)
							{
								//Check all resources
								int r = 0;
								for (r = 0; r < p_solution->Resources.size(); ++r)
								{
									resource *rc = p_solution->Resources[r];
									if (mc->res_consum_processing[r] == 0) continue;
									if ((rc->res_load_t[max_start + y + t] - mc->mach_res_load_t[r][max_start + y + t] + mc->res_consum_processing[r]) > rc->max_level) break;
									if ((ps->proc_res_load_t[r][max_start + y + t] - mc->mach_res_load_t[r][max_start + y + t] + mc->res_consum_processing[r]) > ps->resalarm[r]) break;
								}
								if (r != p_solution->Resources.size()) break;
							}
							if (y == duration) break;
							else t += y + 1;
						}
						//Update start and end times
						opj->temp_start = max_start + t;
						opj->temp_end = opj->temp_start + mc_setup + opj->getProcTime() + mc_cleanup;//mc->speed*opj->processing_time
#endif

						if (!MACHINE_AVAILABILITIES) break;
						if (!RESOURCE_CONSTRAINTS) break;

						if (start_t == opj->temp_start && end_t == opj->temp_end) break;
					}
#endif

					if (opj->start < opj->temp_start)
					{
						opj->start = opj->temp_start;
						opj->end = opj->temp_end;
					}
					else break;
				}
			}
			iter++;

			//Test Printing Intermediate Constructed Solution


			//Update data structures for all resources
			Calculate_Resourse_Loads();

			//for(int y=0;y<machines->GetSize();y++) cout << (machine*)(*machines)[y];
			//cout << *p_solution;
			//cout << "Iter: " << iter << " MachID: " << mc->ID << " OpID: " << op->ID << " JobID: " << op->jobID << " Thesi: " << thesi << endl;
		}

		//cout << "Temporary Solution" << endl;
		//cout << *this;

		if (test){
			if (!scheduleAEAP(false, true))
			{
				cout << "Infeasible Solution Construction! - Check point" << endl;
				for (int y = 0; y<solmachines.size(); y++) cout << solmachines[y];
			}

			if (!testsolution())
			{
				cout << "Infeasible Solution Construction!" << endl;
				return false;
			}
		}
		
		getCost(false);
		//cout << "Cost: " << cost << " " << s_cost << " " << t_cost << " " << e_cost << " " << tard_cost << " " << slack <<endl;
		return true;
	
}


void solution::freezeSchedule(long freezetime){
    //This method assumes that there is an existing constructed schedule already
    //We query the operation final start/end times and we find if we have to split them or not.
    //Job quantities and therefore operation processing times should be modified
    //This method also assumes that there is only one operation per job since this would make things more
    //difficult in terms to fix the job.

    vector<job*> jobs_to_remove = vector<job*>();

    for (int i=0; i<soljobs.size(); i++){
        job *jb = soljobs[i];

        int old_lotsize = jb->lotSize;
        int new_lotsize = 0; //to be calculated

        //Update job release time
        jb->rel = max(jb->rel, freezetime);

        if (jb->operations.size() > 1){
            printf("No more than 1 operation per job are allowed in this method\n");
            assert(false);
        }

        for (int j=0; j<jb->operations.size();j++){
            operation *op = jb->operations[j];

            //Check if we can ommit the operation completely
            if (freezetime > op->end){
                //jb->operations.erase(jb->operations.begin());
                jobs_to_remove.push_back(jb);
                printf("Job %d will be deleted\n", jb->ID);
                continue;
            }

            //Check start, end times
            bool fix_flag = freezetime < op->end && freezetime > op->start;

            if (!fix_flag)
                continue;

            //Update release time
            op->a = max(op->a , freezetime);

            //Fix operation processing times
            //First of all remove the lot_size multiplier
            op->const_processing_time_UM = (int) op->const_processing_time_UM / old_lotsize;
            for (int k = 0; k < exMACHINETYPECOUNT; k++) {
                op->var_processing_time_UM[k] = (int) op->var_processing_time_UM[k] / old_lotsize;
            }

            //Sum the raw processing times
            int total_old_proc_time =  op->const_processing_time_UM;
            for (int k = 0; k < exMACHINETYPECOUNT; k++) {
                total_old_proc_time += op->var_processing_time_UM[k];
            }

            //Identify lot_size required to reach the freezetime

            //Report operation's resource
            //op->prod_res->export_to_txt("test_resource.txt");

            long produced_lotsize = (freezetime - op->start) / total_old_proc_time;
            int produced_lotsize_new = op->prod_res->getQuantityToTime(op->start, freezetime);

            new_lotsize = old_lotsize - produced_lotsize_new;

            if (new_lotsize < 0)
                assert(false);

            //Set new operation processing times
            op->const_processing_time_UM *= new_lotsize;
            for (int k = 0; k < exMACHINETYPECOUNT; k++) {
                op->var_processing_time_UM[k] *= new_lotsize;
            }

            //Set new lot variables to the job
            jb->lotFactor = (float) BlackBoxSize / (float) new_lotsize;
            jb->lotSize = new_lotsize;

            printf("Lot Size of Job %d Reduced to %d\n",
                    jb->ID, jb->lotSize);
            break;
        }
    }


    /* METHOD A : DELETE JOBS AND OPERATIONS

    //Remove jobs
    while (jobs_to_remove.size()){
        job *cand_jb = jobs_to_remove[0];
        jobs_to_remove.erase(jobs_to_remove.begin());

        for (int i=0; i < (int) soljobs.size(); i++){
            job *jb = soljobs[i];

            if (jb->actualID == cand_jb->actualID){
                NumberOfJobs -= 1;

                //Delete related operations
                for (int j=0; j<jb->operations.size(); j++){
                    operation *op = jb->operations[j];

                    for (int k=0;k<soloperations.size();k++){
                        operation *cand_op = soloperations[k];
                        if (cand_op->UID == op->UID){
                            soloperations.erase(soloperations.begin() + k);
                            delete cand_op;
                            break;
                        }
                    }
                }

                //Fix jobIDs for all following jobs and their operations
                for (int j=i+1; j < (int) soljobs.size(); j++){
                    job *next_jb = soljobs[j];
                    next_jb->ID -= 1;
                    for (int k=0; k < (int) next_jb->operations.size(); k++){
                        next_jb->operations[k]->jobID -= 1;
                    }
                }


                //Add deleted quantity to the product inventory
                ProductionOrder *jb_po = this->dataset->orderIDMap.at(jb->orderID);
                if (dataset->productInventory.find(jb_po->productName) != dataset->productInventory.end())
                    this->dataset->productInventory[jb_po->productName] += jb->lotSize;
                else
                    this->dataset->productInventory[jb_po->productName] = jb->lotSize;

                printf("Job %d Deleted\n", cand_jb->actualID);
                soljobs.erase(soljobs.begin() + i);
                delete cand_jb;
                break;
            }
        }
    }

    resetOperationUIDs();

    //Count the number of the actual operations
    GetCounts();

    //Re-initialize Graphs
    delete FGraph;
    delete BGraph;
    createGraphs();

    */


    // METHOD B: Simply set default values for the frozen operations
    for (int i=0;i<jobs_to_remove.size();i++){
        job *jb = jobs_to_remove[i];

        for (int j=0;j<jb->operations.size();j++){
            operation *op = jb->operations[j];
            op->fixed = true;
        }

    }

    //Reconstruct base schedule
    constructBaseSchedule();
}


void solution::splitJobs(event* e){
    //This method assumes that there is an existing constructed schedule already
    //We query the operation final start/end times and we find if we have to split them or not.
    //Job quantities and therefore operation processing times should be modified
    //This method also assumes that there is only one operation per job since this would make things more
    //difficult in terms to fix the job.

    //Based on the rescheduling time split the necessary jobs
    int reschedulingTimeSeconds = (int) difftime(timegm(&e->trigger_time), timegm(&PlanningHorizonStart));

    //Find target line ID
    machine *target_mc = nullptr;
    int target_line_id = -1;
    for (int j=0; j<this->solmachines.size(); j++){
        machine *mc = this->solmachines[j];
        if (std::find(mc->embeddedMachineIDs.begin(), mc->embeddedMachineIDs.end(), e->target) != mc->embeddedMachineIDs.end()){
            target_mc = mc;
            target_line_id = mc->line->ID;
            break;
        }
    }

    if (target_line_id == -1){
        printf("Unable to find affected Line");
        return;
    }

    vector<job*> jobs_to_add = vector<job*>();

    for (int i=0; i<soljobs.size(); i++){
        job *jb = soljobs[i];

        int old_lotsize = jb->lotSize;

        if (jb->operations.size() > 1){
            printf("No more than 1 operation per job are allowed in this method\n");
            assert(false);
        }

        for (int j=0; j<jb->operations.size();j++){
            operation *op = jb->operations[j];

            //Check if we can ommit the operation completely
            if (reschedulingTimeSeconds > op->end){
                //Operation and Job are not affected
                continue;
            }

            if (op->mc->line->ID != target_line_id)
                continue;

            //Check start, end times
            bool fix_flag = reschedulingTimeSeconds < op->end && reschedulingTimeSeconds > op->start;

            if (!fix_flag)
                continue;

            //Identify lot_size required to reach the freezetime

            //Sum the raw processing times
            int total_old_proc_time =  op->getProcTime();

            long produced_lotsize = (reschedulingTimeSeconds - op->start) / total_old_proc_time;
            int produced_lotsize_new = op->prod_res->getQuantityToTime(op->start, reschedulingTimeSeconds);
            int remaining_lotsize = jb->lotSize - produced_lotsize_new;

            //Check if there is nothing to split
            if (remaining_lotsize == old_lotsize)
                continue;

            //Generate new job
            job *new_jb = new job();
            *new_jb = *jb;

            operation *new_op = new operation();
            *new_op = *op;
            new_jb->operations.push_back(new_op);

            //Fix operation processing times
            //First of all remove the lot_size multiplier
            op->const_processing_time_UM = (int) op->const_processing_time_UM / old_lotsize;
            new_op->const_processing_time_UM = op->const_processing_time_UM;
            for (int k = 0; k < exMACHINETYPECOUNT; k++) {
                op->var_processing_time_UM[k] = (int) op->var_processing_time_UM[k] / old_lotsize;
                new_op->var_processing_time_UM[k] = op->var_processing_time_UM[k];
            }

            if (produced_lotsize_new < 0)
                assert(false);

            //Set new operation processing times
            op->const_processing_time_UM *= produced_lotsize_new;
            new_op->const_processing_time_UM *= remaining_lotsize;
            for (int k = 0; k < exMACHINETYPECOUNT; k++) {
                op->var_processing_time_UM[k] *= produced_lotsize_new;
                new_op->var_processing_time_UM[k] *= remaining_lotsize;
            }

            //Set new lot variables to the jobs
            jb->lotFactor = (float) BlackBoxSize / (float) produced_lotsize_new;
            jb->lotSize = produced_lotsize_new;
            jb->quantity = produced_lotsize_new;

            new_jb->lotFactor = (float) BlackBoxSize / (float) remaining_lotsize;
            new_jb->lotSize = remaining_lotsize;
            new_jb->quantity = remaining_lotsize;

            printf("Lot Size of Job %d Reduced to %d\n",
                   jb->ID, jb->lotSize);

            printf("Lot Size of generated Job %d : %d \n",
                   new_jb->ID, new_jb->lotSize);

            jobs_to_add.push_back(new_jb);
            break;
        }
    }
    //Number of Jobs has been fixed above

    while(jobs_to_add.size()){
        job *cand_jb = jobs_to_add[0];
        jobs_to_add.erase(jobs_to_add.begin());

        cand_jb->ID = NumberOfJobs;
        cand_jb->actualID = NumberOfJobs;
        NumberOfJobs++;

        cand_jb->operations[0]->jobID = cand_jb->ID;

        //Insert Job to Solution
        soljobs.push_back(cand_jb);

        //Insert new operation to Solution
        soloperations.insert(soloperations.begin() + soloperations.size() - solmachines.size() * 2,
                cand_jb->operations[0]);

    }

    resetOperationUIDs();

    //Count the number of the actual operations
    GetCounts();

    //Re-initialize Graphs
    delete FGraph;
    delete BGraph;
    createGraphs();

    //Reconstruct base schedule
    constructBaseSchedule();
}


void solution::resetOperationUIDs(){
    for (int i=0;i<soloperations.size();i++)
        soloperations[i]->UID = i;

    //Reset NumberOf Operations
    NumberOfOperations = soloperations.size();
}


//Init Solution Construction Methods
bool solution::constructBaseSchedule(){
    cleanUpSolution(); //This method reset the operation stats and populates the opshelp

    //Sort opshelp based on the release time of the operations
    sort( opshelp.begin(), opshelp.end(), [ ]( const operation* lhs, const operation* rhs )
    {
        return lhs->a < rhs->a;
    });

    //Sort opshelp based on the order id
    sort( opshelp.begin(), opshelp.end(), [ & ]( const operation* lhs, const operation* rhs )
    {
        job *jb_lhs = soljobs[lhs->jobID];
        job *jb_rhs = soljobs[rhs->jobID];
        ProductionOrder *po_lhs = dataset->orderIDMap.at(jb_lhs->orderID);
        ProductionOrder *po_rhs = dataset->orderIDMap.at(jb_rhs->orderID);

        return po_lhs->orderID < po_rhs->orderID;
    });

    //Sort opshelp based on the type of the produced item (schedule 112 codes first then 111 then 110)
    sort( opshelp.begin(), opshelp.end(), [ & ]( const operation* lhs, const operation* rhs )
    {
        job *jb_lhs = soljobs[lhs->jobID];
        job *jb_rhs = soljobs[rhs->jobID];
        ProductionOrder *po_lhs = dataset->orderIDMap.at(jb_lhs->orderID);
        ProductionOrder *po_rhs = dataset->orderIDMap.at(jb_rhs->orderID);
        BOMEntry *bome_lhs = dataset->BOM.at(po_lhs->productName);
        BOMEntry *bome_rhs = dataset->BOM.at(po_rhs->productName);

        return bome_lhs->code_type < bome_rhs->code_type;
    });


    /*
    //Report the cached operations
    for (int i=0;i<p_solution->opshelp.size();i++){
        operation *op = p_solution->opshelp[i];

        printf("Operation %d Release Time %d Default Line %d\n",
                op->UID, op->a, p_solution->soljobs[op->jobID]->order->productionLine);
    }
     --LOOKING GOOD--*/

    while (opshelp.size()){
        operation *op = opshelp[0]; //Fetch first operation

        //Try to schedule this operation on its default line
        job *jb = soljobs[op->jobID];
        ProductionOrder *po = dataset->orderIDMap.at(jb->orderID);
        int default_line = po->productionLine;
        ProductionLine *pl = dataset->productionLineIDMap.at(default_line);

        //Fetch the BOMEntry for the order
        BOMEntry *productBOM = dataset->BOM.at(po->productName);

        //Find the corresponding machine
        machine *probe_mc = nullptr;

        for (int i=0; i<solmachines.size(); i++){
            machine *mc = solmachines[i];
            if (mc->line->ID == default_line){
                probe_mc = mc;
                break;
            }
        }

        //This should not happen but I'm leaving this check here just in case
        if (probe_mc == nullptr){
            printf("Unable to find default machine for Operation [%d-%d] - Order %d\n",
                   op->jobID, op->ID, po->orderID);

            opshelp.erase(opshelp.begin());
            continue;
        }

        printf("Found Machine %d for Operation [%d-%d] - Order %d\n",
               probe_mc->ID, op->jobID, op->ID, po->orderID);


        op->default_machID = probe_mc->ID;

        //Schedule operation in the machine
        op->resetDone();
        //Insert before the machine's last dummy operation
        insertOp(probe_mc, probe_mc->operations.size() - 1, op);
        //Refix times
        //this->resetDone();
        //bool fixStatus = this->fixTimes(); //Myway
        bool fixStatus = this->scheduleAEAP(true, true);

        //TODO add a failsafe mechanism here to save the game if something happens (export input orders as is)

        if (!fixStatus){
            cout << "final_op insertion : SKATAAAAAAAAAAAAAAAAAAAA" << endl;
            cout << *this;
        }

        //Print Temporary solution
        //cout << *this;

        //Remove Operation from the temporary list
        opshelp.erase(opshelp.begin());
    }

    //Calculate Graphs and all extra shit

    //Solution should be ok, storing the temp values
    this->replace_real_values();
    this->Calculate_Resourse_Loads();

    //this->reportResources();
    //const char* outputfile = "temp_output.txt";
    //this->saveToFile(outputfile);

    //cout << *this->Resources[0];

    cout << "Storing solution attributes" << endl;
    if (!this->testsolution())
    {
        cout << "Infeasible Solution AEAP Construction!" << endl;
        cout << *this;
        return false;
    }

    this->getCost(false);

    //Calculate Critical Paths/Blocks/TopSort
    findCriticalPath_viaSlack();
    findCriticalBlocks(1);
    FGraph->topological_sort();

    //Resource Calculation Testing
    //this->Calculate_Resourse_Loads();

    //cout << *this->Resources[0];

    /*
    //Testing
    this->FGraph->topological_sort();

    //Report TSORT
    cout << *this;
    printf("TSort ");
    for (int i = 0; i < this->FGraph->top_sort.size();i++) {
        operation *cand = this->FGraph->top_sort[i];
        printf("%d ", cand->UID);
    }
    printf("\n");
    */

    cout << *this; //Report Solution for debugging purposes

    return true;

}


bool solution::aeapConstruction(int mode, vector<bl_pair> &blacklist, int objective) {
		/*
			AEAP Construction is supposed to assemble a randomized solution for the problem
			using a greedy insertion algorithm withou t calculating operation times.
			The goal is to check precedence conditions and then ScheduleAEAP will take
			care of the times.
		*/

		/*  DIFFERENCES

		Compared with construction() this method tries to fix on every insert, all times of all
		operations of all the jobs of the problem. Construction does not progress if there is an infeasibility
		with predecessor operations. This function checks for all types of infeasibilities, even cyclic ones
		and does not proceed only if there is an infeasibility that can be solved only with an operation rescheduling.
			
			Configuration and Calibration

			CAND_Size controls the amount of candidates that are going to be saved while probing
			CAND_Partition controls the partition of candidates available for selection out of the CAND Size

			mode controls the quality of solution produced

			New type of function

			Usually CAND Size of 10 guarantees good quality solutions.
			Setting just the partition size can move the solution towards good or bad solutions

			Usually settings near CAND_Size/2 trigger the best possible results.
			Values < CAND_Size/2 output much worse results 
			Calues > CAND_Size/2 output slightly worse results 
			
			//CAlibration Example for TA1 Instance
			  0	Params: |S 10 - P  1| Makespan: Avg: 5882.44 Min: 4067.00 Max: 7030.00 Score: 5748.86
			  1 Params: |S 10 - P  2| Makespan: Avg: 1679.84 Min: 1452.00 Max: 2089.00 Score: 1716.10
			  2 Params: |S 10 - P  3| Makespan: Avg: 1589.98 Min: 1426.00 Max: 1862.00 Score: 1611.59
			  3 Params: |S 10 - P  4| Makespan: Avg: 1584.82 Min: 1435.00 Max: 1828.00 Score: 1603.49
			  4 Params: |S 10 - P  5| Makespan: Avg: 1532.00 Min: 1349.00 Max: 1729.00 Score: 1534.80
			  5 Params: |S 10 - P  6| Makespan: Avg: 1526.66 Min: 1398.00 Max: 1704.00 Score: 1536.40
			  6 Params: |S 10 - P  7| Makespan: Avg: 1548.56 Min: 1396.00 Max: 1709.00 Score: 1550.14
			  7 Params: |S 10 - P  8| Makespan: Avg: 1549.94 Min: 1383.00 Max: 1756.00 Score: 1557.76
			  8 Params: |S 10 - P  9| Makespan: Avg: 1552.16 Min: 1382.00 Max: 1707.00 Score: 1549.10
			  9 Params: |S 10 - P 10| Makespan: Avg: 1633.00 Min: 1633.00 Max: 1633.00 Score: 1633.00
		

			According to that, mode can be set to 0,1,2. 
			0: sets optimal setting, 5
			1: sets to avg setting, 3 or 7
			2: sets to bad setting, 2

		*/
		int CAND_Size;
		int CAND_partition;
		switch (mode) {
			case 0:
				CAND_Size = 10;
				CAND_partition = 5;
				break;
			case 1:
				CAND_Size = 20;
				CAND_partition = 10;
				break;
			case 2:
				CAND_Size = 40;
				CAND_partition = 9;
				break;
			case 3:
				CAND_Size = 30;
				CAND_partition = 7;
				break;
			case 4:
				CAND_Size = 20;
				CAND_partition = 3;
				break;
		}

		clock_t start;
		bool check_blacklist = true;
		//Construction
		//int CAND_Size = 20;
		int op_num = totalNumOp;
		Graph<operation> *gr = FGraph;
		
		//int CAND_Size = 40;
		//int CAND_Size = max(10, op_num/2);
		//cout << "AEAP Construction Heuristic" << endl;

		//If there is a blacklist provided completely override the opshelp
		if (blacklist.size()) {
			opshelp.clear();
			for (int i = 0; i < blacklist.size(); i++) {
				bl_pair bop = blacklist[i];
				operation *op = soljobs[bop.first]->operations[bop.second];
				opshelp.push_back(op);
			}
		} else {
			//Cleanup current solution
			this->cleanUpSolution();
		}

		//TODO: TAKE NOTICE OF THE RESOURCE LOADS ONCE WE MAKE THEM WORK AS WELL
		//Initiallize data structures for all resources
		this->Init_Resourse_Loads();

		//Init CandidateList
		vector<Candidate*> candidatelist = vector<Candidate*>();

		//Construct solution
		int iter = 0;
		int evaluated_candidates = 0;

        //Shuffle opshelp
        shuffle(opshelp.begin(), opshelp.end(),std::default_random_engine {});


        //Sort opshelp based on the type of the produced item (schedule 112 codes first then 111 then 110)
        sort( opshelp.begin(), opshelp.end(), [ & ]( const operation* lhs, const operation* rhs )
        {
            job *jb_lhs = soljobs[lhs->jobID];
            job *jb_rhs = soljobs[rhs->jobID];
            ProductionOrder *po_lhs = dataset->orderIDMap.at(jb_lhs->orderID);
            ProductionOrder *po_rhs = dataset->orderIDMap.at(jb_rhs->orderID);
            BOMEntry *bome_lhs = dataset->BOM.at(po_lhs->productName);
            BOMEntry *bome_rhs = dataset->BOM.at(po_rhs->productName);

            return bome_lhs->code_type < bome_rhs->code_type;
        });


        //Preprocess opshelp and remove fixed operations
        int fixed_op_counter = 0;
        int opshelp_index = 0;
        while (fixed_op_counter < fixed_ops.size()){
            operation *op = opshelp[opshelp_index];
            if (op->fixed){
                opshelp.erase(opshelp.begin() + opshelp_index);
                fixed_op_counter++;
                continue;
            } else
                opshelp_index++;
        }

        //First schedule fixed operations

        while (fixed_ops.size()){
            operation *op = fixed_ops[0];
            machine *default_mc = solmachines[op->default_machID];

            op->resetDone();
            insertOp(default_mc, default_mc->operations.size() - 1, op);

            bool fixStatus = this->scheduleAEAP(true, true);

            if (!fixStatus)
                cout << "final_op insertion : SKATAAAAAAAAAAAAAAAAAAAA" << endl;

            printf("Scheduled Fixed Operation %d Start %ld End %ld\n",
                   op->UID, op->temp_start, op->temp_end);

        }



        while (opshelp.size())
		{



            //Find all noncompleted operations whose op_pred has been completed
			

			//Get the uperbound of available positions
			// int up_bound = 0;
			// for (int i=0;i < (int) this->solmachines.size();i++){
			// 	up_bound += (int) this->solmachines[i]->operations.size() - 1;
			// }
			
			//int CAND_Size = up_bound;
			//int CAND_Size = 5;
			//Use CAND_Size from Input
			
			int gndex = -1;
			int final_mach_pos = -1;
			machine *final_mach = NULL;
			operation *final_op = NULL;
			bool has_preds = false;
			evaluated_candidates = 0; //Keep track of the evaluated candidates per operation

			//cout << *this;
			printf("Remaining Operations %d\n", opshelp.size());
			for (int y = 0; y < (int) this->opshelp.size(); y++)
			{
				//Selecting op here
				operation *op = this->opshelp[y];

                if (op->temp_done || op->done) continue; //Check if done

				//Check if pred done
				if (op->has_preds())
				{
                    has_preds = true;
                    operation *op_pred = op->pred[0];
					if (!op_pred->temp_done) continue;
                }

                //Resource constraint feasibility check
                job *op_job = soljobs[op->jobID];
                ProductionOrder *po = dataset->orderIDMap.at(op_job->orderID);
                BOMEntry *be = dataset->BOM.at(po->productName);
                if (be->code_type != 0 && be->requirement != "NONE") {
                    //Make sure that the required quantity is there
                    resource *rc_req = Resources[be->requirement];

                    if (rc_req->end_q < op_job->lotSize) {
                        //printf("Not Enough Resource quantity for operation %d, Order ID %d, Product %s\n",
                        //       op->UID, po->orderID, po->productName.c_str());
                        continue;
                    }
                }



                //printf("Working on Op %d\n", op->UID);
                //if (op->jobID == 9 && op->ID == 1)
                //    printf("break\n");

				//Now iterate on machines
                process_step *ps = this->ProcSteps[op->process_stepID];

                //Find the best machine to schedule the op
				op->resetDone();
				int span = INF;
				this->getCost(objective, true);
				
				float prespan = this->temp_cost_array[objective];
				for (int m = 0; m <(int) ps->machines.size(); m++)
				{
					machine *mc = ps->machines[m];

					if (op->getProcTime(mc) == 0) continue;

                    //cout << "Trying to Schedule: " << " JOB: " << op->jobID << " OP ID: "  << op->ID << " at Mach: " << mc->ID << " Available Positions: " << mc->operations.size() - 1<< endl;

					/*
						Operations can be added in any position at the machine
					*/

					for (int mach_pos = 1; mach_pos < (int) mc->operations.size(); mach_pos++) {
                        bool psFound = false;
                        //Check for fixed successors before insert pos in case of rescheduling
                        if (executionMode == 1) {
                            operation *mach_suc = mc->operations[mach_pos];
                            //Operation positioned in an older point in time. Remove and proceed to a next location
                            if (mach_suc->fixed)
                                continue;
                        }

                        //insert operation to solution
                        this->insertOp(mc, mach_pos, op);
                        //printf("Inserted Operation %d %d \n", op->jobID, op->ID);
                        /*printf("After Insertion Report\n");
                        printf("FORWARD\n");
                        gr->state = FORWARD;
                        gr->report();
                        printf("BACKWARD\n");
                        gr->state = BACKWARD;
                        gr->report();
                        */

                        //Check for cyclic scheduling infeasibilities after insertion
                        gr->dfs(op->UID);

                        if (gr->cycleflag) {
                            removeOp(mc, mach_pos);
                            continue; //If there is an infeasibility continue to next position
                        }

                        //If precedence checks pass I can schedule the op in mc
                        //and keep track of its start time

                        //Fix times
                        setDoneStatus(true, false); //Reset operation status
                        //In case of rescheduling save

                        //start = clock();

                        bool fixStatus = scheduleAEAP(true, true);

                        getCost(objective, true);
                        //printf("TESTING\n");
                        //cout << *this;
                        float current_span = temp_cost_array[objective];
                        //int op_endtime = op->getEndTime();
                        //int op_starttime = op->getStartTime();

                        //this->resetDone();
                        //op->fixTimes();
                        //Remove Op
                        removeOp(mc, mach_pos);

                        //cout << *this;
                        if (!fixStatus) {
                            //Refix to previous state
                            cout << "MALAKIES SE AEAP CONSTRUCTION" << endl;
                            assert(false);
                        }

                        //CANDIDATE SORTING AND SAVING
                        {
                            //int endtime = abs(current_span - prespan);
                            float endtime = current_span;
                            //int endtime = this->calculate_makespan(0, false);
                            //int endtime = op->getEndTime(); //TERRIBLE RESULTS
                            //int endtime = op->getProcTime();
                            //cout << "endtime: " << endtime << endl;

                            //cout << "Checking Position: " << mach_pos <<" Start Time: " << op->getStartTime() << " End Time: " << op->getEndTime() << endl;

                            Candidate *cand = new Candidate();
                            //Populate Candidate
                            cand->op = op;
                            cand->mc = mc;
                            cand->pos = mach_pos;
                            cand->span = endtime;
                            cand->h_index = y;
                            evaluated_candidates++;
                            //Store into the CandidateList according to span
                            int c_pos = 0;
                            for (int i = 0; i < candidatelist.size(); i++) {
                                Candidate *c = candidatelist[i];
                                //if ((cand->span < c->span) || ((cand->span == c->span) && (cand->mc->operations.size() < c->mc->operations.size()))) {
                                if ((cand->span < c->span)) {
                                    break;
                                } else c_pos += 1;
                            }
                            //Insert to found position
                            vInsertAt<Candidate *>(candidatelist, c_pos, cand);

                            //Pop extra candidate
                            if ((int) candidatelist.size() > CAND_Size) {
                                Candidate *cand = candidatelist[CAND_Size - 1];
                                //cand->~Candidate(); Not sure why I'm using this one
                                delete cand;
                                vRemoveAt<Candidate *>(candidatelist, CAND_Size - 1);
                            }

                            if (evaluated_candidates > 2*CAND_Size)
                                break; //Limit the number of evaluated candidates

                        }

                        if (evaluated_candidates > 2*CAND_Size)
                            break; //Limit the number of evaluated candidates
                    }
                    if (evaluated_candidates > 2*CAND_Size)
                        break; //Limit the number of evaluated candidates
				} //End of Machine Loop

                if (evaluated_candidates > 2*CAND_Size)
                    break; //Limit the number of evaluated candidates
            }

			if (candidatelist.size() == 0){
			    printf("Empty candidate list, something went wrong\n");
			    //printf("Current Solution\n");
			    //cout << *this;
			}
			

			//Print Candidates for debugging
            /*
			cout << "Candidates Number: " << candidatelist.size() << endl;
            for (int i = 0; i < candidatelist.size(); i++) {
				Candidate *c = candidatelist[i];
				cout << "Candidate: " << i << " Job " << c->op->jobID << " ID " << c->op->ID << " Machine: " << c->mc->ID << " Machine Load: " << c->mc->operations.size() << " Position: " << c->pos << " Span " << c->span << endl;
			}
			cout << "--------" << endl;
            */


			//At this point one candidate should be selected from the candidatelist
			//I'll select one out of the first best 3 candidates
			//int final_index = rand() % (min(1 + (candidatelist.size()-1)/2, candidatelist.size()));
			
			//Last Used
			int final_index = rand() % (min(1 + ((int) candidatelist.size() - 1)/CAND_partition, (int) candidatelist.size())); //Comparable + better results with greedyConstruction
			
			//int final_index = rand() % (min(1 + ((int) candidatelist.size())/7, (int) candidatelist.size())); //Even better results compared to greedyConstruction
			//printf("Good Partition %d / %d \n", (candidatelist.size()-1)/5, candidatelist.size());
			//int final_index = rand() % (candidatelist.size());

			/*
			int final_index = 0;
			//Roulette Wheel Selection
			{
				float fitness_sum = 0.00001;
			 	int limit = min(1 + ((int) candidatelist.size())/2, (int) candidatelist.size());
			 	//Accumulate weights
			 	for (int i=0; i< limit; i++){
			 		fitness_sum += candidatelist[i]->span;
			 	}
				
			 	//Calculate fitness
			 	for (int i=0; i< limit; i++){
			 		candidatelist[i]->fitness = candidatelist[i]->span / fitness_sum;
			 	}

			 	//Generate probability
			 	float prob = randd();
				
			 	//Roulette Selection
			 	for (int i=0; i< limit; i++){
			 		prob -= candidatelist[i]->fitness;
			 		if (prob <= 0) {final_index = i; break;}
			 	}
			}
			*/

			final_op = candidatelist[final_index]->op;
			final_mach = candidatelist[final_index]->mc;
			final_mach_pos = candidatelist[final_index]->pos;

			//Check the best position found
			if ((final_mach != NULL) && (final_mach_pos != -1)) {
				//cout << "Saved Op: " << final_op->UID << " Machine: " << final_mach->ID << " Position " << final_mach_pos << endl;
				//Finalize app scheduling in mc
				
				final_op->resetDone();
				insertOp(final_mach, final_mach_pos, final_op);
				//Refix times
				//this->resetDone();
				//bool fixStatus = this->fixTimes(); //Myway
				bool fixStatus = this->scheduleAEAP(true, true);
				
				if (!fixStatus) 
					cout << "final_op insertion : SKATAAAAAAAAAAAAAAAAAAAA" << endl;

                printf("Scheduled Operation %d Start %ld End %ld\n",
                       final_op->UID, final_op->temp_start, final_op->temp_end);

				//Remove schedule operations from opshelp
				vRemoveAt<operation*>(opshelp, candidatelist[final_index]->h_index);

			} else {
				cout << "Malakia final_mach" << endl;
			}

			//Cleanup
			for (int i = 0; i <(int) candidatelist.size(); i++) {
				Candidate* cand = candidatelist[i];
				delete cand;
			}
			candidatelist.clear();

			//cout << "PARTIAL SOLUTION " << endl;
			//cout << *this;
			//this->FGraph->report();
			//this->reportResourcesConsole();
			//cout << "\r                                                           ";
			//cout << "\r" << "Remaining Ops: " << this->getRemainingOps() << " " << std::flush;
		}
		//cout << endl;

		//Solution should be ok, storing the temp values
		this->replace_real_values();
		this->Calculate_Resourse_Loads();

		//this->reportResources();
		//const char* outputfile = "temp_output.txt";
		//this->saveToFile(outputfile);

		//cout << *this->Resources[0];


		if (!this->testsolution())
		{
			cout << "Infeasible Solution AEAP Construction!" << endl;
			return false;
		}

		//Calculate costs
		this->getCost(false);
		//this->reportCosts(false);
		
		//Cleanup
		for (int i = 0; i < candidatelist.size(); i++) {
			Candidate* cand = candidatelist[i];
			delete cand;
		}

		candidatelist.clear();
		vector<Candidate*>().swap(candidatelist);


		//Calculate Critical Paths/Blocks/TopSort
		findCriticalPath_viaSlack();
		findCriticalBlocks(1);
		FGraph->topological_sort();

		//Resource Calculation Testing
		//this->Calculate_Resourse_Loads();

		//cout << *this->Resources[0];

		/*
		//Testing
		this->FGraph->topological_sort();

		//Report TSORT
		cout << *this;
		printf("TSort ");
		for (int i = 0; i < this->FGraph->top_sort.size();i++) {
			operation *cand = this->FGraph->top_sort[i];
			printf("%d ", cand->UID);
		}
		printf("\n");
		*/

		return true;
	}

bool solution::aeapConstruction_perOp() {
	/*
		AEAP Construction is supposed to assemble a randomized solution for the problem
		using a greedy insertion algorithm without calculating operation times.
		The goal is to check precedence conditions and then ScheduleAEAP will take
		care of the times.
	*/

	/*  DIFFERENCES
		Compared with aeap_construction() this method tries to schedule one operation at a time
	*/

	clock_t start;

	//Construction
	//int CAND_Size = 20;
	int op_num = totalNumOp;
	
	//int CAND_Size = 40;
	int CAND_Size = max(10, op_num/2);
	//cout << "AEAP Construction Heuristic" << endl;

	//Cleanup current solution
	this->cleanUpSolution(); //1 extra array allocation here that causes a tiny leak. Shouldn't cause runtime problems

	//Initiallize data structures for all resources
	this->Init_Resourse_Loads();

	//Init CandidateList
	vector<Candidate*> candidatelist = vector<Candidate*>();

	//Construct solution
	int iter = 0;

	while (opshelp.size())
	{
		//Find all noncompleted operations whose op_pred has been completed
		

		//Get the uperbound of available positions
		// int up_bound = 0;
		// for (int i=0;i < (int) this->solmachines.size();i++){
		// 	up_bound += (int) this->solmachines[i]->operations.size() - 1;
		// }
		
		//int CAND_Size = up_bound;
		int CAND_Size = 100;
		Graph<operation> *gr = FGraph;
		
		int gndex = -1;
		int final_mach_pos = -1;
		machine *final_mach = NULL;
		operation *final_op = NULL;

		for (int y = 0; y < (int) this->opshelp.size(); y++)
		{
			//Selecting op here
			operation *op = this->opshelp[y];
			if (op->temp_done || op->done) continue; //Check if done

			//Check if pred done
			if (op->has_preds())
			{	
				operation *op_pred = op->pred[0];
				if (!op_pred->temp_done) continue;
			}

			//Now iterate on machines
            process_step *ps = this->ProcSteps[op->process_stepID];


			//Find the best machine to schedule the op
			op->resetDone();
			int span = INF;
			//this->getCost(MAKESPAN, true);
			
			int prespan = this->temp_cost_array[MAKESPAN];


			for (int m = 0; m <(int) ps->machines.size(); m++)
			{
				machine *mc = ps->machines[m];

				if (op->getProcTime(mc) == 0) continue;

				//cout << "Trying to Schedule: " << " JOB: " << op->jobID << " OP ID: "  << op->ID << " at Mach: " << mc->ID << " Available Positions: " << mc->operations.size() - 2<< endl;
				/*
					Operations can be added in any position at the machine
				*/

				for (int mach_pos = 1; mach_pos < (int) mc->operations.size(); mach_pos++) {
					bool psFound = false;
					//Check for successors before insert pos

					//insert operation to solution
					this->insertOp(mc, mach_pos, op);
					//printf("Inserted Operation %d %d \n", op->jobID, op->ID);
					
					//cout << "TEMP SOLUTION" << endl;
					//cout << *this;
					
					//cout << "SOLUTION FORWARD GRAPH" << endl; 
					//this->forwardGraph->report();

					//OLD JIT GRAPH
					//cout << "OLD JIT FORWARD GRAPH" << endl; 
					//Graph *g;
					//g = this->createGraph(1);
					//g->report();

					/*if (!(*g == *this->forwardGraph)){
						cout << "UNEQUAL GRAPHS" <<endl;
						*g == *this->forwardGraph;
						assert(false);
					}
					delete g;*/

					//Check for cyclic scheduling infeasibilities after insertion
					//Node *g_node = graph->findNode(op->jobID, op->ID);
					gr->dfs(op->UID);
					
					if (gr->cycleflag) {
						//cout << "PsFound " << " Machine " << mc->ID << " Pos "<< mach_pos << " Total Machine Ops: " << mc->operations.size() << endl;
						//cout << *this;
						//graph->report();
						//Redo the check for debugging
						//graph->resetVisited();
						//graph_status = graph->dfs(g_node);
						//cout << "Infeasibility Detected, continuing..." << endl;
						this->removeOp(mc, mach_pos);
						continue; //In there is an infeasibility continue to next position
					} 

					//If precedence checks pass I can schedule the op in mc
					//and keep track of its start time


					//Fix times
					this->setDoneStatus(false, false); //Reset operation status

					//start = clock();
					bool fixStatus = this->scheduleAEAP(true, true);
					//bool fixStatus = this->fixTimes();
					//double time = ((clock() - start) / (double) CLOCKS_PER_SEC);
					//cout << "Elapsed Time for scheduleAEAP: " << std::setprecision(2) << std::fixed << time <<endl;

					this->getCost(MAKESPAN, true);
					int op_endtime = op->getEndTime();
					//printf("Testing Temp Span %d \n", this->temp_cost_array[MAKESPAN]);
					//int current_span = max(prespan, op_endtime);
					int current_span = this->temp_cost_array[MAKESPAN];
					
					//this->resetDone();
					//op->fixTimes();
					//Remove Op
					this->removeOp(mc, mach_pos);

					//cout << *this;
					if (!fixStatus) {
						//Refix to previous state
						cout << "MALAKIES SE AEAP CONSTRUCTION" << endl;
						assert(false);
					}

					//CANDIDATE SORTING AND SAVING
					{
						int endtime = current_span - prespan;
						//int endtime = current_span;
						//int endtime = this->calculate_makespan(0, false);
						//int endtime = current_span;
						//int endtime = op->getProcTime();
						//cout << "endtime: " << endtime << endl;

						//cout << "Checking Position: " << mach_pos <<" Start Time: " << op->getStartTime() << " End Time: " << op->getEndTime() << endl;
						
						Candidate *cand = new Candidate();
						//Populate Candidate
						cand->op = op;
						cand->mc = mc;
						cand->pos = mach_pos;
						cand->span = endtime;
						cand->h_index = y;
						//Store into the CandidateList according to span
						int c_pos = 0;
						for (int i = 0; i < candidatelist.size(); i++) {
							Candidate *c = candidatelist[i];
							if ((cand->span < c->span) || ((cand->span == c->span) && (cand->mc->operations.size() < c->mc->operations.size()))) {
								break;
							} else c_pos += 1;
						}
						//Insert to found position
						vInsertAt<Candidate*>(candidatelist, c_pos, cand);
						
						//Pop extra candidate
						if ((int) candidatelist.size() > CAND_Size) {
							Candidate *cand = candidatelist[CAND_Size - 1];
							//cand->~Candidate(); Not sure why I'm using this one
							delete cand;
							vRemoveAt<Candidate*>(candidatelist, CAND_Size - 1);
						}
					}
				}
			} //End of Machine Loop

			//Print Candidates for debugging

			//cout << "Candidates Number: " << candidatelist.size() << endl;
			
			
			
			// for (int i = 0; i < candidatelist.size(); i++) {
			// 	Candidate *c = candidatelist[i];
			// 	cout << "Candidate: " << i << " Job " << c->op->jobID << " ID " << c->op->ID << " Machine: " << c->mc->ID << " Machine Load: " << c->mc->operations.size() << " Position: " << c->pos << " Span " << c->span << endl;
			// }
			// cout << "--------" << endl;
			

			//At this point one candidate should be selected from the candidatelist
			//I'll select one out of the first best 3 candidates
			//int final_index = rand() % (min(1 + (candidatelist.size()-1)/2, candidatelist.size()));
			int final_index = rand() % (min(1 + ((int) candidatelist.size() - 1)/25, (int) candidatelist.size())); //Comparable + better results with greedyConstruction
			//int final_index = rand() % (min(1 + ((int) candidatelist.size())/7, (int) candidatelist.size())); //Even better results compared to greedyConstruction
			//printf("Good Partition %d / %d \n", (candidatelist.size()-1)/5, candidatelist.size());
			//int final_index = rand() % (candidatelist.size());
			
			// int final_index = 0;
			// //Roulette Wheel Selection
			
			// {

			// 	float fitness_sum = 0.00001;
			// 	int limit = min(1 + ((int) candidatelist.size())/5, (int) candidatelist.size());
			// 	//Accumulate weights
			// 	for (int i=0; i< limit; i++){
			// 		fitness_sum += candidatelist[i]->span;
			// 	}
				
			// 	//Calculate fitness
			// 	for (int i=0; i< limit; i++){
			// 		candidatelist[i]->fitness = candidatelist[i]->span / fitness_sum;
			// 	}

			// 	//Generate probability
			// 	float prob = randd();
				
			// 	//Roulette Selection
			// 	for (int i=0; i< limit; i++){
			// 		prob -= candidatelist[i]->fitness;
			// 		if (prob <= 0) {final_index = i; break;}
			// 	}
			// }
			
			final_op = candidatelist[final_index]->op;
			final_mach = candidatelist[final_index]->mc;
			final_mach_pos = candidatelist[final_index]->pos;

			//Check the best position found
			if ((final_mach != NULL) && (final_mach_pos != -1)) {
				//cout << "Saved Op: " << " Machine: " << final_mach->ID << " Position " << final_mach_pos << endl;
				//Finalize app scheduling in mc
				
				final_op->resetDone();
				final_op->changeMach(final_mach, final_mach_pos);
				final_mach->insertOp(final_op, final_mach_pos);
				insertOpToGraphs(final_op);
				//Refix times
				//this->resetDone();
				//bool fixStatus = this->fixTimes(); //Myway
				bool fixStatus = this->scheduleAEAP(true, true);
				//Update solution costs
				this->getCost(MAKESPAN, true);

				
				if (!fixStatus) {
					cout << "SKATAAAAAAAAAAAAAAAAAAAA" << endl;
				}

				//Remove schedule operations from opshelp
				vRemoveAt<operation*>(opshelp, candidatelist[final_index]->h_index);

			} else {
				cout << "Malakia final_mach" << endl;
			}

			//Cleanup
			for (int i = 0; i <(int) candidatelist.size(); i++) {
				Candidate* cand = candidatelist[i];
				delete cand;
			}
			candidatelist.clear();

			//cout << "PARTIAL SOLUTION " << endl;
			//cout << *this;
			//this->reportResourcesConsole();
			//cout << "\r                                                           ";
			//cout << "\r" << "Remaining Ops: " << this->getRemainingOps() << " " << std::flush;

		}
		

	}
	//cout << endl;

	//Solution should be ok, storing the temp values
	this->replace_real_values();
	this->Calculate_Resourse_Loads();

	this->reportResources();
	//const char* outputfile = "temp_output.txt";
	//this->saveToFile(outputfile);

	//cout << *this->Resources[0];


	if (!this->testsolution())
	{
		cout << "Infeasible Solution AEAP Construction!" << endl;
		return false;
	}

	//Calculate costs
	this->getCost(false);
	//this->reportCosts(false);
	
	//Cleanup
	for (int i = 0; i < candidatelist.size(); i++) {
		Candidate* cand = candidatelist[i];
		delete cand;
	}

	candidatelist.clear();
	vector<Candidate*>().swap(candidatelist);


	//Resource Calculation Testing
	//this->Calculate_Resourse_Loads();

	//cout << *this->Resources[0];

	return true
;}

bool solution::randomCandidateConstruction(vector<bl_pair> &blacklist) {
	/*
		This method is intended to be used with the Perturbation mechanism
		This simply iterates on the unscheduled operations, tries to find their available positions
		then selects at random one of those and inserts the operation.
		The process ends until all operations have been inserted.
		Warning #1: The final schedule after those operations WILL NOT BE CORRECT, but its going to be feasible
		relationships wise.
		Warning #2: In order to work faster, operations will be marked as done. They NEED to be reset
	*/
	//cout << "Random Candidate Heuristic" << endl;



	//Init CandidateList
	vector<Candidate*> candidatelist = vector<Candidate*>();
	Graph<operation> *gr = FGraph;
	gr->state = FORWARD;
	//cout << "RANDOMIZED CONSTRUCTION START SOLUTION " << endl;
	//cout << *this;

    //If there is a blacklist provided completely override the opshelp
    if (blacklist.size()) {
        opshelp.clear();
        for (int i = 0; i < blacklist.size(); i++) {
            bl_pair bop = blacklist[i];
            operation *op = soljobs[bop.first]->operations[bop.second];
            opshelp.push_back(op);
        }
    }
    else {
        //Cleanup current solution
        this->cleanUpSolution();
    }

    //Shuffle opshelp
    shuffle(opshelp.begin(), opshelp.end(),std::default_random_engine {});

	//Construct solution
	int index = 0;
	while (opshelp.size())
	{
		if (index > (int) opshelp.size() - 1){
			index = 0;
		}
		//Find all noncompleted operations whose op_pred has been completed
		for (int i = 0; i <(int) candidatelist.size(); i++) {
			Candidate* cand = candidatelist[i];
			delete cand;
		}

		candidatelist.clear();
		int gndex = -1;
		int final_mach_pos = -1;
		machine *final_mach = NULL;
		operation *final_op = NULL;

		
		//Selecting op here
		operation *op = this->opshelp[index];
		if (op->temp_done || op->done) {index++; continue;} //Check if done

		//Check if pred done
		if (op->has_preds())
		{	
			operation *op_pred = op->pred[0];
			if (!op_pred->temp_done) {index++; continue;}
		}

		//Now iterate on machines
        process_step *ps = this->ProcSteps[op->process_stepID];


		//Find the best machine to schedule the op
		op->resetDone();
		
		for (int m = 0; m <(int) ps->machines.size(); m++)
		{
			machine *mc = ps->machines[m];

			if (op->getProcTime(mc) == 0) continue;

			//cout << "Trying to Schedule: " << " JOB: " << op->jobID << " OP ID: "  << op->ID << " at Mach: " << mc->ID << " Available Positions: " << mc->operations.size() - 2<< endl;
			/*
				Operations can be added in any position at the machine
			*/

			for (int mach_pos = 1; mach_pos < (int) mc->operations.size(); mach_pos++) {
				bool psFound = false;

                if (executionMode == 1){
                    operation *mach_suc = mc->operations[mach_pos];
                    //Operation positioned in an older point in time. Remove and proceed to a next location
                    if (mach_suc->fixed)
                        continue;
                }

                //insert operation to solution
				this->insertOp(mc, mach_pos, op);
				//printf("Inserted Operation %d %d \n", op->jobID, op->ID);
				
				//cout << "TEMP SOLUTION" << endl;
				//cout << *this;
				
				//cout << "SOLUTION FORWARD GRAPH" << endl; ins
				//this->forwardGraph->report();

				//OLD JIT GRAPH
				//cout << "OLD JIT FORWARD GRAPH" << endl; 
				//Graph *g;
				//g = this->createGraph(1);
				//g->report();

				/*if (!(*g == *this->forwardGraph)){
					cout << "UNEQUAL GRAPHS" <<endl;
					*g == *this->forwardGraph;
					assert(false);
				}
				delete g;*/

				//Check for cyclic scheduling infeasibilities after insertion
				//Node *g_node = graph->findNode(op->jobID, op->ID);
				gr->dfs(op->UID);
				
				if (gr->cycleflag) {
					//printf("Operation %d %d Infeasible on Machine %d Position %d \n", op->jobID, op->ID,
					//	mc->ID, mach_pos);
					this->removeOp(mc, mach_pos);
					continue; //In there is an infeasibility continue to next position
				}

				//If precedence checks pass I can schedule the op in mc
				//and keep track of its start time

				//Remove Op
				this->removeOp(mc, mach_pos);

				//CANDIDATE SORTING AND SAVING
				{
					Candidate *cand = new Candidate();
					//Populate Candidate
					cand->op = op;
					cand->mc = mc;
					cand->pos = mach_pos;
					cand->span = INF;
					//Store into the CandidateList according to span
					candidatelist.push_back(cand);
				}
			}
		}


		//Print Candidates for debugging

		/*
		cout << "Candidates Number: " << candidatelist.size() << endl;
		for (int i = 0; i < candidatelist.size(); i++) {
			Candidate *c = candidatelist[i];
			cout << "Candidate: " << i << " Job " << c->op->jobID << " ID " << c->op->ID << " Machine: " << c->mc->ID << " Machine Load: " << c->mc->operations.size() << " Position: " << c->pos << " Span " << c->span << endl;
		}
		cout << "--------" << endl;
		*/

		//At this point one candidate should be selected from the candidatelist at random
		if (candidatelist.size()==0){
			printf("Failed to find candidate for operation %d %d \n", op->jobID, op->ID);
			cout << *this;
			gr->report();
			assert(false);
		}

		int final_index = rand() % (candidatelist.size());
		

		final_op = candidatelist[final_index]->op;
		final_mach = candidatelist[final_index]->mc;
		final_mach_pos = candidatelist[final_index]->pos;

		//Check the best position found
		if ((final_mach != NULL) && (final_mach_pos != -1)) {
			//cout << "Saved Op: " << " Machine: " << final_mach->ID << " Position " << final_mach_pos << endl;
			//Finalize app scheduling in mc
			
			final_op->resetDone();
            insertOp(final_mach, final_mach_pos, final_op);

            //Instead of fixing times with scheduleaeap, we mark all operations as done
			this->setDoneStatus(false, true);

			//Remove schedule operations from opshelp
			vRemoveAt<operation*>(opshelp, index);

		} else {
			cout << "Malakia final_mach" << endl;
		}

        //cout << "Inserted Operation " << final_op->jobID << " " << final_op->ID << endl;
		//cout << "PARTIAL SOLUTION " << endl;
		//cout << *this;
		//this->forwardGraph->report();
		index = 0;
	}
	//cout << endl;

	//Solution should be ok, storing the temp values
	
	//Cleanup
	for (int i = 0; i < candidatelist.size(); i++) {
		Candidate* cand = candidatelist[i];
		delete cand;
	}

	candidatelist.clear();
	vector<Candidate*>().swap(candidatelist);

	return true;
}

bool solution::Objective_TS_Comparison(const solution* s1, const solution* s2, bool temp)
{
	//Compare finalized solutions
	float s1_costs[COST_NUM];
	float s2_costs[COST_NUM];

	switch (temp) {
	case true:
		for (int i = 0; i < COST_NUM; i++) {
			s1_costs[i] = s1->temp_cost_array[i];
			s2_costs[i] = s2->temp_cost_array[i];
		}
		break;
	case false:
		for (int i = 0; i < COST_NUM; i++) {
			s1_costs[i] = s1->cost_array[i];
			s2_costs[i] = s2->cost_array[i];
		}
		break;
	}
	
	
	return solution::Objective_TS_Comparison(s1_costs, s2_costs);
}

bool solution::Objective_TS_Comparison(float* new_cost_array, float* old_cost_array)
{
	float diffs[COST_NUM];
	for (int i=0; i < COST_NUM; i++)
		diffs[i] = new_cost_array[i] - old_cost_array[i];
	
	
	//printf("Testing diffs: %d %d %d %d \n", diffs[0], diffs[1], diffs[2], diffs[3]);

	bool flag = false;
	if (optimtype == 1) if (diffs[MAKESPAN] < 0) flag = true;
	if (optimtype == 2) if (diffs[TOTAL_FLOW_TIME] < 0) flag = true;
	if (optimtype == 3) if (diffs[TOTAL_IDLE_TIME] < 0) flag = true;
	if (optimtype == 4) if (diffs[MAKESPAN] < 0 || (diffs[MAKESPAN] <= 0 && diffs[TOTAL_FLOW_TIME] < 0) || (diffs[MAKESPAN] <= 0 && diffs[TOTAL_FLOW_TIME] <= 0 && diffs[TOTAL_IDLE_TIME] < 0)) flag = true;
	if (optimtype == 5) if (diffs[TOTAL_FLOW_TIME] < 0 || (diffs[TOTAL_FLOW_TIME] <= 0 && diffs[TOTAL_IDLE_TIME] < 0) || (diffs[TOTAL_FLOW_TIME] <= 0 && diffs[TOTAL_ENERGY] <= 0 && diffs[MAKESPAN] < 0)) flag = true;
	if (optimtype == 6) if (diffs[TOTAL_IDLE_TIME] < 0 || (diffs[TOTAL_IDLE_TIME] <= 0 && diffs[MAKESPAN] < 0) || (diffs[TOTAL_IDLE_TIME] <= 0 && diffs[MAKESPAN] <= 0 && diffs[TOTAL_FLOW_TIME] < 0)) flag = true;
	if (optimtype == 7) if ((diffs[MAKESPAN] < 0) || (diffs[MAKESPAN] <= 0 && diffs[TOTAL_IDLE_TIME] < 0) || (diffs[MAKESPAN] <= 0 && diffs[TOTAL_IDLE_TIME] <= 0 && diffs[TOTAL_FLOW_TIME] < 0)) flag = true;
	if (optimtype == 8) if (diffs[TOTAL_FLOW_TIME] < 0 || (diffs[TOTAL_FLOW_TIME] <= 0 && diffs[MAKESPAN] < 0) || (diffs[TOTAL_FLOW_TIME] <= 0 && diffs[MAKESPAN] <= 0 && diffs[TOTAL_IDLE_TIME] < 0)) flag = true;
	if (optimtype == 9) if (diffs[TOTAL_IDLE_TIME] < 0 || (diffs[TOTAL_IDLE_TIME] <= 0 && diffs[TOTAL_FLOW_TIME] < 0) || (diffs[TOTAL_IDLE_TIME] <= 0 && diffs[TOTAL_FLOW_TIME] <= 0 && diffs[MAKESPAN] < 0)) flag = true;
	if (optimtype == 10) if (diffs[TOTAL_ENERGY] < 0) flag = true;
	if (optimtype == 11) if (diffs[TOTAL_ENERGY] < 0 || (diffs[TOTAL_ENERGY] <= 0 && diffs[MAKESPAN] < 0) || (diffs[TOTAL_ENERGY] <= 0 && diffs[MAKESPAN] <= 0 && diffs[TOTAL_FLOW_TIME] < 0)) flag = true;
	if (optimtype == 12) if (diffs[TOTAL_ENERGY] < 0 || (diffs[TOTAL_ENERGY] <= 0 && diffs[TOTAL_FLOW_TIME] < 0) || (diffs[TOTAL_ENERGY] <= 0 && diffs[TOTAL_FLOW_TIME] <= 0 && diffs[MAKESPAN] < 0)) flag = true;
	if (optimtype == 13) if (diffs[DISTANCE] < 0) flag = true;
	if (optimtype == 14) if (diffs[DISTANCE] < 0 || (diffs[DISTANCE] <= 0 && diffs[MAKESPAN] < 0) ) flag = true;
	if (optimtype == 15) if (diffs[DISTANCE] < 0 || (diffs[MAKESPAN] <= 0 && diffs[DISTANCE] < 0)) flag = true;
	return flag;
}

void solution::reportCosts(bool temp){
	cout << "Costs: ";
	for (int i=0;i<COST_NUM;i++){
		if (temp) cout << temp_cost_array[i];
		else cout << cost_array[i];
		cout << " ";
	}
	cout << endl;
}


//Forward Graph Methods

void solution::insertOpToForwardGraph(operation* op) {
	//Add operation to solution graph
	Graph<operation> *activeGraph = FGraph;
	
	//Load pred
	operation *pred = op->get_mach_pred();
	//Load suc
	operation *suc = op->get_mach_suc();


	//Step A - Remove temp edge of pred_suc
	//unordered_set<int3_tuple>::iterator res;

	/*
	res = activeGraph->edges.find(make_tuple(pred->UID, suc->UID, 1));
	if (res != activeGraph->edges.end()) {
		activeGraph->edges.erase(res);
		activeGraph->adj_list[pred->UID].t_connection = NULL;
	}
	*/
	if (activeGraph->adj_list[pred->UID].t_connection == suc->UID)
		activeGraph->adj_list[pred->UID].t_connection = -MAX_JOB_RELATIONS;


	//Step B - Add if not already in the new edges
	//Ba- edge with pred
	/*
	res = activeGraph->edges.find(make_tuple(pred->UID, op->UID, 0));
	if (res == activeGraph->edges.end()) {
		activeGraph->edges.insert(make_tuple(pred->UID, op->UID, 1));
		activeGraph->adj_list[pred->UID].t_connection = op;
	}
	*/
	if (activeGraph->adj_list[pred->UID].p_connections[0] != op->UID)
		activeGraph->adj_list[pred->UID].t_connection = op->UID;


	//Ba- edge with suc
	/*
	res = activeGraph->edges.find(make_tuple(op->UID, suc->UID, 0));
	if (res == activeGraph->edges.end()){
		activeGraph->edges.insert(make_tuple(op->UID, suc->UID, 1));
		activeGraph->adj_list[op->UID].t_connection = suc;
	}
	*/

	if (activeGraph->adj_list[op->UID].p_connections[0] != suc->UID)
		activeGraph->adj_list[op->UID].t_connection = suc->UID;

}

void solution::insertOpToBackwardGraph(operation* op) {
	//Add operation to solution graph
	Graph<operation> *activeGraph = BGraph;
	
	//Load pred
	operation *pred = op->get_mach_pred();
	//Load suc
	operation *suc = op->get_mach_suc();

	//Step A - Remove temp edge of pred_suc
	//unordered_set<int3_tuple>::iterator res;

	/*
	res = activeGraph->edges.find(make_tuple(suc->UID, pred->UID, 1));
	if (res != activeGraph->edges.end()) {
		activeGraph->edges.erase(res);
		activeGraph->adj_list[suc->UID].t_connection = NULL;
	}
	*/
	if (activeGraph->adj_list[suc->UID].t_connection == pred->UID)
		activeGraph->adj_list[suc->UID].t_connection = -MAX_JOB_RELATIONS;

	//Step B - Add if not already in the new edges
	//Ba- edge with pred
	/*
	res = activeGraph->edges.find(make_tuple(op->UID, pred->UID, 0));
	if (res == activeGraph->edges.end()) {
		activeGraph->edges.insert(make_tuple(op->UID, pred->UID, 1));
		activeGraph->adj_list[op->UID].t_connection = pred;
	}
	*/
	if (activeGraph->adj_list[op->UID].p_connections[0] != pred->UID)
		activeGraph->adj_list[op->UID].t_connection = pred->UID;

	//Ba- edge with suc
	/*
	res = activeGraph->edges.find(make_tuple(suc->UID, op->UID, 0));
	if (res == activeGraph->edges.end()) {
		activeGraph->edges.insert(make_tuple(suc->UID, op->UID, 1));
		activeGraph->adj_list[suc->UID].t_connection = op;
	}
	*/
	if (activeGraph->adj_list[suc->UID].p_connections[0] != op->UID)
		activeGraph->adj_list[suc->UID].t_connection = op->UID;

}

void solution::removeOpFromForwardGraph(operation* op) {
	//Add operation to solution graph
	Graph<operation> *activeGraph = FGraph;
	
	//Load pred
	operation *pred = op->get_mach_pred();
	//Load suc
	operation *suc = op->get_mach_suc();

	//Step A - Remove temp edge of pred-op
	//unordered_set<int3_tuple>::iterator res;

	/*
	res = activeGraph->edges.find(make_tuple(pred->UID, op->UID, 1));
	if (res != activeGraph->edges.end()) {
		activeGraph->edges.erase(res);
		activeGraph->adj_list[pred->UID].t_connection = NULL;
	}
	*/
	if (activeGraph->adj_list[pred->UID].t_connection == op->UID)
		activeGraph->adj_list[pred->UID].t_connection = -MAX_JOB_RELATIONS;


	//Step A - Remove temp edge of op-suc
	/*
	res = activeGraph->edges.find(make_tuple(op->UID, suc->UID, 1));
	if (res != activeGraph->edges.end()) {
		activeGraph->edges.erase(res);
		activeGraph->adj_list[op->UID].t_connection = NULL;
	}
	*/
	if (activeGraph->adj_list[op->UID].t_connection == suc->UID)
		activeGraph->adj_list[op->UID].t_connection = -MAX_JOB_RELATIONS;

	//Ba- Connect pred with suc
	/*
	res = activeGraph->edges.find(make_tuple(pred->UID, suc->UID, 0));
	if (res == activeGraph->edges.end()) {
		activeGraph->edges.insert(make_tuple(pred->UID, suc->UID, 1));
		activeGraph->adj_list[pred->UID].t_connection = suc;
	}
	*/
	if (activeGraph->adj_list[pred->UID].p_connections[0] != suc->UID) {
		activeGraph->adj_list[pred->UID].t_connection = suc->UID;
	}
}

void solution::removeOpFromBackwardGraph(operation* op) {
	//Add operation to solution graph
	Graph<operation> *activeGraph = BGraph;
	activeGraph->state = BACKWARD;

	//Load pred
	operation *pred = op->get_mach_pred();
	//Load suc
	operation *suc = op->get_mach_suc();

	//Step A - Remove temp edge of pred-op
	//unordered_set<int3_tuple>::iterator res;
	/*
	res = activeGraph->edges.find(make_tuple(op->UID, pred->UID, 1));
	if (res != activeGraph->edges.end()) {
		activeGraph->edges.erase(res);
		activeGraph->adj_list[op->UID].t_connection = NULL;
	}
	*/
	if (activeGraph->adj_list[op->UID].t_connection == pred->UID)
		activeGraph->adj_list[op->UID].t_connection = -MAX_JOB_RELATIONS;

	//Step A - Remove temp edge of op-suc
	/*
	res = activeGraph->edges.find(make_tuple(suc->UID, op->UID, 1));
	if (res != activeGraph->edges.end()) {
		activeGraph->edges.erase(res);
		activeGraph->adj_list[suc->UID].t_connection = NULL;
	}
	*/

	if (activeGraph->adj_list[suc->UID].t_connection == op->UID)
		activeGraph->adj_list[suc->UID].t_connection = -MAX_JOB_RELATIONS;

	//Ba- Connect pred with suc
	/*
	res = activeGraph->edges.find(make_tuple(suc->UID, pred->UID, 0));
	if (res == activeGraph->edges.end()) {
		activeGraph->edges.insert(make_tuple(suc->UID, pred->UID, 1));
		activeGraph->adj_list[suc->UID].t_connection = pred;
	}
	*/
	if (activeGraph->adj_list[suc->UID].p_connections[0] != pred->UID) {
		activeGraph->adj_list[suc->UID].t_connection = pred->UID;
	}
}

//Wrapper Graph methods
void solution::removeOpFromGraphs(operation* op){
    removeOpFromForwardGraph(op);
	removeOpFromBackwardGraph(op);

	//FGraph->report();
	//BGraph->report();
}

void solution::insertOpToGraphs(operation* op){
	insertOpToForwardGraph(op);
	insertOpToBackwardGraph(op);
	
	//FGraph->report();
	//BGraph->report();
}


void solution::insertOp(machine* mc, int pos, operation* op){
	//Change machine in Op
	op->changeMach(mc, pos);
	//Insert operation to machine

	mc->insertOp(op, pos);
	//op->fixTimes(false); //Fix but don't finalize

	insertOpToGraphs(op);
}

void solution::insertOp_noGraphUpdate(machine* mc, int pos, operation* op){
	//Change machine in Op
	op->changeMach(mc, pos);
	//Insert operation to machine

	mc->insertOp(op, pos);
	//op->fixTimes(false); //Fix but don't finalize
}

void solution::insertOpTable(machine* mc, int pos, operation* op){
	//Change machine in Op
	op->changeMach(mc, pos);
	//Insert operation to machine

	//op->fixTimes(false); //Fix but don't finalize

	insertOpToGraphs(op);
}

void solution::removeOp(machine* mc, int pos){
	//Fetch Operation
	operation *op = mc->operations[pos];

	//First remove op from graphs
	
	removeOpFromGraphs(op);
	
	//Now permanently remove op	
	mc->removeOp(pos);
}

void solution::removeOp_noGraphUpdate(machine* mc, int pos){
	//Fetch Operation
	operation *op = mc->operations[pos];

	//Now permanently remove op
	mc->removeOp(pos);
}


//Distance Methods
float solution::calcDistance(solution *s1, solution* s2, const int MODE, bool temp) {
	switch (MODE) {
	case 0:
		return (float) solution::humDistance_single(s1, s2);
		break;
	case 1:
		return (float) solution::humDistance_pairs(s1, s2);
	case 4: //Override
	    return INF;
    case 2:
    case 3:
	case 5:
	case 6:
		printf("Non Implemented Yet\n");
		assert(false);
		break;
	default:
		return INF;
	}
}


int solution::humDistance_self_single_times(solution* s1) {
	//This functions goal is to be used within any LS Move evaluation
	/*The main reason to keep a separate implementation is because the same solution
		while move evaluation will keep on the same solution object the updated and the old values
		in the temp and the fixed fields. This way I can use the same solution for calculating the distance of a probed solution
		compared to its former version
	*/

	int dist = 0;
	for (size_t i = 0; i < s1->soljobs.size(); i++) {
		job *jb1 = s1->soljobs[i];
		
		for (size_t j = 0; j < jb1->operations.size(); j++) {
			operation *op1 = jb1->operations[j];
			
			dist += abs(op1->start - op1->temp_start) + abs(op1->end - op1->temp_end);
		}
	}

	return dist;
}

int solution::humDistance_single_times(solution* s1, solution* s2) {
	if (s1->soljobs.size() != s2->soljobs.size()) {
		printf("hum single SKATA INPUT LUSEIS %ld %ld\n", s1->solmachines.size(), s2->solmachines.size());
		assert(false);
		return -1;
	}

	int dist = 0;
	for (size_t i = 0; i < s1->soljobs.size(); i++) {
		job *jb1 = s1->soljobs[i];
		job *jb2 = s2->soljobs[i];

		for (size_t j = 0; j < jb1->operations.size(); j++) {
			operation *op1 = jb1->operations[j];
			operation *op2 = jb2->operations[j];
			
			dist += abs(op1->temp_start - op2->temp_start) + abs(op1->temp_end - op2->temp_end);
		}
	}
	
	return dist;
}

int solution::humDistance_single(solution* s1, solution* s2) {
	if (s1->solmachines.size() != s2->solmachines.size()) {
		printf("hum single SKATA INPUT LUSEIS %d %d\n", s1->solmachines.size(), s2->solmachines.size());
		assert(false);
		return -1;
	}

	int dist = 0;
	for (size_t i = 0; i < s1->solmachines.size(); i++) {
		machine *m1 = s1->solmachines[i];
		machine *m2 = s2->solmachines[i];
		
		for (size_t j = 1; j < m1->operations.size() - 1; j++) {
			operation *op1 = m1->operations[j];
			//int testID = ((operation*)(*m->operations)[k])->jobID;
			//cout << "Test ID " << testID << " JobID "<< jtestID << endl;
			if (!m2->containsOP(op1)) {
				//find operation in the other solution
				operation *op2 = s2->soljobs[op1->jobID]->operations[op1->ID];
				//int val = (op1->getProcTime() - op2->getProcTime());
				//dist += val*val;
				dist += 1;
			}
		}
	}
	
	return dist;
}

int solution::humDistance_pairs(solution* s1, solution* s2) {
	if (s1->solmachines.size() != s2->solmachines.size()) {
		cout << " hum single: SKATA LUSEIS GIA INPUT" << endl;
		assert(false);
		return -1;
	}

	int dist = 0;
	for (size_t i = 0; i < s1->solmachines.size(); i++) {
		machine *m1 = s1->solmachines[i];
		machine *m2 = s2->solmachines[i];

		for (size_t j = 1; j < m1->operations.size() - 2; j++) {
			operation *op1 = m1->operations[j + 0];
			operation *op2 = m1->operations[j + 1];
			//int jtestID = ((operation*)(*m->operations)[k])->jobID;
			//cout << "Test ID " << testID << " JobID "<< jtestID << endl;
			int id1 = m2->containsOP(op1);
			int id2 = m2->containsOP(op2);
			if (id1) {
				if (id2) {
					operation *oop1 = m2->operations[id1];
					operation *oop2 = m2->operations[id2];
					if (oop1->is_mach_suc(oop2)) { continue; } //Pair exists
				}
			}
			
			dist += 1;
		}
	}

	return dist;
}

void solution::createDistanceMatrix() {
	//Create 3d Matrix
	//cout << *this;
	int ***matrix = new int**[this->solmachines.size()];

	for (size_t k = 0; k < this->solmachines.size(); k++) {
		int **arr = new int*[this->totalNumOp];
		for (int j = 0; j < totalNumOp; j++) {
			int *new_arr = new int[this->totalNumOp];
			memset(new_arr, 0, totalNumOp*sizeof(int));
			arr[j] = new_arr;
		}
		matrix[k] = arr;
	}


	//Populate 3d matrix
	for (size_t k = 0; k < this->solmachines.size(); k++) {
		machine* mc = solmachines[k];
		if (mc->operations.size() <= 3) continue;
		for (size_t j = 1; j < mc->operations.size() - 2; j++) {
			operation* op1 = mc->operations[j];
			operation* op2 = mc->operations[j + 1];
			//Calculate unique op IDS
			int op1_id = op1->jobID * this->soljobs[op1->jobID]->operations.size() + op1->ID;
			int op2_id = op2->jobID * this->soljobs[op2->jobID]->operations.size() + op2->ID;

			matrix[k][op1_id][op2_id] = 1;
		}
	}

	/*printf("3D Matrix populated \n");
	for (int k = 0; k < this->solmachines.size(); k++) {
		printf("Machine %d\n", k);
		for (int j = 0; j < totalNumOp; j++) {
			for (int i = 0; i < totalNumOp; i++) {
				printf("%d ", matrix[k][j][i]);
			}
			printf("\n");
		}
	}
	printf("Done");*/

	//Delete matrix
	for (size_t k = 0; k < this->solmachines.size(); k++) {
		int **arr = matrix[k];
		for (int j = 0; j < totalNumOp; j++) {
			int *new_arr = arr[j];
			delete new_arr;
		}
		delete arr;
	}
	delete matrix;
}


void DGraph::calcLateTimes() {
	//At first find the latest operation
	vector<operation*> late_ops;
}

void solution::fillMachTable() {
	cout << *this;
	mach_table = new MachOpTable();
	for (size_t i = 0; i < solmachines.size(); i++) {
		machine *mc = solmachines[i];
		for (size_t j = 0; j < mc->operations.size() - 1; j++) {
			operation *op1 = mc->operations[j];
			operation *op2 = mc->operations[j + 1];
			printf("Setting Values %d %d %d \n", op1->UID, op2->UID, mc->ID);
			mach_table->set_val(op1->UID, op2->UID, mc->ID);
		}
	}
}

void solution::findCriticalPath() {
	//printf("Critical Path finder\n");
	//cout << *this;
	//this->saveToFile("test_sol");
    FGraph->criticalPath.clear();

    //At first capture the last finishing operations
	int max_end = (int) this->cost_array[MAKESPAN];
	operation *main_critical_op = NULL;
	for (size_t i = 0; i < this->soljobs.size(); i++) {
		job *jb = this->soljobs[i];
		operation *op = jb->operations[jb->operations.size() - 1];
		if (op->end == max_end) {
			if ((main_critical_op == NULL) || (randd() > 0.5f))
				main_critical_op = op;
		}
	}

	bool run = true;
	operation *c_op = main_critical_op;
	while (true) {
		operation *op1, *op2;
		if (c_op->has_preds())
			op1 = c_op->get_pred();
		else
			op1 = NULL;

		if (c_op->has_mach_preds()) {
			op2 = c_op->get_mach_pred();
			if (!op2->valid) op2 = NULL; //Handle dummy ops
		}
		else
			op2 = NULL;

        FGraph->criticalPath.push_back(c_op);

		if ((op1 == NULL) && (op2 == NULL))
			break;
		else if (op2 != NULL) {
			c_op = op2;
			continue;
		}
		else if (op1 != NULL) {
			c_op = op1;
			continue;
		}
		else if (op1->end <= op2->end) {
            c_op = op2;
		}
		else
			c_op = op1;
    }

	
	/*
	//Report Critical Path
	for (int i = 0; i < criticalPath.size(); i++) {
		operation *c_op = criticalPath[i];
		printf("[%d,%d]->", c_op->jobID, c_op->ID);
	}
	printf("\n");
	*/
}

int solution::numberOfCriticalOps() {
    //Calculate tail times for all operations
    int opCounter = this->totalNumOp;
    int counter = 0;

    setDoneStatus(false, false);
    int cmax = (int) cost_array[MAKESPAN];
    while (counter < opCounter) {
        for (int i=0; i < opCounter; i++){
            operation *op = soloperations[i];
            if (!op->valid) continue;
            if (op->done) continue;

            operation *mach_suc = op->get_mach_suc();
            if (!mach_suc->valid) mach_suc = nullptr; //get rid of dummy ops

            operation *job_suc = op->get_suc();

            //Last operations
            if ((mach_suc == nullptr) && (job_suc == nullptr)) {
                //op->tail = 0;
                op->tail = cmax - op->end;
                op->done = true;
                counter++;
                continue;
            }

            int mach_suc_part = 0;
            if (mach_suc != nullptr) {
                if (!mach_suc->done) continue;
                mach_suc_part = mach_suc->tail + mach_suc->getProcTime();
            }


            int job_suc_part = 0;
            if (job_suc != nullptr){
                if (!job_suc->done) continue;
                job_suc_part = job_suc->tail + job_suc->getProcTime();
            }

            op->tail = max(mach_suc_part, job_suc_part);
            op->done = true;
            counter++;

        }
    }
    setDoneStatus(false, true);

    //Find Critical Operations
    int crit_ops_num = 0;
    for (int i=0; i < opCounter; i++) {
        operation *op = soloperations[i];
        if ((op->start + op->getProcTime() + op->tail - (int) this->cost_array[0]) == 0)
            crit_ops_num++;
    }

    return crit_ops_num;
}

void solution::findCriticalPath_viaSlack() {
    //Calculate tail times for all operations
    int opCounter = this->totalNumOp;
    int counter = 0;


    FGraph->criticalPath.clear();

    setDoneStatus(false, false);
	int cmax = (int) cost_array[MAKESPAN];
    while (counter < opCounter){
        for (int i=0; i < opCounter; i++){
            operation *op = soloperations[i];
            if (!op->valid) continue;
            if (op->done) continue;

            operation *mach_suc = op->get_mach_suc();
            if (!mach_suc->valid) mach_suc = nullptr; //get rid of dummy ops

            bool has_job_sucs = false;
#if(COMPLEX_PRECEDENCE_CONSTRAINTS)
            for (int kk=0;kk<MAX_JOB_RELATIONS;kk++){
                operation *job_suc = op->get_suc(kk);
                has_job_sucs =  has_job_sucs || (job_suc != nullptr);
            }
#else
            operation *job_suc = op->get_suc();
            has_job_sucs = !(job_suc == nullptr);
#endif
            //Last operations
            if ((mach_suc == nullptr) && !has_job_sucs) {
                //op->tail = 0;
                op->tail = cmax - op->end;
                op->done = true;
                counter++;
                continue;
            }


            int mach_suc_part = 0;
            if (mach_suc != nullptr) {
                if (!mach_suc->done) continue;
                mach_suc_part = mach_suc->tail + mach_suc->getProcTime();
            }


            int job_suc_part = 0;
            if (has_job_sucs) {
#if(COMPLEX_PRECEDENCE_CONSTRAINTS)
                for (int kk=0; kk<MAX_JOB_RELATIONS; kk++){
                    operation *job_suc = op->get_suc(kk);
                    if (job_suc== nullptr) break;
                    job_suc_part = max(job_suc_part, job_suc->tail + job_suc->getProcTime());
                }
#else
                if (!job_suc->done) continue;
                job_suc_part = job_suc->tail + job_suc->getProcTime();
#endif
            }

            op->tail = max(mach_suc_part, job_suc_part);
            op->done = true;
            counter++;
        }
    }
	setDoneStatus(false, true);

    //Find Critical Operations
    for (int i=0; i < opCounter; i++) {
        operation *op = soloperations[i];
        if ((op->start + op->getProcTime() + op->tail - (int) this->cost_array[0]) == 0) {
            op->critical = true;
            //printf("Critical Operation %d\n", op->UID);
            FGraph->criticalPath.push_back(op);
        }
        else
            op->critical = false;
    }

    sort(FGraph->criticalPath.begin(), FGraph->criticalPath.end(), [](operation* lhs, operation* rhs)
    {
        return lhs->start > rhs->start;
    });


    /*
	printf("Critical Path: ");
    for (int i=0;i<FGraph->criticalPath.size();i++)
        printf("%d ",FGraph->criticalPath[i]->UID);
    printf("\n");
	*/
}


void solution::findCriticalBlocks(int mode) {

	criticalBlocks.clear();

	//Load a critical path
	//cout << *this;
	//vector<operation*> cPath = findCriticalPath();

	findCriticalPath_viaSlack();
    vector<operation*> cPath = FGraph->criticalPath;


    //Search for critical blocks
    switch(mode) {
        //Job Critical Blocks
        case 0:
            //Search for job critical blocks
            for (int i = 0; i < (int) this->soljobs.size(); i++) {
                job *jb = soljobs[i];
                vector<operation*> t_ar = vector<operation*>();

                for (int j = 0; j < (int) cPath.size(); j++) {
                    operation *op = cPath[j];
                    if (op->jobID == jb->ID)
                        t_ar.push_back(op);
                }

                //Post process t_ar
                if (t_ar.size() >= 2) {
                    //printf("Job %d is critical: %d ops\n", jb->ID, t_ar.size());
                    sort(t_ar.begin(), t_ar.end(), [](operation* lhs, operation* rhs)
                    {
                        return lhs->ID < rhs->ID;
                    });

                    //Save range
                    int3_tuple t = int3_tuple(i, t_ar[0]->ID, t_ar[t_ar.size() - 1]->ID);
					criticalBlocks.push_back(t);
                    for (int j = 0; j < (int) t_ar.size(); j++) {
                        operation *op = t_ar[j];
                        //printf("Critical Job-Block Operation %d %d\n", op->jobID, op->ID);
                    }

                }
                //Cleanup
                t_ar.clear();
            }
            break;
        //Search for Machine Critical Blocks
        case 1:
            //NEW VERSION
            for (int m =0;m<(int) solmachines.size();m++){
                machine *mc = solmachines[m];
                int index = 1;
                int block_s = -1;
                for (int index = 1;index < (int) mc->operations.size()-1;index++) {
                    operation *c_op = mc->operations[index];
                    if (c_op->critical){
                        if (block_s == -1)
                            block_s = index;
                    } else {
                        //Save block
                        if ((block_s != -1) && ((index - block_s) > 1)) {
							criticalBlocks.push_back(int3_tuple(m, block_s, index));
                            block_s = -1;
                        }
                    }
					//Detect whole machine critical block
					if ((index == (int) mc->operations.size() - 2) && (block_s != -1) && (index - block_s > 1)) {
						criticalBlocks.push_back(int3_tuple(m, block_s, (int) mc->operations.size() - 1));
					}
				}
            }

            /*
             * //OLD CODE
			//cout << *this;
            //Search for machine critical blocks
            int m_id = -1;
            int range = 0;
            int start_pos = 0;
            for (int i= (int) cPath.size() - 1; i>=0; i--){
                operation *c_op = cPath[i];

                if (c_op->mc->ID != m_id){
                    //Save the block
                    if ((m_id >= 0) && (range > 0)) {
                        cJobRanges.push_back(int3_tuple(m_id, start_pos, start_pos + range + 1));
                    }

                    m_id = c_op->mc->ID;
                    start_pos = c_op->mthesi;
                    range = 0;
                } else {
                    range++;
                }
            }
			//Save Last Block
			if ((m_id >= 0) && (range > 0)) {
				cJobRanges.push_back(int3_tuple(m_id, start_pos, start_pos + range + 1));
			}

             */

			//Report Machine Blocks

			/*
			for (int i=0; i< (int) cJobRanges.size(); i++){
				printf("Block %d Machine %d Range: %d-%d\n",
					   i, get<0>(cJobRanges[i]), get<1>(cJobRanges[i]), get<2>(cJobRanges[i]));
			}
			*/



            break;
    }
}