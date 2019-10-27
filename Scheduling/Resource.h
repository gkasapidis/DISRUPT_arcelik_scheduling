#include <ostream>
#include <vector>
#include "Parameters.h"
#include "CObject.h"
#include <cstdio>
#include <cstdlib>

using namespace std;

#ifndef RESOURCE_H
#define RESOURCE_H


/* PANOS RESOURCE TWEAKS NOTES

res_load_t_aux[t][2];
[] - >

res_load_t[8]
60 60 60 100 100 90 90 90

res_load_t_aux[8][2];
60 100 90
3 2 3

|
|
|
|       11111111111111
|       1
|       1
|	11111
|_________________________


proctime_scenaria[proc time diaforetika operations with diff proc times][thesi sto discrete xrono]

[olo to horizon]
6 [0 0 0 0 0 1 1 1 0 0 0 1 10 0 1 ]
8
10

6 78 79 80 .... 110 111 112 ...
8 78 79
10 102 103 .... 180

*/

//TODO: Bring back the enum for the resource types but make sure that they
//TODO: can be masked in order to identify all resource type combinations

class res_block
{
public:
	int id; //Resource ID
	//float load; //Resource load
	int maxlevel; //Resource Max Level
	int minlevel; //Resource Min Level

	//Specify start-end times of the block
	double time_start;
	double time_end;

	int prod_q; //Produced resource quantity in the block
	int cons_q; //Consumed resource quantity in the block

    //Production Coeffs
    double prod_coeffs[4];
	//Consumption Coeffs
    double cons_coeffs[4];

	//The following attributes store the resource quantities at the start and the end
    //of the duration of the block (obviously these have to be provided externally)
    int start_q;
    int end_q;

    //This flag is used to mark the registered consumption so that we can align production on other resource accordingly
    bool marked;

    //Prototype Consumption Rate function (Temporarily)
    // a*x  + b

    //TODO, make the block have successors and predecessors, it would be much more efficient if this was
    //a double linked list

    /// <summary>
	/// Inserts a time block into the resource
	/// </summary>
	/// <param name="start">The starting time of the block</param>
	/// <param name="end">The ending time of the block</param>
	/// <param name="load">The resource consumption for this block</param>
	/// <param name="maxlevel">The resource consumption for this block</param>
	/// <param name="id">The resource consumption for this block</param>
	res_block(double start, double end, int maxlevel, int id) {
		time_start = start;
		time_end = end;
		prod_q = 0;
		cons_q = 0;
		start_q = 0;
		end_q = 0;
		this->id = id;
        this->maxlevel = maxlevel;
        this->minlevel = 0;
        marked = false;
        //Reset Coefficient Values
        for (int i=0; i<4; i++){
            prod_coeffs[i] = 0.0f;
            cons_coeffs[i] = 0.0f;
        }
    }

	///Default Constructor
	res_block(){
	    time_start = 0;
	    time_end = (double) planhorend;
	    id = -1;
	    maxlevel = 0;
        minlevel = 0;
        cons_q = 0;
        prod_q = 0;
        start_q = 0;
        end_q = 0;
        marked = false;
        //Reset Coefficient Values
        for (int i=0; i<4; i++){
            prod_coeffs[i] = 0.0f;
            cons_coeffs[i] = 0.0f;
        }
    }


	~res_block() {};

	//Checks if the block shares the same coefficients with a 3rd block
	bool isSameType(res_block *b){

	    //Check the equality of the coeffs
        for (int i=0; i<4; i++){
            if (abs(prod_coeffs[i] - b->prod_coeffs[i]) >= 1e-5)
                return false;
            if (abs(cons_coeffs[i] - b->cons_coeffs[i]) >= 1e-5)
                return false;
        }
	    return true;
	}

	bool validate(){
        if (start_q < 0 || end_q < 0){
            printf("Negative Start/End Quantity\n");
            cout << *this;
            return false;
        }
        if (start_q + prod_q - cons_q != end_q){
            printf("Failed Block Equilibrium\n");
            cout << *this;
            return false;
        }
        return true;
	}

	bool includesTime(double time){
        if (time >= time_start && time <= time_end)
	        return true;
	    return false;
	}


    bool includesTimeNoBounds(double time){
        if (time > time_start && time < time_end)
            return true;
        return false;
    }


	bool includesQuantity(int q){
	    //Handle the special case first
	    if (q == start_q && q == end_q)
	        return true;

	    //Plenty of input resource that already includes q
	    if (start_q >= q)
	        return true;

	    if (q >= end_q)
            return false;

	    //If q is within the start/end quantities q is included in the block
        return (start_q <= q && q <= end_q);
    }

    bool canHostConsumption(double coeff_a){
        if (start_q > 0)
            return true;
        else if (start_q == 0 && coeff_a <= 0.0)
            return false;

        //There should be no default case
        return false;
    }

    bool canHostConsumption(){
        return canHostConsumption(cons_coeffs[0] + prod_coeffs[0]);
    }

	//Accumulates the resource quantity from the specified time until time_end
	int accumulateQuantity(double time){
	    //TODO: For now I'll be using the trapezoid rule
	    //TODO: Replace with a proper integral to fit the other resource rate types

	    return (time_end - time);
    }

    //Calculate End Quantity
    int calcEndQuantity(){

	    int calc_val = estimateQuantity(time_end);
	    int calc_cached = start_q + prod_q - cons_q;

	    //This calculation should never fail in order to keep the block consistent
	    if (calc_val != calc_cached){
	        //printf("END QUANTITY HAS BEEN FUCKED UP %d vs %d\n",
	        //        calc_val, calc_cached);
	        //Try to repair the coefficients
            int temp = end_q;
	        end_q = calc_cached; //Temporarily save end_q
            repairCoeffs();

            if (estimateQuantity(time_end) != end_q)
                printf("Still broken coeffs...\n");
	        end_q = temp; //Bring it back so that it can be restored after this method returns
	        //assert(false);
	    }

        return calc_cached;
    }

    //Return the quantity estimate based on the production/consumption rates
    int estimateQuantity(double time){
        //Check if time is within the bounds
        if (time < time_start || time > time_end){
            printf("Input time is out of the block time period\n");
            assert(false);
        }

        double prod_coeff =prod_coeffs[0];
        double cons_coeff = abs(cons_coeffs[0]);
        double time_d = time - time_start;

        //TODO: When consumption usage rates are not linear, I should use the correct derivatives
        //TODO: here. For linear rates the derivative is just coeff_a.

        int val_calc = std::max((int) (start_q + my_round(prod_coeff * time_d) - my_round(cons_coeff * time_d) ) , minlevel);

        return val_calc;
	}

	//Return the quantity on a given time assuming that everything is corrent;
	//TODO: Bullshit I think i need the estimateQuantity at all times
	int getQuantity(double time){
	    return estimateQuantity(time);
    }


    double getTimeToExistingQuantity(int load){
        //TODO: This should be compatible with any extra resource types

        double coeff_a = prod_coeffs[0] + cons_coeffs[0];

        //The maximum consumed quantity should be the min of the start_q and end_q values
        return my_round((load + 0.05 - start_q) / coeff_a + time_start);
	}


    double getTime(int load){
	    return getTime(load, 0.0f, 0.0f,prod_coeffs[0], cons_coeffs[0]);
    }

    float getTime(int load, double prod_coeff, double cons_coeff, double new_prod_coeff, double new_cons_coeff){
        if (prod_coeff + new_prod_coeff >= abs(cons_coeff) + abs(new_cons_coeff))
            return getTimeToProduceLoad(load, prod_coeff, cons_coeff, new_prod_coeff);
        else
            return getTimeToConsumeLoad(load, prod_coeff, cons_coeff, new_cons_coeff);
    }

    float getTimeToConsumeLoad(int load, double prod_coeff, double cons_coeff, double new_cons_coeff){
        //New way

        //Step A: Find time to consume the consume the load
        double abs_new_cons_coeff = abs(new_cons_coeff);
        double t_q = (abs_new_cons_coeff >= 1e-5) ? (double) load / abs_new_cons_coeff + time_start : time_start;
        t_q = my_round(t_q);

        //Step B: Find produced quantity on the same time interval
        long q_p = (t_q - time_start) * prod_coeff;

        //Step C: Find consumed quantity using the old coeffs on the same time interval
        long q_c = (t_q - time_start) * abs(cons_coeff);

        //Step C: Do the check
        if (start_q + q_p - q_c - load >= 0)
            return t_q;
        else
            return -1;

    }

    float getTimeToProduceLoad(int load, double prod_coeff, double cons_coeff, double new_prod_coeff){
        //TODO: This should be compatible with any extra resource types

        //Step A: Find time to produce the load
        double abs_cons_coeff = abs(cons_coeff);
        double t_q = (new_prod_coeff >= 1e-5) ? (double) (load - start_q) / new_prod_coeff + time_start : time_start;
        t_q = my_round(t_q);

        //Step B: Find consumed quantity on the same time interval
        long q_c = (t_q - time_start) * abs_cons_coeff;

        //Step C: Find produced quantity on the same time interval using the old coeffs
        long q_p = (t_q - time_start) * prod_coeff;

        //Step D: Do the check
        if (start_q + load + q_p - q_c >= 0)
            return t_q;
        else
            return -1;

	}


    double getTimeForZeroQuantity(double coeff_a){
        //TODO: This should be compatible with any extra resource types
        if ((coeff_a <= 0) && (start_q == 0))
            return 0.0f;
        else if ((coeff_a >= 0) && (start_q >= 0))
            return -1.0f;

        //The maximum consumed quantity should be the min of the start_q and end_q values
        return my_round(-start_q / coeff_a + time_start);
    }

    double getTimeForZeroQuantity(){
        //TODO: This should be compatible with any extra resource types
        double eff_coeff_a = prod_coeffs[0] + cons_coeffs[0];
        return getTimeForZeroQuantity(eff_coeff_a);
    }

    bool isEmpty(){
	    return (start_q == 0 && end_q == 0);
	}


	bool repairProductionCoeffs(){
        //Repair consumption coeffs
        int counter = 0;

        while(estimateQuantity(time_end) != end_q){
            //printf("Mismatching quantities %d vs %d \n",
            //       estimateQuantity(time_end), end_q);
            if (estimateQuantity(time_end) < end_q)
                prod_coeffs[0] += 0.000001f;
            else
                prod_coeffs[0] -= 0.000001f;

            counter++;
            if (counter > 500000) {
                printf("Mismatching production quantities %d vs %d \n",
                      estimateQuantity(time_end), end_q);
                return false;
            } else if (counter > 5000) {
                //printf("Mismatching production quantities %d vs %d \n",
                //      estimateQuantity(time_end), end_q);
                //printf(" ");
            }
        }
        return true;
	}

	bool repairConsumptionCoeffs(){
        //Repair consumption coeffs
        int counter = 0;

        while(estimateQuantity(time_end) != end_q){
            //printf("Mismatching quantities %d vs %d \n",
            //       estimateQuantity(time_end), end_q);
            if (estimateQuantity(time_end) < end_q)
                cons_coeffs[0] -= (-0.000001f);
            else
                cons_coeffs[0] += (-0.000001f);

            counter++;
            if (counter > 500000){
                printf("Mismatching quantities %d vs %d \n",
                       estimateQuantity(time_end), end_q);
                return false;
            }
            else if (counter > 5000){
                //printf(" ");
            }

        }
        return true;
	}

	bool repairCoeffs(){
        //If there is no consumption rate, fix the production rate
        bool fix_coeffs = !(abs(cons_coeffs[0]) < 1e-5);

        if (fix_coeffs) {
            repairConsumptionCoeffs();
        } else {
            repairProductionCoeffs();
        }

        return true;
    }

    //Operator Overloads

    //Assignment Operator
    res_block& operator=(const res_block &rhs) {
        time_start = rhs.time_start;
        time_end = rhs.time_end;
        id = rhs.id;
        maxlevel = rhs.maxlevel;
        minlevel = rhs.minlevel;
        marked = rhs.marked;

        //Copy start/end quantities
        start_q = rhs.start_q;
        end_q = rhs.end_q;
        cons_q = rhs.cons_q;
        prod_q = rhs.prod_q;



        //Copy coefficients
        for (int i=0;i<4;i++){
            prod_coeffs[i] = rhs.prod_coeffs[i];
            cons_coeffs[i] = rhs.cons_coeffs[i];
        }
        return *this;
    };

	res_block& operator+=(const res_block &b1)
    {
        //Copy coefficients
        for (int i=0;i<4;i++){
            prod_coeffs[i] += b1.prod_coeffs[i];
            cons_coeffs[i] += b1.cons_coeffs[i];
        }

        prod_q += b1.prod_q;
        cons_q += b1.cons_q;
    }

    //Getters

    ///Get Block duration
	double duration() { return time_end - time_start; };


    //Output - Reports

	///Cout Operator for res_block
	friend ostream& operator<<(ostream &output, const res_block &ss) {
		output << "Block Start: " << ss.time_start << "  Block End: " << ss.time_end;
		output << " Start Load: " << ss.start_q <<  " End Load: " << ss.end_q;
        output << " Block Production : " << ss.prod_q <<  " Block Consumption: " << ss.cons_q;
		output << " Minimum Level: " << ss.minlevel << "  Maximum Level: " << ss.maxlevel;
		output << " Production Coeffs: " << ss.prod_coeffs[0] << " " << ss.prod_coeffs[1] << " " << ss.prod_coeffs[2] << " " << ss.prod_coeffs[3];
        output << " Consumption Coeffs: " << ss.cons_coeffs[0] << " " << ss.cons_coeffs[1] << " " << ss.cons_coeffs[2] << " " << ss.cons_coeffs[3];
		return output;
	};


};


class resource
{
public:

	//TODO make max level varying with t
    int ID;
    string name;
	int actualID;

	//Production/Consumption quantities for the block
	int start_q;
    int end_q;
	int prod_q;
	int cons_q;

	double max_level;
    //double *res_load_t; //Serialized Resource Horizon

	vector<res_block*> blocks;
	
	resource() {
		//res_load_t = new double[planhorend];
		prod_q = 0;
		cons_q = 0;
        start_q = 0;
        end_q = 0;
		name = "";
		//memset(res_load_t, 0, sizeof(double) * planhorend);

        //Init Res Blocks
		//Create main block
		res_block *mainblock = new res_block(0, (double) planhorend, INF, ID);

		//Add Main block
		blocks.push_back(mainblock);
	}

	resource& operator=(const resource &rhs) {
		this->clear();

		this->actualID = rhs.actualID;
		this->ID = rhs.ID;
		this->name = rhs.name;
		this->max_level = rhs.max_level;
        this->prod_q = rhs.prod_q;
        this->cons_q = rhs.cons_q;
        this->start_q = rhs.start_q;
        this->end_q = rhs.end_q;

        //Delete main block
        delete blocks[0];
        blocks.clear();

        for (size_t i = 0; i < rhs.blocks.size(); i++) {
			//Create blocks
			res_block *block =  rhs.blocks[i];
			res_block *new_block = new res_block();
			*new_block = *block;
			
			blocks.push_back(new_block);
		}

        return *this;
	}

	resource(const resource &copyin) {
		this->actualID = copyin.actualID;
		this->ID = copyin.ID;
		this->name = copyin.name;
		this->max_level = copyin.max_level;
        this->prod_q = copyin.prod_q;
        this->cons_q = copyin.cons_q;
        this->start_q = copyin.start_q;
        this->end_q = copyin.end_q;
		for (size_t i = 0; i < copyin.blocks.size(); i++) {
			//Create blocks
			res_block *block = copyin.blocks[i];
			res_block *new_block = new res_block();
            *new_block = *block;
        }
	};


	friend ostream& operator<<(ostream &output, const resource &ss) {
		//cout << "Resource Blocks" << endl;
        output << "Resource: " << ss.name;
        output << " Start Quantity: " << ss.start_q;
        output << " End Quantity: " << ss.end_q;
        output << " Total Produced Quantity: " << ss.prod_q;
        output << " Total Consumed Quantity: " << ss.cons_q;
        output << endl;
		for (size_t i = 0; i < ss.blocks.size(); i++) {
			output << *ss.blocks[i];
        }
        return output;
	};

    void split_block(double &time, int block_id) {
        res_block *block = blocks[block_id];
        res_block *new_block = new res_block();
        *new_block = *block;
        new_block->prod_coeffs[1] = 0.0f; //Override constant production/consumption rates
        new_block->cons_coeffs[1] = 0.0f;

        //Fix times
        block->time_end = time;
        new_block->time_start = time;

        //Fix quantities
        block->end_q = block->estimateQuantity(time);
        new_block->start_q = block->end_q;

        //Fix produced and consumed quantities
        int total_prod = block->prod_q;
        block->prod_q = (int) my_round(abs((block->duration() * block->prod_coeffs[0])));
        new_block->prod_q = max(0, total_prod - block->prod_q);

        //TODO add loop to repair production rate

        int total_cons = block->cons_q;
        block->cons_q = (int) my_round(abs((block->duration() * block->cons_coeffs[0])));
        new_block->cons_q = max(0, total_cons - block->cons_q);

        //Reset the coefficients in case of a trivial splitting
        //Old block
        if (block->prod_q == 0)
            block->prod_coeffs[0] = 0.0f;
        if (block->cons_q == 0)
            block->cons_coeffs[0] = 0.0f;

        //New block
        if (new_block->prod_q == 0)
            new_block->prod_coeffs[0] = 0.0f;
        if (new_block->cons_q == 0)
            new_block->cons_coeffs[0] = 0.0f;


        //Try to repair blocks
        if (!block->repairCoeffs()){
            export_to_txt("test_resource.txt");
            assert(false);
        }

        if (!new_block->repairCoeffs()){
            export_to_txt("test_resource.txt");
            assert(false);
        }

        blocks.insert(blocks.begin() + block_id + 1, new_block);
    }


    //Inserts Production over marked consumption blocks
    bool insert_production_over_consumption(int load, double *coeffs, resource * res_cons){

        int block_index = -1;
        int acc_q = 0;

        //Iterate over the consumed resource blocks
        while (load - acc_q > 1){
            block_index++;
            if (block_index == res_cons->blocks.size()){
                printf("Remaining load %d\n", load - acc_q);
                assert(false);
            }

            res_block *block = res_cons->blocks[block_index];

            //If block is not marked, continue
            if (!block->marked){
                continue;
            }

            //Otherwise insert production to the block
            //Calculate the consumed/produced load
            int cons_load = (int) my_round(block->duration() * coeffs[0]);

            if (cons_load + acc_q > load) {
                printf("Need to split the block\n");
            }

            insert_block(cons_load, block->time_start, block->time_end, coeffs);
            acc_q += cons_load;
        }

        return true;
    }

	//Insert Producion should be as easy as inserting a block into the resource
	void insert_production(int load, double start, double end, double *coeffs){
	    //Make sure the the end time is within the planhorizonend
	    if (start > planhorend || end > planhorend){
	        printf("Planning horizon not enough to accomodate consumption\n");
	        printf("Start %f, End %f, PlanHorEnd %ld\n", start, end, planhorend);
	        assert(false);
	    }

	    insert_block(load, start, end , coeffs);
	}

	int calcMaxPossibleConsumption(double start_time){
        int q = INF;
        for (int i=0; i < blocks.size(); i++){
            res_block *block = blocks[i];
            if (start_time >= block->time_end)
                continue;
            q = min(q, block->end_q);
        }
        return q;
    }

	double getTimeForMinEndQuantity(int min_q, int &block_index){
        double time = 0.0;
        for (int i=0; i < (int) blocks.size(); i++){
            res_block *block = blocks[i];
            if (block->end_q < min_q) {
                time = block->time_end;
                block_index = i + 1;
            }
        }
        return time;
    }

	bool insert_consumption(int load, int min_start_load, double min_start_time,
                            double *coeffs, double &cons_start_time, double &cons_end_time){

	    //Step A: Identify a time where the load is equal to the min_start_load
	    int block_index_start;
	    int block_index_end;
	    int block_index = 0;

        double start_time = min_start_time;
        double end_time;

        int min_end_quantity = 0;

        //Estimate minimum duration for the consumption
        float min_duration;
        float op_duration = 0.0f; //This is increasing as we add consumptions to the blocks
        if (abs(coeffs[0]) > 0.0f)
            min_duration = (float) my_round(abs( load * ( 1.0 / coeffs[0] ) ));
        else
            min_duration = 1.0f;

        bool set_final_start_time = false;
        bool set_final_end_time = false;

        int acc_q = 0;

        //Clear block marks
        resetMarks();

        while (acc_q < load){
            start_time = getTimeToQuantity(min_start_load, start_time, block_index_start, block_index);
            end_time = getTimeToQuantity(load - acc_q, start_time + min_duration, block_index_end, block_index);

            if (start_time < 0 || end_time < 0){
                printf("No time period can host the required consumption. Inserted %d/%d\n", acc_q, load);
                return false;
            }

            if ((long) start_time > planhorend){
                printf("Start Time has exceeded the plannhing horizon end\n");
                assert(false);
            }

            //Make sure that the min load does not break the min end quantity blocks
            int last_min_block_index = 0;
            double last_min_time = getTimeForMinEndQuantity(min_start_load, last_min_block_index);
            if (last_min_time > start_time){
                start_time = last_min_time;
                block_index_start = last_min_block_index;
            }

            //This should not happen
            if (block_index_start > (int) blocks.size())
                assert(false);

            //Set actual consumption start time
            if (!set_final_start_time){
                cons_start_time = start_time;
                set_final_start_time = true;
            }

            //Split starting block if necessary
            res_block *block = blocks[block_index_start];
            if (block->includesTimeNoBounds(start_time)){
                split_block(start_time, block_index_start);

                //Checking mechanism
                if (block->getQuantity(start_time) < min_start_load){
                    printf("PROBLEM WITH END QUANTITY OF THE BLOCK\n");
                    return false;
                    //assert(false);
                }

                block_index_start++;
            } else if (block->time_end == start_time)
                block_index_start++;

            //Step B: Start Adding consumption to the blocks until the entire load is fullfiled

            block_index = block_index_start; //Set the first value for the block_index
            while (acc_q < load) {
                res_block *block = blocks[block_index];
                bool zero_endq_block = false;
                //Update start and end block quantities in case the previous iteration changed them
                if (block_index > 0)
                    block->start_q = blocks[block_index - 1]->end_q;
                block->end_q = block->calcEndQuantity();

                //There is no point adding consumption to a block that depletes resource
                //TODO: This shit is identical with the check after the consumption addition. Try to group them
                if (block->end_q == 0){
                    //First update all remaining blocks without adding any quantity
                    for (int i=block_index + 1; i<blocks.size(); i++){
                        res_block *block1 = blocks[i - 1];
                        res_block *block2 = blocks[i];

                        block2->start_q = block1->end_q;
                        block2->end_q = block2->calcEndQuantity();
                    }

                    //Break in order to find the next min_load time
                    start_time = block->time_end;
                    break;
                }

                //Calculate new coeff_a
                double temp_coeff_a = coeffs[0] + block->cons_coeffs[0] + block->prod_coeffs[0];
                int block_consumption_q = (int) my_round(abs(coeffs[0] * block->duration())); //Consumption if the op would be applied on the entire block
                int max_possible_block_consumption = calcMaxPossibleConsumption(block->time_start);
                bool update_next_blocks = false;

                //WARNING
                //IGNORE THE MIN OPERATION DURATION - KEEP EVERYTHING CONSISTENT WITH THE QUANTITIES
                //BRING THIS BACK IF SOMETHING BREAKS
                //if (time_for_full_load > 0)
                //    time_for_full_load = max(time_for_full_load, cons_start_time + min_duration);

                block_consumption_q = min(block_consumption_q, min(load - acc_q, block->end_q));
                block_consumption_q = min(block_consumption_q, max_possible_block_consumption);

                //Make sure that the calculated possible consumption is greater than zero
                if (block_consumption_q == 0){
                    block_index++;
                    continue;
                }

                //Get time to reach the remaining load
                double time_for_full_load = block->getTimeToConsumeLoad(block_consumption_q,
                        block->prod_coeffs[0], block->cons_coeffs[0], coeffs[0]);


                //Two Cases: The block will have sufficient quantity for the load, or not
                //Case A: The time for the full load will be valid and within the block, the block will be split and the look will break
                //Case B: If the new coeff is negative but the required load is not in the block, the zero time will be calculated
                //it will be within the block, the block will be split and we continue from then on
                //Default Case: The new coeff is still non negative, and also the the load of the block is not enough for the required consumption
                //In this case the pre calculated block_consumption_q will be a valid number within the bounds of the block
                //and we also won't have to care about zero time since the coeff is not negative.
                //Lets see how this will work
                if (block->includesTimeNoBounds(time_for_full_load)){
                    split_block(time_for_full_load, block_index);
                } else if (temp_coeff_a < 0.0) {
                    //Find time for 0 quantity
                    double zero_time = block->getTimeForZeroQuantity(temp_coeff_a);
                    //Update Calculate maximum possible quantity
                    max_possible_block_consumption = calcMaxPossibleConsumption(block->time_start);
                    //If we cannot place any other consumption, break and search for a new start_time
                    if (max_possible_block_consumption == 0) break;

                    float zero_time_for_endq = block->getTime(max_possible_block_consumption, block->prod_coeffs[0], block->cons_coeffs[0], 0.0f, coeffs[0]);
                    bool adapted_block_consumption = false;
                    int estim_block_consumption = 0;
                    if ((zero_time_for_endq > 0) && (zero_time_for_endq < zero_time)){
                        zero_time = zero_time_for_endq;
                        adapted_block_consumption = true;
                        estim_block_consumption = max_possible_block_consumption;
                    }

                    if (block->includesTimeNoBounds(zero_time)){
                        //We need to split the block
                        split_block(zero_time, block_index);

                        if (adapted_block_consumption){
                            block_consumption_q = estim_block_consumption;
                        } else {
                            block_consumption_q = block->start_q + block->prod_q - block->cons_q;
                            zero_endq_block = true;
                        }
                    }
                }


                //Store consumption iff the block can accomodate production
                if (block->canHostConsumption(temp_coeff_a)){
                    block->cons_q += block_consumption_q;
                    block->end_q -= block_consumption_q;
                    block->cons_coeffs[0] += coeffs[0];
                    block->marked = true; //Mark consumption block
                    //Repair block
                    if (!block->repairCoeffs()){
                        printf("UNABLE TO REPAIR QUANTITIES ABORT\n");
                        //assert(false);
                        return false;
                    }

                    op_duration += block->duration(); //Add to the duration of the operation
                    acc_q += block_consumption_q;
                }

                //block start quantity should be good at this point

                //Make sure that block's end_q is zero for consistency purposes
                if (zero_endq_block){
                    if (block->getQuantity(block->time_end) != 0)
                        assert(false);
                }

                //If the end quantity of the block is 0, we have to search again for the next available time
                //for the min load. Also if we have reached the required quantity we can quit
                //if (block->end_q == 0 || acc_q >= load || update_next_blocks) {
                //When consumption is added we need to update the next blocks at all times in order to be able to calculate
                //the max possible consumption quantity.
                {
                    //First update all remaining blocks without adding any quantity
                    for (int i=block_index + 1; i<blocks.size(); i++){
                        res_block *block1 = blocks[i - 1];
                        res_block *block2 = blocks[i];

                        block2->start_q = block1->end_q;
                        block2->end_q = block2->calcEndQuantity();
                    }
                }

                if (block->end_q == 0 || acc_q >= load){
                    //Break in order to find the next min_load time
                    start_time = block->time_end;
                    //if the quantity is good, break and also set the actual end time of consumption
                    cons_end_time = block->time_end;
                    break; //breaks the accumulation loop
                }

                //Get to the next block
                block_index++;

                if (block_index >= blocks.size()){
                    printf("Unable to fit consumption. Something went wrong\n");
                    assert(false);
                }
            }
	    }

        //This should never happen
        if (acc_q > load)
            return false;

        update_stats();

        //Validate structure
        return validate_blocks();

    }



    ///Insert Block to Resource - Inserts new resource usage information into the planning horizon
    ///
    /// This method works in two stages: A) The block array is augmented as required to accomodate the new
    /// usage rates. B) The affected blocks are identified and the usage rates are applied.
    ///
    /// @param [in] start - Block start time
    /// @param [in] end - Block end time
    /// @param [in] coeffs - Block resource usage coefficients

    void insert_block(int load, double start, double end, double *coeffs) {

        //First pass split the resource horizon according to the input start/end dates

        //Step A: Do the necessary splitting using the start times
        for (int i = 0; i < (int) blocks.size(); i++) {
            res_block *block = blocks[i];
            if (block->time_start < start && block->time_end > start){
                split_block(start, i);
                break; //Only one block will have to be split (if any)
            }
        }

        //Step B: Do the necessary splitting using the end time
        //The extra loop is added to handle the case that the new block
        //should be split for the end time as well
        for (int i = 0; i < (int) blocks.size(); i++) {
            res_block *block = blocks[i];
            //Split based on the end time
            if (block->time_start < end && block->time_end > end){
                split_block(end, i);
                break; //Only one block will have to be split (if any)
            }
        }

        bool inserted_coeff_b = false;
        int acc_q = 0;
        int block_index = 0;

        //Step C: Add the extra coeffs on the affected blocks
        while(acc_q < load){
            res_block *block = blocks[block_index];

            //Skip all blocks before the start time
            if (block->time_start < start){
                block_index++;
                continue;
            }


            //Skip all blocks before the start time
            if (block->time_end > end)
                break;


            //Calculate extra production_quantity for the block
            int extra_prod_q = (int) my_round(coeffs[0] * block->duration());

            if (extra_prod_q + acc_q > load + 1){
                printf("Need to split the block\n");
                extra_prod_q = min(extra_prod_q, load - acc_q); //Calculate correct quantity

                //Find time to produce the quantity
                double time_for_load_production =  my_round(block->time_start + extra_prod_q / coeffs[0]);

                split_block(time_for_load_production, block_index);

            } else {
                extra_prod_q = min(extra_prod_q, load - acc_q);
            }

            //Add the coeffs to the affected blocks
            for (int i=0; i < 4; i++){
                switch(i){
                    case 1: {
                        if (!inserted_coeff_b){
                            block->prod_coeffs[i] += coeffs[i];
                            inserted_coeff_b = true;
                            acc_q += (int) coeffs[i]; //The constant will be added once on the very first block
                        }
                        break;
                    }
                    default:
                        block->prod_coeffs[i] += coeffs[i];
                }
            }


            //if (extra_prod_q >= 1 &&  extra_prod_q <= 10)
            //    printf("break");

            block->prod_q += extra_prod_q;
            acc_q += extra_prod_q;
            block_index++;
        }

        //Step D: Fix the start/end quantities for all blocks
        for (int i = 0; i < (int) blocks.size(); i++) {
            res_block *block = blocks[i];

            //Skip all blocks before the start time
            if (block->time_start < start)
                continue;

            if (i > 0){
                block->start_q = blocks[i-1]->end_q + (int) block->prod_coeffs[1];
            }
            else {
                block->start_q = (int) block->prod_coeffs[1];
            }

            block->end_q =  block->start_q + block->prod_q - block->cons_q;
        }

        //Validate structure
        validate_blocks();
        update_stats();
    }


    bool validate_blocks(){
        //Check quantity equililbrium for every block
        for (int i=0;i<blocks.size();i++){
            res_block *block = blocks[i];
            if (!block->validate())
                return false;
        }
        return true;
    }

    void update_stats(){
        prod_q = 0;
        cons_q = 0;

        for (int i=0;i<blocks.size();i++){
            res_block *block = blocks[i];

            if (i == 0)
                start_q = block->start_q;
            if (i == (int) blocks.size() - 1)
                end_q = block->end_q;

            prod_q += block->prod_q;
            cons_q += block->cons_q;
        }
    }

    //This function tries to consolidate adjacent blocks if their coefficients are the same
    void consolidate(){
        int block_number =  blocks.size();
        for (int i =0; i < block_number - 1;i++){
            res_block *block_a = blocks[i];
            res_block *block_b = blocks[i + 1];

            if (block_a->isSameType(block_b)){
                //Consolidate blocks
                block_a->end_q = block_b->end_q;
                block_a->time_end = block_b->time_end;

                //Remove block_b from the list;
                blocks.erase(blocks.begin() + i + 1);
                delete block_b;
                block_number--; //Fix the counter
            }

        }

    }

    /// This function tries to find the first feasible time period that can facilitate
    /// the required resource load
    ///
    ///
    /// \param start_load
    /// \param end_load
    /// \param start_time
    /// \param end_time
    void find_feasible_period(int start_load, int end_load, int &start_time, int &end_time){
        float st, et;
        int start_block_index;
        int block_index;
        bool st_ok = false;
        bool et_ok = false;
        int counter = 0;

        while (!st_ok && !et_ok) {
            st_ok = false;
            et_ok = false;


            if (counter > 50){
                printf("Failed to find a correct time period");
                assert(false);
            }

            //Step A find the time where the quantity of the resource is equal to the start_load
            st = getTimeToQuantity(start_load, 0.0, block_index, 0);
            if (st > 0){
                st_ok = true;
                start_block_index = block_index;
            } else {
                printf("Failed to find a correct time period");
                assert(false);
            }

            //Step B find the time where the quantity of th resource is equal to the end_load
            et = getTimeToQuantity(end_load, 0.0, block_index, start_block_index);

            if (et > 0) {
                et_ok = true;
                break;
            }

            counter++;
        }

        //Set final values
        start_time = st;
        end_time = et;
    }

    //Wrapper for getTime
    int getTimeToQuantity(int load, double min_start_time, int &block_index){
        return getTimeToQuantity(load, min_start_time, block_index, 0);
    }

    int getQuantityToTime(double start_time, double end_time){
        int acc_q = 0;
        for (int i=0; i < blocks.size(); i++){
            res_block *block = blocks[i];

            if (block->time_end <= start_time)
                continue;

            bool start_time_included = block->includesTime(start_time);
            bool end_time_included = block->includesTime(end_time);

            if (start_time_included && end_time_included){
                acc_q += block->getQuantity(end_time) - block->getQuantity(start_time);
            } else if (start_time_included){
                acc_q += block->end_q - block->getQuantity(start_time);
            } else if (end_time_included){
                acc_q += block->getQuantity(end_time) - block->start_q;
            } else
                acc_q += block->end_q - block->start_q; //The block time is bounded from the start/end times, we can include the entire block production

        }
        return acc_q;
    }

    //Finds first time that a specific load quantity of the resource is available
    double getTimeToQuantity(int load, double min_start_time, int &block_index, int start_block_index){
        double time = -1.0f;
        for (int i=start_block_index; i < blocks.size(); i++){
            res_block *block = blocks[i];

            //If the resource has end_q = 0 then there is no resource available for the load
            if (block->end_q == 0)
                continue;

            if (block->time_end < min_start_time)
                continue;

            if (block->start_q >= load){
                time = max(min_start_time, block->time_start);
                block_index = i;
                break;
            }

            if (block->includesQuantity(load)){
                block_index = i;
                time = block->getTimeToExistingQuantity(load);
                //At this point I'm sure that the min_start_time is within the block and the quantity is there
                time = max(time, min_start_time);
                break;
            }
        }
        return time;
    }

    // Get an end_time to accumulate a specific load
    void getTimeToAccQuantity(double start_time, int load, double &end_time){
        int acc_q = 0;

        for (int i=0;i<blocks.size();i++){
            res_block *block = blocks[i];

            if (!block->includesTime(start_time))
                continue;

            //Start accumulating quantities
            int temp_q = block->accumulateQuantity(start_time);



            if (acc_q + temp_q > load){
                //Figure out the exact time

            } else
                acc_q += temp_q;

        }

    }


    ///Exports Consumption data to a simple raw format for plotting
	void export_to_txt(char* outputfile) {

        auto *coutbuf  = std::cout.rdbuf();

        ofstream out(outputfile);
        cout.rdbuf(out.rdbuf()); //Redirect cout to the file
        cout << "Resource Max Level " << max_level << endl;
		for (size_t i = 0; i < blocks.size(); i++) {
			res_block *block = blocks[i];
			cout << *block << endl;
		}

		//Return output to console
        cout.rdbuf(coutbuf);
		cout << "Successfully exported Resource Consumption to File" << endl;

	}

	void resetMarks(){
        for (size_t i = 0; i < blocks.size(); i++) {
            blocks[i]->marked = false;
        }
    }

	void clear(){
        //Reset Consumptions/Productions
        for (size_t i = 0; i < blocks.size(); i++) {
            res_block *block = blocks[i];
            delete block;
        }
        blocks.clear();

        prod_q = 0;
        cons_q = 0;
        start_q = 0;
        end_q = 0;

        //Reset Accumulated horizon
        //memset(res_load_t, 0, sizeof(double) * planhorend);

        //Add the default block
        //Create main block
        res_block *mainblock = new res_block(0, planhorend, INF, ID);

        //Add Main block
        blocks.push_back(mainblock);
    }


	~resource() {
		//delete[] res_load_t;
		for (size_t i = 0; i < blocks.size(); i++) {
			res_block *block = blocks[i];
			delete block;
		}
		blocks.clear();

		vector<res_block*>().swap(blocks); //Free Memory of vector
	}
};

#endif // !RESOURCE_H