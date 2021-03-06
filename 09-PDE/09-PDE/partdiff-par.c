/****************************************************************************/
/****************************************************************************/
/**                                                                        **/
/**                 TU München - Institut für Informatik                   **/
/**                                                                        **/
/** Copyright: Prof. Dr. Thomas Ludwig                                     **/
/**            Andreas C. Schmidt                                          **/
/**                                                                        **/
/** File:      partdiff-seq.c                                              **/
/**                                                                        **/
/** Purpose:   Partial differential equation solver for Gauss-Seidel and   **/
/**            Jacobi method.                                              **/
/**                                                                        **/
/****************************************************************************/
/****************************************************************************/

/* ************************************************************************ */
/* Include standard header file.                                            */
/* ************************************************************************ */
#define _POSIX_C_SOURCE 200809L

//Falls Prozess keinen Vorgänger/Nachfolger hat
#define NOBODY -2831

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>
#include <malloc.h>
#include <sys/time.h>
#include <mpi.h>

#include "partdiff-par.h"

struct calculation_arguments
{
  uint64_t  N;              /* number of spaces between lines (lines=N+1)     */
  uint64_t  num_matrices;   /* number of matrices                             */
  double    h;              /* length of a space between two lines            */
  double    ***Matrix;      /* index matrix used for addressing M             */
  double    *M;             /* two matrices with real values                  */
  int rank;     //Nummer des Prozesses
  int size;     //Anzahl der Prozesse
  int from;     //Kümmmert sich ab dieser Zeile
  int to;       //bis zur dieser Zeile
  int num_rows;     //Anzahl der Zeilen
};

struct calculation_results
{
  uint64_t  m;
  uint64_t  stat_iteration; /* number of current iteration                    */
  double    stat_precision; /* actual precision of all slaves in iteration    */
};

/* ************************************************************************ */
/* Global variables                                                         */
/* ************************************************************************ */

/* time measurement variables */
struct timeval start_time;       /* time when program started                      */
struct timeval comp_time;        /* time when calculation completed                */


/* ************************************************************************ */
/* initVariables: Initializes some global variables                         */
/* ************************************************************************ */
static
void
initVariables (struct calculation_arguments* arguments, struct calculation_results* results, struct options const* options)
{
  arguments->N = (options->interlines * 8) + 9 - 1;
  arguments->num_matrices = (options->method == METH_JACOBI) ? 2 : 1;
  arguments->h = 1.0 / arguments->N;

  results->m = 0;
  results->stat_iteration = 0;
  results->stat_precision = 0;
}

static
void
initMpiVariables (struct calculation_arguments* arguments)
{
  //Diese MPI-Variablen wurden schon definiert
  int rank = arguments->rank;
  int size = arguments->size;
  int calculate_lines = arguments->N - 1;

  //Diese sollten nun initialisiert werden
  int* from = &(arguments->from);
  int* to = &(arguments->to);
  int* num_rows = &(arguments->num_rows);
  
  int own_calculate_lines = 0;

  if(rank < calculate_lines % size)
  {
    own_calculate_lines = calculate_lines / size + 1;
    *from = rank * (own_calculate_lines - 1) + rank;
    *to = *from + (own_calculate_lines -1) ;
    *num_rows = own_calculate_lines + 2;      //2 hollow lines
  }

  else
  {
    own_calculate_lines = calculate_lines / size;
    *from = rank * (own_calculate_lines - 1) + (calculate_lines % size) + rank;
    *to = (*from) + (own_calculate_lines - 1);
    *num_rows = own_calculate_lines + 2;
  }
    (*from)++; // soll von 1 starten
    (*to)++;
    
}

/* ************************************************************************ */
/* freeMatrices: frees memory for matrices                                  */
/* ************************************************************************ */
static
void
freeMatrices (struct calculation_arguments* arguments)
{
  uint64_t i;

  for (i = 0; i < arguments->num_matrices; i++)
  {
    free(arguments->Matrix[i]);
  }

  free(arguments->Matrix);
  free(arguments->M);
}

/* ************************************************************************ */
/* allocateMemory ()                                                        */
/* allocates memory and quits if there was a memory allocation problem      */
/* ************************************************************************ */
static
void*
allocateMemory (size_t size)
{
  void *p;

  if ((p = malloc(size)) == NULL)
  {
    printf("Speicherprobleme! (%" PRIu64 " Bytes)\n", size);
    /* exit program */
    exit(1);
  }

  return p;
}

/* ************************************************************************ */
/* allocateMatrices: allocates memory for matrices                          */
/* ************************************************************************ */
static
void
allocateMatrices (struct calculation_arguments* arguments)
{
  uint64_t i, j;

  uint64_t const N = arguments->N;

  arguments->M = allocateMemory(arguments->num_matrices * (N + 1) * (N + 1) * sizeof(double));
  arguments->Matrix = allocateMemory(arguments->num_matrices * sizeof(double**));

  for (i = 0; i < arguments->num_matrices; i++)
  {
    arguments->Matrix[i] = allocateMemory((N + 1) * sizeof(double*));

    for (j = 0; j <= N; j++)
    {
      arguments->Matrix[i][j] = arguments->M + (i * (N + 1) * (N + 1)) + (j * (N + 1));
    }
  }
}



static
void
allocateMpiMatrices (struct calculation_arguments* arguments)
{
  uint64_t i, j;

  uint64_t const N = arguments->N;
  int num_rows = arguments->num_rows;

  arguments->M = allocateMemory(arguments->num_matrices * (N + 1) * num_rows * sizeof(double));
  arguments->Matrix = allocateMemory(arguments->num_matrices * sizeof(double**));

  for (i = 0; i < arguments->num_matrices; i++)
  {
    arguments->Matrix[i] = allocateMemory((N + 1) * sizeof(double*));

    for (j = 0; j <= N; j++)
    {
      arguments->Matrix[i][j] = arguments->M + (i * num_rows * (N + 1)) + (j * (N + 1));
    }
  }
}

/* ************************************************************************ */
/* initMatrices: Initialize matrix/matrices and some global variables       */
/* ************************************************************************ */
static
void
initMatrices (struct calculation_arguments* arguments, struct options const* options)
{
  uint64_t g, i, j;                                /*  local variables for loops   */

  uint64_t const N = arguments->N;
  double const h = arguments->h;
  double*** Matrix = arguments->Matrix;

  /* initialize matrix/matrices with zeros */
  for (g = 0; g < arguments->num_matrices; g++)
  {
    for (i = 0; i <= N; i++)
    {
      for (j = 0; j <= N; j++)
      {
        Matrix[g][i][j] = 0.0;
      }
    }
  }

  /* initialize borders, depending on function (function 2: nothing to do) */
  if (options->inf_func == FUNC_F0)
  {
    for (g = 0; g < arguments->num_matrices; g++)
    {
      for (i = 0; i <= N; i++)
      {
        Matrix[g][i][0] = 1.0 - (h * i);
        Matrix[g][i][N] = h * i;
        Matrix[g][0][i] = 1.0 - (h * i);
        Matrix[g][N][i] = h * i;
      }

      Matrix[g][N][0] = 0.0;
      Matrix[g][0][N] = 0.0;
    }
  }
}

static
void
initMpiMatrices (struct calculation_arguments* arguments, struct options const* options)
{
  uint64_t g, i, j;                                /*  local variables for loops   */

  uint64_t const N = arguments->N;
  double const h = arguments->h;
  double*** Matrix = arguments->Matrix;

  //Die speziellen mpi-Variablen.
  int from = arguments->from;
  int size = arguments->size;
  int rank = arguments->rank;
  int num_rows = arguments->num_rows;

  /* initialize matrix/matrices with zeros */
  for (g = 0; g < arguments->num_matrices; g++)
  {
    for (i = 0; (int) i < num_rows; i++) //Es git Num_rows Zeilen. 0 und num_rows -1 sind die hollow lines.
    {
      for (j = 0; j <= N; j++)
      {
        Matrix[g][i][j] = 0.0;
      }
    }
  }

  if (options->inf_func == FUNC_F0)
  {
    for (g = 0; g < arguments->num_matrices; g++)
    {
      for (i = 1;(int) i < num_rows-1; i++)     //initialisiert alle Zeilen außer die hollow lines.
      {
        Matrix[g][i][0] = 1.0 - (h * (i + from -1));  //0. Spalte
        Matrix[g][i][N] = h * (i + from - 1);   //letzte Spalte
      }
      
      if(rank == 0)
      {

        for(i = 0; i <= N; i++)
        {
          Matrix[g][0][i] = 1.0 - (h * i);  //0. Zeile
        }

        Matrix[g][0][N] = 0.0;        //rechte Ecke korrigieren
      }

      if(rank == size - 1)
      {
        for(i = 0; i <= N; i ++)
        {
          Matrix[g][num_rows-1][i] = h * i;     //letzte Zeile
        }
        Matrix[g][num_rows-1][0] = 0.0;     //linke Ecke korrigieren
      }
    }
  }
  
  //Hier werden die benötigten hollow lines empfangen/gesendet.
  int predecessor = (rank == 0)? NOBODY : rank - 1;
  int successor = (rank == size-1)? NOBODY : rank + 1;

  
  for (g = 0; g < arguments->num_matrices; g++)
  {

    //Vorletzte Zeile wird an den nächsten Prozess an 0. Zeile (erste Zeile) eingefügt
    if(successor != NOBODY)
    {
      MPI_Send(
        Matrix[g][num_rows-2],  //num_rows-1 ist ja seine eigene hollow line!!
        N+1,
        MPI_DOUBLE,
        successor,
        g + 3,
        MPI_COMM_WORLD);
    }
  
    if(predecessor != NOBODY)
    {
      MPI_Recv(
        Matrix[g][0],
        N+1,
        MPI_DOUBLE,
        predecessor,
        g + 3,
        MPI_COMM_WORLD,
        NULL);
    }

    //1. Zeile (bzw. die zweite Zeile) wird an den vorherigen Prozess an der letzte Zeile eingefügt
    
    if(predecessor != NOBODY)
    {
      MPI_Send(
        Matrix[g][1], //Matrix[g][0] ist ja seine eigene hollow line!!
        N+1,
        MPI_DOUBLE,
        predecessor,
        2 * g + 4,
        MPI_COMM_WORLD);
    }
  
    if(successor != NOBODY)
    {
      MPI_Recv(
        Matrix[g][num_rows-1],
        N+1,
        MPI_DOUBLE,
        successor,
        2 * g + 4,
        MPI_COMM_WORLD,
        NULL);
    }
  }
}


/* ************************************************************************ */
/* calculate: solves the equation                                           */
/* ************************************************************************ */
static
void
calculate (struct calculation_arguments const* arguments, struct calculation_results *results, struct options const* options)
{
  int i, j;                                   /* local variables for loops  */
  int m1, m2;                                 /* used as indices for old and new matrices       */
  double star;                                /* four times center value minus 4 neigh.b values */
  double residuum;                            /* residuum of current iteration                  */
  double maxresiduum;                         /* maximum residuum value of a slave in iteration */

  int const N = arguments->N;
  double const h = arguments->h;

  double pih = 0.0;
  double fpisin = 0.0;

  int term_iteration = options->term_iteration;

  /* initialize m1 and m2 depending on algorithm */
  if (options->method == METH_JACOBI)
  {
    m1 = 0;
    m2 = 1;
  }
  else
  {
    m1 = 0;
    m2 = 0;
  }

  if (options->inf_func == FUNC_FPISIN)
  {
    pih = PI * h;
    fpisin = 0.25 * TWO_PI_SQUARE * h * h;
  }

  while (term_iteration > 0)
  {
    double** Matrix_Out = arguments->Matrix[m1];
    double** Matrix_In  = arguments->Matrix[m2];

    maxresiduum = 0;

    /* over all rows */
    for (i = 1; i < N; i++)
    {
      double fpisin_i = 0.0;

      if (options->inf_func == FUNC_FPISIN)
      {
        fpisin_i = fpisin * sin(pih * (double)i);
      }

      /* over all columns */
      for (j = 1; j < N; j++)
      {
        star = 0.25 * (Matrix_In[i-1][j] + Matrix_In[i][j-1] + Matrix_In[i][j+1] + Matrix_In[i+1][j]);

        if (options->inf_func == FUNC_FPISIN)
        {
          star += fpisin_i * sin(pih * (double)j);
        }

        if (options->termination == TERM_PREC || term_iteration == 1)
        {
          residuum = Matrix_In[i][j] - star;
          residuum = (residuum < 0) ? -residuum : residuum;
          maxresiduum = (residuum < maxresiduum) ? maxresiduum : residuum;
        }

        Matrix_Out[i][j] = star;
      }
    }

    results->stat_iteration++;
    results->stat_precision = maxresiduum;

    /* exchange m1 and m2 */
    i = m1;
    m1 = m2;
    m2 = i;

    /* check for stopping calculation, depending on termination method */
    if (options->termination == TERM_PREC)
    {
      if (maxresiduum < options->term_precision)
      {
        term_iteration = 0;
      }
    }
    else if (options->termination == TERM_ITER)
    {
      term_iteration--;
    }
  }

  results->m = m2;
}


/* ************************************************************************ */
/* calculate_gauss: solves the equation                                           */
/* ************************************************************************ */
static
void
calculate_gauss (struct calculation_arguments const* arguments, struct calculation_results *results, struct options const* options)
{
  int i, j;                                   /* local variables for loops  */
  int m1, m2;                                 /* used as indices for old and new matrices       */
  double star;                                /* four times center value minus 4 neigh.b values */
  double residuum;                            /* residuum of current iteration                  */
  double localmaxresiduum;                    /* maximum residuum value of a slave in iteration */
  double globalmaxresiduum;

  int const N = arguments->N;
  double const h = arguments->h;

  double pih = 0.0;
  double fpisin = 0.0;

  //MPI-Variables
  int size = arguments->size;
  int rank = arguments->rank;
  int from = arguments->from;
  int num_rows = arguments->num_rows;
  int predecessor = (rank == 0)? NOBODY : rank - 1;
  int successor = (rank == size-1)? NOBODY : rank + 1;

  int term_iteration = options->term_iteration;
  int kind_of_termination = options->termination;

  m1 = 0;
  m2 = 0;

  globalmaxresiduum = 0;

  if (options->inf_func == FUNC_FPISIN)
  {
    pih = PI * h;
    fpisin = 0.25 * TWO_PI_SQUARE * h * h;
  }

  while (term_iteration > 0)
  {

    double** Matrix_Out = arguments->Matrix[m1];
    double** Matrix_In  = arguments->Matrix[m2];

    //Hier warten alle auf die erste hollow line (außer P0) des vorherigen Prozesses
    if(predecessor != NOBODY)
    {
      MPI_Recv(
        Matrix_In[0],
        N+1,
        MPI_DOUBLE,
        predecessor,
        22,
        MPI_COMM_WORLD,
        NULL);
    }

    localmaxresiduum = 0;

    /* over all rows */
    for (i = 1; i < num_rows - 1; i++)
    {
      double fpisin_i = 0.0;

      if (options->inf_func == FUNC_FPISIN)
      {
        fpisin_i = fpisin * sin(pih * (double) (i + (from -1)));
      }

      /* over all columns */
      for (j = 1; j < N; j++)
      {
        star = 0.25 * (Matrix_In[i-1][j] + Matrix_In[i][j-1] + Matrix_In[i][j+1] + Matrix_In[i+1][j]);

        if (options->inf_func == FUNC_FPISIN)
        {
          star += fpisin_i * sin(pih * (double)j);
        }

        if (options->termination == TERM_PREC || term_iteration == 1)
        {
          residuum = Matrix_In[i][j] - star;
          residuum = (residuum < 0) ? -residuum : residuum;
          localmaxresiduum = (residuum < localmaxresiduum) ? localmaxresiduum : residuum;
        }


        Matrix_Out[i][j] = star;
      }
      //Falls die erste berechnete Line geschickt wird, kann sie als hollow line für den vorherigen
      //Prozess gesendet werden
      if((i == 1) && (predecessor != NOBODY) && ((term_iteration - 1) > 0) )
      {
        MPI_Send(
          Matrix_Out[1],  //Matrix[g][0] ist ja seine eigene hollow line!!
          N+1,
          MPI_DOUBLE,
          predecessor,
          21,
          MPI_COMM_WORLD);
      }
    }

    results->stat_iteration++;
    results->stat_precision = localmaxresiduum;

    /* exchange m1 and m2 */
    i = m1;
    m1 = m2;
    m2 = i;

          if (kind_of_termination == TERM_ITER)
    {
      term_iteration--;
    }
    
    if (kind_of_termination == TERM_PREC)
    {
      //Hier treffen sich alle parallel laufenden
      //Prozesse, um sich gegenseitig mitzuteilen
      //ob die term_precision erreicht wurde
      //(Sind natürlich in unterschiedlichen Iterationen)
      //Abstände des Treffens sind size Iterationen

      if((int) results->stat_iteration >= size - rank)
      {
        int local_kind_of_termination = kind_of_termination;
        if (localmaxresiduum < options->term_precision)
        {
          local_kind_of_termination = TERM_ITER;
        }
        //Falls einer es schafft, so wechselt man
        //in den Iterationsabbruch
        MPI_Allreduce(  &local_kind_of_termination,
            &kind_of_termination,
            1,
            MPI_INT,
            MPI_MIN,
            MPI_COMM_WORLD);

        //und bringt alle Prozesse auf die Iteration
        //von P0
        if(kind_of_termination == TERM_ITER)
        {
          term_iteration = rank;
        }
      }

    }


    if(successor != NOBODY)
    {
      //Hier wird die letzte line für die erste hollow line an den nächsten
      //Prozess gesendet
      MPI_Send(
        Matrix_Out[num_rows-2], //num_rows-1 ist ja seine eigene hollow line!!
        N+1,
        MPI_DOUBLE,
        successor,
        22,
        MPI_COMM_WORLD);
      
      //Die zweite hollow line wird benötigt, wenn man eine weitere Iteration
      //durchgehen muss
      if(term_iteration > 0)
      {
        MPI_Recv(
          Matrix_In[num_rows-1],
          N+1,
          MPI_DOUBLE,
          successor,
          21,
          MPI_COMM_WORLD,
          NULL);
      }
    }
  }

  //Hier wird das globale maxresiduum ermittelt
  if(term_iteration < 1)
  {
    MPI_Allreduce(  &localmaxresiduum,
        &globalmaxresiduum,
        1,
        MPI_DOUBLE,
        MPI_MAX,
        MPI_COMM_WORLD);
  
    results->stat_precision = globalmaxresiduum;
  }

  results->m = m2;
}

/* ************************************************************************ */
/*  displayStatistics: displays some statistics about the calculation       */
/* ************************************************************************ */
static
void
displayStatistics (struct calculation_arguments const* arguments, struct calculation_results const* results, struct options const* options)
{
  int N = arguments->N;
  double time = (comp_time.tv_sec - start_time.tv_sec) + (comp_time.tv_usec - start_time.tv_usec) * 1e-6;

  printf("Berechnungszeit:    %f s \n", time);
  printf("Speicherbedarf:     %f MiB\n", (N + 1) * (N + 1) * sizeof(double) * arguments->num_matrices / 1024.0 / 1024.0);
  printf("Berechnungsmethode: ");

  if (options->method == METH_GAUSS_SEIDEL)
  {
    printf("Gauss-Seidel");
  }
  else if (options->method == METH_JACOBI)
  {
    printf("Jacobi");
  }

  printf("\n");
  printf("Interlines:         %" PRIu64 "\n",options->interlines);
  printf("Stoerfunktion:      ");

  if (options->inf_func == FUNC_F0)
  {
    printf("f(x,y) = 0");
  }
  else if (options->inf_func == FUNC_FPISIN)
  {
    printf("f(x,y) = 2pi^2*sin(pi*x)sin(pi*y)");
  }

  printf("\n");
  printf("Terminierung:       ");

  if (options->termination == TERM_PREC)
  {
    printf("Hinreichende Genaugkeit");
  }
  else if (options->termination == TERM_ITER)
  {
    printf("Anzahl der Iterationen");
  }

  printf("\n");
  printf("Anzahl Iterationen: %" PRIu64 "\n", results->stat_iteration);
  printf("Norm des Fehlers:   %e\n", results->stat_precision);
  printf("\n");
}

/****************************************************************************/
/** Beschreibung der Funktion DisplayMatrix:                               **/
/**                                                                        **/
/** Die Funktion DisplayMatrix gibt eine Matrix                            **/
/** in einer "ubersichtlichen Art und Weise auf die Standardausgabe aus.   **/
/**                                                                        **/
/** Die "Ubersichtlichkeit wird erreicht, indem nur ein Teil der Matrix    **/
/** ausgegeben wird. Aus der Matrix werden die Randzeilen/-spalten sowie   **/
/** sieben Zwischenzeilen ausgegeben.                                      **/
/****************************************************************************/
static
void
DisplayMatrix (struct calculation_arguments* arguments, struct calculation_results* results, struct options* options)
{
  int x, y;

  double** Matrix = arguments->Matrix[results->m];

  int const interlines = options->interlines;

  printf("Matrix:\n");

  for (y = 0; y < 9; y++)
  {
    for (x = 0; x < 9; x++)
    {
      printf ("%7.4f", Matrix[y * (interlines + 1)][x * (interlines + 1)]);
    }

    printf ("\n");
  }

  fflush (stdout);
}

/**
 * rank and size are the MPI rank and size, respectively.
 * from and to denote the global(!) range of lines that this process is responsible for.
 *
 * Example with 9 matrix lines and 4 processes:
 * - rank 0 is responsible for 1-2, rank 1 for 3-4, rank 2 for 5-6 and rank 3 for 7.
 *   Lines 0 and 8 are not included because they are not calculated.
 * - Each process stores two hollow lines in its matrix (except for ranks 0 and 3 that only store one).
 * - For instance: Rank 2 has four lines 0-3 but only calculates 1-2 because 0 and 3 are hollow lines for other processes. It is responsible for (global) lines 5-6.
 */
static
void
DisplayMatrix2 (struct calculation_arguments* arguments, struct calculation_results* results, struct options* options, int rank, int size, int from, int to)
{
  //printf("Displaying - rank %d: from %d to %d\n", rank, from, to);
  
  int const elements = 8 * options->interlines + 9;

  int x, y;
  double** Matrix = arguments->Matrix[results->m];
  MPI_Status status;

  /* first line belongs to rank 0 */
  if (rank == 0)
    from--;

  /* last line belongs to rank size - 1 */
  if (rank + 1 == size)
    to++;

  if (rank == 0)
    printf("Matrix:\n");

  for (y = 0; y < 9; y++)
  {
    int line = y * (options->interlines + 1);

    //printf("DM: Doing MPI stuff... line:%d\n", line);
    if (rank == 0)
    {
      /* check whether this line belongs to rank 0 */
      if (line < from || line > to)
      {
        /* use the tag to receive the lines in the correct order
         * the line is stored in Matrix[0], because we do not need it anymore */
        MPI_Recv(Matrix[0], elements, MPI_DOUBLE, MPI_ANY_SOURCE, 42 + y, MPI_COMM_WORLD, &status);
      }
    }
    else
    {
      if (line >= from && line <= to)
      {
        /* if the line belongs to this process, send it to rank 0
         * (line - from + 1) is used to calculate the correct local address */
        MPI_Send(Matrix[line - from + 1], elements, MPI_DOUBLE, 0, 42 + y, MPI_COMM_WORLD);
      }
    }

    //printf("MPI stuff finished\n");

    if (rank == 0)
    {
      for (x = 0; x < 9; x++)
      {
        int col = x * (options->interlines + 1);

        if (line >= from && line <= to)
        {
          /* this line belongs to rank 0 */
          printf("%7.4f", Matrix[line][col]);
        }
        else
        {
          /* this line belongs to another rank and was received above */
          printf("%7.4f", Matrix[0][col]);
        }
      }

      printf("\n");
    }
  }

  //printf("DisplayMatrix2 finished");

  fflush(stdout);
}

/* ************************************************************************ */
/*  main                                                                    */
/* ************************************************************************ */
int
main (int argc, char** argv)
{
  struct options options;
  struct calculation_arguments arguments;
  struct calculation_results results;

  MPI_Init(&argc, &argv);                         // MPI initialisieren
  MPI_Comm_rank(MPI_COMM_WORLD, &arguments.rank);           // Nummer des P holen 
  MPI_Comm_size(MPI_COMM_WORLD, &arguments.size);           // Anzahl der P holen
  int rank = arguments.rank;

  if(arguments.rank == 0)
  {
    /* get parameters */
    AskParams(&options, argc, argv);              
  }
  
  //Der unschöne Weg! Broadcastet jede Variable in options.
  MPI_Bcast(&(options.number),1, MPI_UINT64_T, 0, MPI_COMM_WORLD);
  MPI_Bcast(&(options.method),1,MPI_UINT64_T, 0, MPI_COMM_WORLD);
  MPI_Bcast(&(options.interlines),1, MPI_UINT64_T, 0, MPI_COMM_WORLD);
  MPI_Bcast(&(options.inf_func),1, MPI_UINT64_T, 0, MPI_COMM_WORLD);
  MPI_Bcast(&(options.termination),1, MPI_UINT64_T, 0, MPI_COMM_WORLD);
  MPI_Bcast(&(options.term_iteration),1, MPI_UINT64_T, 0, MPI_COMM_WORLD);
  MPI_Bcast(&(options.term_precision),1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  
  initVariables(&arguments, &results, &options);           

  if ((options.method == METH_JACOBI) && (arguments.rank == 0))     // Bei nicht Jacobi einfach das Vorherige mit P 0 tun.
  {

    allocateMatrices(&arguments);             /*  get and initialize variables and matrices  */
    initMatrices(&arguments, &options);          

    gettimeofday(&start_time, NULL);                    /*  start timer         */
    calculate(&arguments, &results, &options);          /*  solve the equation  */
    gettimeofday(&comp_time, NULL);                     /*  stop timer          */

    displayStatistics(&arguments, &results, &options);
    DisplayMatrix(&arguments, &results, &options);

    freeMatrices(&arguments);                           /*  free memory     */
  }

  if ((options.method == METH_GAUSS_SEIDEL))
  {

  initMpiVariables (&arguments);
  allocateMpiMatrices(&arguments); //TODO: Allokiert zu viel! Da kein Platzmangel, schadet es auch nicht.
  initMpiMatrices (&arguments,&options);
  if(rank == 0) gettimeofday(&start_time, NULL);                    /*  start timer         */

  calculate_gauss (&arguments,&results,&options);

  if(rank == 0) gettimeofday(&comp_time, NULL);                     /*  stop timer          */
  if(rank == 0) displayStatistics(&arguments, &results, &options);

  DisplayMatrix2 (&arguments,&results,&options,arguments.rank,arguments.size,arguments.from,arguments.to);
  freeMatrices(&arguments);
  }

  MPI_Finalize();             //beendet MPI

  return 0;
}
