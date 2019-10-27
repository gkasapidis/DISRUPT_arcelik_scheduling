//Version 8
//Bug Fixes: IDs and ActualIDs for operations, process steps and machines + cutoff values for interoute exchange and relocate
//Assigned weights to machines based on priorities
//Fixed Tidle time calculation and added weights on the calculation (see scenario 0)
//Queries for reading resource alarms and consumptions data


//Version 9
//Control panel for internal parameter tuning + time limit


//Version 10
//Code enhancements in the definition of classes
//Parallel machines
//Multiple hierarchical objective functions
//....


//TODO List
//Create function to update planhorizonend global variable
//Consider in all functions Release and Due date constraints + calculation of Tardiness
//Create new optimization types for tardiness
//Incorporate buffer inventories
//Incorporate lines
//Merge findConflicts & testSolution


//DONE List
//Enhance combined neighborhoods to consider all optimization types
//New data structures for the RC feasibility checks
//Fix RC code, its working now
//Incorporate replace_temp to the operations class
//Delete store best from class problem
//Move to class solution print, getcost, claculate functions, testsolution
//In class problem define current_sol and tabu_sol ; delete cost variables
//Check pred status on relocate & exchange moves

#include <iomanip>
#include <sstream>
#include <ostream>
#include <map>
#include <algorithm>
#include <memory.h>
#include <sys/stat.h>
#include <time.h>
//#include <libzvbi.h>


using namespace std;

//CUSTOM CODE INCLUDES
#include "Parameters.h"
#include "ListObject.h"
#include "ProbabilisticMoveClass.h"
#include "ProblemInterface.h"
#include "StageClass.h"
#include "Resource.h"
//DataClasses
#include "DataClasses/ProblemDataset.h"

#include "SolutionClass.h"
#include "LSSolutionClass.h"
#include "simplePool.h"
#include "SSPool.h"

//CUSTOM UTILITIES
#include "string_util.h"

//Global parameters defined from main arguments
int jobID = 0;
int alternativeActionID = 0;
int executionMode = 0;
int scheduleCount = 0;
string reschedulingTime;
string productionStartTime;
string productionEndTime;
int eventCount = 0;
int *eventIDs;
int scopetype = 0;

//Choose objective function
//Single Objective
//1.	makespan
//2.	TFT
//3.	Tidle
//10. Energyt

//Hierarchical
//4.	makespan - TFT - Tidle
//5.	TFT - Tidle   - makespan
//6.	Tidle - makespan - TFT
//7.	makespan - Tidle - TFT
//8.	TFT - makespan - Tidle
//9.	Tidle - TFT - makespan

//11. Energy - Total Flow Time - Makespan
//12. Energy - Makespan -  Total Flow Time

//int optimtype = 1; //MAKESPAN
int optimtype = 7;
int seed = 4987924;
double time_elapsed = 0.0;
int procID = -1;
int OLDexecutionID = -1;

struct tm PlanningHorizonStart_Minus;
struct tm ReschedulingTime;
struct tm PlanningHorizonStart;
struct tm PlanningHorizonEnd;

time_t now = time(NULL);
time_t temp = time(NULL);

long planhorstart = 0;
long planhorend = INF;
int reopt_time = 0;

int NumberOfOrders = 0;
int NumberOfJobs = 0;
int NumberOfMachines = 0;
int NumberOfOperations = 0;
int NumberOfStages = 0;
int NumberOfProcSteps = 0;
int NumberOfResources = 0;
int NumberOfArticles = 0;

//DISRUPT Parameters
int LotSize = 100;
int BlackBoxSize = 50;

//Init Globals Interface
ProblemGlobals *globals = new ProblemGlobals();
string benchfile;
string solutionfile;
string tabuprogressfile;

//Algorithms Enum
enum algorithms{TABU_SEARCH, VNS, VNS_TS, SCATTER_SEARCH, ALGORITHMS_COUNT};

//Algorithm Parameters init
int algo = TABU_SEARCH;
int restart_iterations = 100;
int tabu_iterations = 300;
int	tabu_tenure = 30;
bool firstbest = false;
int time_limit = 15 * 6000;
int tabu_perturbation_limit = 1000;
float tabu_relaxation = 0.5;

//Pool Settings 
int pool_size = 5;
int pool_lifespan = 20;

//Operation Frequency Map Settings
int timehorizon = 4000;
int timeinterval = 5;

//Setup Cost Calculation Flags
int MAKESPAN_F = 1;
int TOTAL_FLOW_TIME_F = 1;
int TOTAL_IDLE_TIME_F = 1;
int TOTAL_ENERGY_F = 0;
int TARDINESS_F = 0;
int SLACK_F = 1;
int DISTANCE_F = 4;


//Temp data
vector<bl_pair> bllist = vector<bl_pair>();
vector<solution*> solutions = vector<solution*>();

//DATABASE STUFF
struct database_settings arcelik_db_settings;
struct database_settings disrupt_db_settings;
struct database_connection arcelik_db_conn;
struct database_connection disrupt_db_conn;

//-------------------------------------
//-------------------------------------
//SETTINGS PARSERS
int read_int_setting(ifstream &file){
    istringstream Sin;
    string setting_name;
    int setting;
    char Line[1500];
    file.getline(Line, 1500);
    Sin.clear();
    Sin.str(Line);
    Sin >> setting_name >> setting;
    printf("\t%s = %d\n",setting_name.c_str(), setting);
    return setting;
}

string read_string_setting(ifstream &file){
    istringstream Sin;
    string setting_name;
    string setting;
    char Line[1500];
    file.getline(Line, 1500);
    Sin.clear();
    Sin.str(Line);
    Sin >> setting_name >> setting;
    printf("\t%s = %s\n",setting_name.c_str(), setting.c_str());
    return setting;
}


int read_settings() {
    //Try to parse algorithm data from settings.ini;
    string file = "./settings.ini";
    //Check If settings file exists
    struct stat buf;
    bool file_exists = false;
    if (stat(file.c_str(), &buf) != -1) {
        ifstream myfile;
        istringstream Sin;
        printf("Reading Settings from settings.ini\n");
        file_exists = true;
        myfile.open(file);
        //Algorithm
        algo = read_int_setting(myfile);
        //Restarts
        restart_iterations = read_int_setting(myfile);
        //FirstBest
        firstbest = (bool) read_int_setting(myfile);
        //Time Limit
        time_limit = read_int_setting(myfile);
        //Tabu Iterations
        tabu_iterations = read_int_setting(myfile);
        //Tabu Perturbation Limit
        tabu_perturbation_limit = read_int_setting(myfile);
        //Tabu Tenure
        tabu_tenure = read_int_setting(myfile);
        myfile.close();
    } else {
        printf("Missing Settings File, Using defaults...");
        ofstream myfile;
        myfile.open(file);
        //Set Values for runtime
        algo = TABU_SEARCH;
        restart_iterations = 250; //Number of restarts (Greedy Randomized Construction Heuristic - Tabu Search) (must be >= 1)
        firstbest = false; //True - Best Accept / False - First Best
        time_limit = 15 * 6000; //Time limit of Tabu Search (in seconds)
        tabu_iterations = 20000; //Number of internal tabu search iterations without observing improvement (must be >=1)
        tabu_perturbation_limit  = 1000; //Number of non improving iterations after which perturbation will be applied
        tabu_tenure = 20; //Tabu Move Life/Search History (must be between 12 and 60) must be >= tabu_list_size to make sense
        tabu_relaxation = 0.0f;
        //Write the settings file
        myfile << "Algorithm " << algo << endl;
        myfile << "Restarts " << restart_iterations << endl;
        myfile << "FirstBest " << firstbest << endl;
        myfile << "TimeLimit " << time_limit << endl;
        myfile << "TabuIterations " << tabu_iterations << endl;
        myfile << "TabuPerturbationLimit " << tabu_perturbation_limit << endl;
        myfile << "TabuTenure " << tabu_tenure << endl;
        myfile.close();
    }

    //Read Database settings
    //Parse db.ini
    file = "./db.ini";
    ifstream myfile;

    if (stat(file.c_str(), &buf) != -1){
        myfile.open(file);
        char Line[1500];
        istringstream Sin;
        string setting;

        string hostname = read_string_setting(myfile);
        string user = read_string_setting(myfile);
        string pass = read_string_setting(myfile);
        string arcelik_dbname = read_string_setting(myfile);
        string disrupt_dbname = read_string_setting(myfile);
        int port = read_int_setting(myfile);
        myfile.close();

        //Populate Connections
        arcelik_db_settings.hostname = hostname;
        arcelik_db_settings.user = user;
        arcelik_db_settings.pass = pass;
        arcelik_db_settings.db_name = arcelik_dbname;
        arcelik_db_settings.port = port;

        disrupt_db_settings.hostname = hostname;
        disrupt_db_settings.user = user;
        disrupt_db_settings.pass = pass;
        disrupt_db_settings.db_name = disrupt_dbname;
        disrupt_db_settings.port = port;

    } else {
        printf("Missing Database Configuration File, aborting execution...");
        return false;
    }

    return true;
}







//*************************************************************************************************************
//*************************************************************************************************************
//Scheduling classes, paramaters and variables

//Scheduling Algorithm Classes
class neighbor
{
public:
	bool feasible;
	int cost1;
	int cost2;
	int cost3;
	int cost4;

	friend ostream &operator<<(ostream &output, const neighbor &aaa) {
		output << aaa.feasible << ' ' << aaa.cost1 << ' ' << aaa.cost2 << ' ' << aaa.cost3 << ' ' <<aaa.cost4 << endl;
		return output;
	}
	neighbor() {
		feasible = false;
		cost1 = INF;
		cost2 = INF;
		cost3 = INF;
		cost4 = INF;
	}
	// Copy constructor to handle pass by value.
	neighbor(const neighbor &copyin) {
		cost1 = copyin.cost1;
		cost2 = copyin.cost2;
		cost3 = copyin.cost3;
		cost4 = copyin.cost4;
		feasible = copyin.feasible;
	}
	~neighbor() {}
	neighbor &operator=(const neighbor &rhs) {
		this->cost1 = rhs.cost1;
		this->cost2 = rhs.cost2;
		this->cost3 = rhs.cost3;
		this->cost4 = rhs.cost4;
		this->feasible = rhs.feasible;
		return *this;
	}
	int operator==(const neighbor &rhs) const {
		if (this->cost1 != rhs.cost1) return 0;
		if (this->cost2 != rhs.cost2) return 0;
		if (this->cost3 != rhs.cost3) return 0;
		if (this->cost4 != rhs.cost4) return 0;
		if (this->feasible != rhs.feasible) return 0;
		return 1;
	};
};


bool validate_args(){
    //Connect to both databases
    connect_to_db(&disrupt_db_settings, &disrupt_db_conn);

    stringstream query;
    char *table_name;

    //Verify Job ID
    query.str("");
    table_name = "job";
    query << "SELECT * FROM " << table_name << " WHERE jobID = " << jobID;
    if (!(mysql_query(&disrupt_db_conn.m, query.str().c_str()) == 0)){
        cout << "Query '" << query.str() <<  "' Unsuccessfull" << endl;
        return false;
    }

    MYSQL_RES *result = mysql_store_result(&disrupt_db_conn.m);
    if (result->row_count == 0) {
        printf("Invalid jobID %d", jobID);
        mysql_free_result(result);
        return false;
    }
    mysql_free_result(result);

    //Verify alternativePlanID
    query.str("");
    table_name = "alternative_plan";
    query << "SELECT * FROM " << table_name << " WHERE alternativePlanID=" << alternativeActionID;
    if (!(mysql_query(&disrupt_db_conn.m, query.str().c_str()) == 0)){
        cout << "Query '" << query.str() <<  "' Unsuccessfull" << endl;
        return false;
    }
    result = mysql_store_result(&disrupt_db_conn.m);
    if (result->row_count == 0) {
        printf("Invalid alternativePlanID %d", alternativeActionID);
        mysql_free_result(result);
        return false;
    }
    mysql_free_result(result);

    disconnect_from_db(&disrupt_db_conn);
    return true;
}

class problem_RCJSS :public CObject
{
public:
	solution *p_solution;
    solution *base_solution;
	localSearch *lsObject; //Assign Local Search module to the problem
	simplePool *sol_pool;
	search_options options;
    ProblemDataset *dataset;

    problem_RCJSS()
	{
	    //Common Sol Data
	    dataset = new ProblemDataset();

		p_solution = new solution();
		base_solution = new solution();
		lsObject = new localSearch();
		sol_pool = new simplePool(pool_size, pool_lifespan);
		
		//tabu_solution = new tabusolution(); //I could keep an ubersolution class with all stuff inside

		//jobs = new CObArray;
		//machines = new CObArray;
	}
	~problem_RCJSS()
	{
		delete globals->elite;

		globals->RCL.clear(); vector<operation*>().swap(globals->RCL);

		//for (int i = 0; i<p_solution->soljobs->GetSize(); i++) delete (job*)(*jobs)[i];
		//p_solution->soljobs->RemoveAll(); delete p_solution;
		//for (int i = 0; i<p_solution->solmachines.size(); i++) delete (machine*)(*machines)[i];
		//p_solution->solmachines.clear(); delete machines;

		//Delete Main Solution
		delete p_solution;

        //Delete Dataset
        delete dataset;

        //Delete Solution pool
        delete sol_pool;

        //Delete local Search Module
		delete lsObject;

	}

	//DATASET PARSERS


    /*void generate_job(int &job_counter, int &op_uid_counter, int start_line, int end_line,
            ProductionOrder *po, Product *p, solution *sol, int quantity) {

        job *jb = new job();
        jb->ID = job_counter;
        jb->order = po;
        jb->quantity = quantity;
        jb->lotSize = quantity; //NOTE: when quantity is less than the lot_size we should use the quantity instead
        jb->lotFactor = (float) BlackBoxSize / quantity;

        printf("Generating Job %d for order %d \n",
                job_counter, po->orderID);

        //Max 3 operations should be added per job.
        //1 for each line type of the plant floor (automated line, manual line, testing line)

        //Add necessary operations
        int op_local_counter = 0;
        for (int i = start_line; i < end_line; i++) {

            operation *op = new operation();
            op->ID = op_local_counter;
            switch(i){
                case exlSMT:
                    op->process_stepID = lSMT;
                    break;
                case exlTHT:
                    op->process_stepID = lTHT;
                    break;
                case exlMANUAL:
                    op->process_stepID = lMANUAL;
                    break;
                case exlTESTING:
                    op->process_stepID = lTESTING;
                    break;
            }

            op->jobID = jb->ID;
            op->UID = op_uid_counter;

            int start_proc = exmSMT;
            int end_proc = exMACHINETYPECOUNT;

            switch (i) {
                //Manage automated line processes
                case exlSMT:
                    start_proc = exmSMT;
                    end_proc = exmTHT;
                    break;
                case exlTHT:
                    start_proc = exmTHT;
                    end_proc = exmMANUAL_INSERTION;
                    break;
                case exlMANUAL:
                    start_proc = exmMANUAL_INSERTION;
                    end_proc = exmSAKI;
                    break;
                case exlTESTING:
                    start_proc = exmSAKI;
                    end_proc = exMACHINETYPECOUNT;
                    break;
            }

            //Calculate aggregated processing time for the operation
            op->const_processing_time_UM[0] = 0; //This hold the generic processing time
            op->const_processing_time_UM[1] = 0; //This is used to hold the SMT_SPEC processing time which is the only time
            for (int k=0;k<exMACHINETYPECOUNT;k++){
                op->var_processing_time_UM[0][k] = 0; //This hold the generic processing time
                op->var_processing_time_UM[1][k] = 0; //This is used to hold the SMT_SPEC processing time which is the only time
            }

            //where the processing time is different and it does not depend directly on machine capabilities
            for (int j = start_proc; j < end_proc; j++) {

                if (j == exmREPAIR) continue; //Do not use repair operations for now

                ProductProcessingTime *pt = p->productProcessingTimes[j];

                switch (j) {
                    case exmSMT: {
                        //Add bottom processing on normal SMTs
                        if (pt->SideOfPCB[SIDE_BOTTOM] > 0) {
                            op->const_processing_time_UM[0] +=
                                //pt->ProcessingTime[SIDE_BOTTOM] * quantity / pf->productFamilyFactorPCB;
                                pt->ProcessingTime[SIDE_BOTTOM] * quantity; //Processing time is measured in sec/product
                                //So the factor doesn't need to be used anywhere.
                        }

                        //Add top processing if required
                        if (pt->SideOfPCB[SIDE_TOP] > 0) {
                            op->const_processing_time_UM[0] +=
                                    //pt->ProcessingTime[SIDE_TOP] * quantity / pf->productFamilyFactorPCB;
                                    pt->ProcessingTime[SIDE_TOP] * quantity;

                        }
                        break;
                    }

                    //Boards get separated in the ICT machines so until then, the productFamilyFactorPCB should be
                    //incorporated in the processing time of the operation
                    //Manual, ICT, FT processing times change depending on the parallel processors of the corresponding
                    //line machines
                    case exmMANUAL_INSERTION:
                        op->var_processing_time_UM[0][j] +=
                                //pt->ProcessingTime[SIDE_DEFAULT] * quantity / pf->productFamilyFactorPCB;
                                pt->ProcessingTime[SIDE_DEFAULT] * quantity;
                        break;
                    case exmICT:
                    case exmFT:
                        op->var_processing_time_UM[0][j] += pt->ProcessingTime[SIDE_DEFAULT] * quantity;
                        break;
                    case exmMASKING:
                    case exmFLUX:
                    case exmPOTA_SOLTEC:
                    case exmSAKI:
                    case exmARVISION:
                    case exmTHT:
                        op->const_processing_time_UM[0] +=
                                //pt->ProcessingTime[SIDE_DEFAULT] * quantity / pf->productFamilyFactorPCB;
                                pt->ProcessingTime[SIDE_DEFAULT] * quantity;
                        break;
                    case exmPACKAGING:
                        op->const_processing_time_UM[0] += 10 * quantity; // Add a constant processing time for packaging
                    default:
                        op->const_processing_time_UM[0] += pt->ProcessingTime[SIDE_DEFAULT] * quantity;
                }

            }

            //Set Precedence relationships of op
            if (i > start_line) {
                operation *op_pred = jb->operations[op_local_counter - 1];
                op->pred[0] = op_pred;
                op_pred->suc[0] = op;
            }

            printf("Generating Operation [%d, %d] for Processes %d - %d\n",
                   op->jobID, op->ID, start_proc, end_proc - 1);

            jb->operations.push_back(op);
            sol->soloperations.push_back(op);
            op_local_counter++;
            op_uid_counter++;

        }

        sol->soljobs.push_back(jb);
        job_counter++;
    }*/


    void preprocess_DISRUPT_orders_wResources(){
        //This method preprocess input orders, and converts them to a job with a single operation

        //Counters
        int job_counter = 0;
        int op_uid_counter = 0;

        //Keep quantities to be produced
        unordered_map<string, int> TOBE_PRODUCED = unordered_map<string,int>();
        unordered_map<string, int> QUANTITY_REQUIRED = unordered_map<string,int>();

        //Fetch a default processing time
        ProductProcessingTime *default_pt = dataset->productsNameMap["default"]->productProcessingTimes[0];

        //Preprocess  Parts to be produced
        for (auto po : dataset->orders) {
            Product *p = dataset->productsIDMap[po->productID];
            string product_name = p->productName;

            BOMEntry *baseProductBOM;
            if (dataset->BOM.find(product_name) != dataset->BOM.end())
                baseProductBOM = dataset->BOM[product_name];
            else
                baseProductBOM = nullptr;

            if (baseProductBOM == nullptr) {
                printf("Product ID missing from the BOM Catalog. Skipping order\n");
                continue;
            }

            printf("Preprocessing %s \n", product_name.c_str());

            //Identify product code type
            int rem = po->quantityRemaining; //Override remaining quantity with the status of the process inv

            if (TOBE_PRODUCED.find(product_name) == TOBE_PRODUCED.end()){
                TOBE_PRODUCED[product_name] = 0;
            }

            TOBE_PRODUCED[product_name] += rem;

            //Decide Lot Size for order
            int lot_size;
            if (rem >= 10000)
                lot_size = 10000;
            else lot_size = rem;

            while (rem > 0) {
                //Adjust lot_size
                lot_size = min(lot_size, rem);

                //Generate job
                job *jb = new job();
                jb->ID = job_counter;
                jb->actualID = job_counter;
                jb->orderID = po->orderID;
                jb->quantity = lot_size;
                jb->rel = (int) dataset->orderIDMap.at(jb->orderID)->releaseTime;
                jb->lotSize = lot_size; //NOTE: when quantity is less than the lot_size we should use the quantity instead
                jb->lotFactor = (float) BlackBoxSize / (float) lot_size;

                printf("Generating Job %d for order %d. Handling quantity %d\n",
                       job_counter, po->orderID, lot_size);

                int op_local_counter = 0;

                //Set default process steps
                int start_proc = exmSMT;
                int end_proc = exMACHINETYPECOUNT;

                switch (baseProductBOM->code_type) {
                    case 2: //Preprocess Manual-Test Line operations
                        start_proc = exmMANUAL_INSERTION;
                        end_proc = exMACHINETYPECOUNT;
                        break;
                    case 1: //Preprocess THT Line operations
                        start_proc = exmTHT;
                        end_proc = exmMANUAL_INSERTION;
                        break;
                    case 0://Preprocess SMT Line operations
                        start_proc = exmSMT;
                        end_proc = exmTHT;
                        break;
                    default:
                        printf("Missing proper code type\n");
                }

                operation *op = new operation();
                op->ID = op_local_counter;
                op->jobID = jb->ID;
                op->UID = op_uid_counter;
                op->process_stepID = baseProductBOM->code_type;

                //Calculate aggregated processing time for the operation
                op->const_processing_time_UM = 0; //This holds the generic processing time
                for (int k = 0; k < exMACHINETYPECOUNT; k++) {
                    op->var_processing_time_UM[k] = 0; //This holds the varying processing time
                }

                //where the processing time is different and it does not depend directly on machine capabilities
                for (int j = start_proc; j < end_proc; j++) {
                    if (j == exmREPAIR) continue; //Do not use repair operations for now

                    //Try to find a processing time in the product
                    ProductProcessingTime *pt;
                    if (p->productProcessingTimes.find(j) != p->productProcessingTimes.end())
                        pt = p->productProcessingTimes[j];
                    else
                        pt = default_pt;

                    switch (j) {
                        case exmSMT: {
                            //Add bottom processing on normal SMTs
                            if (pt->SideOfPCB[SIDE_BOTTOM] > 0) {
                                op->const_processing_time_UM +=
                                        //pt->ProcessingTime[SIDE_BOTTOM] * quantity / pf->productFamilyFactorPCB;
                                        pt->ProcessingTime[SIDE_BOTTOM] *
                                        lot_size; //Processing time is measured in sec/product
                                //So the factor doesn't need to be used anywhere.
                            }

                            //Add top processing if required
                            if (pt->SideOfPCB[SIDE_TOP] > 0) {
                                op->const_processing_time_UM +=
                                        //pt->ProcessingTime[SIDE_TOP] * quantity / pf->productFamilyFactorPCB;
                                        pt->ProcessingTime[SIDE_TOP] * lot_size;
                            }
                            break;
                        }

                            //Boards get separated in the ICT machines so until then, the productFamilyFactorPCB should be
                            //incorporated in the processing time of the operation
                            //Manual, ICT, FT processing times change depending on the parallel processors of the corresponding
                            //line machines
                        case exmMANUAL_INSERTION:
                            op->var_processing_time_UM[j] +=
                                    //pt->ProcessingTime[SIDE_DEFAULT] * quantity / pf->productFamilyFactorPCB;
                                    pt->ProcessingTime[SIDE_DEFAULT] * lot_size;
                            break;
                        case exmICT:
                        case exmFT:
                            op->var_processing_time_UM[j] += pt->ProcessingTime[SIDE_DEFAULT] * lot_size;
                            break;
                        case exmMASKING:
                        case exmFLUX:
                        case exmPOTA_SOLTEC:
                        case exmSAKI:
                        case exmARVISION:
                        case exmTHT:
                            op->const_processing_time_UM +=
                                    //pt->ProcessingTime[SIDE_DEFAULT] * quantity / pf->productFamilyFactorPCB;
                                    pt->ProcessingTime[SIDE_DEFAULT] * lot_size;
                            break;
                        case exmPACKAGING:
                            op->const_processing_time_UM +=
                                    2.5f * lot_size; // Add a constant processing time for packaging
                            break;
                        default:
                            op->const_processing_time_UM += pt->ProcessingTime[SIDE_DEFAULT] * lot_size;

                    }
                }

                //Insert operation to the start of the job operations
                jb->operations.insert(jb->operations.begin(), op);

                p_solution->soloperations.push_back(op);

                //Increase counters
                op_local_counter += 1;
                op_uid_counter += 1;


                //Insert job
                p_solution->soljobs.push_back(jb);
                job_counter++; //Increase job counter
                rem -= lot_size; //Decrease remaining order quantity
            }
        }

        //Check Inventory equilibrium

        //Accumulate required quantities
        for (auto e : TOBE_PRODUCED){
            BOMEntry *bome = dataset->BOM.at(e.first);

            //Need to check equilibrium for THT and Manual/Test orders
            if (bome->code_type != 0 && bome->requirement != "NONE") {
                if (QUANTITY_REQUIRED.find(bome->requirement) == QUANTITY_REQUIRED.end())
                    QUANTITY_REQUIRED[bome->requirement] = 0;
                QUANTITY_REQUIRED[bome->requirement] += e.second;
            }
        }

        //Fix initial inventories
        for (auto e : QUANTITY_REQUIRED){
            BOMEntry *bome = dataset->BOM.at(e.first);

            int req_quantity = e.second;
            int produced_quantity = 0;
            int inventory_quantity = 0;

            //Fetch produced quantity
            if (TOBE_PRODUCED.find(e.first) != TOBE_PRODUCED.end())
                produced_quantity = TOBE_PRODUCED.at(e.first);

            //Try to fetch inventory
            if (dataset->productInventory.find(e.first) != dataset->productInventory.end()){
                inventory_quantity = dataset->productInventory.at(e.first);
            }

            //If equilibrium is not satisfied increase the inventory
            if (req_quantity > produced_quantity + inventory_quantity){
                if (dataset->productInventory.find(e.first) == dataset->productInventory.end()){
                    dataset->productInventory[e.first] = 0;
                }

                //Make sure that there is at least another blackbox available
                dataset->productInventory[e.first] += req_quantity - inventory_quantity - produced_quantity + 50;
                printf("New Inventory for %s : %d\n",
                       e.first.c_str(), dataset->productInventory[e.first]);
            }
        }

    }

    void applyEvents() {

        for (int i=0;i<dataset->events.size();i++){
            event *e = dataset->events[i];
            e->report();

            switch (e->type){
                case 1: //Malfunction Events
                    //Step 1: find affected line-machine

                    machine *target_mc = nullptr;
                    for (int j=0;j<p_solution->solmachines.size();j++){
                        machine *mc = p_solution->solmachines[j];
                        if (std::find(mc->embeddedMachineIDs.begin(), mc->embeddedMachineIDs.end(), e->target) != mc->embeddedMachineIDs.end()){
                            target_mc = mc;
                            break;
                        }
                    }

                    if (target_mc == nullptr){
                        printf("Unable to find machine to apply event %d .", e->ID);
                        continue;
                    }

                    time_t event_startDate = timegm(&e->trigger_time);
                    tm event_endTime_struct = e->trigger_time;
                    event_endTime_struct.tm_sec += e->arg1;
                    time_t event_endDate =  timegm(&event_endTime_struct);
                    int event_duration = e->arg1;


                    if ((event_startDate < timegm(&PlanningHorizonStart)) &&
                        (event_endDate < timegm(&PlanningHorizonStart))){
                        printf("Event Starts & Ends before the planning horizon starts. Nothing to Do\n");
                        continue;
                    }

                    if ((event_startDate > timegm(&PlanningHorizonEnd)) &&
                        (event_endDate > timegm(&PlanningHorizonEnd))
                    ){
                        printf("Event Starts & Ends after the planning horizon ends. Nothing to Do\n");
                        continue;
                    }

                    if (event_startDate < timegm(&PlanningHorizonStart)){
                        e->trigger_time = PlanningHorizonStart;
                    }

                    if (event_endDate > timegm(&PlanningHorizonEnd)){
                        printf("Clamping event to the planning horizon\n");
                        event_duration = difftime(timegm(&PlanningHorizonEnd), timegm(&PlanningHorizonStart));
                    } else
                        event_duration = difftime(event_endDate, timegm(&e->trigger_time));

                    //Step 2: //Initialize an unavailability window and figure out the time
                    mach_unav *window = new mach_unav();
                    window->start_unav = timegm(&e->trigger_time) - timegm(&PlanningHorizonStart);
                    window->end_unav = window->start_unav + event_duration; //Add delay

                    //Step 3: //Save event to machine
                    target_mc->applyUnavWindow(window);

                    //Step 4: Update Rescheduling Time
                    ReschedulingTime = e->trigger_time;
                    break;
            }
        }
    }

    void preprocess_DISRUPT_orders() {
	    //This method is preprocessing orders as stored in the DISRUPT database
	    //Each order does not represent a particular order of product items, but it is dedicated to specific production
	    //lines. So each of them should be preprocessed and the particular operation should be
	    //
	    //Init Production Process Inventories to keep track of the produced quantities on each production level
	    //Note, these maps can serve in order to input initial product inventory of ready or semi-finished products

	    unordered_map<string, int> TOBE_PRODUCED = unordered_map<string, int>();
        unordered_map<string, int> localProductInventory = unordered_map<string, int>();


        //Initiate dictionaries
        for (auto k : dataset->BOM){
            TOBE_PRODUCED[k.first] = 0;
        }

        //Copy the product Inventory Dictionary
        for (auto k: dataset->productInventory){
            localProductInventory[k.first] = k.second;
        }

        //Fill to-be produced inventory based on the input orders
        for (ProductionOrder *po : dataset->orders) {
            int rem_q = po->quantityRemaining;
            string product_name = dataset->productsIDMap[po->productID]->productName;

            if (TOBE_PRODUCED.find(product_name) == TOBE_PRODUCED.end()) {
                printf("Missing Product from BOM %s\n", product_name.c_str());
                TOBE_PRODUCED[product_name] = 0;
            }

            TOBE_PRODUCED[product_name] += rem_q;
        }

        //Report TO BE PRODUCED PRODUCTS
        for (auto entry : TOBE_PRODUCED){
            if (entry.second > 0)
                printf("We have to produce %d from Product %s. Initial Inventory %d\n",
                        entry.second, entry.first.c_str(), dataset->productInventory[entry.first]);
        }


        int job_counter = 0;
        int op_uid_counter = 0;

        //Generate a default processing time
        ProductProcessingTime default_pt = ProductProcessingTime();

        int t_code_start, t_code_end;

        //Iterate in Orders backwards starting from the manual orders and heading for the automated
        for (int t_code_type = 2; t_code_type >=0 ; t_code_type--) {

            //Preprocess  Parts to be produced
            for (auto po : dataset->orders) {
                Product *p = dataset->productsIDMap[po->productID];
                string product_name = p->productName;

                BOMEntry *baseProductBOM;
                if (dataset->BOM.find(product_name) != dataset->BOM.end())
                    baseProductBOM = dataset->BOM[product_name];
                else
                    baseProductBOM = nullptr;

                if (baseProductBOM == nullptr) {
                    printf("Product ID missing from the BOM Catalog. Skipping order\n");
                    continue;
                }

                if (baseProductBOM->code_type != t_code_type)
                    continue;

                if (TOBE_PRODUCED[product_name] == 0) {
                    printf("Quantity Mismatch from order %s \n",
                           product_name.c_str());
                    continue;
                }

                printf("Preprocessing %s \n",
                       product_name.c_str());

                //Identify product code type
                int rem = po->quantityRemaining; //Override remaining quantity with the status of the process inv

                //Decide Lot Size for order
                int lot_size;
                if (rem >= 10000)
                    lot_size = 5000;
                else if (rem >= 5000)
                    lot_size = 2000;
                else if (rem >= 1000)
                    lot_size = 1000;
                else if (rem >= 500)
                    lot_size = 500;
                else if (rem >= 200)
                    lot_size = 200;
                else if (rem >= 100)
                    lot_size = 100;
                else
                    lot_size = rem;


                while (rem > 0) {
                    //Adjust lot_size
                    lot_size = min(lot_size, rem);

                    //Generate job
                    job *jb = new job();
                    jb->ID = job_counter;
                    jb->orderID = po->orderID;
                    jb->quantity = lot_size;
                    jb->lotSize = lot_size; //NOTE: when quantity is less than the lot_size we should use the quantity instead
                    jb->lotFactor = (float) BlackBoxSize / (float) lot_size;

                    printf("Generating Job %d for order %d. Handling quantity %d\n",
                           job_counter, po->orderID, lot_size);

                    int op_local_counter = 0;
                    //Reset productBOM
                    BOMEntry *productBOM = baseProductBOM;
                    string op_product_name = product_name;

                    while (productBOM != nullptr) {
                        int start_proc = exmSMT;
                        int end_proc = exMACHINETYPECOUNT;

                        switch (productBOM->code_type) {
                            case 2: //Preprocess Manual-Test Line operations
                                start_proc = exmMANUAL_INSERTION;
                                end_proc = exMACHINETYPECOUNT;
                                break;
                            case 1: //Preprocess THT Line operations
                                start_proc = exmTHT;
                                end_proc = exmMANUAL_INSERTION;
                                break;
                            case 0://Preprocess SMT Line operations
                                start_proc = exmSMT;
                                end_proc = exmTHT;
                                break;
                            default:
                                printf("Missing proper code type\n");
                        }

                        operation *op = new operation();
                        op->ID = op_local_counter;
                        op->jobID = jb->ID;
                        op->UID = op_uid_counter;
                        op->process_stepID = productBOM->code_type;

                        //Calculate aggregated processing time for the operation
                        op->const_processing_time_UM = 0; //This holds the generic processing time
                        for (int k = 0; k < exMACHINETYPECOUNT; k++) {
                            op->var_processing_time_UM[k] = 0; //This holds the varying processing time
                        }

                        //where the processing time is different and it does not depend directly on machine capabilities
                        for (int j = start_proc; j < end_proc; j++) {
                            if (j == exmREPAIR) continue; //Do not use repair operations for now

                            //Try to find a processing time in the product
                            ProductProcessingTime *pt;
                            if (p->productProcessingTimes.find(j) != p->productProcessingTimes.end())
                                pt = p->productProcessingTimes[j];
                            else
                                pt = &default_pt;


                            switch (j) {
                                case exmSMT: {
                                    //Add bottom processing on normal SMTs
                                    if (pt->SideOfPCB[SIDE_BOTTOM] > 0) {
                                        op->const_processing_time_UM +=
                                                //pt->ProcessingTime[SIDE_BOTTOM] * quantity / pf->productFamilyFactorPCB;
                                                pt->ProcessingTime[SIDE_BOTTOM] *
                                                lot_size; //Processing time is measured in sec/product
                                        //So the factor doesn't need to be used anywhere.
                                    }

                                    //Add top processing if required
                                    if (pt->SideOfPCB[SIDE_TOP] > 0) {
                                        op->const_processing_time_UM +=
                                                //pt->ProcessingTime[SIDE_TOP] * quantity / pf->productFamilyFactorPCB;
                                                pt->ProcessingTime[SIDE_TOP] * lot_size;
                                    }
                                    break;
                                }

                                //Boards get separated in the ICT machines so until then, the productFamilyFactorPCB should be
                                //incorporated in the processing time of the operation
                                //Manual, ICT, FT processing times change depending on the parallel processors of the corresponding
                                //line machines
                                case exmMANUAL_INSERTION:
                                    op->var_processing_time_UM[j] +=
                                            //pt->ProcessingTime[SIDE_DEFAULT] * quantity / pf->productFamilyFactorPCB;
                                            pt->ProcessingTime[SIDE_DEFAULT] * lot_size;
                                    break;
                                case exmICT:
                                case exmFT:
                                    op->var_processing_time_UM[j] += pt->ProcessingTime[SIDE_DEFAULT] * lot_size;
                                    break;
                                case exmMASKING:
                                case exmFLUX:
                                case exmPOTA_SOLTEC:
                                case exmSAKI:
                                case exmARVISION:
                                case exmTHT:
                                    op->const_processing_time_UM +=
                                        //pt->ProcessingTime[SIDE_DEFAULT] * quantity / pf->productFamilyFactorPCB;
                                        pt->ProcessingTime[SIDE_DEFAULT] * lot_size;
                                    break;
                                case exmPACKAGING:
                                    op->const_processing_time_UM +=
                                            10 * lot_size; // Add a constant processing time for packaging
                                    break;
                                default:
                                    op->const_processing_time_UM += pt->ProcessingTime[SIDE_DEFAULT] * lot_size;
                            }
                        }

                        //Check if precedence relations should be imposed
                        if (jb->operations.size() > 0){
                            operation *op_suc = jb->operations[0];
                            op->suc[0] = op_suc;
                            op_suc->pred[0] = op;
                        }

                        //Insert operation to the start of the job operations
                        jb->operations.insert(jb->operations.begin(), op);

                        p_solution->soloperations.push_back(op);

                        //Increase counters
                        op_local_counter += 1;
                        op_uid_counter += 1;

                        //Remove corresponding product quantity from the to-be produced dictionary
                        TOBE_PRODUCED[op_product_name] -= lot_size;

                        //Load next level of dependencies
                        op_product_name = productBOM->requirement;
                        if (dataset->BOM.find(op_product_name) != dataset->BOM.end()) {
                            productBOM = dataset->BOM[op_product_name];
                        } else
                            productBOM = nullptr;

                        //Exit if the is sufficient material from the product inventory
                        if (localProductInventory[op_product_name] >= lot_size){
                            printf("Breaking job backtracking due to sufficient product inventory\n");
                            localProductInventory[op_product_name] -= lot_size;
                            break;
                        }
                    }

                    //Insert job
                    p_solution->soljobs.push_back(jb);
                    job_counter++; //Increase job counter
                    rem -= lot_size; //Decrease remaining order quantity
                }
            }
        }

        //Report Remaining produce quantities
        for (auto k : TOBE_PRODUCED){
            if (TOBE_PRODUCED[k.first] > 0)
                printf("Missed the production of %d from product ID %s\n",
                        k.second, k.first.c_str());
            else if (TOBE_PRODUCED[k.first] < 0)
                printf("Corrected Inventory - Inserted extra %d from product ID %s\n",
                       k.second, k.first.c_str());
        }

        for (auto k : localProductInventory){
            if (k.second > 0){
                printf("Remaining ununsed product inventory %s : %d\n",
                        k.first.c_str(), k.second);
            }
        }


    }

    void setOperationUIDs(){
		int count = (int) p_solution->soloperations.size();

		//Set dummy machine ops now
		for (int i = 0; i < p_solution->solmachines.size(); i++) {
			machine *mc = p_solution->solmachines[i];
            operation *op1 = mc->operations[0];
            operation *op2 = mc->operations[mc->operations.size()-1];
            p_solution->soloperations.push_back(op1);
            p_solution->soloperations.push_back(op2);
            op1->UID = count;
            count++;
            op2->UID = count;
            count++;
        }

        //Set Number of Operations
        NumberOfOperations = count;
		NumberOfOrders = (int) dataset->orders.size();
		NumberOfJobs = (int) p_solution->soljobs.size();

        if (NumberOfOperations > MAX_OPERATIONS){
            printf("NOT ENOUGH MEMORY TO STORE ALL OPERATIONS %d != %d", NumberOfOperations, MAX_OPERATIONS);
            assert(false);
        } else if (NumberOfMachines > MAX_MACHINES){
            printf("NOT ENOUGH MEMORY TO STORE ALL MACHINES %d != %d",NumberOfMachines, MAX_MACHINES);
            assert(false);
        }
	}

	void read()
	{
#if(!FLEX)
		char Line[1000];
		ifstream fin("c:\\benchmarks\\jobshop\\times1_15_15.txt");

		NumberOfResources = 3;
		planhorend = 2000;

		int g = 0;
		while (!fin.eof())
		{
			fin.getline(Line, 1000);  // Diavazoume mia grammi apo to arxeio
									 // kai tin topothetoume sto Line
			istringstream Sin(Line);

			job *jb = new job();
			jb->actualID = g + rand();
			jb->ID = g;

#if(REL_DUE_DATES)
			if (g % 2 == 0) {
				jb->rel = planhorstart;
				jb->due = planhorend;
			}
			else {
				jb->rel = planhorstart + 50;
				jb->due = 10000;
			}
#endif

			int j = 0;
			while (!Sin.eof())
			{
				operation *op = new operation();
				if (j != 0) {
					operation *opp = jb->operations[jb->operations.size() - 1];
					op->pred.push_back(opp);
					opp->suc.push_back(op);
				}
				op->jobID = g;
				op->ID = j;
				op->start = 0;
				op->end = 0;
				op->a = jb->rel;
				op->b = jb->due;

#if(CHANGEOVERTIMES)
				if (j % 2 == 0) {
					op->opertypeID = 0;
				}
				else {
					op->opertypeID = 1;
				}
#endif

				Sin >> op->processing_time;

				jb->operations.push_back(op);
				j++;
			}
			p_solution->soljobs.push_back(jb);
			g++;
		}
		fin.close();

		ifstream fin2("c:\\benchmarks\\jobshop\\machines1_15_15.txt");

		g = 0;
		while (!fin2.eof())
		{
			fin2.getline(Line, 500);  // Diavazoume mia grammi apo to arxeio
									  // kai tin topothetoume sto Line
			istringstream Sin(Line);

			job *jb = p_solution->soljobs[g];
			int j = 0;
			while (!Sin.eof())
			{
				operation *op = jb->operations[j];
				Sin >> op->process_stepID;
				op->process_stepID--;
				j++;
			}
			g++;
		}
		fin2.close();
#endif
	}
	
	void Proc_Mach_Read_InitFlex(string file)
	{
		/*	This function just takes all the input machines
			and saves them into one process step
			This should be fixed with proper input files
			so that each machine has its own process step id
			and also there is a fixed hierarchy of machines
		*/

#if(LINE_CONSTRAINTS)

		cout << "FETCHING MACHINES AND PROCESS STEPS" <<endl;
		cout << "Input File: " << file << endl;
		char Line[1000] = {'0'};
		ifstream fin(file);
		if (!fin.good()) { cout << "FILE DOES NOT EXIST !" << endl; return;} 
		
		fin.getline(Line, 1000);
		istringstream Sin(Line);
		//Get Number of Stages
		Sin >> NumberOfStages;
		cout << "Number of Stages: " << NumberOfStages << endl;
		//Fetch stage-line data from the rest lines
		for (int t=0; t<NumberOfStages; t++){
			fin.getline(Line, 1000);
			istringstream iss(Line);			
			//Get Number of lines available in the stage
			int line_num;
			iss >> line_num;

			printf("Parsing Stage: %d Lines: %d \n", t, line_num);
			//Create Stage
			stage *s = new stage();

			//Get Line data
			for (int g=0;g<line_num;g++){
				cout << "Parsing line/Process Step: " << g << endl;
				//Get Line configuration attribute
				int Line_config;
				iss >> Line_config;

				//Get Number of Machines on the line
				int line_mach_num;
				iss >> line_mach_num;

				//Create Process Step Class Here
				//process_step *ps = new process_step();
				listObject<process_step> *ps = new listObject<process_step>();
				ps->ID = g;
				ps->actualID = g + rand();
				ps->config = Line_config;
				
				for (int m=0; m<line_mach_num; m++){
					int machID, machType;
					iss >> machID;
					iss >> machType;

					printf("Parsing Machine: %d %d \n",machID, machType);
					//Create Machine Here
					//machine *mm = new machine();
					listObject<machine> *mm = new listObject<machine>();
					mm->ID = machID;
					mm->actualID = machID + rand();
					mm->process_stepID = g; //Assign Line ID

#if(MACHINE_AVAILABILITIES)
					mach_unav *muv1 = new mach_unav();
					muv1->start_unav = 10;
					muv1->end_unav = 30;
					mm->unavailability.push_back(muv1);
					+
					mach_unav *muv2 = new mach_unav();
					muv2->start_unav = 80;
					muv2->end_unav = 90;
					mm->unavailability.push_back(muv2);
#endif
					//Set machine hierarchies
					if (m > 0) {
						listObject<machine> *mp = ps->mach_avail[m-1];

						//Set pred on current machine
						mm->pred.push_back(mp);	
						//Set suc on previous machine
						mp->suc.push_back(mm);
					}
					
					ps->mach_avail.push_back(mm); //Add machine to line
					p_solution->solmachines.push_back(mm); //Add to the solution machines
					//process_step *ps = p_solution->ProcSteps[mm->process_stepID];
					//ps->mach_avail.push_back(mm);
					//p_solution->solmachines.push_back(mm);
				}
				p_solution->ProcSteps.push_back(ps); //i should remove that when stages are fully incorporated
				s->psteps.push_back(ps);
			}
			p_solution->Stages.push_back(s);
		}

		//Report Saved stuff
		printf("Stage Report#########\n");
		for (int s = 0; s < NumberOfStages; s++){
			stage *st = p_solution->Stages[s];
			printf("Stage: %d \n", s);
			for (int t = 0; t < st->psteps.size(); t++){
				process_step *ps = st->psteps[t];
				printf("Process Step %d Number of Machines %d \n", ps->ID, ps->mach_avail.size());
				for (int g=0;g<ps->mach_avail.size();g++){
					listObject<machine> *mm = ps->mach_avail[g];
					int pred, suc;
					pred = -1;
					suc = -1;
					if (mm->pred.size()>0) pred = mm->pred[0]->ID;
					if (mm->suc.size()>0) suc = mm->suc[0]->ID;
					printf("\t Machine ID: %d Type %d Pred %d Suc %d \n", mm->ID, 0, pred, suc);
				}
			}
		}

		//scanf("%d ");

#else
		cout << "Running Old Code" << endl;
		for (int g = 0; g<NumberOfProcSteps; ++g)
		{
			//process_step *ps = new process_step();
            process_step *ps = new process_step();
			ps->ID = g;
			ps->actualID = g + rand();
			p_solution->ProcSteps.push_back(ps);
		}
		
		for (int g = 0; g<NumberOfMachines; ++g)
		{
			//machine *mm = new machine();
			machine *mm = new machine();
			mm->ID = g;
			mm->actualID = g + rand();
			mm->process_stepID = 0;

			//Set first dummy operation properties
			mm->operations[0]->ID = g;
			mm->operations[1]->ID = g;

#if(MACHINE_AVAILABILITIES)

			mach_unav *muv1 = new mach_unav();
			muv1->start_unav = 10;
			muv1->end_unav = 30;
			mm->unavailability.push_back(muv1);
			
			mach_unav *muv2 = new mach_unav();
			muv2->start_unav = 80;
			muv2->end_unav = 90;
			mm->unavailability.push_back(muv2);
#endif
            process_step *ps = p_solution->ProcSteps[mm->process_stepID];
			ps->machines.push_back(mm);
			p_solution->solmachines.push_back(mm);
		}
#endif
	}

	void Proc_Mach_Read_Init()
	{
		NumberOfProcSteps = 15;
		for (int g = 0; g<NumberOfProcSteps; ++g)
		{
            process_step *ps = new process_step();
			ps->ID = g;
			ps->actualID = g + rand();
			p_solution->ProcSteps.push_back(ps);
		}

		NumberOfMachines = 20;
		for (int g = 0; g<NumberOfMachines; ++g)
		{
			machine *mm = new machine();
			if (g < 15) {
				mm->ID = g;
				mm->actualID = g + rand();
				mm->process_stepID = g;

#if(MACHINE_AVAILABILITIES)

				mach_unav *muv1 = new mach_unav;
				muv1->start_unav = 10;
				muv1->end_unav = 30;
				mm->unavailability.push_back(muv1);

				mach_unav *muv2 = new mach_unav;
				muv2->start_unav = 80;
				muv2->end_unav = 90;
				mm->unavailability.push_back(muv2);
#endif
			}
			else
			{
				mm->ID = g;
				mm->actualID = g + rand();
				mm->process_stepID = g - 15;

#if(MACHINE_AVAILABILITIES)

				mach_unav *muv1 = new mach_unav;
				muv1->start_unav = 40;
				muv1->end_unav = 50;
				mm->unavailability.push_back(muv1);

				mach_unav *muv2 = new mach_unav;
				muv2->start_unav = 90;
				muv2->end_unav = 100;
				mm->unavailability.push_back(muv2);

#endif
			}

            process_step *ps = p_solution->ProcSteps[mm->process_stepID];
			ps->machines.push_back(mm);

			p_solution->solmachines.push_back(mm);
		}
		return;
	}
	void Resource_Read_Init()
	{

#if(RESOURCE_CONSTRAINTS)
		
		//Initiallize resources
		for (int r = 0; r<NumberOfResources; ++r)
		{
			resource *rc = new resource();
			rc->actualID = r + rand();
			rc->ID = r;
			rc->max_level = NumberOfMachines*10 + rand();
			rc->max_level = NumberOfMachines*10*(r+1); // Return this back after testing
			//rc->max_level = (2 + 1) * (NumberOfMachines - 1)* (r + 1);
			//((res_block*)(*rc->blocks)[0])->res_maxlevel = rc->max_level;
			//rc->max_level = INF;
			p_solution->Resources.push_back(rc);
		}


		//Define max caps at each process step
		for (int p = 0; p<p_solution->ProcSteps.size(); ++p)
		{
			process_step *ps = p_solution->ProcSteps[p];

			for (int r = 0; r<NumberOfResources; ++r)
			{
				resource *rc = p_solution->Resources[r];
				ps->resalarm[r] = rc->max_level;
				/*while (true) {
					ps->resalarm[r] = rand();
					if (ps->resalarm[r] < rc->max_level) break;
				}*/
			}
		}

		//Define resource consumptions at the machines
		for (int m = 0; m<p_solution->solmachines.size(); ++m)
		{
			machine *mc = p_solution->solmachines[m];
			process_step *ps = p_solution->ProcSteps[mc->process_stepID];
			for (int r = 0; r<NumberOfResources; ++r)
			{
				resource *mc_res = mc->resources[r];
				resource *prob_res = p_solution->Resources[r];

				*mc_res = *prob_res;
				//*((resource*) (*mc->resources)[r]) = *p_solution->Resources[r]; //THis should have worked but it doesn't. Its exactly the same with the other one

				while (true) {
					//int cons = (m + 1)*rand();
					int cons = (m + 1) * (NumberOfMachines - 1)* (r + 1);
					mc->res_consum_cleanup[r] = cons*0.2;
					mc->res_consum_processing[r] = cons;
					mc->res_consum_setup[r] = cons*0.3;
					if (mc->res_consum_cleanup[r] <= ps->resalarm[r] && mc->res_consum_processing[r] <= ps->resalarm[r] && mc->res_consum_setup[r] <= ps->resalarm[r]) break;
				}
			}
		}
#endif
		return;
	}


	void reportOrders(){
		printf("\n##########ORDER REPORT#########\n");
		for (int i=0;i<dataset->orders.size();i++)
            dataset->orders[i]->report();
		printf("###############################\n");
	}

    void exportProductionPlan(solution *s){
		cout << *s;
	    printf("Plan Costs\n");
		printf("Cmax:  %4.3f min\n", s->cost_array[MAKESPAN]/(60.0 *10));
		printf("TFT:  %4.3f min\n", s->cost_array[TOTAL_FLOW_TIME]/(60.0 * 10));
		printf("Tidle:  %4.3f min\n", s->cost_array[TOTAL_IDLE_TIME]/(60.0 * 10));
		for (int i=0; i<dataset->productionLines.size(); i++) {
            ProductionLine *pl = dataset->productionLines[i];
            printf("Line %s\n", pl->Name.c_str());
            //Fetch related machines from the solution
			for (int j=0; j<s->solmachines.size(); j++) {
				machine *mc = s->solmachines[j];
				if (mc->line->ID == pl->ID)
					if (mc->operations.size() > 2){
                        //printf("Machine %d \n", mc->ID);
                        printf("\t %-4s | %-9s | %-6s | %-6s | %-11s | %-11s | %-11s \n","Order", "Quantity", "Process", "UID", "start", "end", "pt");
				        int order_id = -1;
				        vector<int> jbs = vector<int>();
				        int quantity = -1;
                        int start_time = -1;
                        int end_time = -1;
                        int process_time = -1;

				        int o = 1;
				        while(o < (int) mc->operations.size()){
                            operation *op = mc->operations[o];
                            job *jb = s->soljobs[op->jobID];
                            //Init
                            if (o == 1){
                                order_id = jb->orderID;
                                quantity = jb->quantity;
                                start_time = op->start;
                                end_time = op->end;
                                process_time = op->getProcTime();
                                jbs.push_back(jb->ID);
                            }
                            //Output and break
                            else if (op->jobID <0) {
                                //Order changed report accumulated results
                                printf("\t %5d | %9d | %7d | %6d | %11d | %11d | %11d \n",
                                       order_id, quantity, mc->process_stepID, -1, start_time, end_time, process_time);
                                jbs.clear();
                                break;
                            }
                            //Output results and reset
                            else if(jb->orderID != order_id){
                                //Order changed report accumulated results
                                printf("\t %5d | %9d | %7d | %6d | %11d | %11d | %11d \n",
                                       order_id, quantity, mc->process_stepID, -1, start_time, end_time, process_time);

                                //Reset metrics
                                order_id = jb->orderID;
                                quantity = jb->quantity;
                                start_time = op->start;
                                end_time = op->end;
                                process_time = op->getProcTime();
                                jbs.clear();
                            } else if (!vContains<int>(jbs, jb->ID)) {
                                //Job has not been processed but we're in the same order
                                quantity += jb->quantity;
                                end_time = op->end;
                                process_time += op->getProcTime();
                            } else {
                                //Same job same order, just need to update end/process times
                                end_time = op->end;
                                process_time += op->getProcTime();
                            }
                            o++; //Proceed to next op
				        }

				        //Report final Results


				}
            }
		}
    }

	void Init_Graphs() {
		//Create Solution Graphs
		//Init Graphs
		//TODO Move to a small method
		//delete p_solution->backwardGraph;
		delete p_solution->FGraph;
		delete p_solution->BGraph;
		p_solution->createGraphs();
		
		//p_solution->FGraph->report();
		//p_solution->BGraph->report();
		
		/*
		printf("After Read Reports \n");
		p_solution->DGraph->state = FORWARD;
		p_solution->DGraph->report();
		p_solution->DGraph->state = BACKWARD;
		p_solution->DGraph->report();
		*/
		return;
    }

	int changeovertime(int mcID, int opfrom, int opto)
	{
		string key;
		char buffer[33];

		itoa(mcID, buffer);
		key += buffer;

		itoa(opfrom, buffer);
		key += buffer;

		itoa(opto, buffer);
		key += buffer;

		if (p_solution->chovertime.count(key)>0) {
			return p_solution->chovertime[key];
		}
		else {
			return 0;
		}
	}
	string make_neighbor_key(int k, int l, int i, int j)
	{
		string key;
		char buffer[33];

		itoa(k, buffer);
		key += buffer;

		itoa(l, buffer);
		key += buffer;

		itoa(i, buffer);
		key += buffer;

		itoa(j, buffer);
		key += buffer;

		return key;
	}

	//void print_solution()//OK
	//{
	//	printf("Cost 1: %d\t \n", cost);
	//	printf("Cost 2: %d\t \n", s_cost);
	//	printf("Cost 3: %d\t \n", t_cost);
	//	printf("Cost 4: %d\t \n", e_cost);
	//	for (int g = 0; g<p_solution->soljobs->GetSize(); g++)
	//	{
	//		job *jb = (job*)(*p_solution->soljobs)[g];
	//		int sum = 0;
	//		if (jb->operations.size() > 1)
	//		{
	//			for (int h = 1; h<jb->operations.size(); h++)
	//			{
	//				operation *op1 = jb->operations[h - 1];
	//				operation *op2 = jb->operations[h];
	//				sum += op2->start - op1->end;
	//			}
	//		}
	//		printf("JobB %d %d %d %d \n", jb->actualID, jb->ID, jb->operations.size(), sum);
	//	}
	//	for (int y = 0; y < p_solution->solmachines.size(); y++)
	//	{
	//		machine *mc = p_solution->solmachines[y];
	//		cout << *mc;
	//	}
	//	return;
	//}

	//bool p_solution->compareSolution(elite)
	//{
	//	bool flag = false;
	//	if (optimtype == 1) if (cost < elite->cost) flag = true;
	//	if (optimtype == 2) if (s_cost < elite->s_cost) flag = true;
	//	if (optimtype == 3) if (t_cost < elite->t_cost) flag = true;
	//	if (optimtype == 4) if (cost < elite->cost || (cost <= elite->cost && s_cost < elite->s_cost) || (cost <= elite->cost && s_cost <= elite->s_cost && t_cost < elite->t_cost))         flag = true;
	//	if (optimtype == 5) if (s_cost < elite->s_cost || (s_cost <= elite->s_cost && t_cost < elite->t_cost) || (s_cost <= elite->s_cost && t_cost <= elite->t_cost && cost < elite->cost)) flag = true;
	//	if (optimtype == 6) if (t_cost < elite->t_cost || (t_cost <= elite->t_cost && cost < elite->cost) || (t_cost <= elite->t_cost && cost <= elite->cost && s_cost < elite->s_cost))    flag = true;
	//	if (optimtype == 7) if (cost < elite->cost || (cost <= elite->cost && t_cost < elite->t_cost) || (cost <= elite->cost && t_cost <= elite->t_cost && s_cost < elite->s_cost))        flag = true;
	//	if (optimtype == 8) if (s_cost < elite->s_cost || (s_cost <= elite->s_cost && cost < elite->cost) || (s_cost <= elite->s_cost && cost <= elite->cost && t_cost < elite->t_cost))    flag = true;
	//	if (optimtype == 9) if (t_cost < elite->t_cost || (t_cost <= elite->t_cost && s_cost < elite->s_cost) || (t_cost <= elite->t_cost && s_cost <= elite->s_cost && cost < elite->cost))    flag = true;
	//	if (optimtype == 10) if (e_cost < elite->e_cost) flag = true;
	//	if (optimtype == 11) if (e_cost < elite->e_cost || (e_cost <= elite->e_cost && cost < elite->cost) || (e_cost <= elite->e_cost && cost <= elite->cost && s_cost < elite->s_cost))    flag = true;
	//	if (optimtype == 12) if (e_cost < elite->e_cost || (e_cost <= elite->e_cost && s_cost < elite->s_cost) || (e_cost <= elite->e_cost && s_cost <= elite->s_cost && cost < elite->cost))    flag = true;
	//	return flag;
	//}

	/*bool p_solution->compareSolution(tabu_solution)
	{
		bool flag = false;
		if (optimtype == 1) if (cost < tabu_cost)    flag = true;
		if (optimtype == 2) if (s_cost < tabu_scost) flag = true;
		if (optimtype == 3) if (t_cost < tabu_tcost) flag = true;
		if (optimtype == 4) if (cost < tabu_cost || (cost <= tabu_cost && s_cost < tabu_scost) || (cost <= tabu_cost && s_cost <= tabu_scost && t_cost < tabu_tcost)) flag = true;
		if (optimtype == 5) if (s_cost < tabu_scost || (s_cost <= tabu_scost && t_cost < tabu_tcost) || (s_cost <= tabu_scost && t_cost <= tabu_tcost && cost < tabu_cost)) flag = true;
		if (optimtype == 6) if (t_cost < tabu_tcost || (t_cost <= tabu_tcost && cost < tabu_cost) || (t_cost <= tabu_tcost && cost <= tabu_cost && s_cost < tabu_scost)) flag = true;
		if (optimtype == 7) if (cost < tabu_cost || (cost <= tabu_cost && t_cost < tabu_tcost) || (cost <= tabu_cost && t_cost <= tabu_tcost && s_cost < tabu_scost)) flag = true;
		if (optimtype == 8) if (s_cost < tabu_scost || (s_cost <= tabu_scost && cost < tabu_cost) || (s_cost <= tabu_scost && cost <= tabu_cost && t_cost < tabu_tcost)) flag = true;
		if (optimtype == 9) if (t_cost < tabu_tcost || (t_cost <= tabu_tcost && s_cost < tabu_scost) || (t_cost <= tabu_tcost && s_cost <= tabu_scost && cost < tabu_cost)) flag = true;
		if (optimtype == 10) if (e_cost < tabu_ecost) flag = true;
		if (optimtype == 11) if (e_cost < tabu_ecost || (e_cost <= tabu_ecost && cost < tabu_cost) || (e_cost <= tabu_ecost && cost <= tabu_cost && s_cost < tabu_scost)) flag = true;
		if (optimtype == 12) if (e_cost < tabu_ecost || (e_cost <= tabu_ecost && s_cost < tabu_scost) || (e_cost <= tabu_ecost && s_cost <= tabu_scost && cost < tabu_cost)) flag = true;
		return flag;
	}*/



	void Calculate_range()
	{
		for (int g = 0; g<p_solution->soljobs.size(); g++)
		{
			job *jb = p_solution->soljobs[g];
			for (size_t h = 0; h<jb->operations.size(); h++)
			{
				operation *op1 = jb->operations[h];
				if (!op1->has_preds()) op1->sca = 0;
				else
				{
					operation *op1pred = op1->pred[0];
					op1->sca = op1pred->end;
				}
				if (op1->sca < op1->a) op1->sca = op1->a;
				if (!op1->has_sucs()) op1->scb = planhorend;
				else
				{
					operation *op1suc = op1->suc[0];
					op1->scb = op1suc->start;
				}
				if (op1->scb > op1->b) op1->scb = op1->b;
			}
		}
		return;
	}



	void print_sol()
	{
		/*for(int y=0;y<p_solution->solmachines.size();y++)
		{
		machine *mc=p_solution->solmachines[y];
		cout<<"MACHINE "<<mc->ID<<endl;
		for(int g=0;g<mc->operations.size();g++)
		{
		operation *op=mc->operations[g];
		cout<<"ID:"<<op->jobID<<" "<<op->ID<<" ST:"<<op->temp_start<<" ";
		}
		cout<<endl;
		}*/
		for (size_t u = 0; u < p_solution->soljobs.size(); u++)
		{
			job *jb = p_solution->soljobs[u];
			for (size_t y = 0; y < jb->operations.size(); y++)
			{
				operation *op = jb->operations[y];
				cout << op->mthesi << " ";
			}
			cout << endl;
		}

	}

	void print_soltrue()
	{
		for (size_t y = 0; y < p_solution->solmachines.size(); y++)
		{
			machine *mc = p_solution->solmachines[y];
			cout << "MACHINE " << mc->ID << endl;
			for (size_t g = 0; g < mc->operations.size(); g++)
			{
				operation *op = mc->operations[g];
				cout << "ID:" << op->jobID << " " << op->ID << " ST:" << op->start << " ";
			}
			cout << endl;
		}
	}


	bool tabuSearch(solution *input) {

		//Initiallize TS

		//cout << "Mem Testing Psolution" << endl;
		//p_solution->memTestSolution();

		lsObject->setup(input);

		//return lsObject->tabu_search(vector<int>{CRIT_SMART_RELOCATE, CRIT_BLOCK_UNION}, 14);
        return lsObject->tabu_search(vector<int>{RELOCATE}, 14);
		//return lsObject->old_tabu_search();
		//return lsObject->vns();
		//return lsObject->vns_ts();
	}

	void solPerturb(solution *input) {

		//Initialize TS

		//Prepare lsObject
		delete lsObject->currentSol;

		//Setup starting solutions
		lsObject->currentSol = new solution();
		*lsObject->currentSol = *input;

		//cout << *input;
		//cout << "FreqMap before" << endl;
		//globals->op_freq_map->report(input);

		//Get distant solution
		lsObject->distant_sol_finder(0.4f, 50, 4, 14);

		//Replace input with the distant solution
		*input = *lsObject->currentSol;

		//cout << "FreqMap after" << endl;
		//globals->op_freq_map->report(input);
	}

	int new_algo() {
		//Construct Initial solution
		
		//cout << "INIT Solution" << endl;
		//cout << *p_solution;

		//cout << "INIT after TS Solution" << endl;
		//cout << *lsObject->localElite;

		//Computational Loop
		time(&now); // get current time
		time_elapsed = difftime(time(NULL), now);

		//Init Pool with refined solutions
		printf("Populating Pool\n");
		solution *t_sol;
		costObject* stat;
		while (sol_pool->solutions.size() < (size_t) pool_size) {
			//if (!p_solution->greedyConstruction(true)) {
			if (!p_solution->aeapConstruction(rand() % 5, bllist, MAKESPAN)) {
				cout << "-1 APO AEAP_CONSTRUCTION" << endl; return -1;
			}

			t_sol = new solution();
			//Apply tabu search on this solution
			if (!tabuSearch(p_solution)) return -1;
			
			*t_sol = *lsObject->localElite;
			int pool_update_status = sol_pool->update(t_sol);

			if (pool_update_status == 2) {
				printf(BOLDWHITE "NEW BEST " RESET);
				printf("Iteration %d ", 0);
				sol_pool->best->reportCosts(false);
				//Store Best to Elite
				*globals->elite = *sol_pool->best;
				
				//Save the best sol to filedirectly
				ofstream solfile;
				solfile.open(solutionfile);
				globals->elite->saveToFile(solutionfile.c_str());
				solfile.close();
				//Save Progress
				stat = new costObject(sol_pool->best, 0);
				globals->elite_prog.push_back(stat);
			}

		}

 		
		//while (time_elapsed < options.time_limit) {
		int iters = 0;

		iters = 0;
		while ( iters < options.ms_iters){
			
			// REAL METHOD
			t_sol = new solution();
			solution *s_sol = sol_pool->select();
			*t_sol = *s_sol;

			//t_sol->createDistanceMatrix();
			//cout << "Starting Solution" << endl;
			//cout << *t_sol;
			
			//Perturb the ls result
			//Perturbation should be working good
			//Pertubation method 1 : Use distance from best
			//float perturb_strength = std::max(0.1f, 1.0f - float(t_sol->weight) / t_sol->totalNumOp);
			//Pertubation method 2 : Use pool health
			// std::max(0.01f, std::min(1.0f - sol_pool->health, 0.3f));
			//float perturb_strength = clamp<float>(1.0f - sol_pool->health, 0.1f, 0.4f);
			
			//Map 0.6-1.0 health values to 0.05 - 0.25 strength values
			float min_perturb = 0.10f;
			float max_perturb = 0.50f;
			float min_health =  0.20f;
			float max_health =  1.00f;

			float temp = (min_perturb - max_perturb)/(max_health - min_health);
			float perturb_strength = sol_pool->health*temp + (max_perturb - min_health*temp);
			//cout << "Selected Solution" << endl;
			//cout << *t_sol;
			//t_sol->memTestSolution();
			//printf("Perturbing with Strength %f\n", perturb_strength);
			
			lsObject->Perturb(t_sol, perturb_strength);
			//assert(t_sol->forwardGraph->checkNodes());
			//cout << "Perturbed Solution" << endl;
			//cout << *t_sol;

			
			//Apply tabu search on the perturbed solution
			if (!tabuSearch(t_sol)) return -1;
			else {
				//lsObject->localElite->reportCosts(false);
				//lsObject->report();
			}

			*t_sol = *lsObject->localElite;
			
			int pool_update_status = sol_pool->update(t_sol);

			if (!pool_update_status) {
			 	delete t_sol;
			} else {
				//Solution was inserted to the pool, recompense the mother solution 
				s_sol->life = 0;
				//Check for the new best
				if (pool_update_status == 2) {
					printf(BOLDWHITE "NEW BEST " RESET);
					printf("Iteration %d ", iters);
					sol_pool->best->reportCosts(false);
					//Save Progress
					stat = new costObject(sol_pool->best, iters);
					globals->elite_prog.push_back(stat);
				}
			};
			
			//printf("Iteration %d - ", iters);
			sol_pool->report(true);
			sol_pool->tick();
			
			//Update elapsed time
			time_elapsed = difftime(time(NULL), now); 
			//cout << "Total Time Elapsed: " << std::setprecision(8) << time_elapsed << " sec" << endl;
			iters++;
		}

		//Finish it
		//*globals->elite = *sol_pool->best;

	};
	//Testing and Comparison Routines
	void calibrateConstruction(){
		int CS_min = 10;
		int CS_max = 200;

		int CP_min = 1;
		int CP_max = 10;

		int counter = 0;
		int exp_num = 50;
		printf("AEAPConstruction Calibration Process \n");
		for (int i=CS_min; i <= CS_max; i+=10){
			for (int j=CP_min; j <= min(i, CP_max); j+=1){

				float avg_makespan = 0.0f;
				float min_makespan = INF;
				float max_makespan = 0.0f;
				
				for (int e=0; e < exp_num; e++){
					p_solution->cleanUpSolution();
					p_solution->aeapConstruction(rand() % 3, bllist, MAKESPAN);
					p_solution->getCost(false);
					//printf("Current Makespan %d \n", p_solution->cost_array[MAKESPAN]);
					float current_span = (float) p_solution->cost_array[MAKESPAN];
					avg_makespan += current_span;
					if (current_span < min_makespan) min_makespan = current_span;
					if (current_span > max_makespan) max_makespan = current_span;
				}

				avg_makespan = avg_makespan / exp_num;
				float score = 0.6f * avg_makespan + 0.2f * min_makespan + 0.2f * max_makespan;
				printf("%4d Params: |S %2d - P %2d| Makespan: Avg: %6.2f Min: %6.2f Max: %6.2f Score: %6.2f\n", 
					counter, i, j, avg_makespan, min_makespan, max_makespan, score);
				counter++;
			}
		}
	}

	void compareConstructionMethods(){
		

		int exp_num = 1000;
		float avg_makespan, min_makespan, max_makespan;

		avg_makespan = 0.0f;
		min_makespan = INF;
		max_makespan = 0.0;
		for (int i=0; i<exp_num; i++){
			p_solution->cleanUpSolution();
			p_solution->greedyConstruction(true);
			p_solution->getCost(false);
			//printf("Current Makespan %d \n", p_solution->cost_array[MAKESPAN]);
			float current_span = (float) p_solution->cost_array[MAKESPAN];
			avg_makespan += current_span;
			if (current_span < min_makespan) min_makespan = current_span;
			if (current_span > max_makespan) max_makespan = current_span;
		}

		avg_makespan = avg_makespan / exp_num;

		printf("GreedyConstruction Makespan: Avg: %4.2f Min: %f Max: %f\n", avg_makespan, min_makespan, max_makespan);

		avg_makespan = 0.0f;
		min_makespan = INF;
		max_makespan = 0.0f;
		for (int i=0; i<exp_num; i++){
			p_solution->cleanUpSolution();
			p_solution->aeapConstruction_perOp();
			p_solution->getCost(false);
			//printf("Current Makespan %d \n", p_solution->cost_array[MAKESPAN]);
			float current_span = (float) p_solution->cost_array[MAKESPAN];
			avg_makespan += current_span;
			if (current_span < min_makespan) min_makespan = current_span;
			if (current_span > max_makespan) max_makespan = current_span;
		}

		avg_makespan = avg_makespan / exp_num;

		printf("AEAPConstruction_perOp Makespan: Avg: %f Min: %f Max: %f\n", avg_makespan, min_makespan, max_makespan);

	}

	void comparePerturbationMethods(){
		

		int exp_num = 1000;
		float span_1 = 0.0f;
		float span_2 = 0.0f;

		p_solution->cleanUpSolution();
		p_solution->aeapConstruction(rand() %3, bllist, MAKESPAN);
		
		for (int i=0; i<exp_num; i++){
			//Create and Init
			solution *t1_sol = new solution();
			solution *t2_sol = new solution();
			
			*t1_sol = *p_solution;
			*t2_sol = *p_solution;
			
			lsObject->RemoveAndReinsert(t1_sol);
			lsObject->Perturb(t2_sol, 0.10f);

			//Sum Makespans
			span_1 += (float) t1_sol->cost_array[MAKESPAN];
			span_2 += (float) t2_sol->cost_array[MAKESPAN];
			
			delete t1_sol;
			delete t2_sol;
		}

		//Calculate Average Makespans
		span_1 = span_1 / exp_num;
		span_2 = span_2 / exp_num;
		
		printf("Perturbation %f Perturb %f\n", span_1, span_2);
	}


	void new_representation_test() {
		printf("setsetset\n");

		//This example will work with 4 operations +2 dummy machine operations
		//The goal is to test the two methods doing one relocation move
		//First using the new representation, then using the existing vector representation

		//At first allocate the new representation scheme
		int totalOps = (NumberOfMachines * 2 + p_solution->totalNumOp);
		bool ***mach_table = (bool***)malloc(sizeof(bool**) * NumberOfMachines);
		//Allocate a single table for each machine
		for (int i = 0; i<NumberOfMachines; i++) {
			bool **single_mach_table = (bool**)malloc(sizeof(bool*) * totalOps);
			for (int j = 0; j<totalOps; j++) {
				bool *single_row = (bool*)malloc(sizeof(bool) * totalOps);
				single_mach_table[j] = single_row;
			}
			mach_table[i] = single_mach_table;
		}

		//Init machine table to 0
		for (int k = 0; k<NumberOfMachines; k++) {
			bool **single_mach_table = mach_table[k];
			for (int i = 0; i<totalOps; i++) {
				bool *row = single_mach_table[i];
				for (int j = 0; j<totalOps; j++)
					row[j] = 0;
			}
		}

		//Setup sample solution to vectors
		p_solution->insertOp(p_solution->solmachines[0], 1, p_solution->soljobs[0]->operations[0]);
		p_solution->insertOp(p_solution->solmachines[0], 2, p_solution->soljobs[0]->operations[1]);
		p_solution->insertOp(p_solution->solmachines[1], 1, p_solution->soljobs[1]->operations[0]);
		p_solution->insertOp(p_solution->solmachines[1], 2, p_solution->soljobs[1]->operations[1]);

		cout << *p_solution;

		//Setup sample to machine tables

		mach_table[0][p_solution->solmachines[0]->operations[0]->UID][p_solution->soljobs[0]->operations[0]->UID] = 1;
		mach_table[0][p_solution->soljobs[0]->operations[0]->UID][p_solution->soljobs[0]->operations[1]->UID] = 1;
		mach_table[0][p_solution->soljobs[0]->operations[1]->UID][p_solution->solmachines[0]->operations[3]->UID] = 1;

		mach_table[1][p_solution->solmachines[1]->operations[0]->UID][p_solution->soljobs[1]->operations[0]->UID] = 1;
		mach_table[1][p_solution->soljobs[1]->operations[0]->UID][p_solution->soljobs[1]->operations[1]->UID] = 1;
		mach_table[1][p_solution->soljobs[1]->operations[1]->UID][p_solution->solmachines[1]->operations[3]->UID] = 1;


		//Print Tables
		for (int k = 0; k<NumberOfMachines; k++) {
			bool **single_mach_table = mach_table[k];
			printf("Machine %d Table\n", k);
			for (int i = 0; i<totalOps; i++) {
				bool *row = single_mach_table[i];
				for (int j = 0; j<totalOps; j++)
					printf("%d ", row[j]);
				printf("\n");
			}
		}

		int exp_num = 1000000;
		double e_vec, e_tbl;
		e_vec = 0.0f;
		e_tbl = 0.0f;
		clock_t begin, end;

		begin = clock();

		//Probe Vector move
		for (int m = 0; m<exp_num; m++) {
			//Do the move
			operation *movOp = p_solution->solmachines[0]->operations[1];
			p_solution->removeOp(p_solution->solmachines[0], 1);
			p_solution->insertOp(p_solution->solmachines[1], 1, movOp);

			//cout << *p_solution;
			//Take back the move
			p_solution->removeOp(p_solution->solmachines[1], 1);
			p_solution->insertOp(p_solution->solmachines[0], 1, movOp);
		}

		end = clock();
		e_vec = double(end - begin) / CLOCKS_PER_SEC;

		printf("Time Elapsed for vector Version %f\n", e_vec);


		begin = clock();
		//Probe table move
		for (int m = 0; m<exp_num; m++) {
			//Do the move
			operation* dops0 = p_solution->solmachines[0]->operations[0];
			operation* dope0 = p_solution->solmachines[0]->operations[3];
			operation* dops1 = p_solution->solmachines[1]->operations[0];
			operation* dope1 = p_solution->solmachines[1]->operations[3];

			//Fix stuff on m0
			mach_table[0][dops0->UID][p_solution->soljobs[0]->operations[0]->UID] = 0;
			mach_table[0][p_solution->soljobs[0]->operations[0]->UID][p_solution->soljobs[0]->operations[1]->UID] = 0;
			mach_table[0][dops0->UID][p_solution->soljobs[0]->operations[1]->UID] = 1;
			//Fix stuff on m1
			mach_table[1][dops1->UID][p_solution->soljobs[1]->operations[0]->UID] = 0;
			mach_table[1][dops1->UID][p_solution->soljobs[0]->operations[0]->UID] = 1;
			mach_table[1][p_solution->soljobs[0]->operations[0]->UID][p_solution->soljobs[1]->operations[0]->UID] = 1;

			// for (int k=0; k<NumberOfMachines; k++){
			// bool **single_mach_table = mach_table[k];
			// printf("Machine %d Table\n", k);
			// for (int i=0;i<totalOps;i++){
			// 	bool *row = single_mach_table[i];
			// 	for (int j=0;j<totalOps;j++)
			// 		printf("%d ",row[j]);
			// 	printf("\n");
			// 	}
			// }

			//Take back the move
			//Fix stuff on m0
			mach_table[0][dops0->UID][p_solution->soljobs[0]->operations[0]->UID] = 1;
			mach_table[0][p_solution->soljobs[0]->operations[0]->UID][p_solution->soljobs[0]->operations[1]->UID] = 1;
			mach_table[0][dops0->UID][p_solution->soljobs[0]->operations[1]->UID] = 0;
			//Fix stuff on m1
			mach_table[1][dops1->UID][p_solution->soljobs[1]->operations[0]->UID] = 1;
			mach_table[1][dops1->UID][p_solution->soljobs[0]->operations[0]->UID] = 0;
			mach_table[1][p_solution->soljobs[0]->operations[0]->UID][p_solution->soljobs[1]->operations[0]->UID] = 0;
		}

		end = clock();
		e_tbl = double(end - begin) / CLOCKS_PER_SEC;

		printf("Time Elapsed for table Version %f\n", e_tbl);
		printf("SpeedUP %f\n", e_vec / e_tbl);

	}

	int Scheduling_Algorithm()
	{
        solution *s; //Temp Solution pointer
	    //Save base_solution to the solution list
        //Save solution
        s= new solution();
        *s = *base_solution;
        solutions.push_back(s);

        float base = 100.0f / scheduleCount;
		float p_complete = 0.0f;
		cout << "----------------" << endl;
		cout << "Completed: " << p_complete << "%" << endl;
		cout << "----------------" << endl;
        //ofstream tabufile;
        //tabufile.open(tabuprogressfile);
        for (int y = 0; y < scheduleCount; y++)
		{
            //Override settings
            if (y==0){
                lsObject->options->ts_time_limit = 30; //Force a very fast run of 30 seconds for the first run
            } else
                lsObject->options->ts_time_limit = time_limit; //Use the input limit for the second run

            p_complete += 1.0;
			//Clear Frequency Map
			//globals->op_freq_map->clear(p_solution);
			
			//Original Construction
			//if (!p_solution->greedyConstruction(true)) {
			//My Construction
			//cout << *p_solution;

			bool filter = false;

            DISTANCE_F = 4;

            *p_solution = *base_solution;
            /* DO NOT CONSTRUCT SOLUTION - USE BASE SOLUTION
            if (!p_solution->aeapConstruction(rand() % 5, bllist, MAKESPAN)) {
                //My alternative Construction
                //if (!p_solution->aeapConstruction_perOp()) {
				cout << "-1 APO CONSTRUCTION" << endl; return -1;
			}
            else cout << "Construction: Iter# " << y << " ";
            */

            DISTANCE_F = 0;
            //Report Solution
            cout << *p_solution;
            p_solution->reportCosts(false);

            //Consider only constructed solutions
			//cout <<*p_solution;
            /*
            if ( p_solution->compareSolution(globals->elite) ) {
                *globals->elite = *p_solution;
                //print_soltrue();
                printf(BOLDWHITE "NEW BEST " RESET);
                globals->elite->reportCosts(false);
            }
            continue;
             */

            if (!tabuSearch(p_solution)){
                //Set TS localelite equal to the base solution
                *lsObject->localElite = *p_solution;
                continue;
            } else {
				cout << "Tabu Search: Iter# " << y << " ";
				lsObject->localElite->reportCosts(false);
				lsObject->report();
                //Write progress
                /*
                tabufile << "Iter: " << y << " ";
                tabufile << "Start: " << p_solution->cost_array[MAKESPAN] << " ";
                tabufile << "End: " << lsObject->localElite->cost_array[MAKESPAN] << " " << endl;
                tabufile << "TabuIters: " << lsObject->tabu_progress.size() << " ";
                for (int i=0;i<lsObject->tabu_progress.size();i++)
                    tabufile << lsObject->tabu_progress[i] << " ";
                tabufile << endl;
                */
            }

			//Save solution
			s = new solution();
			*s = *lsObject->localElite;
			solutions.push_back(s);

            //Update global
			if ( lsObject->localElite->compareSolution(globals->elite) ) {
				*globals->elite = *lsObject->localElite;
				//print_soltrue();
				printf(BOLDWHITE "NEW BEST " RESET);
				//cout << "New Best ";
				globals->elite->reportCosts(false);

				//Save the best sol to filedirectly
				saveToFile(solutionfile.c_str());

                //Store to progress vector
				costObject* stat = new costObject(globals->elite, y);
				globals->elite_prog.push_back(stat);
			}

            //Report Elite
			cout << "Elite - ";
		    globals->elite->reportCosts(false);

			//cout << *tabu_solution;
			cout << "----------------" << endl;
			p_complete = base*(y + 1);
			cout << "Completed: " << p_complete << "%" << endl;
			cout << "----------------" << endl;
		
		}

        //tabufile.close();
        return 0;
	}
	
	void saveSolveProgress(const char* filename){
		ofstream myfile;
		myfile.open(filename);
		
		myfile << "Improvement Steps " << globals->elite_prog.size() << endl;
		//Report Progress
		for (size_t i=0; i<globals->elite_prog.size(); i++){
			costObject* cOb = globals->elite_prog[i];
			myfile << cOb->iter << " ";
			for (int j = 0; j < COST_NUM; j++)
				myfile << cOb->costs[j] << " ";
			 myfile << endl;
		}
		myfile.close();
	}

	void testconstruction() {
		solution* backsol = new solution();
		int minhum = 0;
		int maxhum = 0;
		int repeats = 100;
		int values[100];
		//First setup
		p_solution->greedyConstruction(false);
		*backsol = *p_solution;
		for (int i = 0; i < repeats; i++) {
			p_solution->greedyConstruction(false);
			int hum = 0;
			//Test Solution
			//Iterate in machines
			for (size_t j = 0; j < p_solution->solmachines.size(); j++) {
				machine *m = p_solution->solmachines[j];
				machine *mb = backsol->solmachines[j];
				//cout << "New sol: " << m->actualID << " Back sol: " << mb->actualID << endl;

				for (size_t k = 1; k < m->operations.size()-1; k++) {
					operation * op = m->operations[k];
					//int jtestID = ((operation*)(*m->operations)[k])->jobID;
					//cout << "Test ID " << testID << " JobID "<< jtestID << endl;
					if (mb->containsOP(op)) hum += 1;
					}
			}
			minhum = std::min(minhum, hum);
			maxhum = std::max(maxhum, hum);
			values[i] = hum;
		}

		cout << "HUMMING DISTANCE HISTOGRAM " << endl;
		plot_histogram(values, repeats, maxhum);
	}

	void plot_histogram(int *input, int size, int maxval) {
		int hist[10];
		int sec_num = 10;
		int new_max = maxval + (maxval % sec_num);
		int interval = new_max / sec_num;

		for (int j = 0; j < sec_num; j++) {
			hist[j] = 0; //Init
			for (int i = 0; i < size; i++) {
				if ( (input[i] >= j*interval) && (input[i] < (j+1)*interval)) hist[j] += 1;
			}
		}

		//Print histogram
		char s[100];
		cout << "--START OF HISTOGRAM--" << endl;
		for (int i = 0; i < 10; i++) {
			sprintf(s, "| Values %2d - %2d | ",i*interval,(i+1)*interval);
			//cout << "Values " << i*interval << " - " << (i + 1)*interval;
			cout << s;
			for (int j = 0; j < hist[i]; j++) cout << "*";
			cout << endl;
		}
		cout << "--END OF HISTOGRAM--" << endl;

	}

	string tm_to_str(const char *fmt, struct tm *timestruct){
	    char buffer[200];
        strftime(buffer, sizeof(buffer), fmt, timestruct);

        return string(buffer);
	}

	void saveToStream(ostringstream& stream) {
	    //This method outputs the solution in a more compact form meant for easier parsing

	    //Save Orders
        stream << "Orders " << NumberOfOrders << endl;
        for (int i=0; i<NumberOfOrders; i++) {
            ProductionOrder *po = dataset->orders[i];
            char buffer_rel[100];
            char buffer_due[100];
            strftime(buffer_rel, sizeof(buffer_rel), "%Y-%m-%d %H:%M", &po->releaseDate);
            strftime(buffer_due, sizeof(buffer_due), "%Y-%m-%d %H:%M", &po->dueDate);

            stream << po->orderID << " " << po->orderName <<
                   " " << po->quantityRemaining << " " << po->quantityTotal <<
                   " " << po->productID << " " << po->urgent <<
                   " " << buffer_rel << " " << buffer_due << endl;

        }

        //Save Events
        stream << "Events " << eventCount << endl;
        for (int i=0; i < dataset->productionLines.size(); i++) {
            ProductionLine *pl = dataset->productionLines[i];
            if (pl->unavailabilities.size()) {
                for (int j=0;j<pl->unavailabilities.size();j++) {
                    mach_unav *unav = pl->unavailabilities[j];
                    stream << pl->Name << " " << unav->start_unav << " " << unav->end_unav << endl;
                }
            }
        }

        //Call solution method to save jobs/machines/operations
        globals->elite->savetoStream(stream);

        printf("Test5");

	}



	void saveToFile(const char* filename) {
		ofstream myfile;

		myfile.open(filename);
		
		//Save solver properties
		myfile << "Dataset: " << benchfile << endl;
		myfile << "Solver Parameters: " << endl;
		myfile << "Optimtype: " << optimtype;
		myfile << " Seed: "<< seed;
		myfile << " Restarts: "<< options.ms_iters;
		myfile << " Time Elapsed: " << time_elapsed << endl;
		myfile << "##################################" << endl;
	#ifdef TABU_SEARCH
		myfile << "Tabu Iterations: "<< lsObject->options->ts_iters;
		myfile << " Tabu Tenure: "<< lsObject->options->ts_tenure;
		myfile << " Tabu Time Limit: "<< lsObject->options->ts_time_limit;
    #endif

		//Write main contents
        ostringstream content;

        saveToStream(content);

		myfile << content.str();
        myfile.close();
    }

    void saveScheduleToDB(int scheduleLocalID){

	    ostringstream Sin;
	    ostringstream schedule;

        //DB Stuff Init
        connect_to_db(&disrupt_db_settings, &disrupt_db_conn);

        stringstream query;
        char *table_name;


        table_name = "optimized_plan";

        char *c_query = new char[2*1024*1024]; //Set max query size to 2Mb and we'll see how it goes
        char *q_stat = "INSERT INTO %s (date, jobID, alternativePlanID, optimizedPlan, gannt) VALUES (NOW(), %d, %d, '%s', '%s');";
        int len = sprintf(c_query, q_stat, table_name, jobID, alternativeActionID, "", "");

        //Store teh schedule to the optimized plan table
        execute_query(&disrupt_db_conn.m, c_query, "success");

        //Fetches the optimizedPlanID right after the last query
        int optimizedPlanID = mysql_insert_id(&disrupt_db_conn.m);
        printf("OptimizedPlanID %d\n", optimizedPlanID);

        //Calculate the aggregated schedule and make the queries
        table_name = "production_order";

        //Aggregate Schedule
        solution *sol = solutions[scheduleLocalID];
        cout << *sol;

        //Export Schedule Metrics
        schedule << "Costs: ";
        for (int i=0;i<COST_NUM;i++)
            schedule << sol->cost_array[i] << " ";
        schedule << endl;

        int order_counter = 1;
        //Start by iterating on each machine/line
        for (int i=0; i < sol->solmachines.size(); i++) {
            machine *mc = sol->solmachines[i];
            schedule << "Machine ID " << mc->ID;
            schedule << " Line ID " << mc->line->ID << " - Name " << mc->line->Name << endl;

            //Iterate on all operations of the machine and try to group them according to their order ID
            int j = 1;
            int active_order_id = -1;
            int active_order_quantity = 0;
            long active_order_start = 0;
            long active_order_end = 0;
            ProductionOrder *active_order = nullptr;

            while(j < mc->operations.size() - 1) {
                operation *op = mc->operations[j];
                //Query order
                ProductionOrder *po = dataset->orderIDMap.at(sol->soljobs[op->jobID]->orderID);

                if (po->orderID != active_order_id){
                    if (active_order_id == -1){
                        //Start grouping - First init
                        active_order_id = po->orderID;
                        active_order = po;
                        active_order_quantity = sol->soljobs[op->jobID]->quantity; //Reset Order Quantity
                        active_order_start = op->start;
                        active_order_end = op->end;
                        j++;
                        continue;
                    } else {
                        //Report aggregation
                        schedule << "Order ID " << setw(3) <<  active_order->orderID;
                        schedule << " - Product Name: " << dataset->productsIDMap[active_order->productID]->productName;
                        schedule << " - Product ID: " << active_order->productID;
                        schedule << " - Quantity: " << active_order_quantity;
                        schedule << " - Start Time " << active_order_start;
                        schedule << " - End Time " << active_order_end << endl;

                        //Send query to database
                        query.str("");
                        query << "INSERT INTO " << table_name << " (productionOrderID, orderName, quantityTotal, quantityRemaining, "
                                                                 "urgentBatch, releaseDate, dueDate, productID, productionLineID, optimizedPlanID) VALUES ( "
                              << order_counter << ", "
                              << "'" << active_order->orderName << "', "
                              << active_order_quantity << ", "
                              << active_order_quantity << ", "
                              << active_order->urgent << ", "
                              <<  "\"" << tm_to_str("%Y-%m-%d %H:%M", &active_order->releaseDate) << "\", "
                              <<  "\"" << tm_to_str("%Y-%m-%d %H:%M", &active_order->dueDate) << "\", "
                              << active_order->productID << ", "
                              << mc->line->ID << ", "
                              << optimizedPlanID << " );";
                        execute_query(&disrupt_db_conn.m, query.str().c_str(), "success");

                        //Init with new order stats
                        active_order_id = po->orderID;
                        active_order = po;
                        active_order_quantity = sol->soljobs[op->jobID]->quantity;
                        active_order_start = op->start;
                        active_order_end = op->end;
                        order_counter++;
                        j++;
                        continue;
                    }
                } else {
                    //Merge quantities
                    active_order_quantity += sol->soljobs[op->jobID]->quantity;
                    active_order_end = op->end;
                    j++;
                    continue;
                }
            }

            if (active_order_quantity > 0){
                //Report aggregation
                schedule << "Order ID " << setw(3) <<  active_order->orderID;
                schedule << " - Product Name: " << dataset->productsIDMap[active_order->productID]->productName;
                schedule << " - Product ID: " << active_order->productID;
                schedule << " - Quantity: " << active_order_quantity;
                schedule << " - Start Time " << active_order_start;
                schedule << " - End Time " << active_order_end << endl;

                //Send query to database
                query.str("");
                query << "INSERT INTO " << table_name << " (productionOrderID, orderName, quantityTotal, quantityRemaining, "
                                                         "urgentBatch, releaseDate, dueDate, productID, productionLineID, optimizedPlanID) VALUES ( "
                      << order_counter << ", "
                      << "'" << active_order->orderName << "', "
                      << active_order_quantity << ", "
                      << active_order_quantity << ", "
                      << active_order->urgent << ", "
                      << "\"" << tm_to_str("%Y-%m-%d %H:%M", &active_order->releaseDate) << "\", "
                      << "\"" << tm_to_str("%Y-%m-%d %H:%M", &active_order->dueDate) << "\", "
                      << active_order->productID << ", "
                      << mc->line->ID << ", "
                      << optimizedPlanID << " );";
                execute_query(&disrupt_db_conn.m, query.str().c_str(), "success");
                order_counter++;
            }
        }

        //Save Aggregated plan to disk
        ofstream outfile;
        outfile.open("test_output.out");
        outfile << schedule.str();
        outfile.close();

        //Prepare Plot
        char *plot_cmd = new char[50];
        sprintf(plot_cmd, "./gant_charter.py %d", 0);
        system(plot_cmd);

        //Fetch Image
        ifstream f;
        f.open("gant.png", std::ios::binary);

        f.seekg(0, std::ios::end);
        size_t fsize = (size_t) f.tellg();

        //Allocate char array
        char *data = new char[fsize];
        char *chunk = new char[2*fsize + 1];

        //Fetch data
        f.seekg(0, std::ios::beg);
        f.read(data, fsize);
        f.close();

        //Flatten data
        mysql_real_escape_string(&disrupt_db_conn.m, chunk, data, fsize);


        //Update optimized_plan table entry with the just calculated aggregated schedule
        table_name = "optimized_plan";
        sprintf(c_query, "UPDATE %s SET optimizedPlan='%s' WHERE optimizedPlanID=%d;",
                table_name, schedule.str().c_str(), optimizedPlanID);
        execute_query(&disrupt_db_conn.m, c_query, "success");

        //Update optimized_plan table entry with the just calculated aggregated schedule
        table_name = "optimized_plan";
        sprintf(c_query, "UPDATE %s SET gannt='%s' WHERE optimizedPlanID=%d;",
                table_name, chunk, optimizedPlanID);
        execute_query(&disrupt_db_conn.m, c_query, "success");


        //Report Aggregated Schedule to console
        cout << schedule.str();

        disconnect_from_db(&disrupt_db_conn);

        delete[] data;
        delete[] chunk;
        delete[] c_query;
		delete[] plot_cmd;
    }
};

int strlen(char s[])/* strlen: return length of s */
{
	int i = 0;
	while (s[i] != '\0')
		++i;
	return i;
}

void reverse(char s[])/* reverse:  reverse string s in place */
{
	int i, j;
	char c;

	for (i = 0, j = strlen(s) - 1; i<j; i++, j--) {
		c = s[i];
		s[i] = s[j];
		s[j] = c;
	}
}

void itoa(int n, char s[]) /* itoa:  convert n to characters in s */
{
	int i, sign;

	if ((sign = n) < 0)  /* record sign */
		n = -n;          /* make n positive */
	i = 0;
	do {       /* generate digits in reverse order */
		s[i++] = n % 10 + '0';   /* get next digit */
	} while ((n /= 10) > 0);     /* delete it */
	if (sign < 0)
		s[i++] = '-';
	s[i] = '\0';
	reverse(s);
}

double randd() //Return a random double between 0 and 1
{
	return (double)rand() / RAND_MAX;
}

int min(int a, int b) {
	return a < b ? a : b;
}



//**************************************************************************************
void usage() {
    for (int i=0;i<20;i++) printf("#"); printf("\n");
    printf("#### AUEB Optimization Tool for DISRUPT v0.0000000001\n");
    printf("#### Usage: 'tool.exe jobID alternativeActionID scheduleCount executionMode eventCount eventList productionStartTime reschedulingTime'\n");
    for (int i=0;i<20;i++) printf("#"); printf("\n");
}

//Data Parser
int readFromDatabase(problem_RCJSS* &pr){
    ProblemDataset *ds = new ProblemDataset();
    //Re-Establish connections
    connect_to_db(&arcelik_db_settings, &arcelik_db_conn);
    connect_to_db(&disrupt_db_settings, &disrupt_db_conn);

    ds->parse_data(arcelik_db_conn, disrupt_db_conn, jobID, eventCount, eventIDs);


    printf("Disconnecting...\n");

    //Disconnect from Databases
    disconnect_from_db(&arcelik_db_conn);
    disconnect_from_db(&disrupt_db_conn);

    //Generate Problem and set dataset
    pr = new problem_RCJSS();
    pr->dataset = ds;

    cout << "Start Preprocessing" << endl;

    //Assign process Steps and machines from the dataset to the solution
    for (int i=0; i<ds->processSteps.size(); i++){
        process_step *ps = ds->processSteps[i];
        pr->p_solution->ProcSteps.push_back(ps);
    }

    for (int i=0; i<ds->machines.size(); i++){
        machine *mc = ds->machines[i];
        pr->p_solution->solmachines.push_back(mc);
    }

    //Set solution dataset
    pr->p_solution->dataset = ds;

    //Data preprocessing and solution construction

    //Preprocess Orders and generate jobs/operations
    //pr->preprocess_DISRUPT_orders();
    pr->preprocess_DISRUPT_orders_wResources();
    pr->setOperationUIDs();
    pr->p_solution->GetCounts();
    pr->p_solution->Setup_Resources();

    //Initiate Graphs
    pr->Init_Graphs();

    //Construct base schedule without any events
    pr->p_solution->constructBaseSchedule();
    *pr->base_solution = *pr->p_solution;



    //Apply Events
    pr->applyEvents();

    //If there is an event try to split the affected jobs
    if (pr->dataset->events.size() > 0)
        pr->p_solution->splitJobs(pr->dataset->events[0]);

    *pr->base_solution = *pr->p_solution;






    //Process base schedule and split orders according to the rescheduling time;

    /*
        int reschedulingTimeSeconds = (int) difftime(mktime(&ReschedulingTime), mktime(&PlanningHorizonStart_Minus));
        reschedulingTimeSeconds += 20000; //Testing
        pr->p_solution->freezeSchedule(reschedulingTimeSeconds);
        *pr->base_solution = *pr->p_solution;
    */

    printf("Preprocessing Successful\n");

    return 0;
}


void exp_main(){
    //Override Planning Horizon End
    planhorend = 2 << 22;

    //Experiment A
    //Try to add a linear rate consumption in an existing resource

    //Experiment with resources
    resource *r = new resource();
    cout << *r << "-----\n";

    //Coeffs Array
    double coeffs[4] = {0.0, 0.0, 0.0, 0.0};

    //Notes:
    //Production should be added given a start and end time as well as a production rate
    //Quantity should be optional since it can be crosschecked with the time interval difference
    //and the production rate

    //Consumption should be added given a quantity and a minimum quantity to start the consumption.
    //At first a feasible time period is detected and then the production is added until all
    //required consumption is satisfied


    //Add production
    coeffs[0] = 2.0f;
    r->insert_production(200,0, 100, coeffs);
    r->validate_blocks();
    r->update_stats();
    cout << *r << "-----\n";

    r->insert_production(200,400, 500, coeffs);
    r->validate_blocks();
    r->update_stats();
    cout << *r << "-----\n";

    //Add consumption
    coeffs[0] = -3.0f;
    double cons_actual_start, cons_actual_end;
    r->insert_consumption(400, 50, 0.0,
            coeffs, cons_actual_start, cons_actual_end);
    r->validate_blocks();
    r->update_stats();
    cout << *r << "-----\n";

    //Add another production block
    coeffs[0] = 1.0f;
    r->insert_production(100, 0, 100, coeffs);
    r->validate_blocks();
    r->update_stats();
    cout << *r << "-----\n";

    //Add another consumption block
    coeffs[0] = -2.0f;
    r->insert_consumption(100, 50, 200, coeffs,
            cons_actual_start, cons_actual_end);
    r->validate_blocks();
    r->update_stats();
    r->export_to_txt("resource_data");
    cout << *r << "-----\n";

    delete r;
}



int main(int argc, char **argv)
{
    //Random number to use as a seed
	seed = (unsigned long) time(NULL);
	//seed = rand();
	//seed = 192837465;
	//seed = 1507826970;
	//seed = 1512573107;
	//seed = 1515453855;
	//seed = 1516975531;
    //seed = 1569351861;
	srand(seed);
	
	//int rand_num = rand();
	//srand(rand_num);


	//Reroute to experimental main for testing resources
	//exp_main();
	//return 0;

	//Get Input Data file from cmd if passed
	//string benchfile; //Global definition
	string benchlfile;
	string outputfile;
	string progressfile;
    solutionfile = "sol_" + to_string(seed) + ".out";
    tabuprogressfile = "tabuprog_tiebreaker" + to_string(TIE_BREAKER) + "_" + to_string(seed) + ".out";

    if (argc < 5){
        printf("Wrong CMD Parameters\n");
        usage();
        //Set input file manually here
        //Linux Test
        outputfile = "test_output.out";
        progressfile = "test_prog.out";
        return -1;
    }

	if (argc >= 5) {
		//jobID
        jobID = atoi(argv[1]);
        //alternativeActionID
        alternativeActionID = atoi(argv[2]);
		//ScheduleCount
        scheduleCount = atoi(argv[3]);
        //Fix schedule count to 3
        //ExecutionMode: 0: ScheduleGeneration, 1: Rescheduling
        executionMode = atoi(argv[4]);

        //EventCount
		eventCount = atoi(argv[5]);
		//Parse Events if any
		if (eventCount>0){
			eventIDs = new int[eventCount];
			for (int i=0;i<eventCount;i++)
				eventIDs[i] = atoi(argv[6 + i]);
		}

        productionStartTime = argv[6 + eventCount];
        //strptime(productionStartTime.c_str(), "%Y/%m/%d %H:%M", &PlanningHorizonStart);
        strptime(productionStartTime.c_str(), "%Y/%m/%d", &PlanningHorizonStart); //Fetch date info only

        //Make sure to reset time info
        PlanningHorizonStart.tm_hour = 0;
        PlanningHorizonStart.tm_min = 0;
        PlanningHorizonStart.tm_sec = 0;

        //Keep a time structure a day before
        time_t day_seconds = 24 * 60 * 60;
        time_t date_seconds = timegm(&PlanningHorizonStart) - day_seconds;
        PlanningHorizonStart_Minus = *localtime(&date_seconds);

        productionEndTime = argv[7 + eventCount];
        strptime(productionEndTime.c_str(), "%Y/%m/%d %H:%M", &PlanningHorizonEnd);

        //By default set Rescheduling time equal to the start of the planning horizon
        ReschedulingTime = PlanningHorizonStart;

        //Set planhorizonend
        //planhorend = difftime(mktime(&PlanningHorizonEnd), mktime(&PlanningHorizonStart_Minus));

        //Add 10 weeks ahead
        //planhorend += 10 * (7 * (24 * (60 * 60)));


        outputfile = "test_output.out";
		progressfile = "test_prog.out";
    }

	if (!read_settings())
	    return -1;

    if (!validate_args())
        return -1;

    //Algorithm Input / Control Panel
	tabu_relaxation = 0.0f;

	setbuf(stdout, NULL);
	cout << "Temp Solution File: " << solutionfile << endl;

	problem_RCJSS *pr;
    if (readFromDatabase(pr) < 0){
        printf("Error while reading the database. Aborting execution\n");
        return -1;
    }

	//Report Orders
	pr->reportOrders();

    //Populate search options
	pr->options.ms_iters = restart_iterations;
	pr->options.strategy = firstbest;
	pr->options.time_limit = time_limit;
    //pr1.options.time_limit = 1;

    int algo = TABU_SEARCH;
    switch(algo) {
        case TABU_SEARCH:
        case SCATTER_SEARCH: {
            tabu_search_options *ts_opts = new tabu_search_options();
            ts_opts->ts_iters = tabu_iterations;
            ts_opts->ts_tenure = tabu_tenure;
            ts_opts->ts_strategy = firstbest;
            ts_opts->ts_time_limit = time_limit;
            ts_opts->ts_perturb_limit = tabu_perturbation_limit;
            pr->lsObject->options = ts_opts;
            break;
        }
        default:
            printf("Not Implemented Algorithm Options Setting\n");
            break;
    }

    //Testing Section
	//pr1.p_solution->forwardGraph->report();
	//pr1.p_solution->forwardGraph->backreport();
	//pr1.compareConstructionMethods();
	//pr1.comparePerturbationMethods();
	//pr1.calibrateConstruction();
	
	//End of Testing Section
	time(&now); // get current time
	
	/*int algo_id = 0;
	printf( "Select Algorithm [0, 1]:\n" );
	scanf( "%d", &algo_id );
	if ( algo_id == 0 )
		pr1.Scheduling_Algorithm();
	else if (algo_id == 1)
		pr1.new_algo();
	else {
		printf("Non Valid Algorithm Selection");
		assert(false);
	} */
	
	//cout << "FreqMap Init" << endl;
	//globals->op_freq_map->report(pr1.p_solution);

    switch(algo){
        case TABU_SEARCH:
        	pr->Scheduling_Algorithm();
            break;
        case SCATTER_SEARCH:
            printf("No proper wrapper call\n");
            break;
        case VNS:
            printf("No proper wrapper call\n");
            break;
        case VNS_TS:
            printf("No proper wrapper call\n");
            break;
		default:
            printf("Algorithm Not Implemented\n");
            break;
    }

	//pr1.new_algo();

    time_elapsed = difftime(time(NULL), now);
	
	cout << "Total Time Elapsed: " << std::setprecision(8) << time_elapsed << " sec" << endl;
	//cout << *globals->elite;

	//Report optimal schedule
    //pr->exportProductionPlan(globals->elite);

	//Save progress
	pr->saveSolveProgress(progressfile.c_str());

	//Save solution to file
	pr->saveToFile(outputfile.c_str());

	//Save solutions to database

	for (int i=0;i < solutions.size(); i++){
        pr->saveScheduleToDB(i);
	}

    //cout << *globals->elite;

	//Delete temp solutions file
    remove(solutionfile.c_str());


	//Delete saved solutions
    for (int i=0;i<scheduleCount;i++)
        delete solutions[i];
    solutions.clear();

    delete pr;

    //Save resources
	//globals->elite->reportResources();

	return 0;

}



