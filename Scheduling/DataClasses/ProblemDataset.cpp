#include "ProblemDataset.h"

//Local Shit
std::stringstream query;
char *table_name;
MYSQL_ROW row;
int num_fields;

using namespace nlohmann;

void ProblemDataset::parseProductFamilies(database_connection& conn){
    printf("Parsing Product Families\n");

    query.str("");
    table_name = "product_family";
    query << "SELECT * FROM " << table_name;
    exec_query(conn, query.str().c_str());
    num_fields = mysql_num_fields(conn.result);
    //query << "SELECT * FROM " << table_name <<" ORDER BY processStepId ASC";
    //Iterate in data
    while((row = mysql_fetch_row(conn.result)))
    {
        //for (int i=0;i<num_fields;i++) printf("%s ", row[i]);
        //printf("\n");
        ProductFamily *pf = new ProductFamily();
        pf->productFamilyID = stoi(row[0]);
        pf->productFamilyName = row[1];
        pf->productFamilyDescription = row[2];
        pf->numFixtures_FT = stoi(row[3]);
        pf->numFixtures_CT = stoi(row[4]);
        pf->productFamilyFactorPCB = (row[8] != NULL) ? (stoi(row[8]) ==0) ? 1 : stoi(row[8]) : 1;
        pf->productFamilyBlackboxCount = (row[9] != NULL) ? stoi(row[9]) : BlackBoxSize;

        //Add production order to problem
        productFamilies.push_back(pf);
        productFamiliesNameMap[pf->productFamilyName] = pf;
        productFamiliesIDMap[pf->productFamilyID] = pf;
        //printf("Product %d - %s\n", p->productID, p->productName.c_str());
    }
    mysql_free_result(conn.result);
    printf("Loaded a total of %d product Families\n", productFamilies.size());

}

void ProblemDataset::parseProducts(database_connection& conn){
    printf("Parsing Products...\n");

    query.str("");
    table_name = "product";
    //query << "SELECT * FROM " << table_name <<" ORDER BY processStepId ASC";
    //query << "SELECT * FROM " << table_name << " WHERE dynUploadSetID = " << dynUploadSetID;
    query << "SELECT * FROM " << table_name; //Do not use dynuploadsetID
    exec_query(conn, query.str().c_str());
    num_fields = mysql_num_fields(conn.result);

    //Check number of rows
    if (mysql_num_rows(conn.result) == 0){
        printf("No entries for this particular dynUploadSetID");
        assert(false);
    }

    //Iterate in data
    while((row = mysql_fetch_row(conn.result)))
    {
        //for (int i=0;i<num_fields;i++) printf("%s ", row[i]);
        //printf("\n");
        Product *p = new Product();
        p->productID = stoi(row[0]);
        p->productName = row[1];
        p->productDescription = row[2];
        if (row[3] == NULL){
            printf("Warning NULL productFamilyID for product ID %d\n", p->productID);
            p->productFamilyID = -1;
        } else
            p->productFamilyID = stoi(row[3]);

        //Add production order to problem
        products.push_back(p);
        productsIDMap[p->productID] = p;
        productsNameMap[p->productName] = p;

        if (p->productFamilyID >0)
            printf("Product %d - %s - Family %d \n", p->productID, p->productName.c_str(), p->productFamilyID);
    }
    printf("Loaded a total of %d products\n", products.size());


    //Add default product
    Product *dp = new Product();
    dp->productID = products.size();
    dp->productName = "default";
    dp->productFamilyID = -1;
    products.push_back(dp);
    productsIDMap[dp->productID] = dp;
    productsNameMap[dp->productName] = dp;

    free_result(&conn);
}

void ProblemDataset::parseBOM(database_connection& conn){
    printf("Parse BOM\n");

    //Read BOM from local csv file
    //Try to parse constraints
    std::ifstream infile("BOM.csv");
    if (!infile) {
        printf("Input file not found\n");
        assert(false);
    }

    std::string line;

    int line_counter = 0;
    std::getline(infile, line);
    std::istringstream iss(line);
    std::vector<std::string> split;

    int entries;
    iss >> entries;

    for (int i = 0; i < entries; i++) {
        std::getline(infile, line);
        iss.clear();
        iss.str(line);
        split = getNextLineAndSplitIntoTokens(iss);

        string main_code = split[0];
        string req = split[1];
        int code_type = stoi(split[2]);

        if (code_type == -1){
            printf("Unknown BOM Entry Main Code %s Req %s Code Type %d \n",
                   main_code.c_str(), req.c_str(), code_type);
        } else{
            BOMEntry *e = new BOMEntry();
            e->requirement = req;
            e->code_type = code_type;
            //For safety reasons check if entry key already exists in dictionary

            if (BOM.find(main_code) != BOM.end()){
                printf("Warning Entry %s already exists\n", main_code.c_str());
            } else{
                //Finally save the entry to the BOM table
                BOM[main_code] = e;
            }
        }
    }

    //Report BOM
    for (auto e : BOM){
        printf("Main Code %s Req %s Code Type %d \n",
               e.first.c_str(), e.second->requirement.c_str(), e.second->code_type);
    }

}

void ProblemDataset::parseProductInventory(database_connection& conn){
    printf("Parsing Product Inventory...\n");

    query.str("");
    table_name = "product_inventory";
    //query << "SELECT * FROM " << table_name <<" ORDER BY processStepId ASC";
    query << "SELECT * FROM " << table_name << " WHERE dynUploadSetID = " << dynUploadSetID;
    exec_query(conn, query.str().c_str());
    num_fields = mysql_num_fields(conn.result);
    //Iterate in data
    while((row = mysql_fetch_row(conn.result)))
    {
        int product_id = stoi(row[2]);
        int product_quantity = stoi(row[1]);
        //Note that product quantity is measured in blackbox count

        Product *p = productsIDMap[product_id];

        if (productInventory.find(p->productName) != productInventory.end()){
            productInventory[p->productName] += product_quantity * 24;
        } else
            productInventory[p->productName] = product_quantity * 24;
    }
    //Report the product Inventory
    printf("Product Inventory\n");
    for (auto c : productInventory){
        printf("Product %s Quantity %d\n", c.first.c_str(), c.second);
    }

    mysql_free_result(conn.result);

}

void ProblemDataset::parseProcessingTimes(database_connection& conn){
    printf("Parsing Processing Times of Products...\n");

    table_name = "product_processing_times";
    for (auto pf: productFamilies) {
        //Load Rows for product family
        printf("Fetching ProcessingTimes. ProductFamily %s - ID: %d ... ",
               pf->productFamilyName.c_str(), pf->productFamilyID);

        //Make query
        query.str("");
        query << "SELECT * FROM " << table_name << " WHERE productFamilyID = " << pf->productFamilyID << " AND dynUploadSetID = " << dynUploadSetID;
        exec_query(conn, query.str().c_str());

        if (conn.result->row_count == 0) {
            printf("Missing ProcessingTimes.\n",
                   pf->productFamilyName.c_str(), pf->productFamilyID);
            continue;
        }

        //Otherwise try to work on data
        while((row = mysql_fetch_row(conn.result)))
        {
            //Step A: Load the Product
            int productID = stoi(row[10]);
            int machineTypeID = stoi(row[11]);
            Product *p = productsIDMap[productID];

            //Step B: Aggregated the processing Time
            ProductProcessingTime *pt = p->productProcessingTimes[machineTypeID];

            //TODO: The new data seems to completely ommit backprocessing data
            //thus I am only saving the data for the forward processing now.
            pt->ProcessingTime[0] += (int) my_round(stof(row[2]));
            pt->AggregatedProcessingTimes++;
            pt->ParallelProcessed = min(pt->ParallelProcessed, row[7] == NULL ? 0 : stoi(row[7]));
            pt->SideOfPCB[0] = 1; //Force SideOfPCB for forward processing
            break; //Make sure to load just one row of data
        }
        printf("...Done\n");

        mysql_free_result(conn.result);
    }

    //Post Process Products and Apply Aggregation Counters
    for (auto p : products){
        for (int i=0;i<MACHINETYPECOUNT;i++){
            ProductProcessingTime *pt = p->productProcessingTimes[i];
            //Apply the aggregation only on the forward processing time
            if (pt->AggregatedProcessingTimes > 0){
                for (int j=0;j<SIDE_COUNT;j++){
                    float new_pt = (float) (pt->ProcessingTime[j]) / pt->AggregatedProcessingTimes;
                    pt->ProcessingTime[j] = (int) new_pt;
                }
            }
        }
    }

    //Add default product and processing time
    Product *dp = productsNameMap["default"];
    ProductProcessingTime *dt = new ProductProcessingTime();
    for (int i=0;i<MACHINETYPECOUNT;i++){
        dp->productProcessingTimes[i] = dt;
    }

    printf("Done.\n");

}

void ProblemDataset::parseLineTypes(database_connection& conn){
    printf("Parse Line Types...\n");

    query.str("");
    table_name = "production_line_types";
    query << "SELECT * FROM " << table_name << " ORDER BY ProductionLineTypeID";

    exec_query(conn, query.str().c_str());
    num_fields = mysql_num_fields(conn.result);
    //Iterate in data
    while((row = mysql_fetch_row(conn.result)))
    {
        //for (int i=0;i<num_fields;i++) printf("%s ", row[i]);
        //printf("\n");
        ProductionLineType *lt = new ProductionLineType();
        int dbproductionLineTypeID = stoi(row[0]);
        switch (dbproductionLineTypeID){
            case 1:
                lt->productionLineTypeID = exlTHT;
                break;
            case 2:
                lt->productionLineTypeID = exlMANUAL;
                break;
            case 3:
                lt->productionLineTypeID = exlTESTING;
                break;
            case 4:
                lt->productionLineTypeID = exlSMT;
                break;
        }
        lt->productionLineType = row[1];

        productionLineTypes[dbproductionLineTypeID] = lt;
    }
    mysql_free_result(conn.result);
}

void ProblemDataset::generateProcessSteps(database_connection& conn){
    printf("Generate Process Steps\n");

    for (int i=0; i< lLINETYPECOUNT; i++) {
        //Generate Workstation
        process_step *mw = new process_step();
        mw->ID = i;
        mw->processID = i;
        processSteps.push_back(mw);
    }
    NumberOfProcSteps = (int) processSteps.size();
}

void ProblemDataset::parseLines(database_connection& conn){
    printf("Parsing Lines...\n");

    query.str("");
    table_name = "production_lines";
    query << "SELECT * FROM " << table_name << " ORDER BY ProductionLineID";
    exec_query(conn, query.str().c_str());

    num_fields = mysql_num_fields(conn.result);

    //Iterate in data
    int machine_counter = 0;
    while((row = mysql_fetch_row(conn.result)))
    {
        //Skipping specific non existent lines
        string line_name = row[1];

        if (line_name == "AA6" || line_name == "AD6" || line_name == "AD7"){
            ignoredLines.push_back(stoi(row[0])); //Save Line ID
            continue;
        }

        //for (int i=0;i<num_fields;i++) printf("%s ", row[i]);
        //printf("\n");
        ProductionLine *pl = new ProductionLine();
        pl->ID = stoi(row[0]);
        pl->Name = line_name;
        pl->type = productionLineTypes[stoi(row[4])];

        //For each line generate one machine
        machine *mc = new machine();
        mc->ID = machine_counter;
        mc->actualID = pl->ID;
        mc->process_stepID = pl->type->productionLineTypeID;
        mc->line = pl;

        printf("Line %s ID %d LineTypeID %d - Process Step ID %d\n",
               pl->Name.c_str(), pl->ID, pl->type->productionLineTypeID, mc->process_stepID);
        //Set machine's dummy operation IDs to match the machineID
        mc->operations[0]->ID = machine_counter;
        mc->operations[1]->ID = machine_counter;

        //Add machine to the corresponding Workstation
        process_step *mw = processSteps[mc->process_stepID];

        mw->machines.push_back(mc);
        machines.push_back(mc);//Add machine to the general vector

        machine_counter++;

        productionLines.push_back(pl);
        productionLineIDMap[pl->ID] = pl;
    }
    mysql_free_result(conn.result);

    NumberOfMachines = machine_counter;
}


void ProblemDataset::populateWorkstationsAndLines(database_connection& conn){
    printf("Populate Workstations and Lines\n");

    //Iterate on machines
    for (int i=0;i< machines.size();i++){
        //Load Machine
        machine *mc = machines[i];

        //Iterate on machine types
        for (int k=1; k<MACHINETYPECOUNT + 1; k++) {

            ProductionLine *pl = mc->line;

            //Query machine table
            query.str("");
            table_name = "machines";
            query << "SELECT * FROM " << table_name << " WHERE productionLineID = "
                  << pl->ID << " AND machineTypeID = " << k;
            exec_query(conn, query.str().c_str());

            num_fields = mysql_num_fields(conn.result);
            while ((row = mysql_fetch_row(conn.result))) {
                printf("\t");
                for (int i = 0; i < num_fields; i++) printf("%s ", row[i]);
                printf("\n");


                int mc_typeID;

                switch (k){
                    case mSMT:
                        mc_typeID = exmSMT;
                        mc->parallel_processors[mc_typeID] = 1;
                        break;
                    case mFLUX:
                        mc_typeID = exmFLUX;
                        mc->parallel_processors[mc_typeID] = 1;
                        break;
                    case mPOTA_SOLTEC:
                        mc_typeID = exmPOTA_SOLTEC;
                        mc->parallel_processors[mc_typeID] = 1;
                        break;
                    case mMASKING:
                        mc_typeID = exmMASKING;
                        mc->parallel_processors[mc_typeID] = 1;
                        break;
                    case mTHT:
                        mc_typeID = exmTHT;
                        mc->parallel_processors[mc_typeID] = 1;
                        break;
                    case mSAKI:
                        mc_typeID = exmSAKI;
                        mc->parallel_processors[mc_typeID] = 1;
                        break;
                    case mARVISION:
                        mc_typeID = exmARVISION;
                        mc->parallel_processors[mc_typeID] = 1;
                        break;
                    case mMANUAL_INSERTION:
                        mc_typeID = exmMANUAL_INSERTION;
                        mc->parallel_processors[mc_typeID] = 1;
                        break;
                    case mICT:
                        mc_typeID = exmICT;
                        mc->parallel_processors[mc_typeID] = (int) conn.result->row_count;
                        break;
                    case mFT:
                        mc_typeID = exmFT;
                        mc->parallel_processors[mc_typeID] = (int) conn.result->row_count;
                        break;
                    case mREPAIR:
                        mc_typeID = exmREPAIR;
                        mc->parallel_processors[mc_typeID] = 1;
                        break;
                    case mPACKAGING:
                        mc_typeID = exmPACKAGING;
                        mc->parallel_processors[mc_typeID] = 1;
                        break;
                    default:
                        printf("Forgot a machine type\n");
                }

                printf("Machine %d - Machine Type ID %d -  Parallel Processors %d\n",
                       mc->ID, mc_typeID,  mc->parallel_processors[mc_typeID]);

                break; //In any case add only one machine per line
            }
            mysql_free_result(conn.result);

        }
    }

    //Fetch all machines again and assign the actual machine IDs to the generated line-machines
    //Query machine table
    query.str("");
    table_name = "machines";
    query << "SELECT * FROM " << table_name;
    exec_query(conn, query.str().c_str());

    num_fields = mysql_num_fields(conn.result);
    while ((row = mysql_fetch_row(conn.result))) {
        int machineID = stoi(row[0]);
        int machinelineID = stoi(row[5]);

        for (int i=0;i<machines.size();i++){
            machine *mc = machines[i];

            if (mc->line->ID != machinelineID)
                continue;

            mc->embeddedMachineIDs.push_back(machineID);
        }
    }


    //Merge machines for manual and test lines into the manual
    for (int i=0;i<machines.size();i++){
        machine *mc_manual = machines[i];
        if (mc_manual->line->type->productionLineTypeID != exlMANUAL)
            continue;

        string target_name = mc_manual->line->Name;
        std::replace(target_name.begin(), target_name.end(), 'D', 'A');
        printf("Looking for Line %s\n", target_name.c_str());

        //Find corresponding testing line
        for (int j=0; j<machines.size();j++){
            machine *mc_test = machines[j];
            if (mc_test->line->type->productionLineTypeID != exlTESTING)
                continue;
            //Check the name
            if (mc_test->line->Name != target_name)
                continue;

            //Merge the machine information
            for (int k=0;k<MACHINETYPECOUNT + 1;k++){
                mc_manual->parallel_processors[k] = max(mc_manual->parallel_processors[k],
                                                        mc_test->parallel_processors[k]);
            }

            //Merge embedded machineIDs
            for (int k=0; k<mc_test->embeddedMachineIDs.size(); k++){
                mc_manual->embeddedMachineIDs.push_back(mc_test->embeddedMachineIDs[k]);
            }
        }

    }


    //TODO: Maybe I should delete the machines generated for testing
    //For now I think if I just let them empty it won't hurt anything

    //Report Process Steps
    printf("\tProcesses \n");
    for (int i = 0; i < processSteps.size(); i++){
        process_step *mw = processSteps[i];
        printf("Process %d\n", mw->processID);
        for (int j=0;j<mw->machines.size();j++){
            machine *mc = mw->machines[j];
            printf("\tMachine %d - RealID %d - Line (%s, %d) - Process %d\n",
                   mc->ID, mc->actualID, mc->line->Name.c_str(), mc->line->ID, mc->process_stepID);
            printf("\t\t Parallel Processors: ");
            for (int k=1; k<MACHINETYPECOUNT + 1; k++)
                printf("%d ", mc->parallel_processors[k]);
            printf("\n");
            printf("\t\tEmbedded Machine IDs: ");
            for (int k=1; k<mc->embeddedMachineIDs.size(); k++)
                printf("%d ", mc->embeddedMachineIDs[k]);
            printf("\n");
        }
    }

}

void ProblemDataset::parseOrders(database_connection& conn){
    printf("Parsing Orders\n");

    //Lastly parse orders and try to generate jobs/operations
    if (executionMode == 1) {
        //Parsing Existing Schedule from database
        query.str("");
        table_name = "production_orders";
        query << "SELECT * FROM " << table_name << " WHERE dynUploadSetID = " << dynUploadSetID << " ORDER BY seqID";
        exec_query(conn, query.str().c_str());

        num_fields = mysql_num_fields(conn.result);

        //Check number of rows
        if (mysql_num_rows(conn.result) == 0){
            printf("No entries for this particular dynUploadSetID");
            assert(false);
        }

        //Iterate in data
        while((row = mysql_fetch_row(conn.result)))
        {
            //Load release date first
            struct tm relDate, dueDate;
            std::string po_releaseDate = row[5];
            strptime(po_releaseDate.c_str(), "%Y-%m-%d %H:%M:%S", &relDate);

            //Check if order is within the planning horizon
            //With freezing neglected parse everything
            //if (difftime(timegm(&relDate), timegm(&PlanningHorizonStart_Minus)) < 0.0)
            //    continue;


            //do not filter orders base on the plannin horizon start
            //we assume that the first orders will
            if (difftime(timegm(&relDate), timegm(&PlanningHorizonStart)) < 0.0)
                continue;

            if (difftime(timegm(&PlanningHorizonEnd), timegm(&relDate)) < 0.0)
                continue;

            //Skip order if the remaining quantity is 0
            if (stoi(row[3]) == 0)
                continue;

            //Skip order that are scheduled in the ignored lines
            int temp_line_id = (row[8] != NULL) ? stoi(row[8]) : -1;
            if (find(ignoredLines.begin(), ignoredLines.end(), temp_line_id) != ignoredLines.end())
                continue;

            ProductionOrder *po = new ProductionOrder();
            po->orderID = stoi(row[0]);
            po->orderName = row[1];
            po->quantityTotal = stoi(row[2]);
            po->quantityRemaining = stoi(row[3]);
            po->urgent = (bool) stoi(row[4]);

            //Testing - Try to parse the date
            strptime(row[5], "%Y-%m-%d %H:%M", &relDate);
            strptime(row[6], "%Y-%m-%d %H:%M", &dueDate);

            po->releaseDate = relDate;
            po->releaseTime = difftime(timegm(&relDate), timegm(&PlanningHorizonStart));
            po->dueDate = dueDate;
            po->dueTime = difftime(timegm(&dueDate), timegm(&PlanningHorizonStart));

            //po->releaseDate = (row[5]!= NULL) ? stoi(row[5]) : -1;
            //po->dueDate = (row[6]!= NULL) ? stoi(row[6]) : -1;
            po->productID = (row[7]!= NULL) ? stoi(row[7]) : -1;
            po->productName = productsIDMap[po->productID]->productName;
            po->productionLine = (row[8] != NULL) ? stoi(row[8]) : -1;

            printf("Storing Order %s - Order ID %d - Product Name %s - Production Line ID %d - Remaining Quantity %d\n",
                   po->orderName.c_str(), po->orderID, po->productName.c_str(), po->productionLine, po->quantityRemaining);

            orders.push_back(po);
            orderIDMap[po->orderID] = po;
        }


        if (orders.size()==0){
            printf("Could not find orders within the planning horizon \n");
            printf("Nothing to do \n");
            assert(false);
        }

        mysql_free_result(conn.result);

    } else if (executionMode == 3) {
        //Simulate Database orders because ATC ARE FUCKING RETARDS
        ProductionOrder *po;

        po = new ProductionOrder();
        po->orderID = 1;
        po->orderName = "Order_AS1_0001";
        po->quantityTotal = 0;
        po->quantityRemaining = 10000;
        po->urgent = false;
        //po->releaseDate = timegm();
        //po->dueDate = tm();
        po->productID = 1;
        po->productionLine = 1;

        orders.push_back(po);
        orderIDMap[po->orderID] = po;

        po = new ProductionOrder();
        po->orderID = 2;
        po->orderName = "Order_AS1_0002";
        po->quantityTotal = 0;
        po->quantityRemaining = 300;
        po->urgent = false;
        //po->releaseDate = -1;
        //po->dueDate = -1;
        po->productID = 21;
        po->productionLine = 1;

        orders.push_back(po);
        orderIDMap[po->orderID] = po;


        po = new ProductionOrder();
        po->orderID = 3;
        po->orderName = "Order_AS2_0001";
        po->quantityTotal = 0;
        po->quantityRemaining = 2360;
        po->urgent = false;
        //po->releaseDate = -1;
        //po->dueDate = -1;
        po->productID = 77;
        po->productionLine = 2;

        orders.push_back(po);
        orderIDMap[po->orderID] = po;

        po = new ProductionOrder();
        po->orderID = 27;
        po->orderName = "Order_AD1_0008";
        po->quantityTotal = 0;
        po->quantityRemaining = 3000;
        po->urgent = false;
        //po->releaseDate = -1;
        //po->dueDate = -1;
        po->productID = 76;
        po->productionLine = 10;

        orders.push_back(po);
        orderIDMap[po->orderID] = po;

        po = new ProductionOrder();
        po->orderID = 99;
        po->orderName = "Order_AA1_0006";
        po->quantityTotal = 0;
        po->quantityRemaining = 3000;
        po->urgent = false;
        //po->releaseDate = -1;
        //po->dueDate = -1;
        po->productID = 76;
        po->productionLine = 16;

        orders.push_back(po);
        orderIDMap[po->orderID] = po;

    }

}

void ProblemDataset::parseJobParameters(database_connection& conn, int jobID){
    //Init affected parameters
    dynUploadSetID = -1;

    printf("Parsing Job Parameters from Database\n");


    //Parsing Existing Schedule from database
    query.str("");
    table_name = "job";
    query << "SELECT * FROM " << table_name << " WHERE jobID = " << jobID;
    exec_query(conn, query.str().c_str());

    num_fields = mysql_num_fields(conn.result);
    //Iterate in data
    while((row = mysql_fetch_row(conn.result)))
    {
        dynUploadSetID = stoi(row[4]);
    }
    mysql_free_result(conn.result);

    //Force dynUploadSetID
    //dynUploadSetID = 1903;

    if (dynUploadSetID < 0) {
        printf("Failed to Fetch DynUploadSetID from DISRUPT DB\n");
        assert(false);
    } else
        printf("Using DynUploadSetID %d\n", dynUploadSetID);

}


void ProblemDataset::parseEvents(database_connection& conn){
    //Parse Events
    printf("Parsing Events from Database\n");

    //Normally I need to load events from the database and load them here
    //For now I'll just impose a machine anavailability event for the whole line

    for (int i=0;i<eventCount;i++){
        int event_id = eventIDs[i];

        //Fetch event from database
        query.str("");
        table_name = "event";
        query << "SELECT * FROM " << table_name << " WHERE companyID = 1 AND eventID = " << event_id;
        exec_query(conn, query.str().c_str());
        num_fields = mysql_num_fields(conn.result);

        row = mysql_fetch_row(conn.result);

        if (row == nullptr)
            continue;

        //Parse event
        int eventTypeID = stoi(row[3]);
        int eventTargetID = stoi(row[6]);


        //Malfunction event
        if (stoi(row[3]) == 1){
            //Load

            json descr = json::parse(row[8]); //Parse Event Parameters
            string starttime_text = descr.at("/startTime"_json_pointer);
            string breakdownTime_text = descr.at("/estimatedBreakdown"_json_pointer);
            int breakdownTime = stoi(breakdownTime_text) * 60 * 60; //Convert to seconds


            //Fix weird date format
            std::replace(starttime_text.begin(), starttime_text.end(), 'T', ' ');
            starttime_text.erase(std::remove(starttime_text.begin(), starttime_text.end(), 'Z'),
                    starttime_text.end());

            //Report Converted time
            printf("Converted Event Start Time %s\n",
                    starttime_text.c_str());

            //Save Event
            event *e = new event();
            e->ID = event_id;
            e->target = eventTargetID;
            e->type = eventTypeID;
            e->arg1 = breakdownTime;

            //Save Trigger time directly to the event
            strptime(starttime_text.c_str(), "%Y-%m-%d %H:%M", &e->trigger_time);

            //Save event
            events.push_back(e);

        } else
            printf("Warning Not Supported Event. Contact Developer!\n");

        free_result(&conn);
    }


    //Add a custom event
    //Save Event
    event *e = new event();
    e->ID = 9999;
    e->target = 53;
    e->type = 1;
    e->arg1 = 3600;

    //Save Trigger time directly to the event
    strptime("2019-10-25 01:30", "%Y-%m-%d %H:%M", &e->trigger_time);

    //Save event
    events.push_back(e);

}
