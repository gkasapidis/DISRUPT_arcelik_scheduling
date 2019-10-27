#include "Parameters.h"

#ifndef LISTOBJ_H
#define LISTOBJ_H

using namespace std;

template <class T>
class listObject
{
public:
	//CObArray *mach_avail;
	vector<T*> suc;
	vector<T*> pred;

	listObject<T>();

	~listObject();
	listObject &operator=(const listObject &rhs);
};

#endif