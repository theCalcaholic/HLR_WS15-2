#include <mpi.h>
#include <malloc.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sys/unistd.h>
#include <math.h>

//MasterID
const int MID = 0;
const int MESSAGE_LENGTH = 47;

int main(int argc, char **argv)
{
  //Die Id des Prozesses und die Anzahl der Prozesse.
  int p_id;
  int number_of_p;

  //Die lokalen Eintreffzeiten und die
  //kürzeste Eintreffzeit eines Prozess 
  long usec_local_min = 0;
  long usec_global_min = 0;

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
		MPI_Send(message, MESSAGE_LENGTH,MPI_CHAR, MID, 0, MPI_COMM_WORLD);	

		//Merkt sich die benötigte Eintreffzeit	
		usec_local_min = tv.tv_usec;
	}	  
  //Der Masterprozess empfängt alle Message der Reihe nach
  //und printet alle Ergebnisse zurück.
  else
  {
		//Der MID soll nicht mitgerechnet werden!
		usec_local_min = INFINITY; 
		int i;
		char *buf = malloc(sizeof(char) * MESSAGE_LENGTH);
		for (i= 1; i < number_of_p; ++i)
		{
			
			MPI_Recv(buf, MESSAGE_LENGTH, MPI_CHAR, i, 0, MPI_COMM_WORLD, NULL);
			
			printf("%s\n",(char*)buf);
		}
		free(buf);	
  }

  //Hier warten nun alle Prozesse außer der Masterprozess
  MPI_Barrier(MPI_COMM_WORLD);

  //Masterprozess ermittelt die kürzeste Eintreffzeit
  MPI_Reduce(&usec_local_min, &usec_global_min, 1, MPI_LONG, MPI_MIN, MID,MPI_COMM_WORLD);
  if(p_id == MID)
  {
	printf("%06ld\n", usec_global_min);
  }

  //Erst wenn der Masterprozess fertig ist, dürfen alle andere Prozesse weitermachen.
  MPI_Barrier(MPI_COMM_WORLD);

  printf("Rang %d beendet jetzt\n", p_id);

  //Beendet die MPI-Umgebung
  MPI_Finalize();

  return 0;
}
