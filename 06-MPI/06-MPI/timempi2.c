#include <mpi.h>
#include <malloc.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

//MasterID
const int MID = 0;


int main(int argc, char **argv)
{
  //Es wird für jeden Prozess die Startzeit generiert
  struct timeval tv;

  
  //Die Id des Prozesses und die Anzahl der Prozesse.
  int p_id;
  int number_of_p;

  //Initialisierung der MPI und der Variabeln.
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &p_id);
  MPI_Comm_size(MPI_COMM_WORLD, &number_of_p);
			
  //Jeder Nebenprozess ermittelt seinen Zeitstempel 
  //und sendet es an den Masterprozess.
  if(MID != p_id)
  {
	//			
  }	  
  //Der Masterprozess empfängt alle Message der Reihe nach
  //und printet alle Ergebnisse zurück.
  else
  {
	//...
  }
  //Beendet die MPI-Umgebung
  MPI_Finalize();
}
