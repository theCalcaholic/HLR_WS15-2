#include <mpi.h>
#include <malloc.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sys/unistd.h>

//MasterID
const int MID = 0;
const int MESSAGE_LENGTH = 47;

int main(int argc, char **argv)
{
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
	//Einige Konstanten für die Message
	const int TIME_LENGTH = 27;
	const int HOSTNAME_LENGTH = 20;

	//Generiert den Zeitstring			
	char time_except_usec[TIME_LENGTH - 7], 
	     time[TIME_LENGTH];
	struct timeval tv;
	struct tm *time_except_usec_data;
	gettimeofday(&tv,NULL);
	time_except_usec_data = localtime(&tv.tv_sec);
	strftime(time_except_usec, sizeof time_except_usec,
		"%Y-%m-%d %H:%M:%S", time_except_usec_data);
	snprintf(time, sizeof time,
		"%s.%06ld", time_except_usec, tv.tv_usec);

	//Generiert den Hostname
	char hostname[HOSTNAME_LENGTH];
	gethostname(hostname, sizeof hostname);

	//Merget nun Hostname und Zeistring
	char message[MESSAGE_LENGTH];
	snprintf(message, sizeof message, "%s : %s", hostname, time);
	
	//Sendet 
	MPI_Send(message, MESSAGE_LENGTH,MPI_CHAR,MID,
	0, MPI_COMM_WORLD);	
  }	  
  //Der Masterprozess empfängt alle Message der Reihe nach
  //und printet alle Ergebnisse zurück.
  else
  {
	int i;
	char *buf = malloc(sizeof(char) * MESSAGE_LENGTH);
	for (i= 1; i < number_of_p; ++i)
	{
		
		MPI_Recv(buf, MESSAGE_LENGTH,
		MPI_CHAR, i,0,
		MPI_COMM_WORLD,NULL);
		
		printf("%s\n",(char*)buf);
     	}
	free(buf);	
  }
  //Hier warten nun alle Prozesse
  MPI_Barrier(MPI_COMM_WORLD);
  printf("Rang %d beendet jetzt\n", p_id);

  //Beendet die MPI-Umgebung
  MPI_Finalize();

  return 0;
}