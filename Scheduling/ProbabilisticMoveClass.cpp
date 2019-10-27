#include "ProbabilisticMoveClass.h"

using namespace std;

void ProbabilisticMove::clear() {
	//Reset Values
	for (int i = 0; i < moveCount; i++) {
		moves[i] = 0;
		impr_moves[i] = 0;
		move_frac[i] = 0.0f;
		n_move_frac[i] = 0.0f;
	}
}

void ProbabilisticMove::update(int move, bool improvement) {
	moves[move]++;
	if (improvement) {
		impr_moves[move]++;
	}
	move_frac[move] = (float) impr_moves[move] / (moves[move] + 1);
	
	//Update Move Fractions
	float m_sum = 0.0f;
	for (int i = 0; i < moveCount; i++)
		m_sum += move_frac[i];

	//Normalize
	for (int i = 0; i < moveCount; i++)
		n_move_frac[i] = move_frac[i] / m_sum;

};

int ProbabilisticMove::selectMove() {
	//Move Selection
	int selection = -1;
	//Sort Fractions
	sort(impr_order, impr_order + moveCount, [&](const int lhs, const int rhs)
	{
		return n_move_frac[lhs] < n_move_frac[rhs];
	});

	float r_c = randd();
	float res = 0.0f;
	for (int i = 0; i < moveCount; i++) {
		if (n_move_frac[impr_order[i]] < 1e-5) continue;
		if (r_c > res) {
			selection = impr_order[i];
			res += n_move_frac[impr_order[i]];
		}
		else break;
	}

	//Failsafe mechanism

	while (selection < 0){
		int sel = rand() % moveCount;
		if (n_move_frac[sel] > 0) {
			selection = sel; break;
		}
	}

	return selection;
};

