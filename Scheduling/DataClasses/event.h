//
// Created by gregkwaste on 9/21/19.
//

#ifndef SCHEDULING_EVENT_H
#define SCHEDULING_EVENT_H

#include <time.h>
#include <cstdio>
#include <string>

using namespace std;

//Generic Event Class
class event{
public:
    int ID;
    int target;
    int type;
    int arg1;
    string arg2;
    tm trigger_time;

    event(){
        trigger_time = tm(); //Init to zero
    };
    ~event(){};

    void report(){
        //Convert trigger time to string
        char buffer[40];
        strftime(buffer,40, "%Y-%m-%d %H:%M:%S", &trigger_time);
        std::printf("Event ID %d TypeID %d Trigger Time: %s TargetID %d Arg1 %d Arg2 %s\n",
                ID, type, buffer, target, arg1, arg2.c_str());
    }
};

#endif //SCHEDULING_EVENT_H
