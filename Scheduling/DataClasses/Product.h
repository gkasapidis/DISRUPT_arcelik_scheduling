//
// Created by gregkwaste on 4/8/18.
//

#ifndef SCHEDULING_PRODUCT_H
#define SCHEDULING_PRODUCT_H

#include "../Parameters.h"

using namespace std;

class ProductProcessingTime {
public:
    int MachineTypeID;
    int SideOfPCB[3];
    int ProcessingTime[3];
    int ParallelProcessed;
    int AggregatedProcessingTimes;

    ProductProcessingTime(){
        MachineTypeID  = -1;
        SideOfPCB[0] = 1;
        SideOfPCB[1] = 1;
        SideOfPCB[2] = 1;
        //Set default processing times to 2.5 (3) seconds
        ProcessingTime[0] = 3;
        ProcessingTime[1] = 3;
        ProcessingTime[2] = 3;
        ParallelProcessed = 1;
        AggregatedProcessingTimes = 0;
    }

    ~ProductProcessingTime(){}
};


class Product{
public:
    int productID;
    std::string productName;
    std::string productDescription;
    int proFamTime;
    int productFamilyID;
    unordered_map<int, ProductProcessingTime*> productProcessingTimes;

    Product(){
        productProcessingTimes = unordered_map<int, ProductProcessingTime*>();

        //Init ProcessingTimes for product
        for (int i=0; i< MACHINETYPECOUNT;i++){
            ProductProcessingTime *pt = new ProductProcessingTime();
            pt->MachineTypeID = i;
            productProcessingTimes[i] = pt;
        }
    }

    void report(){
        //Report Product Family
        printf("Product %s - ID: %d \n", productName.c_str(), productID);
        printf("ProcessingTimes\n");
        for (int j=1; j<=MACHINETYPECOUNT; j++){
            ProductProcessingTime *pt = productProcessingTimes[j];
            if (pt == nullptr) continue;
            for (int k=0;k<3;k++){
                if( pt->SideOfPCB[k] <= 0) continue;
                printf("\tMachineType %d ExtraType %d ProcessingTime %d \n", j, pt->SideOfPCB[k], pt->ProcessingTime[k]);
            }
        }

    }


    ~Product(){
        productProcessingTimes.clear();
    }
};


#endif //SCHEDULING_PRODUCT_H
