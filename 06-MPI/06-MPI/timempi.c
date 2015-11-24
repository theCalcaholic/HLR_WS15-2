#include <mpi.h>
#include <malloc.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <sys/unistd.h>
#include <stdlib.h>


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
	char *new_message = malloc(sizeof(char) * MESSAGE_LENGTH);
	char *message_to_shell = malloc(sizeof(char) * MESSAGE_LENGTH * number_of_p + 17 + 2 * number_of_p);
	char echo[10] = "printf\ '";
	strcpy(message_to_shell,echo);
	int i;
	for (i= 1; i < number_of_p; ++i)
	{
		MPI_Recv(new_message, MESSAGE_LENGTH,
		MPI_CHAR, i,0,
		MPI_COMM_WORLD,NULL);
		strcat(message_to_shell,new_message);
		strcat(message_to_shell,"\n");
     	}
	strcat(message_to_shell,"' ; wait");
	system(message_to_shell);
	free(new_message);
	free(message_to_shell);	
  }
  //Hier warten nun alle Prozesse
  MPI_Barrier(MPI_COMM_WORLD);
  printf("Rang %d beendet jetzt\n", p_id);

  //Beendet die MPI-Umgebung
  MPI_Finalize();

  return 0;
}
