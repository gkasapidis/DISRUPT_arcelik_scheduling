//
// Created by gregkwaste on 4/8/18.
//
#include <string>
#include <ctime>

#ifndef SCHEDULING_PRODUCTIONORDER_H
#define SCHEDULING_PRODUCTIONORDER_H
class ProductionOrder {
public:
    int orderID;
    std::string orderName;
    int quantityTotal;
    int quantityRemaining;
    bool urgent;
    struct tm releaseDate; //Release date
    double releaseTime; //Release date in seconds from the start of the planning horizon
    struct tm dueDate;
    double dueTime; //Release date in seconds from the start of the planning horizon
    int productID;
    std::string productName;
    int productionLine; //Used to store the assigned production Line in the base schedule

    void report() {
        printf("Order %d ProductID %d QuantityTotal %d QuantityRemaining %d\n",
        orderID, productID, quantityTotal, quantityRemaining);
    }
};
#endif //SCHEDULING_PRODUCTIONORDER_H
