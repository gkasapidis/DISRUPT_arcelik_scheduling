#include <string>
#include <unordered_map>

#ifndef SCHEDULING_PRODUCTFAMILY_H
#define SCHEDULING_PRODUCTFAMILY_H


class ProductFamily{
public:
    int productFamilyID;
    string productFamilyName;
    string productFamilyDescription;
    int numFixtures_FT;
    int numFixtures_CT;
    int productFamilyFactorPCB;
    int productFamilyBlackboxCount;

    void report(){
        //Report Product Family
        printf("Product Family %s - ID: %d \n", productFamilyName.c_str(), productFamilyID);

    }

    //Deconstructor
    ~ProductFamily(){

    }
};



#endif //SCHEDULING_PRODUCTFAMILY_H
