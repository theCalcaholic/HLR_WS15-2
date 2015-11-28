#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

const int PRINT_TAG = 1; // Tag for printing ints in correct order
const int CIRCLE_TAG = 2; //Tag for sending/receiving new values
const int TARGETINT_TAG = 3; //Tag for informing last rank about target value


void print_rand_array(char* text, int rank, int maxnumber, int size, int* buf)
{
	int message[maxnumber+1];
  if(rank == 0)
  {
    printf("\n%s\n",text);
		printf("rank 0\n");
  
		for(int k = 0; buf[k] < 25; k++) {
			printf(" %d,", buf[k]);
		}
		for (int i = 1; i < size; i++){
			MPI_Recv (message, maxnumber+1, MPI_INT, i, PRINT_TAG, MPI_COMM_WORLD, NULL);
			printf("\nrank %d: \n", i);
			for(int l = 0; message[l] < 25; l++) {
				printf(" %d,", message[l]);
			}
		}
  }
  else {
		MPI_Send(buf, maxnumber+1, MPI_INT, 0, PRINT_TAG, MPI_COMM_WORLD); 
	}
	if( rank==0 ) printf("\n");
}

int* init_buffer (int number, int maxnumber, int rank)
{
  int* buf = malloc(sizeof(int)* (maxnumber));    //Es wird Speicher für die neuen Zufallszahlen reserviert.

  srand(time(NULL)+ rank);  //Der Seed wird mit rank addiert, da ansonsten Prozesse gleiche Zufallszahlen
                            //beinhalten können (Bei gleichzeitiger Benutztung in der selben Sekunde).
  int i;
  for (i = 0; i < number; i++)
  {
    buf[i] = rand() % 25;           //do not modify %25
  }
	buf[i] = 25; //markiert das Ende der Zufallszahlen
  return buf;
}

void* circle (int* buf,int rank, int predecessor, int successor, int size, int maxnumber)
{
  
  int* newbuf = malloc(sizeof(int) * (maxnumber + 1));            //ein neuer buf für das Erhalten von neuen buffs. 
  int special_integer;                    //Die Integerzahl, worauf der letzte Prozess wartet.
  int cycle_end;                    //Boolean für das Ende des Zyklen
  MPI_Request request;
  MPI_Status status;
  
  if(rank == 0) {
		//printf("Rank 0 sending to rank %d of %d...\n", predecessor, size);
		MPI_Send(&buf[0], 1, MPI_INT, predecessor, TARGETINT_TAG, MPI_COMM_WORLD);     //P 0 sendet die spezielle Zahl
	}
  if(rank == size-1) {
		//printf("Rank %d receiving special int from rank 0\n", rank);
    MPI_Recv (&special_integer, 1, MPI_INT, successor, TARGETINT_TAG, MPI_COMM_WORLD, NULL);  //letzter P empfängt diese spezielle Zahl 
    cycle_end = special_integer == newbuf[0];          //letzter P überprüft, ob Bedinung erfüllt ist
  }
  MPI_Bcast(&cycle_end, 1, MPI_INT, size-1, MPI_COMM_WORLD);         //und teilt das jeden Prozess mit.


 //Rotieren der Zufallszahlen.
  while(!cycle_end)
  {
    MPI_Isend(buf, maxnumber + 1, MPI_INT, successor, CIRCLE_TAG, MPI_COMM_WORLD, &request);
    MPI_Recv (newbuf, maxnumber + 1, MPI_INT, predecessor, CIRCLE_TAG, MPI_COMM_WORLD, NULL);  //letzter P empfängt diese spezielle Zahl 

    MPI_Wait(&request, &status);
    if(rank == size-1) {
      cycle_end = special_integer == newbuf[0];        //letzter P überprüft ob Bedingung erfüllt ist
    }
    MPI_Bcast(&cycle_end,1,MPI_INT,size-1, MPI_COMM_WORLD);         //teilt das Ergebnis allen mit

    MPI_Barrier(MPI_COMM_WORLD);                //Alle P warten auf P 0 bzw. letzten P

    buf = newbuf;                   //der buf wird nun erneut
    newbuf = malloc(sizeof(int) * (maxnumber + 1));         //Empfangbuf wird anderen neuen Platz geschaffen
  }
  return buf;                     //der buf wird anschließend zurückgegeben.
}

int main (int argc, char** argv)
{
  int N;          //Die Länge des eindimensionales Array

  //prozessspezifische Variablen
  int rank;           //Die Nummer des Prozesses
  int successor;        //Die Nummer des Nachfolgers
  int predecessor;        //Die Nummer des Vorgängers
  int size;           //Die Anzahl der Prozesse
  int number_of_rand;         //Anzahl der Zufallszahlen, die der Prozess momentan enthält.
  int number_of_rand_max;     //Anzahl der Zufallszahlen, die ein Prozess enthalten kann.
  int* buf;           //Ein Puffer für die Zufallszahlen für jeden Prozess


  if (argc < 2)         //Es sollte mindestens ein Parameter angegeben werden.
  {
    printf("Arguments error\n");    //ansonsten gib Fehlermeldung aus 
    return EXIT_FAILURE;      //und beende das Programm
  }

  //array length
  N = atoi(argv[1]);        //die angegebene Länge des Arrays wird in einen Integer umgewandelt und N übergebenen

  MPI_Init(&argc, &argv);       //MPI initialisieren
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);   //Nummer des Prozesses holen
  MPI_Comm_size(MPI_COMM_WORLD, &size);   //Anzahl der Prozesse holen
  successor = (N + rank - 1) % size;    //Nachfolger bestimmen
  predecessor = (N + rank + 1) % size;    //Vorgänger bestimmen

	//printf("rank %d: pre=%d, suc=%d", rank, predecessor, successor);

  //bestimmt die Anzahl der Zufallszahlen, die der Prozess erstellen soll
  number_of_rand = N / size;
  if( (N % size) > rank ) {
		printf("rank %d: %d %% %d: %d\n", rank, N, size, (int)(N % size));
		number_of_rand++;
  }
  number_of_rand_max = N / size + 1;

  buf = init_buffer(number_of_rand, number_of_rand_max, rank);//ermittelt number_of_rand Zufallszahlen.

	//printf("before-print started\n");
  print_rand_array("Before", rank, number_of_rand_max, size, buf);
	//printf("before-print finished\n");
  if(size > 1) {
		//printf("circle started\n");
    buf = circle(buf, rank, predecessor, successor, size, number_of_rand_max);
  }
  print_rand_array("After",rank,number_of_rand_max,size,buf);
  MPI_Finalize();
  return EXIT_SUCCESS;
}
