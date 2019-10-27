#include <string>
#include <vector>
#include "../MachineClass.h"
#include "../MachineUnav.h"

#ifndef SCHEDULING_PRODUCTIONLINE_H
#define SCHEDULING_PRODUCTIONLINE_H

//enum LineTypes{lSMT=1, lTHT, lMANUAL, lTESTING, lLINETYPECOUNT};
enum LineTypes{lTHT=0, lMANUAL, lTESTING, lSMT, lLINETYPECOUNT};
enum exLineTypes{exlSMT=0, exlTHT, exlMANUAL, exlTESTING, exlLINETYPECOUNT};

class ProductionLineType{
public:
    int productionLineTypeID;
    string productionLineType;
};

class ProductionLine{
public:
    int ID;
    string Name;
    ProductionLineType* type;
    vector<mach_unav*> unavailabilities;

    ProductionLine(){
        ID = -1;
        Name = "";
        type = nullptr;
        unavailabilities = vector<mach_unav*>();
        //machines = vector<machine*>();
    }

    void applyUnavWindow(mach_unav *w){
        unavailabilities.push_back(w);
    }

    ~ProductionLine(){
        for (int i=0;i<unavailabilities.size();i++){
                delete unavailabilities[i];
        }
    }

};

#endif //SCHEDULING_PRODUCTIONLINE_H
