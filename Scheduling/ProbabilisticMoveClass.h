#include "Parameters.h"
#ifndef PROB_MOVE_CLASS
#define PROB_MOVE_CLASS

class ProbabilisticMove {
public:
	int moveCount;
	int *impr_moves;
	int *impr_order;
	int *moves;
	float *move_frac;
	float *n_move_frac;

	//Constructor
	ProbabilisticMove(int mC) {
		moveCount = mC;
		impr_moves = new int[mC];
		impr_order = new int[mC];
		moves = new int[mC];
		move_frac = new float[mC];
		n_move_frac = new float[mC];
		//Init values to zero
		//Let the user provide any setup 
		
		for (int i = 0; i < mC; i++) {
			moves[i] = 0;
			impr_order[i] = i;
			impr_moves[i] = 0;
			move_frac[i] = 0.0f;
			n_move_frac[i] = 0.0f;
		}
	}

	~ProbabilisticMove() {
		delete[] impr_moves;
		delete[] impr_order;
		delete[] moves;
		delete[] move_frac;
		delete[] n_move_frac;
	}

	//Methods
	void update(int move, bool improvement);
	void clear();
	int selectMove();
};



#endif
