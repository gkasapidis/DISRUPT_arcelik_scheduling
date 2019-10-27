#include "ListObject.h"
#include "MachineClass.h"
#include "ProcessStep.h"
#include "OperationClass.h"

template <class T>
listObject<T>::listObject() {
	//cout << "Creating ListObject" << endl;
};

template <class T>
listObject<T>& listObject<T>::operator=(const listObject<T>& rhs){
	cout << "ListObject assignment operator" << endl;
	//T::operator=(rhs);
	return *this;
};

template <class T>
listObject<T>::~listObject<T>() {
	//cout << "ListObject deconstructor" << endl;
	//Clear all connections
	suc.clear();
	pred.clear();
	vector<T*>().swap(suc);
	vector<T*>().swap(pred);
};


//Usefull instantiations
template class listObject<machine>;
template class listObject<process_step>;