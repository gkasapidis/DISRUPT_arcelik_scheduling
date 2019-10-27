#include <iostream>
#include <ostream>
#include "OperationClass.h"

using namespace std;

//Operation Class Methods

operation::operation() {
	UID = -1;
	ID=-1;
	jobID=-1;
	
	actualprocess_stepID = -1;
	process_stepID = -1;
	opertypeID = 0;
	machID = -1;
	default_machID = -1;
	temp_machID = -1;
	
	setup_time = 0;
	cleanup_time = 0;
	start = -1;
	temp_start = -1;
	end = -1;
	temp_end = -1;
	a = planhorstart;
	b = planhorend;
	is_temp = false;
	done = false;
	temp_done = false;
	inh_ok = NULL;
	for (int i = 0; i < MAX_JOB_RELATIONS; i++) {
		pred[i] = NULL;
		suc[i] = NULL;
	}
	mc = NULL;
    tail = 0;
	


#if(FLEX)
    //Uses 1-Indexing to match with the database as well as the rest of the structs
	var_processing_time_UM = new int[exMACHINETYPECOUNT];
    const_processing_time_UM = 0;
#else
	processing_time = 0;
#endif

	prod_res = new resource();

	//Connections Vector
	//WARNING: Set maximum connections to 4
	g_state = NULL;
};

operation::~operation()
{
	mc = NULL;
#if(FLEX)
    delete[] var_processing_time_UM;
#endif

    //Delete Local Resource
    delete prod_res;
};

operation::operation(const operation &copyin) {
	ID = copyin.ID;
	UID = copyin.UID;
	articlestepsequenceID = copyin.articlestepsequenceID;
	opertypeID = copyin.opertypeID;
	jobID = copyin.jobID;
	actualprocess_stepID = copyin.actualprocess_stepID;
	process_stepID = copyin.process_stepID;
	prodphase = copyin.prodphase;

	setup_time = copyin.setup_time;
	cleanup_time = copyin.cleanup_time;
	a = copyin.a;
	b = copyin.b;

	start = copyin.start;
	temp_start = copyin.temp_start;
	end = copyin.end;
	temp_end = copyin.temp_end;
	
	machID = copyin.machID;
	temp_machID = copyin.temp_machID;
	default_machID = copyin.default_machID;
	mthesi = copyin.mthesi;
	temp_mthesi = copyin.temp_mthesi;
	done = copyin.done;
	temp_done = copyin.temp_done;
	is_temp = copyin.is_temp;
	sca = copyin.sca;
	scb = copyin.scb;
    fixed = copyin.fixed;

	for (int i = 0, i_end = MAX_JOB_RELATIONS; i < i_end; i++) pred[i] = copyin.pred[i];

	for (int i = 0, i_end = MAX_JOB_RELATIONS; i < i_end; i++) suc[i] = copyin.suc[i];


    const_processing_time_UM = copyin.const_processing_time_UM;
    for (int k=0; k<exMACHINETYPECOUNT; k++)
        var_processing_time_UM[k] = copyin.var_processing_time_UM[k];

    //Copy resource
    *prod_res = *copyin.prod_res;

};

operation& operation::operator=(const operation &rhs) {
	ID = rhs.ID;
	UID = rhs.UID;
	articlestepsequenceID = rhs.articlestepsequenceID;
	opertypeID = rhs.opertypeID;
	this->jobID = rhs.jobID;
	this->actualprocess_stepID = rhs.actualprocess_stepID;
	this->process_stepID = rhs.process_stepID;
	this->prodphase = rhs.prodphase;

	this->setup_time = rhs.setup_time;
	this->cleanup_time = rhs.cleanup_time;
	this->a = rhs.a;
	this->b = rhs.b;
	this->weight = rhs.weight;
	
	start = rhs.start;
	temp_start = rhs.temp_start;
	end = rhs.end;
	temp_end = rhs.temp_end;
	machID = rhs.machID;
	temp_machID = rhs.temp_machID;
    default_machID = rhs.default_machID;
	mthesi = rhs.mthesi;
	temp_mthesi = rhs.temp_mthesi;

	this->done = rhs.done;
	this->temp_done = rhs.temp_done;
    this->is_temp = rhs.is_temp;
	this->sca = rhs.sca;
	this->scb = rhs.scb;

	this->moving = rhs.moving;
	this->move_freq = rhs.move_freq;
	this->fixed = rhs.fixed;
	
	for (int i = 0, i_end = MAX_JOB_RELATIONS; i < i_end; i++) this->pred[i] = rhs.pred[i];

	for (int i = 0, i_end = MAX_JOB_RELATIONS; i < i_end; i++) this->suc[i] = rhs.suc[i];

#if(FLEX)
    const_processing_time_UM = rhs.const_processing_time_UM;
    for (int k=0; k<exMACHINETYPECOUNT; k++)
        var_processing_time_UM[k] = rhs.var_processing_time_UM[k];
#else
	this->processing_time = rhs.processing_time;
#endif

	Node::operator=(rhs);

    //Copy resource
    *prod_res = *rhs.prod_res;
    return *this;
};

bool operation::operator==(const operation &rhs){
	if ((this->jobID != rhs.jobID) && (this->ID != rhs.ID) && (this->mc->ID != rhs.mc->ID)) return false;

	//Check Node attributes and Connections
	if (!Node::operator==(rhs)) return false;

	//if they are both invalid get out of here
	if ((this->valid && rhs.valid) == false) return true;
	
	//Connections should be checked from the graph
    return true;
}


bool operation::operator!=(const operation &rhs){
	if (*this == rhs) return false;
	return true;
}

bool operation::has_preds() {
    if (this->pred[0] != nullptr) return true;
	return false;
}

bool operation::has_sucs() {
	if (this->suc[0] != nullptr) return true;
	return false;
}

bool operation::has_mach_sucs() {
	if (this->mc == nullptr) return false;

	int mthesi_t;
	if (this->is_temp) mthesi_t = temp_mthesi;
	else mthesi_t = mthesi;

	if ((mthesi_t < (int) mc->operations.size() - 2) && (mthesi_t > 0)) return true;	

	return false;
}

bool operation::has_mach_preds() {
	if (this->mc == NULL) return false;

	int mthesi_t;
	if (this->is_temp) mthesi_t = temp_mthesi;
	else mthesi_t = mthesi;

	//if ((mthesi_t < (int) mc->operations.size() - 1) && (mthesi_t > 1)) return true;	
	//Modified to support dummy operations
	if ((mthesi_t < (int) mc->operations.size() - 1) && (mthesi_t > 0)) return true;	
	
	return false;
}

operation* operation::get_suc() {
	return this->suc[0];
}
operation* operation::get_suc(int k) {
    return this->suc[k];
}
operation* operation::get_pred() {
	return this->pred[0];
}
operation* operation::get_pred(int k) {
    return this->pred[k];
}

operation* operation::get_mach_suc() {
	// Return the machine successor
	// No need to check for proper machine position in here
	// has_mach_sucs, has probably been called before
	if (mc == NULL) return NULL;

	int mthesi_t = temp_mthesi;

	if (!is_temp)
		mthesi_t = mthesi;

    return mc->operations[mthesi_t + 1];
}

operation* operation::get_mach_pred() {
	// Return the machine successor
	// No need to check for proper machin position in here
	// has_mach_sucs, has probably been called before
	if (mc == NULL) return NULL;

	int mthesi_t = temp_mthesi;
	if (!is_temp)
		mthesi_t = mthesi;
	
	return mc->operations[mthesi_t - 1];
}


bool operation::is_pred(operation *op) {
	if (this->has_preds()) {
		for (int i = 0; i < MAX_JOB_RELATIONS; i++) {
			operation *pop = this->pred[i];
			if (pop == NULL) break;
			if (op->isEqual(pop)) return true; //Check if we hit the pred
		}
	}
	return false;
}

bool operation::is_mach_pred(operation *op) {
	if (this->has_mach_preds()) {
		operation* mach_pred = get_mach_pred();
		if (mach_pred == NULL) return false;
		if (mach_pred->isEqual(op)) return true;
	}
	return false;
}


bool operation::is_suc(operation *op) {
	if (this->has_sucs()) {
		for (int i = 0; i < MAX_JOB_RELATIONS; i++) {
			operation *sop = this->suc[i];
			if (sop == NULL) break;
			if (op->isEqual(sop)) return true;
		}
	}

	return false;
}

bool operation::is_mach_suc(operation *op) {
	if (this->has_mach_sucs()) {
		operation* mach_suc = get_mach_suc();
		if (mach_suc == NULL) return false;
		if (mach_suc->isEqual(op)) return true;
	}
	return false;
}

bool operation::isEqual(operation *op) {
	if ((op->jobID == this->jobID) && (op->ID == this->ID)) return true;
	return false;
}

bool operation::check_predRelation(operation *op) {
#if(COMPLEX_PRECEDENCE_CONSTRAINTS)
    for (int i=0;i<MAX_JOB_RELATIONS;i++){
        operation *pr = this->pred[i];
        if (pr == nullptr) break;
        if (pr->isEqual(op)) return true;
    }
#else
	if ((op->jobID == this->jobID) && (op->ID < this->ID)) return true;
#endif
	return false;
}

bool operation::check_sucRelation(operation *op) {
#if(COMPLEX_PRECEDENCE_CONSTRAINTS)
    for (int i=0;i<MAX_JOB_RELATIONS;i++){
        operation *sc = this->suc[i];
        if (sc == nullptr) break;
        if (sc->isEqual(op)) return true;
    }
#else
    if ((op->jobID == this->jobID) && (op->ID > this->ID)) return true;
#endif

	return false;
}

//Return process time of op on assigned machine
int operation::getProcTime() const {
//CHECK IF NULL FOR DUMMY OPERATIONS
if (this->mc == NULL) return 0;

#if (IDENTICAL_MACHINES)
	return this->processing_time;
#elif (UNIFORM_MACHINES)
	return this->mc->speed * this->processing_time;
#elif (UNRELATED_MACHINES)
	#if(FLEX)
	    float pt = (float) const_processing_time_UM;

	    for (int i=exmSMT; i<exMACHINETYPECOUNT; i++)
            pt += (float) var_processing_time_UM[i] / max(1, mc->parallel_processors[i]);

	    return (int) ceil(pt);

        #else
		return this->processing_time;
	#endif
#endif
}

//Return process time of op on any system machine
int operation::getProcTime(machine *mmc)  const  {

#if(FLEX)

    float pt = (float) const_processing_time_UM;

    for (int i=exmSMT; i<exMACHINETYPECOUNT; i++)
        pt += (float) var_processing_time_UM[i] / max(1, mmc->parallel_processors[i]);

    return (int) ceil(pt);

#else
	return this->processing_time;
#endif
}

void operation::resetProcessingTimes(){
#ifdef FLEX
    const_processing_time_UM = 0;
    for (int i=exmSMT; i <exMACHINETYPECOUNT; i++){
        var_processing_time_UM[i] = 0;
    }
#endif
}

int operation::getStartTime()  const {
	if (this->is_temp) return this->temp_start;
	else return this->start;
}

int operation::getEndTime()  const {
	if (this->is_temp) return this->temp_end;
	else return this->end;
}

void operation::setStartTime(int value) {
	if (this->is_temp) this->temp_start = value;
	else this->start = value;
}

void operation::setEndTime(int value) {
	if (this->is_temp) this->temp_end = value;
	else this->end = value;
}

bool operation::recursiveCheck(operation* next, int level) {
	bool currentReport = false;
	bool prevReport = false;
	bool predReport = false;
	
	int newlevel = level + 1;
	if (newlevel > 500) {return true;}; //If limit has been reached we're probably in a circle loop
	if (inh_ok != NULL) return inh_ok;

	//Check op
	currentReport = this->check_predRelation(next);

	//Check prev if there is any
	//Working with temp position
	if (this->temp_mthesi > 1) {
		operation* prev = mc->operations[this->temp_mthesi - 1];
		
		prevReport = prev->recursiveCheck(next, newlevel);
	}

	//Check pred if there is any
	if (this->has_preds()) {
		operation* pred = this->pred[0];
		predReport = pred->recursiveCheck(next, newlevel);
	}

	this->inh_ok = prevReport || currentReport || predReport; //Store 
	return this->inh_ok;
}

bool operation::fixTimes(bool finalize) {
	//Explicitly handle first dummy ops here
	if (this->mthesi == 0 || this->temp_mthesi==0) {
		setStartTime(0);
		setEndTime(0);
		this->done = true;
		this->temp_done = true;
		return true;
	}

	//If operation done, this process has been done before, exit
	//if (this->done && !this->is_temp) return;

	//Init Values
	int prev_start = 0;
	int pred_start = 0;
	
	//Reset Start
	setStartTime(0);
	//Change Flag
	bool changeFlag = false;

	//Check Preds
	if (has_preds()) {
		//Check only first pred for now
		operation *pred = this->pred[0];
		
		if (!pred->done){
			//cout << this->jobID << " " <<this->ID << " ->exiting because pred " <<pred->jobID << " " << pred->ID << " not done" << endl;
			return false; //Exit if predecessor is not done
		}
		
		pred_start = pred->getEndTime();
		if (getStartTime() < pred_start) {
			changeFlag = true;
			setStartTime(pred_start); 
		}
	}

	//Check Prevs
	if (this->mthesi > 1) {
		operation *prev = mc->operations[mthesi - 1];
		prev_start = prev->getEndTime();
		if (getStartTime() < prev_start) {
			changeFlag = true;
			setStartTime(prev_start);
		}
	}

	//Set Start & End Times
	//this->setStartTime(max(prev_start, pred_start));
	this->setEndTime(this->getStartTime() + this->getProcTime());
	this->storeTemp();
	
	//Fix times on next operations if changes happened

	if (changeFlag) {
		//Handle next in machine queue
		if ((this->mthesi + 1) <(int) this->mc->operations.size() - 1) {
			operation *nop = mc->operations[this->mthesi + 1];
			if (nop->getStartTime() < this->getEndTime()) {
				nop->setStartTime(this->getEndTime());
				nop->setEndTime(nop->getStartTime() + nop->getProcTime());
			}
		}

		//Handle job successors
		if (this->has_sucs()) {
			operation *suc = this->suc[0];
			if ((suc->mc != NULL)) {
				if (suc->getStartTime() < this->getEndTime()) {
					//NEW TEST
					//DO NOT FIX ANYTHING HERE
					//IF SUCCESSOR START TIMES AER INFEASIBLE RETURN FALSE
					//return false;

					//MY OLD WAY, TRY TO FIX SUCCESSOR TIMES AS WELL					
					
				
						suc->setStartTime(this->getEndTime());
					suc->setEndTime(suc->getStartTime() + suc->getProcTime());
				}
			}
		}
	}

	
	if (finalize) this->setDone();
	return true;

}

void operation::changeMach(machine *mc, int mach_pos) {

	//Set new machine
	this->mc = mc;
	machID = mc->ID;
	mthesi = mach_pos;
	temp_machID = mc->ID;
	temp_mthesi = mach_pos;
	
	//Changing machines results into temporary mode
	this->resetDone();
}

void operation::storeTemp() {
	start = temp_start;
	end = temp_end;
	done = temp_done;
	mthesi = temp_mthesi;
	is_temp = false;
}

void operation::storePerm() {
	temp_start = start;
	temp_end = end;
	temp_done = done;
	temp_mthesi = mthesi;
	is_temp = false;
}

void operation::resetDone() {
	done = false;
	temp_done = false;
	is_temp = true;
}

void operation::setDone() {
	done = true;
	temp_done = true;
	is_temp = false;
}

void operation::setDoneStatus(bool temp, bool status){
	switch (temp) {
	case 1:
		temp_done = status;
		break;
	case 0:
		done = status;
		temp_done = status;
		break;
	}
	
	is_temp = !status;
}

void operation::setMoving() {
    moving = true;
}

void operation::setInfeasible() {
    infeasible = true;
}

bool operation::isDone() {
	switch (is_temp) {
	case 0:
		return done;
	case 1:
		return temp_done;
	}
}

//Operation Graph Methods
bool operation::connect(operation *op, bool temp, int type) {
	switch (*g_state) {
	case FORWARD: return f_connect(op, temp, type); break;
	case BACKWARD: return b_connect(op, temp, type); break;
	}
}

bool operation::f_connect(operation *op, bool temp, int type) {
	printf("Not Implemented/n");
	assert(false);

	return true;
}

bool operation::b_connect(operation *op, bool temp, int type) {
	printf("Not Implemented/n");
	assert(false);

	return true;
}

bool operation::disconnect(operation *op) {
	//printf("Disconnecting node %d %d from node %d %d \n", JobID, OpID, node->JobID, node->OpID);
	//Proceed with the disconnection
	removeConnection(tIndex);
	return true;
}

void operation::removeConnection(int i) {
	//Nullify the connection in the specified index
	switch (*g_state)
	{
		case FORWARD: f_removeConnection(i); break;
		case BACKWARD: b_removeConnection(i); break;
	}
}

void operation::f_removeConnection(int i) {
	//Nullify the connection in the specified index
	printf("Not Implemented/n");
	assert(false);
}

void operation::b_removeConnection(int i) {
	printf("Not Implemented/n");
	assert(false);
}


bool operation::isConnected(operation *node) {

	switch (*g_state) {
		case FORWARD:  return f_isConnected(node); break;
		case BACKWARD: return b_isConnected(node); break;
	}
}

bool operation::f_isConnected(operation *node) {

	printf("Not Implemented/n");
	assert(false);
	return false;
}
bool operation::b_isConnected(operation *node) {

	printf("Not Implemented/n");
	assert(false);
	return false;
}

void operation::report(){
    printf("Operation %d - Process %d - Job [%d,%d] ", UID, process_stepID, jobID, ID);
    printf("Processing Times: ");
    for (int i=0;i<NumberOfProcSteps+2;i++)
        if (getProcTime()>0) printf("%d:%d ",i, getProcTime());
    printf("\n");
}



template<>
void Graph<operation>::visit(int node) {
    if (visited[node]) return;
	//Set Node visited
	//printf("Visiting Node %d %d \n", node->JobID, node->OpID);
	adj_list_elem<operation> node_op = adj_list[node];
	visited[node] = true;
	visitedNodes[visitedNodes_n] = node_op.op;
	visitedNodes_n++;
	stack_list[node] = true;

    //Call custom function
    //CUSTOM FUNCTION CALL NOT NEEDED SINCE WE DO NOT USE THE INFORMATION FOR NOW
    //(*node_op.op.*fptr)();

	//Visit Connected nodes
	int conn;
	//Iterate in permanent Connections first
#if MAX_JOB_RELATIONS > 1
	for (int i = 0; i < MAX_JOB_RELATIONS; i++) {
		conn = node_op.p_connections[i];
		if (conn < 0) break;

		if (!visited[conn])
			//conn->visit(gr, state, stack, cycleflag, visitedNodes, visitedNodes_n);
			visit(conn);
		else if (stack_list[conn]) {
			cycleflag = true;
			return;
		}
	}
#else
	//Visit p_conn
	conn = adj_list[node].p_connections[0];
	if (conn != NULL) {
		if (!visited[conn])
			visit(conn);
		else if (stack[conn]) {
			cycleflag = true;
			return;
		}
	}
#endif

	//Visit t_conn
	conn = node_op.t_connection;
	if (conn >= 0) {
		if (!visited[conn])
			visit(conn);
		else if (stack_list[conn]) {
			cycleflag = true;
			return;
		}
	}

	//Reset the status
	stack_list[node] = false;

	return;

}




template<>
void Graph<operation>::t_sort(int node) {
	if (visited[node]) return;
	//Set Node visited
	//printf("Visiting Node %d %d \n", node->JobID, node->OpID);
	adj_list_elem<operation> node_op = adj_list[node];
	visited[node] = true;
	visitedNodes[visitedNodes_n] = node_op.op;
	visitedNodes_n++;
	stack_list[node] = true;


	//Visit Connected nodes
	int conn;
	//Iterate in permanent Connections first
#if MAX_JOB_RELATIONS > 1
	for (int i = 0; i < MAX_JOB_RELATIONS; i++) {
		conn = node_op.p_connections[i];
		if (conn < 0) break;

		if (!visited[conn])
			//conn->visit(gr, state, stack, cycleflag, visitedNodes, visitedNodes_n);
			t_sort(conn);
		else if (stack_list[conn]) {
			//cout << " Cycle Detected" << endl;
			//int shit;
			//scanf("%d\n", &shit);
			cycleflag = true;
			return;
		}
	}
#else
	//Visit p_conn
	conn = adj_list[node].p_connections[0];
	if (conn != NULL) {
		if (!visited[conn])
			t_sort(conn);
		else if (stack[conn]) {
			cycleflag = true;
			return;
		}
	}
#endif

	//Visit t_conn
	conn = node_op.t_connection;
	if (conn >= 0) {
		if (!visited[conn])
			t_sort(conn);
		else if (stack_list[conn]) {
			cycleflag = true;
			return;
		}
	}

	//Reset the status
	stack_list[node] = false;

	//Push to stack
	top_sort.push_front(node_op.op);

	return;

}

template<>
void Graph<operation>::report_topological_sort(){
	printf("Graph Topological Sort \n");
	for (int i = 0; i < size; i++) {
		//if (top_sort[i]->valid)
        printf("%d ", top_sort[i]->UID);
	}
	printf("\n");
}