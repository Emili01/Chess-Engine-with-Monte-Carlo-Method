#ifndef CHESS_H
#define CHESS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MOVLEN 512

#define FLG_MOV 0x01
#define FLG_PAS 0x02

typedef enum {
   EMPTY, PAWN, ROOK, KNIGHT, BISHOP, QUEEN, KING
} piece_t;

typedef enum {
   NONE, BLACK, WHITE
} color_t;

typedef struct {
   char flg[64];
   char sqr[64];
   int  age;
   color_t turn;
} board_t;


color_t get_color( const board_t* bd, int pos );
piece_t get_piece( const board_t* bd, int pos );

// Regresa un tablero dinámico vacío.
board_t* board_alloc();

// Regresa un tablero dinámico inicializado.
board_t* board_create();

// Copia el tablero bd en cp
void board_copy( const board_t* bd, board_t* cp );

// Elimina las piezas (y banderas) del tablero.
void board_clear( board_t* bd );

// Inicializa el tablero con las piezas en su posición inicial.
void board_init( board_t* bd );

// Convierte coordenadas en un índice en el tablero
// Las coordenadas tienen la forma "a1", "d3", etc.
int ind( const char* pos );

// Convierte un índice en coordenadas.
int coord( int pos, char* str );

// Determina si el caracter c corresponde con una pieza válida:
// p r n b q k P R N B Q K
int is_piece( char c );

// Determina si la posición pos está en ataque
// db    = tablero
// pos   = casilla a evaluat
// color = color de la pieza en la casilla pos, o el color que "tendría" una
//         pieza en esa casilla (si está vacía). Las piezas enemigas son del
//         color opuesto a lo que se indique en 'color'.
int is_attack( const board_t* bd, int pos, color_t color );

// Determina si el rey está en jaque
// bd  = tablero
// pos = posición del rey (blanco o negro).
int is_check( const board_t* bd, int pos );

// Determina cuando el juego termina en tablas
// Regresa 1 cuando no se ha movido un peon en más de 50 movimientos, y 0 en
// caso contrario.
int is_draw( const board_t* bd );

// Verifica si el movimiento pos está en moves
//
// Esta función se usa para comparar el movimiento elegido por el jugador (pos)
// contra todos los movimientos válidos de una pieza (moves). Estos movimientos
// deben obtenerse aparte.
int in_moves( int pos, const int* moves, int sz );

// Test if the king of `color` is in check.
int test_check( const board_t* bd );

// Obtiene una lista de los movimientos posibles para la pieza en pos
// bd    = tablero
// pos   = casilla con la pieza a evaluar
// moves = vector donde se almacenan los movimientos
// sz    = nímero de movimientos encontrados
int get_moves( const board_t* bd, int pos, int* moves, int* sz );

// Regresa el color de las piezas que tienen el turno
color_t get_turn( const board_t* bd );

// Hace un movimiento aplicando captura al paso, promoción y castling, pero sin
// verificar que el movimiento sea válido.
void apply_move( board_t* bd, int a, int b );

// Mueve la pieza en la posición 'a' a la posición 'b'
// La función revisa que el movimiento sea válido
// buscándolo en `moves`.
//
// Return:
// 0 = No error, valid movement.
// 1 = There is no piece to move at position `a`.
// 2 = Wrong turn.
// 3 = Wrong move.
int make_move( board_t* bd, int a, int b, int* moves, int sz );

// Mueve una pieza de 'a' a 'b' aunque el movimiento no sea válido
// WARNING: No hace castlings, ni en passant, ni promoción.
int force_move( board_t* bd, int a, int b );

// Coloca una pieza en la posición 'pos'
// bd    = tablero
// pos   = posición
// piece = pieza a colocar (p r n b q k P R N B Q K)
int place_piece( board_t* bd, int pos, char piece );

// Elimina una pieza de la posición 'pos'
int remove_piece( board_t* bd, int pos );

// Imprime el tablero en pantalla.
void board_print( const board_t* bd );

// Imprime las banderas del tablero en pantalla.
void board_print_flags( const board_t* bd );

// Imprime el tablero y agrega marcas.
// bd    = tablero
// marks = arreglo de posiciones a marcar.
// sz    = número de marcas en marks
void board_mark_print( const board_t* bd, int* marks, int sz );

#endif /* CHESS_H */

