#ifndef JOBCLASS_H
#define JOBCLASS_H

class job :public CObject
{
public:
	int actualID;
	int orderID;
	int ID;
	bool done;
	int quantity;
	int lotSize;
	float lotFactor; //Used only when predecessors/successors are present (it won't be the case in the new way)
	long rel;
	long due;
	float weight;
	int saID;

	//CObArray *operations;
	vector<operation*> operations;
	job();
	~job();
	job(const job &copyin);
	job &operator=(const job &rhs);
	bool operator==(const job &rhs);
	bool operator!=(const job &rhs);
	friend ostream& operator<<(ostream &output, const job &jb);

	//Time fixing
	void fixTimes();
};

#endif