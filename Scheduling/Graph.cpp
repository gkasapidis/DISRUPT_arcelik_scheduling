#include "Parameters.h"
#include "Graph.h"
//
using namespace std;


//Node Methods
Node::Node() {
	//Setup Node attributes
	this->nodeID = -1; //This will be set when the node is added on a graph
	this->valid = false;
    this->critical = false;
	this->visited = false;

	//Use the following registers in order to temporarily store connection search results
	this->tConnectionsID = -1;
	this->tIndex = -1;
}

Node::~Node() {};

Node& Node::operator=(const Node &rhs) {
	this->nodeID = rhs.nodeID;
	this->valid = rhs.valid;
	this->critical = rhs.critical;
	//Node Connections cannot be added from this method
	return *this;
}

bool Node::operator==(const Node &rhs) {
	//Check attributes
	//if they are both invalid get out of here
	if ((this->valid && rhs.valid) == false) return true;
	if (this->nodeID != rhs.nodeID) return false;

	return true;
}

bool Node::isConnected(Node *node) {
	printf("Not Implemented\n");
	assert(false);
	return false;
}

bool Node::isEqual(Node *node) {
	printf("Not Implemented\n");
	assert(false);
	return false;
}

bool Node::connect(Node *node, bool temp, int type) {
	printf("Not Implemented\n");
	assert(false);
	return false;
}

bool Node::disconnect(Node *node) {
	printf("Not Implemented\n");
	assert(false);
	return false;
}

void Node::removeConnection(int i) {
	printf("Not Implemented\n");
	assert(false);
	return;
}

void Node::visit(Graph<Node> *gr, int state, bool *stack, bool &cycleflag, Node** visitedNodes, int &visitedNodes_n) {
	//If node has been visited before return true, Circle Found
	printf("Node Visit Not Implemented");
	assert(false);
	return;
}
