#include "CObject.h"
#include "Parameters.h"

using namespace std;




#ifndef GRAPH_H
#define GRAPH_H

enum graph_state { FORWARD, BACKWARD, STATE_NUM };

/*
	Directional Graph Class applied specifically on the job scheduling case
	and matches the specific data structures of the problem

*/

/*
Graph Node Connection Class
*/




/*
Graph Node Class
*/
template<class NODE_CLASS> class Connection;
template<class NODE_CLASS> class Graph;

class Node
{
public:
	Node(); //Constructor
	int nodeID;
	bool valid;
	bool critical;

	Node &operator=(const Node &rhs);
	bool operator==(const Node &rhs);
	
	bool isConnected(Node *node);
	bool isEqual(Node *node);
	bool connect(Node *node, bool temp, int type);
	bool disconnect(Node *node);
	void removeConnection(int i);
	void visit(Graph<Node> *gr, int state, bool *stack, bool &cycleflag, Node** visitedNodes, int &visitedNodes_n);
	
	//Search Properties
	bool visited = false;
	
	//Temp registers
	int tConnectionsID = -1;
	int tIndex = -1;

	~Node();
};

template<class NODE_CLASS> class Connection
{
public:
	Connection<NODE_CLASS>(NODE_CLASS* node, int type);
	Connection<NODE_CLASS>();
	~Connection() {};
	bool operator==(const Connection &rhs);
	NODE_CLASS* node;
	int type;
};

template<class NODE_CLASS> struct adj_list_elem {
	int p_connections[MAX_JOB_RELATIONS];
	int t_connection;
	NODE_CLASS* op;
};

template <class NODE_CLASS> class Graph
{
public:
	//Constructor is initializing the 2d list of the Nodes property
	Graph<NODE_CLASS>(int n);
	~Graph();
	//Edge Set
	//unordered_set<int3_tuple> edges;
	//Adjacency List
	adj_list_elem<NODE_CLASS> *adj_list;
	
	Graph<NODE_CLASS> &operator=(const Graph<NODE_CLASS> &rhs);
	bool operator==(const Graph<NODE_CLASS> &rhs);
	bool operator!=(const Graph<NODE_CLASS> &rhs);

    vector<NODE_CLASS*> criticalPath;

	int state; //Set backward or forward mode
	int size;
	int max_size;
	int cInd;
	bool cycleflag;
	void addNode(NODE_CLASS *node);
	void removeNode(NODE_CLASS *node);
	NODE_CLASS* findNode(int UID);
	//void insertNode(NODE_CLASS *node); //Not needed
	int getConnectionNum();

	void dfs(int node);
	void topological_sort();
    void topological_sort(int *visiting_order);
	
	void visit(int node);
	void t_sort(int node);
	void visitNode(NODE_CLASS *node, bool *stack);
	void report();
    void report_topological_sort();
	void resetVisited();


    //NODECLASS function pointer for stuff
    void (NODE_CLASS::*fptr)() = NULL;

	//Checking Methods
	bool checkNodes();

	//Temp Storage
	int visitedNodes_n;
	NODE_CLASS** visitedNodes;
	bool *stack_list;
	bool *visited;
	deque<NODE_CLASS*> top_sort;
	int *top_sort_index;
};


/*
	--TEMPLATE---
	---METHODS---
	----ONLY-----
*/


//Connection Class Methods
template<class NODE_CLASS>Connection<NODE_CLASS>::Connection() {
	this->node = NULL;
	this->type = 0;
}

template<class NODE_CLASS>Connection<NODE_CLASS>::Connection(NODE_CLASS* node, int type) {
	this->node = node;
	//Type of 0 is a permanent connection
	//Type of 1 is a temporary connectino
	this->type = type;
}

template<class NODE_CLASS> bool Connection<NODE_CLASS>::operator==(const Connection &rhs) {
	//Check if the referenced node and the type of the 2 connections is the same
	if (this->node->JobID != rhs.node->JobID ||
		this->node->OpID != rhs.node->OpID) return false;

	return true;
}

//Graph Methods
template <class NODE_CLASS> Graph<NODE_CLASS>::Graph(int n) {
	cInd = 0;
	cycleflag = false;
	size = 0;
	max_size = n;
	state = FORWARD;

	adj_list = new adj_list_elem<NODE_CLASS>[n];
	criticalPath = vector<NODE_CLASS*>();
	//nodeNum = 0;
	//Temp Storage
	visitedNodes = new NODE_CLASS*[n];
	
	stack_list = new bool[n]; 
	visited = new bool[n]; 
	top_sort = deque<NODE_CLASS*>();
    top_sort_index = new int[n];
}

template <class NODE_CLASS> Graph<NODE_CLASS>::~Graph() {
	
	//Make sure to delete the nodes manually
	//Using operations as nodes it deletion is not needed

	/*for (size_t i = 0; i < this->Nodes.size(); i++) {
		NODE_CLASS *node = this->Nodes[i];
		delete node;
	}*/

	//Clear adjacency List
	delete[] adj_list;
	
	//Clear criticalPath
	criticalPath.clear();
	
	//Temp Stuff
	delete[] visitedNodes;
	delete[] stack_list;
	delete[] visited;
	delete[] top_sort_index;
	top_sort.clear();
	//delete top_sort; Not needed for stack containers?!?!?!
}


//Not used. Copy is manually accomplished in the solution =operator
template <class NODE_CLASS> Graph<NODE_CLASS>& Graph<NODE_CLASS>::operator=(const Graph &rhs) {
	//Copy attributes
	this->size = rhs.size;
	this->state = rhs.state;

	//Copy Nodes
	for (int i = 0; i<rhs.Nodes.size(); i++) {
		NODE_CLASS *node = rhs.Nodes[i];
		NODE_CLASS *new_node = new NODE_CLASS();
		*new_node = *node;
		this->Nodes.push_back(new_node);
		//Update g_state
		new_node->g_state = &this->state;
	}
	
	//Assign proper connections to the nodes
	for (int i = 0; i<rhs.size; i++) {
		NODE_CLASS *node = rhs.Nodes[i];
		NODE_CLASS *new_node = this->Nodes[i];
		
		//Assign Forward Connections
		for (int k = 0; k< node->f_Connection_Num; k++) {
			pair<NODE_CLASS*, int> p = node->f_Connections[k];
			NODE_CLASS *conn = p.first;
			NODE_CLASS *new_conn = this->findNode(conn->ID);
			new_node->connect(new_conn, true, p.second);
		}

		//Assign Forward Connections
		for (int k = 0; k< node->f_Connection_Num; k++) {
			pair<NODE_CLASS*, int> p = node->f_Connections[k];
			NODE_CLASS *conn = p.first;
			NODE_CLASS *new_conn = this->findNode(conn->ID);
			new_node->connect(new_conn, true, p.second);
		}
		
	}

	return *this;
}

template<class NODE_CLASS> bool Graph<NODE_CLASS>::operator==(const Graph &rhs) {
	//Check attributes
	if (this->size != rhs.size) return false;

	printf("Not Implemented \n");

	return true;
}

template<class NODE_CLASS> bool Graph<NODE_CLASS>::operator!=(const Graph &rhs) {
	return !(*this == rhs);
}

template<class NODE_CLASS> void Graph<NODE_CLASS>::addNode(NODE_CLASS *node) {
	//DO NOT CALL THIS
	//Make sure to set the valid property beforehand
	//Nodes.push_back(node);
	node->nodeID = cInd; //Set node index
	node->g_state = &state;
	this->cInd += 1; //Update graph node counter
	return;
};

template<class NODE_CLASS> void Graph<NODE_CLASS>::removeNode(NODE_CLASS *node) {
	//Just remove temporary/machine connections
	for (int i = node->Connections.size() - 1; i >= 0; i--) {
		if (node->Connections[i].second == 1)
			node->disconnect(node->Connections[i].first);
	}
};

template<class NODE_CLASS> NODE_CLASS* Graph<NODE_CLASS>::findNode(int UID) {
	return this->Nodes[UID];
};

template<class NODE_CLASS> void Graph<NODE_CLASS>::resetVisited() {
	for (int i = 0; i < size; i++) 
		visited[i] = false;
}

//Depth-First-Search Wrapper Function
template<class NODE_CLASS> void Graph<NODE_CLASS>::dfs(int node) {
	cycleflag = false;
	for (int i = 0; i < size; i++) {
		stack_list[i] = false;
		visited[i] = false;
	}

	visitedNodes_n = 0;
	visit(node);
}

template<class NODE_CLASS> void Graph<NODE_CLASS>::topological_sort() {
	cycleflag = false;
	top_sort.clear();
	for (int i = 0; i < size; i++) {
		stack_list[i] = false;
		visited[i] = false;
	}

	visitedNodes_n = 0;
	for (int i = 0; i < size; i++) 
		t_sort(i);

	//Update index map for the t_sort
    for (int i=0;i<size;i++)
        top_sort_index[top_sort[i]->UID] = i;
}

template<class NODE_CLASS> void Graph<NODE_CLASS>::topological_sort(int *visiting_order) {
    cycleflag = false;
    top_sort.clear();
    for (int i = 0; i < size; i++) {
        stack_list[i] = false;
        visited[i] = false;
    }

    visitedNodes_n = 0;
    for (int i = 0; i < size; i++)
        t_sort(visiting_order[i]);

    //Update index map for the t_sort
    for (int i=0;i<size;i++)
        top_sort_index[top_sort[i]->UID] = i;
}

//template<class NODE_CLASS> void Graph<NODE_CLASS>::visit(int node) {
//	printf("Not Implemented\n");
//	assert(false);
//}


template<class NODE_CLASS> void Graph<NODE_CLASS>::visitNode(NODE_CLASS *node, bool *stack) {
	
}

template<class NODE_CLASS> bool Graph<NODE_CLASS>::checkNodes() {
	printf("Not Implemented\n");
	return true;
}


//Reporting
template<class NODE_CLASS> void Graph<NODE_CLASS>::report() {
	int graph_size = this->size;
	cout << "Graph Size: " << graph_size << endl;
	//cout << "Graph Nodes: " << nodeNum << endl;

	//For now this method will only work with OpNode but whatever
	for (int i = 0; i < graph_size; i++) {
		adj_list_elem<NODE_CLASS> n = adj_list[i];
		NODE_CLASS *n_op = n.op;
		if (n_op == NULL) continue;

		printf("Node (%d-%d) UID %d\n", n_op->jobID, n_op->ID, n_op->UID);
		
		cout << "\t " << "Node connections: " << endl;
		cout << "\t";

		//Report Permanent Connections
		for (int k = 0; k < MAX_JOB_RELATIONS; k++) {
			int conn_node = n.p_connections[k];
			if (conn_node < 0) break;
			cout << "(" << conn_node << ") P ";
		}

		//Report T Connections
		if (n.t_connection >= 0)
			cout << "(" << n.t_connection << ") T ";

		cout << endl;
	}
}

//Connection Number
template<class NODE_CLASS> int Graph<NODE_CLASS>::getConnectionNum() {
	printf("Not Implemented\n");
	assert(false);
	return -1;
}




#endif





