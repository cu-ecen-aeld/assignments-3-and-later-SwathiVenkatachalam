/*
 * Filename   : writer.c
 * Description: C Code that writes input text string to input filename on the filesystem (where full file path is given by user)
 *            : Debug and error messages syslogged
 * Author     : Swathi Venkatachalam
 * Reference  : For open log - https://www.man7.org/linux/man-pages/man3/syslog.3.html
 *            : For fopen - https://www.geeksforgeeks.org/basics-file-handling-c/
 *            : For fprintf - https://www.geeksforgeeks.org/fprintf-in-c/
 */

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#define REQ_ARGS (3)
#define FAILURE  (1)
#define SUCCESS  (0)

int main(int argc, char* argv[])
{
	//opens a connection to the system logger for a program
	openlog(NULL, LOG_PID, LOG_USER); //openlog(ident, option, facility)
	// ident: string pointed to is prepended to every message; default NULL - program name is used
	// option: bit mask constructed by ORing together any val; LOG_PID - includes caller's PID with each message
	// facility: specify what type of program is logging the message; default LOG_USER - generic user-level messages
	
	if (argc != REQ_ARGS) //checks for specified args
	{
        syslog(LOG_ERR,"Required arguments are 3, Specify correctly!\n"); //syslog error
		printf("Error! Correct Usage: ./writer <writefilepath> <writestr>\n"); //prints error
        return FAILURE; //Exits with value 1 error
	}
	
	char *writepath = argv[1]; //get 1st arg file path to write str
	char *writestr = argv[2]; //get 2nd arg str to write
	
	//FILE* file_ptr = fopen(const char *file_name, const char *access_mode);
	FILE* file_ptr; //stores val ret by fopen
	if ((file_ptr = fopen(writepath, "w+")) == NULL) //opens file in write mode (contents overwritten) and checks if error
	{
        syslog(LOG_ERR,"Error while opening given file!\n"); //syslog error
		printf("Error! Opening given file\n"); //prints error
		return FAILURE; //Exits with value 1 error
	}		
	
	if ((fprintf(file_ptr, "%s\n", writestr)) < 0) //write str to file and check if it's error (negative val returned)
	{
		syslog(LOG_ERR,"Error writing to file!\n"); //syslog error
		printf("Error! Writing to file\n"); //prints error
		return FAILURE; //Exits with value 1 error
	}		
	syslog(LOG_DEBUG,"Writing %s to %s!\n", writestr, writepath);
	
	fclose(file_ptr); //close file
	closelog(); //log close
	
	return SUCCESS; //success
}
