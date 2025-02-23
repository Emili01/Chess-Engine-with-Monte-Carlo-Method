/*
Codigo Final


*/


#include "play.h"
#include <stdio.h>
#include <mpi.h>
#include "chess.h"

void get_turn_pieces(const board_t* bd, int* pieces, int* sz) {
    *sz = 0;
    for (int i = 0; i < 64; i++) {
        if (get_color(bd, i) != bd->turn) continue;
        pieces[*sz] = i;
        *sz += 1;
    }
}

int random_move(board_t* bd) {
    int pieces[64];
    int moves[64];
    int p = 0, m = 0, a, b, r;

    if (is_draw(bd)) return -1;

    get_turn_pieces(bd, pieces, &p);
    if (p == 0) return 0; 

    while (1) {
        r = pcg32_boundedrand(p);
        a = pieces[r];

        get_moves(bd, a, moves, &m);
        if (m > 0) break;


        pieces[r] = pieces[p - 1];
        p--;

        if (p == 0) return 0; 
    }


    r = pcg32_boundedrand(m);
    b = moves[r];

 
    make_move(bd, a, b, moves, m);
    return 1;
}

color_t random_game(const board_t* bd) {
    board_t cp;
    int r;

    board_copy(bd, &cp);

    while (1) {
        r = random_move(&cp);
        if (r > 0) continue;
        if (r < 0) return NONE; 
        if (!test_check(&cp)) return NONE;
        break;
    }

    return (cp.turn == BLACK) ? WHITE : BLACK;
}

int get_all_moves(const board_t* bd, int* moves, int* sz) {
    int mlist[512];
    int m = 0;
    *sz = 0;

    for (int a = 0; a < 64; a++) {
        if (get_color(bd, a) != bd->turn) continue;

        get_moves(bd, a, mlist, &m);
        for (int b = 0; b < m; b++) {
            if (*sz + 2 > 512) {
                fprintf(stderr, "Error: Exceeded moves buffer size.\n");
                return -1; 
            }
            moves[*sz + 0] = a;
            moves[*sz + 1] = mlist[b];
            *sz += 2;
        }
    }
    return 1;
}

int end_of_game(const board_t* bd) {
    if (test_check(bd)) return 1;
    if (is_draw(bd)) return 1;

    int all_moves[512];
    int sz = 0;
    get_all_moves(bd, all_moves, &sz);
    if (sz == 0) return 1;

    return 0;
}

int monte_carlo_move(const board_t* bd, uint64_t iters, int* a, int* b) {
    int mlist[512];
    int64_t mfreq[512] = {0};
    int64_t tfreq[512] = {0};
    int sz = 0, mv = 0;
    board_t cp;
    color_t out;

    if (end_of_game(bd)) return 0;

    get_all_moves(bd, mlist, &sz);
    if (sz == 0) return 0;

    mv = sz >> 1;

    for (uint64_t i = 0; i < iters; i++) {
        for (int k = 0; k < mv; k++) {
            board_copy(bd, &cp);
            apply_move(&cp, mlist[2 * k + 0], mlist[2 * k + 1]);
            out = random_game(&cp);

            if (out == bd->turn) mfreq[k]++;
            if (out != bd->turn && out != NONE) mfreq[k]++;
        }
    }

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Reduce(mfreq, tfreq, mv, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        int k = 0;
        for (int i = 1; i < mv; i++) {
            if (tfreq[i] > tfreq[k]) k = i;
        }
        *a = mlist[2 * k + 0];
        *b = mlist[2 * k + 1];
        return 1;
    }

    return -1;
}
