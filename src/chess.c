#include "chess.h"

//-----------------------------------------------------------------------------
// Private functions
//-----------------------------------------------------------------------------

color_t get_color( const board_t* bd, int pos ) {
   switch ( bd->sqr[pos] ) {
      case 'P':
      case 'R':
      case 'N':
      case 'B':
      case 'Q':
      case 'K':
         return WHITE;
      case 'p':
      case 'r':
      case 'n':
      case 'b':
      case 'q':
      case 'k':
         return BLACK;
      default:
         return NONE;
   }
   return NONE;
}

piece_t get_piece( const board_t* bd, int pos ) {
   switch ( bd->sqr[pos] ) {
      case 'p':
      case 'P':
         return PAWN;
      case 'r':
      case 'R':
         return ROOK;
      case 'n':
      case 'N':
         return KNIGHT;
      case 'b':
      case 'B':
         return BISHOP;
      case 'q':
      case 'Q':
         return QUEEN;
      case 'k':
      case 'K':
         return KING;
      default:
         return EMPTY;
   }
   return EMPTY;
}

static int add( int pos, int dr, int dc ) {
   int r = (pos >> 3) + dr;
   int c = (pos &  7) + dc;
   if ( r < 0 || r > 7 ) return -1;
   if ( c < 0 || c > 7 ) return -1;
   return 8*r+c;
}

// Establece una bandera
// La función usa flg como máscara para elegir la bandera a establecer:
// 0x01 = movimiento, la casilla ha cambiado
// 0x02 = "en passant", marca captura al paso válida
static int set_flag( board_t* bd, int pos, int flg ) {
   if ( pos < 0 ) return 0;
   bd->flg[pos] |= flg;
   return 1;
}

// Elimina una bandera
static int clr_flag( board_t* bd, int pos, int flg ) {
   if ( pos < 0 ) return 0;
   bd->flg[pos] &= ~flg;
   return 1;
}

// Obtiene una bandera
static int get_flag( const board_t* bd, int pos, int flg ) {
   if ( pos < 0 ) return 0;
   if ( bd->flg[pos] & flg ) return 1;
   return 0;
}

static int pawn_age( board_t* bd, int a, int b ) {

   if ( get_piece(bd,a) == PAWN || get_piece(bd,b) == PAWN ) {
      bd->age = 0;
      return 0;
   }

   bd->age ++;
   return 1;
}

static int en_passant( board_t* bd, int a, int b ) {

   if ( get_piece(bd,a) != PAWN ) return 0;
   int k = ( get_color(bd,a) == BLACK ) ? -1 : 1;

   // Aplicar captura al paso
   if ( get_flag(bd,b,FLG_PAS) ) bd->sqr[add(b,-k,0)] = '.';

   // Remover banderas de captura al paso
   for( int i=16; i<24; i++ ) clr_flag(bd,i,FLG_PAS);
   for( int i=40; i<48; i++ ) clr_flag(bd,i,FLG_PAS);

   // Habilitar captura al paso
   if ( b == add(a,2*k,0) ) set_flag(bd,add(a,k,0),FLG_PAS);

   return 1;
}

static int pawn_promotion( board_t* bd, int a, int b ) {

   if ( get_piece(bd,a) != PAWN ) return 0;
   color_t color = get_color(bd,a);

   int row = b >> 3;

   if ( color == WHITE ) {
      if ( row != 7 ) return 0;
      bd->sqr[a] = 'Q';
      return 1;
   }

   if ( color == BLACK ) {
      if ( row != 0 ) return 0;
      bd->sqr[a] = 'q';
      return 1;
   }

   return 0;
}

static int castling( board_t* bd, int a, int b ) {
   int pos1, pos2;

   if ( get_piece(bd,a) != KING ) return 0;

   //Move king rook
   if ( b == add(a,0,2) ) {
      pos1 = add(a,0,1);
      pos2 = add(a,0,3);
      bd->sqr[pos1] = bd->sqr[pos2];
      bd->sqr[pos2] = '.';

      set_flag(bd,pos1,FLG_MOV);
      set_flag(bd,pos2,FLG_MOV);
      return 1;
   }

   // Move queen rook
   if ( b == add(a,0,-2) ) {
      pos1 = add(a,0,-1);
      pos2 = add(a,0,-4);
      bd->sqr[pos1] = bd->sqr[pos2];
      bd->sqr[pos2] = '.';

      set_flag(bd,pos1,FLG_MOV);
      set_flag(bd,pos2,FLG_MOV);
      return 1;
   }

   return 0;
}

// Color es el color de la pieza hipotética en pos, que puede estar vacio.
static int slide_attack(
      const board_t* bd,
      int            pos,
      int            dr,
      int            dc,
      color_t        color,
      int*           end
) {
   int x = pos;

   while( 1 ) {
      if ( (x = add(x,dr,dc)) < 0 ) break;
      if ( get_piece(bd,x) == EMPTY ) continue;
      if ( get_color(bd,x) == color ) return 0;
      *end = x;
      return 1;
   }
   return 0;
}

static int step_attack(
      const board_t* bd,
      int            pos,
      int            dr,
      int            dc,
      color_t        color,
      int*           end
) {
   int x;
   if ( (x = add(pos,dr,dc)) < 0 ) return 0;
   if ( get_piece(bd,x) == EMPTY ) return 0;
   if ( get_color(bd,x) == color ) return 0;
   *end = x;
   return 1;
}

static int slide_moves(
      const board_t* bd,
      int            pos,
      int            dr,
      int            dc,
      int*           moves,
      int*           sz
) {
   int n = *sz;
   int x = pos;

   while( 1 ) {
      x = add(x,dr,dc);
      if ( x < 0 ) break;
      if ( get_piece(bd,x) == EMPTY ) moves[n++] = x;
      else {
         if ( get_color(bd,x) != get_color(bd,pos) ) moves[n++] = x;
         break;
      }
   }
   *sz = n;
   return 1;
}

static int step_moves(
      const board_t* bd,
      int            pos,
      int            dr,
      int            dc,
      int*           moves,
      int*           sz
) {
   int x = add(pos,dr,dc);
   if ( x < 0 ) return 0;
   if ( get_color(bd,x) == get_color(bd,pos) ) return 0;
   moves[*sz] = x;
   *sz += 1;
   return 1;
}

// x = capture position
// color = of the playing pieces
static int pawn_capture( const board_t* bd, int x, int s, color_t color ) {

   // normal capture
   if ( x < 0 ) return 0;
   if ( get_piece(bd,x) != EMPTY ) {
     if ( get_color(bd,x) == color ) return 0; // Do not auto-capture
     return 1;
   }

   // en passant
   int y = add(x,s,0);
   if ( !get_flag(bd,x,FLG_PAS)  ) return 0;
   if ( get_piece(bd,y) != EMPTY ) return 0;
   return 1;
}

//int pawn_moves( const board_t* bd, int pos, int sig, int* moves, int* sz ) {
static int pawn_moves( const board_t* bd, int pos, int* moves, int* sz ) {
   int n = *sz;
   int f, x;

   if ( get_piece(bd,pos) != PAWN ) return 0;

   color_t color = get_color(bd,pos);
   int s = ( color == BLACK ) ? -1 : 1;

   // mover una casilla enfrente
   x = add(pos,s,0);
   if ( x >= 0 ) {
      f = ( get_piece(bd,x) == EMPTY ) ? 1 : 0;
      if ( f ) moves[n++] = x;
   }

   // mover dos casillas enfrente
   x = add(pos,2*s,0);
   if ( !get_flag(bd,pos,FLG_MOV) && x >= 0 ) {
      if ( (get_piece(bd,x) == EMPTY) && f ) moves[n++] = x;
   }

   // captura diagonal derecha
   x = add(pos,s,1);
   if ( pawn_capture(bd,x,s,color) ) moves[n++] = x;

   // captura diagonal izquierda
   x = add(pos,s,-1);
   if ( pawn_capture(bd,x,s,color) ) moves[n++] = x;

   *sz = n;
   return 1;
}

static int rook_moves( const board_t* bd, int pos, int* moves, int* sz ) {
   slide_moves(bd,pos, 0, 1,moves,sz);
   slide_moves(bd,pos, 0,-1,moves,sz);
   slide_moves(bd,pos,-1, 0,moves,sz);
   slide_moves(bd,pos, 1, 0,moves,sz);
   return 1;
}

static int knight_moves( const board_t* bd, int pos, int* moves, int* sz ) {
   step_moves(bd,pos, 2, 1,moves,sz);
   step_moves(bd,pos, 2,-1,moves,sz);
   step_moves(bd,pos,-2, 1,moves,sz);
   step_moves(bd,pos,-2,-1,moves,sz);
   step_moves(bd,pos, 1, 2,moves,sz);
   step_moves(bd,pos,-1, 2,moves,sz);
   step_moves(bd,pos, 1,-2,moves,sz);
   step_moves(bd,pos,-1,-2,moves,sz);
   return 1;
}

static int bishop_moves( const board_t* bd, int pos, int* moves, int* sz ) {
   slide_moves(bd,pos, 1, 1,moves,sz);
   slide_moves(bd,pos, 1,-1,moves,sz);
   slide_moves(bd,pos,-1, 1,moves,sz);
   slide_moves(bd,pos,-1,-1,moves,sz);
   return 1;
}

static int queen_moves( const board_t* bd, int pos, int* moves, int* sz ) {
   slide_moves(bd,pos, 0, 1,moves,sz);
   slide_moves(bd,pos, 0,-1,moves,sz);
   slide_moves(bd,pos,-1, 0,moves,sz);
   slide_moves(bd,pos, 1, 0,moves,sz);
   slide_moves(bd,pos, 1, 1,moves,sz);
   slide_moves(bd,pos, 1,-1,moves,sz);
   slide_moves(bd,pos,-1, 1,moves,sz);
   slide_moves(bd,pos,-1,-1,moves,sz);
   return 1;
}

static int king_side_castling(
      const board_t* bd,
      int pos,
      color_t color,
      int* moves,
      int*sz
) {
   if ( get_flag(  bd,add(pos,0,3),FLG_MOV)  ) return 0; // rook moved
   if ( get_piece( bd,add(pos,0,3)) != ROOK  ) return 0; // no rook
   if ( get_piece( bd,add(pos,0,1)) != EMPTY ) return 0;
   if ( get_piece( bd,add(pos,0,2)) != EMPTY ) return 0;
   if ( is_attack( bd,add(pos,0,1),color)    ) return 0;
   if ( is_attack( bd,add(pos,0,2),color)    ) return 0;
   moves[*sz] = add(pos,0,2);
   *sz += 1;
   return 1;
}

static int queen_side_castling(
      const board_t* bd,
      int pos,
      color_t color,
      int* moves,
      int*sz
) {
   if ( get_flag(  bd,add(pos,0,-4),FLG_MOV)  ) return 0; // rook moved
   if ( get_piece( bd,add(pos,0,-4)) != ROOK  ) return 0; // no rook
   if ( get_piece( bd,add(pos,0,-1)) != EMPTY ) return 0;
   if ( get_piece( bd,add(pos,0,-2)) != EMPTY ) return 0;
   if ( get_piece( bd,add(pos,0,-3)) != EMPTY ) return 0;
   if ( is_attack( bd,add(pos,0,-1),color)    ) return 0;
   if ( is_attack( bd,add(pos,0,-2),color)    ) return 0;
   moves[*sz] = add(pos,0,-2);
   *sz += 1;
   return 1;
}

static int castling_moves( const board_t* bd, int pos, int* moves, int*sz ) {

   if ( get_piece(bd,pos) != KING ) return 0;
   if ( get_flag(bd,pos,FLG_MOV) ) return 0; //king moved
   if ( is_check(bd,pos)  ) return 0;

   color_t color = get_color(bd,pos);
   king_side_castling(bd,pos,color,moves,sz);
   queen_side_castling(bd,pos,color,moves,sz);
   return 0;
}

static int king_moves( const board_t* bd, int pos, int* moves, int* sz ) {
   step_moves(bd,pos, 0, 1,moves,sz);
   step_moves(bd,pos, 0,-1,moves,sz);
   step_moves(bd,pos,-1, 0,moves,sz);
   step_moves(bd,pos, 1, 0,moves,sz);
   step_moves(bd,pos, 1, 1,moves,sz);
   step_moves(bd,pos, 1,-1,moves,sz);
   step_moves(bd,pos,-1, 1,moves,sz);
   step_moves(bd,pos,-1,-1,moves,sz);
   castling_moves(bd,pos,moves,sz);
   return 1;
}

//-----------------------------------------------------------------------------
// Public functions
//-----------------------------------------------------------------------------

// Crea un tablero dinámico sin inicializar
//board_t* board_new() {
board_t* board_alloc() {
   board_t* bd = malloc( sizeof(board_t) );
   board_clear(bd);
   return bd;
}

// Crea un tablero dinámico inicializado
//board_t* board_new_init() {
board_t* board_create() {
   board_t* bd = malloc( sizeof(board_t) );
   board_init(bd);
   return bd;
}

// Crea una copia dinámica de un tablero
void board_copy( const board_t* bd, board_t* cp ) {
   for ( int i=0; i<64; i++ ) cp->flg[i] = bd->flg[i];
   for ( int i=0; i<64; i++ ) cp->sqr[i] = bd->sqr[i];
   cp->age  = bd->age;
   cp->turn = bd->turn;
}

void board_clear( board_t* bd ) {
   for ( int i=0; i<64; i++ ) bd->flg[i] = 0;
   for ( int i=0; i<64; i++ ) bd->sqr[i] = '.';
   bd->age  = 0;
   bd->turn = WHITE;
}

// Inicializa un tablero
void board_init( board_t* bd ) {

   // flags
   for ( int i=0;  i<64; i++ ) bd->flg[i] = 0;
   bd->age  = 0;
   bd->turn = WHITE;

   bd->sqr[0] = 'R';
   bd->sqr[1] = 'N';
   bd->sqr[2] = 'B';
   bd->sqr[3] = 'Q';
   bd->sqr[4] = 'K';
   bd->sqr[5] = 'B';
   bd->sqr[6] = 'N';
   bd->sqr[7] = 'R';

   for ( int i=8;  i<16; i++ ) bd->sqr[i] = 'P';
   for ( int i=16; i<48; i++ ) bd->sqr[i] = '.';
   for ( int i=48; i<56; i++ ) bd->sqr[i] = 'p';

   bd->sqr[56] = 'r';
   bd->sqr[57] = 'n';
   bd->sqr[58] = 'b';
   bd->sqr[59] = 'q';
   bd->sqr[60] = 'k';
   bd->sqr[61] = 'b';
   bd->sqr[62] = 'n';
   bd->sqr[63] = 'r';
}

int ind( const char* pos ) {
   if ( pos[0] < 'a' || pos[0] > 'h' ) return -1;
   if ( pos[1] < '1' || pos[1] > '8' ) return -1;
   return 8*(pos[1] - '1') + (pos[0] - 'a');
}

int coord( int pos, char* str ) {
   if ( pos < 0 || pos >= 64 ) return 0;
   str[0] = (pos & 7) + 'a';
   str[1] = (pos >> 3 ) + '1';
   str[2] = 0;
   return 1;
}

int is_piece( char c ) {
   switch ( c ) {
      case 'P':
      case 'R':
      case 'N':
      case 'B':
      case 'Q':
      case 'K':
      case 'p':
      case 'r':
      case 'n':
      case 'b':
      case 'q':
      case 'k':
         return 1;
      default:
         return 0;
   }
   return 0;
}

// color = Color de la pieza en pos, o color supuesto si la casilla está vacía.
int is_attack( const board_t* bd, int pos, color_t color ) {
   int rq[4][2] = { { 0, 1}, { 0,-1}, { 1, 0}, {-1, 0} };
   int bq[4][2] = { { 1, 1}, { 1,-1}, {-1, 1}, {-1,-1} };
   int n[8][2]  = { { 2, 1}, { 2,-1}, {-2, 1}, {-2,-1},
                    { 1, 2}, {-1, 2}, { 1,-2}, {-1,-2} };
   int k[8][2]  = { { 0, 1}, { 0,-1}, { 1, 0}, {-1, 0},
                    { 1, 1}, { 1,-1}, {-1, 1}, {-1,-1} };
   int end;

   int p = ( color == BLACK ) ? -1 : 1;

   // Rook and Queen
   for ( int i=0; i<4; i++ ) {
      if ( !slide_attack(bd,pos,rq[i][0],rq[i][1],color,&end) ) continue;
      if ( get_piece(bd,end) == ROOK  ) return 1;
      if ( get_piece(bd,end) == QUEEN ) return 1;
   }

   // Bishop and Queen
   for ( int i=0; i<4; i++ ) {
      if ( !slide_attack(bd,pos,bq[i][0],bq[i][1],color,&end) ) continue;
      if ( get_piece(bd,end) == BISHOP ) return 1;
      if ( get_piece(bd,end) == QUEEN  ) return 1;
   }

   // Knight
   for ( int i=0; i<8; i++ ) {
      if ( !step_attack(bd,pos,n[i][0],n[i][1],color,&end) ) continue;
      if ( get_piece(bd,end) == KNIGHT ) return 1;
   }

   // Pawn
   if ( step_attack(bd,pos,p,1,color,&end) ) {
      if ( get_piece(bd,end) == PAWN ) return 1;
   }

   if ( step_attack(bd,pos,p,-1,color,&end) ) {
      if ( get_piece(bd,end) == PAWN ) return 1;
   }

   // King
   for ( int i=0; i<8; i++ ) {
      if ( !step_attack(bd,pos,k[i][0],k[i][1],color,&end) ) continue;
      if ( get_piece(bd,end) == KING ) return 1;
   }

   return 0;
}

int is_check( const board_t* bd, int pos ) {
   if ( get_piece(bd,pos) != KING ) return 0;
   return is_attack(bd,pos,get_color(bd,pos));
}

int is_draw( const board_t* bd ) {
   if ( bd->age >= 50 ) return 1;
   return 0;
}

int in_moves( int pos, const int* moves, int sz ) {
   for ( int i=0; i<sz; i++ ) {
      if ( pos == moves[i] ) return 1;
   }
   return 0;
}

// Filters out the moves that capture a king
int filter_king_captures( const board_t* bd, int* moves, int* sz ) {
   int i = 0;

   while ( i < *sz ) {
     if ( get_piece(bd,moves[i]) == KING ) {
       moves[i] = moves[*sz-1];
       *sz -= 1;
       continue;
     }
     i ++;
   }
   return 1;
}

// Test if the king of the `color` in turn is in check
int test_check( const board_t* bd ) {
   for ( int i=0; i<64; i++ ) {
      if ( get_piece(bd,i) != KING  ) continue;
      if ( get_color(bd,i) != bd->turn ) continue;
      if ( is_attack(bd,i,bd->turn) ) return 1;
      return 0;
   }
   return 0;
}

// Filters all moves that end with the king in check
// bd    = board
// pos   = position of the piece that is evaluated
// moves = list of valid moves for the piece in pos
// sz    = number of elements in moves
int filter_king_in_check( const board_t* bd, int pos, int* moves, int* sz ) {
   board_t cp;
   int     a, b;
   int     check;

   int i = 0;
   while ( i < *sz ) {
      board_copy(bd,&cp);

      // Apply the move
      a = pos;
      b = moves[i];

      // Pawn
      pawn_age(&cp,a,b);
      en_passant(&cp,a,b);
      pawn_promotion(&cp,a,b);

      // King
      castling(&cp,a,b);

      cp.sqr[b] = cp.sqr[a];
      cp.sqr[a] = '.';

      set_flag(&cp,a,FLG_MOV);
      set_flag(&cp,b,FLG_MOV);

      // Test if the king is in check
      check = test_check(&cp);

      // Restore board
      // board_copy(bd,&cp);

      // If still in check, filter the move
      // if ( check ) {
      if ( test_check(&cp) ) {
         moves[i] = moves[*sz-1];
         *sz -= 1;
         continue;
      }

      i ++;
   }
   return 1;
}

// Obtiene los movimientos de la pieza en la posición pos.
int get_moves( const board_t* bd, int pos, int* moves, int* sz ) {
   *sz = 0;

   switch ( get_piece(bd,pos) ) {
      case PAWN:
         pawn_moves(bd,pos,moves,sz);
         break;
      case ROOK:
         rook_moves(bd,pos,moves,sz);
         break;
      case KNIGHT:
         knight_moves(bd,pos,moves,sz);
         break;
      case BISHOP:
         bishop_moves(bd,pos,moves,sz);
         break;
      case QUEEN:
         queen_moves(bd,pos,moves,sz);
         break;
      case KING:
         king_moves(bd,pos,moves,sz);
         break;
      default:
         return 0;
   }

   // Filter moves that capture the king
   filter_king_captures(bd,moves,sz);

   // Filter moves that end with the king in check
   filter_king_in_check(bd,pos,moves,sz);

   return 1;
}

// Gets the color of the current turn pieces
color_t get_turn( const board_t* bd ) {
   return bd->turn;
}

// Applies a move without validation.
// This function applies "en passant", promotions, castlings, and ages the
// pawns.
void apply_move( board_t* bd, int a, int b ) {

   // Pawn
   pawn_age(bd,a,b);
   en_passant(bd,a,b);
   pawn_promotion(bd,a,b);

   // King
   castling(bd,a,b);

   bd->sqr[b] = bd->sqr[a];
   bd->sqr[a] = '.';

   set_flag(bd,a,FLG_MOV);
   set_flag(bd,b,FLG_MOV);

   bd->turn = (bd->turn == BLACK) ? WHITE : BLACK;
}

// Returns an error:
// 0 = No error, valid movement.
// 1 = There is no piece to move at position `a`.
// 2 = Wrong turn.
// 3 = Wrong move.
int make_move( board_t* bd, int a, int b, int* moves, int sz ) {

   if ( get_piece(bd,a) == EMPTY ) return 1;
   if ( get_color(bd,a) != bd->turn ) return 2;

   if ( in_moves(b,moves,sz) ) {
      apply_move(bd,a,b);
      return 0;
   }

   return 3;
}

int force_move( board_t* bd, int a, int b ) {
   if ( bd->sqr[a] == '.' ) return 0;

   bd->sqr[b] = bd->sqr[a];
   bd->sqr[a] = '.';

   set_flag(bd,a,FLG_MOV);
   set_flag(bd,b,FLG_MOV);
   return 1;
}

int place_piece( board_t* bd, int pos, char piece ) {
   bd->sqr[pos] = piece;
   clr_flag(bd,pos,FLG_MOV);
   return 1;
}

int remove_piece( board_t* bd, int pos ) {
   if ( get_piece(bd,pos) == EMPTY ) return 0;
   bd->sqr[pos] = '.';
   clr_flag(bd,pos,FLG_MOV);
   return 1;
}

void board_print( const board_t* bd ) {
   int pos;

   printf("   a b c d e f g h\n");
   printf(" -------------------\n");

   for ( int j=7; j>=0; j-- ) {
      printf("%i| ",j+1);
      for ( int i=0; i<8; i++ ) {
         pos = 8*j + i;
         if ( get_piece(bd,pos) == KING ) {
            if ( is_check(bd,pos) ) {
               printf("%c!",bd->sqr[pos]);
               continue;
            }
         }
         printf("%c ",bd->sqr[pos]);
      }
      printf("|%i\n",j+1);
   }

   printf(" -------------------\n");
   printf("   a b c d e f g h\n");
}

void board_print_flags( const board_t* bd ) {
   int pos;

   printf("   a b c d e f g h   %i\n",bd->age);
   printf(" -------------------\n");

   for ( int j=7; j>=0; j-- ) {
      printf("%i| ",j+1);
      for ( int i=0; i<8; i++ ) {
         pos = 8*j + i;
         if      ( get_flag(bd,pos,FLG_PAS) ) printf("x ");
         else if ( get_flag(bd,pos,FLG_MOV) ) printf("o ");
         else                                 printf("- ");
      }
      printf("|%i\n",j+1);
   }

   printf(" -------------------\n");
   printf("   a b c d e f g h   ");
   if ( bd->turn == WHITE ) printf("W");
   if ( bd->turn == BLACK ) printf("B");
   printf("\n");
}

void board_mark_print( const board_t* bd, int* moves, int sz ) {
   char mask[64];
   int pos;

   for ( int i=0; i<64; i++ ) mask[i] = 0;
   for ( int i=0; i<sz; i++ ) mask[moves[i]] = 1;

   printf("   a b c d e f g h\n");
   printf(" -------------------\n");

   for ( int j=7; j>=0; j-- ) {
      printf("%i| ",j+1);
      for ( int i=0; i<8; i++ ) {
         pos = 8*j + i;
         if ( mask[pos] ) {
            if ( get_flag(bd,pos,FLG_PAS) ) printf("= ");
            else if ( get_piece(bd,pos) == EMPTY ) printf("+ ");
            else if ( get_piece(bd,pos) == KING  ) printf("@ ");
            else if ( get_color(bd,pos) == BLACK ) printf("x ");
            else printf("X ");
         }
         else printf("%c ",bd->sqr[pos]);
      }
      printf("|%i\n",j+1);
   }

   printf(" -------------------\n");
   printf("   a b c d e f g h\n");
}

