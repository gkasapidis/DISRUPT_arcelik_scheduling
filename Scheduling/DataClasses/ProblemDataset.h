


#ifndef SCHEDULING_PROBLEMDATASET_H
#define SCHEDULING_PROBLEMDATASET_H


#include <string>


//Include custom classes
#include "ProductionOrder.h"
#include "Product.h"
#include "ProductFamily.h"
#include "ProductionLine.h"
#include "../OperationClass.h"
#include "../JobClass.h"
#include "../MachineClass.h"
#include "../ProcessStep.h"
#include "../json.hpp"
#include "event.h"

//Include utilities
//TODO: Importing stuff from a parent folder is BS
#include "../db_utils.h"
#include "../csv_utils.h"

using namespace std;


//Static BOM dictionary until it is saved in the DB
class BOMEntry{
public:
    string requirement;
    int code_type;

    BOMEntry(){
        requirement = "";
        code_type = -1;
    }
};



class ProblemDataset {
public:
    //Dataset Contents
    vector<ProductionOrder*> orders;
    unordered_map<int, ProductionOrder*> orderIDMap;
    vector<Product*> products;
    unordered_map<string, BOMEntry*> BOM;
    vector<ProductFamily*> productFamilies;
    unordered_map<int, Product*> productsIDMap;
    unordered_map<string, Product*> productsNameMap;
    unordered_map<string, int> productInventory;
    vector<ProductionLine*> productionLines;
    vector<int> ignoredLines; //Used to keep track of the ignored production lines
    unordered_map<int, ProductionLine*> productionLineIDMap;
    unordered_map<int, ProductionLineType*> productionLineTypes;
    unordered_map<int, ProductFamily*> productFamiliesIDMap;
    unordered_map<string, ProductFamily*> productFamiliesNameMap;
    vector<process_step*> processSteps;
    vector<machine*> machines;
    vector<event*> events;
    int dynUploadSetID;

    //Default Constructor
    ProblemDataset(){
        orders = vector<ProductionOrder*>();
        orderIDMap = unordered_map<int, ProductionOrder*>();
        products = vector<Product*>();
        BOM = unordered_map<string, BOMEntry*>();
        productFamilies = vector<ProductFamily*>();
        productsIDMap = unordered_map<int, Product*>();
        productsNameMap = unordered_map<string, Product*>();
        productInventory = unordered_map<string, int>();
        productionLines = vector<ProductionLine*>();
        ignoredLines = vector<int>();
        productionLineIDMap = unordered_map<int, ProductionLine*>();
        productionLineTypes = unordered_map<int, ProductionLineType*>();
        productFamiliesIDMap = unordered_map<int, ProductFamily*>();
        productFamiliesNameMap = unordered_map<string, ProductFamily*>();
        processSteps = vector<process_step*>();
        machines = vector<machine*>();
        dynUploadSetID = 0;
    }

    //Default Deconstructor
    ~ProblemDataset(){
        //Delete Production Orders;
        for (int i=0; i < orders.size(); i++)
            delete orders[i];
        orders.clear();
        orderIDMap.clear();

        //Delete Products;
        for (int i=0; i < products.size(); i++)
            delete products[i];
        products.clear();

        //Delete Events
        for (int i=0;i<events.size();i++)
            delete events[i];
        events.clear();

        //Clear BOM;
        for (auto e : BOM)
            delete e.second;
        BOM.clear();

        //Delete ProductFamilies;
        for (int i=0; i < productFamilies.size(); i++)
            delete productFamilies[i];
        productFamilies.clear();

        //Delete ProductionLines
        productsIDMap.clear();
        productsNameMap.clear();
        productInventory.clear();

        for (int i=0; i < productionLines.size(); i++)
            delete productionLines[i];
        productionLines.clear();
        productionLineIDMap.clear();

        //Delete ProductionLineTypes
        for (auto e : productionLineTypes)
            delete e.second;
        productionLineTypes.clear();

        //Delete ProductFamiliesIDMap
        productFamiliesIDMap.clear();

        //Delete ProductFamiliesIDMap
        productFamiliesNameMap.clear();
    }


    void parseProductFamilies(database_connection& db_conn);
    void parseProducts(database_connection& db_conn);
    void parseBOM(database_connection& db_conn);
    void parseProductInventory(database_connection& db_conn);
    void parseProcessingTimes(database_connection& db_conn);
    void parseLineTypes(database_connection& db_conn);
    void generateProcessSteps(database_connection& db_conn);
    void parseLines(database_connection& db_conn);
    void populateWorkstationsAndLines(database_connection& db_conn);
    void parseOrders(database_connection& db_conn);
    void parseEvents(database_connection& db_conn);
    void parseJobParameters(database_connection& db_conn, int jobID);

    //Database Parser
    int parse_data(database_connection& arcelik_db_conn, database_connection& disrupt_db_conn, int jobID, int eventCount, int *eventIDs){

        parseJobParameters(disrupt_db_conn, jobID);

        parseProductFamilies(arcelik_db_conn);
        parseProducts(arcelik_db_conn);
        parseBOM(arcelik_db_conn);
        parseProductInventory(arcelik_db_conn);
        parseProcessingTimes(arcelik_db_conn);

        //LOAD PLANT FLOOR
        printf("\nPARSING PLANT FLOOT DATA\n");

        parseLineTypes(arcelik_db_conn);
        generateProcessSteps(arcelik_db_conn);
        parseLines(arcelik_db_conn);
        populateWorkstationsAndLines(arcelik_db_conn);

        //Parse Orders
        parseOrders(arcelik_db_conn);

        //Connect to DISRUPT DB
        parseEvents(disrupt_db_conn);


        return 0;
    }

};


#endif //SCHEDULING_PROBLEMDATASET_H
