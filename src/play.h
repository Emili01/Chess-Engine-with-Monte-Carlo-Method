#ifndef PLAY_H
#define PLAY_H
#include "chess.h"
#include "pcg_basic.h"

int random_move(board_t* bd);

// 1 = gana turno actual.
// 0 = empate.
// -1 = gana opente.

int random_play(board_t* bd);

int monte_carlo_move(const board_t* bd, uint64_t iters, int* a,int* b);
int end_of_game(const board_t *bd);

#endif /* PLAY_H */