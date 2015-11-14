#!/bin/bash

#Erzeugt ein humandreadable effectivenessdata.txt


file1="./effectivenessdata.txt"

#Stellt sicher, dass der File leer ist, um die neue Testreihe hinzuzufügen.
if [ ! -e "$file1" ] ;
  then
	touch "$file1"
  else
	> "$file1"
fi

#Stellt sicher, dass die notwendigen Binärdateien enthalten sind
if [ ! -e "./partdiff-seq" ] && [ ! -e "./partdiff-posix" ];
  then
	echo "Binärdateien fehlen"
	exit
fi

echo -e "**********Leistungsanalyse********** 



Es werden die Ausführungszeiten von partdiff-seq und partdiff-posix überprüft

Hier wurde 3 mal partdiff-seq ausgeführt: \n">>"$file1"

for((l=1;l <= 3;l ++))
do
	 srun  ./partdiff-seq 1 2 512 2 2 65  | grep Berechnungszeit |tr -d "Berechnungszeit:"| tr -d '\n'>>"$file1"
done

echo -e "\n
Hier wurde 3 mal partdiff-posix ausgeführt. Die erste Zeile ist mit einen Thread.
Jede weitere um einen Thread inkrementiert. Es gibt 12 Zeilen (bis 12 Thread)
\n" >> "$file1"

for((j=1; j <= 12; j++))
do
	for (( k=1; k <= 3; k++ ))
	do
	  srun  ./partdiff-posix $j 2 512 2 2 65  | grep Berechnungszeit |tr -d "Berechnungszeit:"| tr -d '\n'>>"$file1" 
  	done
  echo -e "\n" >> "$file1"
done
 
		
