#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

void
print_rand_array(char* text, int rank, int number_of_rand, int size,int* buf)
{

  if(rank == 0)
  {
  	printf("\n%s\n",text);
	
	for(int k = 0; k < number_of_rand;k++)
	{
		printf("rank %d: %d\n", rank, buf[k]);
	}
  	for (int i = 1; i < size; i++)
  	{
	  int number;
	  MPI_Recv (&number,1,MPI_INT,i,1,MPI_COMM_WORLD,NULL);

	  int randmessage[number];
	  MPI_Recv (randmessage,number,MPI_INT,i,100,MPI_COMM_WORLD,NULL);

	  for(int l = 0; l < number; l++)
	  {
    	    	printf ("rank %d: %d\n", i, randmessage[l]);
	  }
  	}
  }
  else
  {
	MPI_Send(&number_of_rand, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
	MPI_Send(buf, number_of_rand, MPI_INT, 0, 100, MPI_COMM_WORLD);	
  }
}

int*
init (int number, int rank)
{
  //todo
  int* buf = malloc(sizeof(int)* number);		//Es wird Speicher für die neuen Zufallszahlen reserviert.

  srand(time(NULL)+ rank);				//Prozesse erhalten ohne rank das Selbe.

  for (int i = 0; i < number; i++)
  {
    buf[i] = rand() % 25; //do not modify %25
  }

  return buf;
}

void
circle (int* buf,int number,int rank,int predecessor, int successor, int size)
{
	
  int* bufcpy = malloc(sizeof(int)* number);						//Eine Copy des buf für Prozess size-1
  int numbercpy = number;								//Ein Copy für die Anzahl der Zufallszahlen für size-1
  int special_integer;									//Die Integerzahl, worauf der letzte Prozess wartet.
  if(rank == size-1)
  {
	 int* special_integer_buf = malloc(sizeof(int));				//Zeiger für den Erhalt der special_integer
 	 MPI_Recv (special_integer_buf,1,MPI_INT,successor,11,MPI_COMM_WORLD,NULL);	//Empfangen der Zahl 
	 special_integer = special_integer_buf[0];					//Merken dieser Zahl
  }
  if(rank == 0)
  {
	MPI_Send(&buf[0], 1, MPI_INT, predecessor, 11, MPI_COMM_WORLD);			//Senden der speziellen Zahl
  } 
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

  buf = init(number_of_rand,rank);//ermittelt number_of_rand Zufallszahlen.

  print_rand_array("Before",rank,number_of_rand,size,buf);
// circle(buf);
circle (buf,number_of_rand,rank,predecessor,successor,size);
  print_rand_array("After",rank,number_of_rand,size,buf);
  MPI_Finalize();
  return EXIT_SUCCESS;
}
