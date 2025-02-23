//htop
//mpicc main.c chess.c pcg_basic.c play.c -o chess 
//mpirun --use-hwthread-cpus -n [numero de procesos] ./chess [numero de simulaciones por proceso] [black o white o self]


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <mpi.h>
#include "chess.h"
#include "play.h"

#ifndef MOVLEN
#define MOVLEN 128
#endif

#ifndef BUFLEN
#define BUFLEN 512
#endif

void trim( char* str ) { 
   int i = 0;
   while ( str[i] != 0 ) {
      if ( str[i] == '\n' || str[i] == '\r' ) {
         str[i] = 0;
         break;
      }
      i++;
   }
}

int streq( const char* str1, const char* str2 ) { 
   if ( strcmp(str1,str2) == 0 ) return 1;
   return 0;
}

int parse( const char* str, const char* lcom, const char* scom ) {
   if ( strcmp(str,lcom) == 0 ) return 1;
   if ( strcmp(str,scom) == 0 ) return 1;
   return 0;
}

int print_moves( int* moves, int sz ) {
  char str[3];
  if ( sz == 0 ) return 0;
  coord(moves[0],str);
  printf("%s",str);
  for ( int i=1; i<sz; i++ ) {
    coord(moves[i],str);
    printf(" %s",str);
  }
  printf("\n");
  return 1;
}

// 0 = No error, valid movement.
// 1 = There is no piece to move at position `a`.
// 2 = Wrong turn.
// 3 = Wrong move.
int print_move_error( int err ) {
  switch ( err ) {
    case 0:
      printf("No error.\n");
      break;
    case 1:
      printf("No piece to move.\n");
      break;
    case 2:
      printf("Wrong turn.\n");
      break;
    case 3:
      printf("Wrong move.\n");
      break;
  }  
  return 0;
}

int valid_coords( const char* str){
   if(strlen(str) != 4) return 0;
   if(str[0] < 'a' || str[0] > 'h') return 0;
   if(str[1] < '1' || str[1] > '8') return 0;
   if(str[2] < 'a' || str[2] > 'h') return 0;
   if(str[3] < '1' || str[3] > '8') return 0;
   return 1;
}

void print_help(){
   printf("Usage: chess <iters> <user> \n");
   printf("iters = statistic iterations \n");
   printf("user = white, black, self \n");
   exit(1);
}

void error_msg(const char* msg){
   fprintf(stderr, "ERROR: %s \n",msg);
   exit(2);
}

int main(int argc, char** argv) {
   char buffer[BUFSIZ];
   int  moves[512];
   int  sz = 0, pos, a, b, e;
   char str[64];
   color_t turn;


   
   MPI_Init(NULL,NULL); // MPI INICIALIZADO------------------------------

   int rank;

   MPI_Comm_rank(MPI_COMM_WORLD,&rank);

   int size;
   MPI_Comm_size(MPI_COMM_WORLD,&size);

   
   board_t bd;          
   board_init(&bd);

   if(argc != 3) print_help();
   if(streq(argv[2], "black")) turn = BLACK;
   else if(streq(argv[2], "white")) turn = WHITE;
   else if(streq(argv[2], "self")) turn = NONE;
   else print_help();

   uint64_t iter = atol(argv[1]);
   uint64_t tseed =time(0);
   uint64_t pseed = (uint64_t)(&bd);
   pcg32_srandom(tseed,pseed);

   if(iter == 0) error_msg("Iter is zero.");
   if(rank == 0)
   {

      board_print(&bd);
 
      while ( 1 ) {

         if ( get_turn(&bd) == WHITE ) printf("W: ");
         else printf("B: ");
         fflush(stdout);
         if(turn == NONE || turn == get_turn(&bd)){
            fgets(buffer,BUFSIZ, stdin);
            trim(buffer);

            if (streq(buffer, "quit")) {
                printf("Exiting the game...\n");
                bd.turn = NONE;  
                MPI_Bcast((char*)(&bd), sizeof(board_t), MPI_CHAR, 0, MPI_COMM_WORLD);
                break; 
            }
         }
         if(turn == get_turn(&bd)){
            if(!valid_coords(buffer)) continue;
            a = ind(buffer);
            if( a < 0 ) continue;
            b = ind(buffer+2);
            if( b < 0) continue;
            get_moves(&bd,a,moves,&sz);
            e = make_move(&bd,a,b,moves,sz);
            if( e == 1) break;
            if( !e){printf("\n");
               board_print(&bd);}
            else print_move_error(e);
            continue;
         }
        

         MPI_Bcast((char*)(&bd), sizeof(board_t), MPI_CHAR,0,MPI_COMM_WORLD);
         if(monte_carlo_move(&bd,iter,&a,&b) == 0) break;
         coord(a,str);
         printf("%s",str);
         coord(b,str);
         printf("%s\n", str);

         get_moves(&bd,a,moves,&sz);
         e = make_move(&bd,a,b,moves,sz);
         if(e ==1 ) break;
         if(!e){ 
            printf("\n"); 
            board_print(&bd);
         }
         else print_move_error(e);
      }
      if(is_draw(&bd)) printf("Draw. \n");
      else if(test_check(&bd)){
         printf("Check mate!\n");
         if(get_turn(&bd) == WHITE) printf("Black Wins. \n");
         else printf("White Wins. \n");
      }
      else if(!end_of_game(&bd)) printf("Stalemate.\n");
   }
   else{
      while(1){
         MPI_Bcast((char*)(&bd),sizeof(board_t),MPI_CHAR,0,MPI_COMM_WORLD);
         if(monte_carlo_move(&bd,iter,&a,&b) == 0) break;
      }
   }
   MPI_Finalize();
   return 0;
}

