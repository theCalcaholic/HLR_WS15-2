#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

void
print_rand_array(char* text, int rank,int maxnumber,int size,int* buf)
{

  if(rank == 0)
  {
  	printf("\n%s\n",text);
	
	printf("rank %d\n", rank);
	for(int k = 0; buf[k] < 25 ;k++) {
		printf(" %d,", buf[k]);
	}
  	for (int i = 1; i < size; i++)
  	{
		int randmessage[maxnumber+1];
	  	MPI_Recv (randmessage,maxnumber+1,MPI_INT,i,100,MPI_COMM_WORLD,NULL);
			printf("\nrank %d, \n", i);
	  	for(int l = 0;randmessage[l] < 25; l++) {
				printf(" %d,", randmessage[l]);
			}
  	}
  }
  else MPI_Send(buf, maxnumber+1, MPI_INT, 0, 100, MPI_COMM_WORLD);	
}

int*
init (int number,int maxnumber, int rank)
{
  int* buf = malloc(sizeof(int)* (maxnumber + 1));		//Es wird Speicher für die neuen Zufallszahlen reserviert.

  srand(time(NULL)+ rank);					//Der Seed wird mit rank addiert, da ansonsten Prozesse gleiche Zufallszahlen
  								//beinhalten können (Bei gleichzeitiger Benutztung in der selben Sekunde).
  
  int i;
  for (i = 0; i < number;i++)
  {
    buf[i] = rand() % 25; 					//do not modify %25
  }
  buf[i] = 25;							//25 ist das Ende der Zufallszahlen (Es darf ja keine 25 geben).
  return buf;
}

void*
circle (int* buf,int rank,int predecessor, int successor, int size, int maxnumber)
{
	
  int* newbuf = malloc(sizeof(int) * (maxnumber + 1)); 						//ein neuer buf für das Erhalten von neuen buffs. 
  int special_integer;										//Die Integerzahl, worauf der letzte Prozess wartet.
  int cycle_end;										//Boolean für das Ende des Zyklen
  
  if(rank == size-1) MPI_Recv (&special_integer,1,MPI_INT,successor,11,MPI_COMM_WORLD,NULL);	//letzter P empfängt diese spezielle Zahl 
  if(rank == 0) MPI_Send(&buf[0], 1, MPI_INT, predecessor, 11, MPI_COMM_WORLD);			//P 0 sendet die speziellen Zahl
  if(rank == size-1) cycle_end = special_integer == newbuf[0];					//letzter P überprüft, ob Bedinung erfüllt ist
  MPI_Bcast(&cycle_end,1,MPI_INT,size-1, MPI_COMM_WORLD);					//und teilt das jeden Prozess mit.

 //Rotieren der Zufallszahlen.
  while(!cycle_end)
  {
	  if(rank != 0)
	  { 
		MPI_Recv(newbuf,maxnumber + 1,MPI_INT,predecessor,3,MPI_COMM_WORLD,NULL);	//Alle außer der P 0 warten auf den buf
		MPI_Send(buf,maxnumber + 1,MPI_INT,successor,3,MPI_COMM_WORLD);			//und senden es anschließend
	  }
	  else
	  {
		MPI_Send(buf,maxnumber + 1,MPI_INT,successor,3,MPI_COMM_WORLD);			//P 0 gibt den Anstoß
		MPI_Recv(newbuf,maxnumber + 1,MPI_INT,predecessor,3,MPI_COMM_WORLD,NULL);	//und wartet auf den letzten P
	  } 
	  MPI_Barrier(MPI_COMM_WORLD);								//Alle P warten auf P 0 bzw. letzten P
	  if(rank == size-1) cycle_end = special_integer == newbuf[0];				//letzter P überprüft ob Bedingung erfüllt ist
	  MPI_Bcast(&cycle_end,1,MPI_INT,size-1, MPI_COMM_WORLD); 				//teilt das Ergebnis allen mit
	  buf = newbuf;										//der buf wird nun erneut
	  newbuf = malloc(sizeof(int) * (maxnumber + 1));					//Empfangbuf wird anderen neuen Platz geschaffen
	  MPI_Barrier(MPI_COMM_WORLD);								//Alle P. werden synchronisiert
  }
  return buf;											//der buf wird anschließend zurückgegeben.
}

int
main (int argc, char** argv)
{
  char arg[256];				//reserviert für die angegebene Länge des Arrays 256 Zeichen bzw. die Maximalgröße von 10^256
  int N; 					//Die Länge des eindimensionales Array

  						//prozessspezifische Variablen
  int rank; 					//Die Nummer des Prozesses
  int successor; 				//Die Nummer des Nachfolgers
  int predecessor;				//Die Nummer des Vorgängers
  int size; 					//Die Anzahl der Prozesse
  int number_of_rand; 				//Anzahl der Zufallszahlen, die der Prozess momentan enthält.
  int number_of_rand_max;			//Anzahl der Zufallszahlen, die ein Prozess enthalten kann.
  int* buf; 					//Ein Puffer für die Zufallszahlen für jeden Prozess

  
  if (argc < 2)					//Es sollte mindestens ein Parameter angegeben werden.
  {
    printf("Arguments error\n");		//ansonsten gib Fehlermeldung aus 
    return EXIT_FAILURE;			//und beende das Programm
  }

  sscanf(argv[1], "%s", arg);			//Der erste Parameter (Die angegebene Länge des Arrays) wird nun in arg geschrieben.

  //array length
  N = atoi(arg); 				//die angegebene Länge des Arrays wird in einen Integer umgewandelt und N übergebenen
  
  MPI_Init(&argc, &argv); 			//MPI initialisieren
  MPI_Comm_rank(MPI_COMM_WORLD, &rank); 	//Nummer des Prozesses holen
  MPI_Comm_size(MPI_COMM_WORLD, &size);		//Anzahl der Prozesse holen
  successor = (size+rank+1) %size;		//Nachfolger bestimmen
  predecessor = (size+rank-1) %size;		//Vorgänger bestimmen
  number_of_rand = N % size <= rank ?  N / size : N / size + 1;//bestimmt die Anzahl der Zufallszahlen, die der Prozess erstellen soll
  number_of_rand_max = N / size + 1;

  buf = init(number_of_rand,number_of_rand_max,rank);//ermittelt number_of_rand Zufallszahlen.

  print_rand_array("Before",rank,number_of_rand_max,size,buf);
  if(size > 1) buf = circle(buf,rank,predecessor,successor,size, number_of_rand_max);
  print_rand_array("After",rank,number_of_rand_max,size,buf);
  MPI_Finalize();
  return EXIT_SUCCESS;
}
