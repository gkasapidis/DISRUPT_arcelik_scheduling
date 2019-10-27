#ifndef POOL_ITEM_H
#define POOL_ITEM_H

class PoolItem {
public:
	float d_fromBest;
	float d_fromWorst;
	float weight;
	float fitness; //Used for fitness proportional selection
	int life;
	int hits;
	int suc_hits;
	
	//Trivial Constructor
	PoolItem() {
		d_fromBest = -100000;
		d_fromWorst = -100000;
		weight = 0.0f;
		fitness = 0.0f;
		life = 0;
		hits = 1;
		suc_hits = 1;
	}
};

#endif // !POOL_ITEM_H
