/*
 * Filename   : aesdsocket.c
 * Description: C Code that 
 *            : 
 * Author     : Swathi Venkatachalam
 * Reference  : [1] Socket server - https://beej.us/guide/bgnet/html/
              : [2] getaddreinfo - https://man7.org/linux/man-pages/man3/getaddrinfo.3.html
              : [3] socket - https://www.man7.org/linux/man-pages/man2/socket.2.html
              : [4] bind - https://www.man7.org/linux/man-pages/man2/bind.2.html
              : [5] listen - https://www.man7.org/linux/man-pages/man2/listen.2.html
 */

/*************************************************************************
 *                            Header Files                               *
 *************************************************************************/
#include <stdio.h>         // Standard input output library
#include <stdlib.h>        // General purpose utility functions
#include <syslog.h>        // System logging
#include <string.h>        // String manipulations

#include <sys/types.h>     // Data types used in system calls
#include <sys/socket.h>    // Socket programming

#include <unistd.h>        // POSIX API; fork
#include <signal.h>        // User interrupts (SIGINT), segmentation faults (SIGSEGV)
#include <netinet/in.h>    // IP struct; struct sockaddr_in
#include <arpa/inet.h>     // Internet addresses to text representations; inet_ntoa
#include <netdb.h>         // Network database ops; getaddrinfo

/*************************************************************************
 *                            Macros                                     *
 *************************************************************************/
#define RET_FAILURE (-1)   // Return value check
#define FAILURE     (1)
#define SUCCESS     (0)

#define PORT        ("9000")
#define DATA_FILE   ("/var/tmp/aesdsocketdata")
#define BUFFER_SIZE (128)

/*************************************************************************
 *                  Global Variables                                     *
 *************************************************************************/
int sockfd;               // Socket function return val
FILE *file_ptr;           // fopen return val
char* ip_address;         //inet_ntoa return val

/*************************************************************************
 *             Signal Handler Function                                   *
 *************************************************************************/
void signalhandler (int signal)
{
    // Logs message to the syslog “Caught signal, exiting” when SIGINT or SIGTERM is received.
    if (signal == SIGINT || signal == SIGTERM )
    {
        syslog(LOG_INFO, "Caught signal, exiting");
        close(sockfd);
        remove(DATA_FILE);
        closelog();
        exit(SUCCESS);
    }
}

/*************************************************************************
 *                       Main Function                                   *
 *************************************************************************/
int main(int argc, char* argv[])
{
    /*************************************************************************
     *                            Logging                                    *
     *************************************************************************/
    // Ref: https://www.man7.org/linux/man-pages/man3/syslog.3.html
    //opens a connection to the system logger for a program
    //openlog(ident, option, facility)
    openlog(NULL, LOG_PID, LOG_USER); // Program name, Caller's PID, default LOG_USER - generic user-level messages
    syslog(LOG_INFO,"Success: Starting log...\n");

    /*************************************************************************
     *                            Daemon mode                                *
     *************************************************************************/ 
     int daemon = 0;
     if((argc > 1) && (strcmp(argv[1], "-d") == 0))
     {
     	daemon = 1; 
     	syslog(LOG_INFO,"Success: Running in daemon mode...\n");     	
     }

    
    /*************************************************************************
     *                     Signal Handler                                    *
     *************************************************************************/    
    if (signal(SIGINT, signalhandler ) == SIG_ERR)
    {
        syslog( LOG_ERR, "Error signal handler for SIGNINT\n");
        closelog();
        exit(FAILURE);
    }
    
    if (signal(SIGTERM, signalhandler ) == SIG_ERR )
    {
        syslog( LOG_ERR, "Error signal handler for SIGNTERM\n");
        closelog();
        exit(FAILURE);
    }

    /*************************************************************************
     *                            Get address info                           *
     *************************************************************************/
    // Ref: [1]
    struct addrinfo hints, *res;
    struct sockaddr_in clientaddr;
    socklen_t clientaddrlen;
    
    // Ref: [2]
    /* getaddrinfo()'s hints arg points to addrinfo struct
           struct addrinfo {
               int              ai_flags;
               int              ai_family;
               int              ai_socktype;
               int              ai_protocol;
               socklen_t        ai_addrlen;
               struct sockaddr *ai_addr;
               char            *ai_canonname;
               struct addrinfo *ai_next;
           };*/

    // Ref: [1]
    // Load up address structs with getaddrinfo()
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;        // IPv4
    hints.ai_socktype = SOCK_STREAM;    // stream sockets
    hints.ai_flags    = AI_PASSIVE;     // fill in IP

    // Ref: [2]
    // int getaddrinfo(const char *restrict node, const char *restrict service, const struct addrinfo *restrict hints, struct addrinfo **restrict res);
    int rc = getaddrinfo(NULL, PORT, &hints, &res);
    if (rc != SUCCESS)
    {
        syslog(LOG_ERR,"Error getting address info; getaddrinfo() failure\n"); //syslog error
        printf("Error! getaddrinfo() failure\n"); //prints error
        freeaddrinfo(res);
        closelog();
        exit(FAILURE);       
    }
    syslog(LOG_INFO,"Success: getaddrinfo()\n");
    
    if (res == NULL)
    {
        syslog(LOG_ERR,"Error storing info in res; malloc() failure\n"); //syslog error
        printf("Error! malloc() failure\n"); //prints error
        freeaddrinfo(res);
        closelog();
        exit(FAILURE);    
    }
    
    /*************************************************************************
     *                            Create Socket                              *
     *************************************************************************/    
    // Ref: [3]
    // socket: creates endpoint for communication; returns a fd that refers to that endpoint
    // int socket(int domain, int type, int protocol);
    sockfd = socket(PF_INET, SOCK_STREAM, 0); //IPv4, stream, TCP
    int newfd; //for client after accepting connection from server
    if (sockfd == RET_FAILURE)
    {
        syslog(LOG_ERR,"Error creating socket; socket() failure\n"); //syslog error
        printf("Error! socket() failure\n"); //prints error
        freeaddrinfo(res);
        closelog();
        close(sockfd);
        exit(FAILURE);
    }
    syslog(LOG_INFO,"Success: socket()\n");
    
    int yes=1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) 
    {
        syslog(LOG_ERR,"Error setting socket options; setsockopt() failure\n"); //syslog error
        printf("Error! setsockopt() failure\n"); //prints error
        freeaddrinfo(res);
        closelog();
        close(sockfd);
    	exit(FAILURE);
    }
    
    /*************************************************************************
     *                           Daemon enabled                              *
     *************************************************************************/
    if (daemon)
    {
        pid_t pid = fork();

    	if (pid == RET_FAILURE)
    	{
        	syslog(LOG_ERR, "Error forking: %m");
        	close(sockfd);
    		closelog();
        	exit(FAILURE);
    	} 	
    	else if (pid != 0)
    	{
    	        close(sockfd);
    		closelog();
        	exit(SUCCESS); // Parent exits
    	}
    }
    
     /*************************************************************************
      *                            Bind                                       *
      *************************************************************************/ 
    //Ref: [4], [1]
    // bind: bind a name to a socket
    //int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
    rc = bind(sockfd, res->ai_addr, res->ai_addrlen);
    if (rc == RET_FAILURE)
    {
        syslog(LOG_ERR,"Error binding to the port we passed in to getaddrinfo(); bind() failure\n"); //syslog error
        printf("Error! bind() failure\n"); //prints error
        freeaddrinfo(res);
        closelog();
        close(sockfd);
        exit(FAILURE);       
    }
    syslog(LOG_INFO,"Success: bind()\n");
    freeaddrinfo(res);
    
     /*************************************************************************
      *                            Listen                                     *
      *************************************************************************/ 
    // Ref: [5], [1]
    // listen - listen for connections on a socket
    // int listen(int sockfd, int backlog);
    int backlog = 10; // no of connections allowed on the incoming queue (incoming connections are going to wait in this queue until you accept() them; limit on how many can queue up.
    rc = listen(sockfd, backlog);
    if (rc == RET_FAILURE)
    {
        syslog(LOG_ERR,"Error listening for connections on a socket; listen() failure\n"); //syslog error
        printf("Error! listen() failure\n"); //prints error
        closelog();
        exit(FAILURE);        
    }
    syslog(LOG_INFO,"Success: listen()\n");

    while(1) //accept() loop
    {
     /*************************************************************************
      *                            Accept                                     *
      *************************************************************************/ 
        //Ref: https://www.man7.org/linux/man-pages/man2/accept.2.html
        // accept - accept a connection on a socket
        //int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen); 
        clientaddrlen = sizeof(clientaddr);
        newfd =  accept(sockfd, (struct sockaddr *)&clientaddr, &clientaddrlen);
        if (newfd == RET_FAILURE)
        {
            syslog(LOG_ERR,"Error accepting a connection on a socket; accept() failure\n"); //syslog error
            printf("Error! accept() failure\n"); //prints error
            closelog();
            exit(FAILURE);       
        }
        syslog(LOG_INFO,"Success: accept()\n");
        
        //Logs message to the syslog “Accepted connection from xxx” where XXXX is the IP address of the connected client. 
        // Ref: https://linux.die.net/man/3/inet_ntoa, https://www.man7.org/linux/man-pages/man3/sockaddr.3type.html
        // inet_ntoa - Internet network to ASCII; function converts the Internet host address in, given in network byte order, to a string in IPv4 dotted-decimal notation.
        
        /*
           struct sockaddr_in {
           sa_family_t     sin_family;     // AF_INET
           in_port_t       sin_port;       // Port number
           struct in_addr  sin_addr;       // IPv4 address
           }; 
        */
           
        // Get the IP address as a string
        ip_address = inet_ntoa(clientaddr.sin_addr);
        syslog(LOG_USER,"Accepted connection from %s\n", ip_address);
        
        //Receives data over the connection and appends to file /var/tmp/aesdsocketdata, creating this file if it doesn’t exist.
        
     /*************************************************************************
      *                            Receive                                    *
      *************************************************************************/ 
        if ((file_ptr = fopen(DATA_FILE, "a+")) == NULL) //opens file in append and update mode and checks if error
		{
        	syslog(LOG_ERR,"Error while opening given file!\n"); //syslog error
			printf("Error! Opening given file\n"); //prints error
			return FAILURE; //Exits with value 1 error
		}
    	char buffer[BUFFER_SIZE];
    	
        while(1)
        {
            // Ref: [1]
            // receive - returns the number of bytes actually read into the buffer
        	// int recv(int sockfd, void *buf, int len, int flags);
        	int num_bytes = recv(newfd, buffer, sizeof(buffer), 0);
        	if (num_bytes == RET_FAILURE)
        	{
        		syslog(LOG_ERR,"Error while receiving!\n"); //syslog error
			printf("Error! Receiving/n"); //prints error	
			break;
        	}
        	
        	fwrite(buffer, 1, num_bytes, file_ptr);
        	
        	if (memchr(buffer, '\n', num_bytes) != NULL)
            	break;
        }
        
        fclose(file_ptr);

     /*************************************************************************
      *                            Send                                       *
      *************************************************************************/ 
              
        if ((file_ptr = fopen(DATA_FILE, "r")) == NULL) //opens file in append and update mode and checks if error
	{
        	syslog(LOG_ERR,"Error while opening given file!\n"); //syslog error
		printf("Error! Opening given file\n"); //prints error
		return FAILURE; //Exits with value 1 error
	}
        
    	while (!feof(file_ptr))
    	{
        	ssize_t read_bytes = fread(buffer, 1, sizeof(buffer), file_ptr);
        	if (read_bytes <= 0) 
            		break;

        	send(newfd, buffer, read_bytes, 0);
    	}
    	fclose(file_ptr);
        
    }
    
    /*************************************************************************
     *                            Close Socket                               *
     *************************************************************************/
    
    syslog(LOG_USER, "Closed connection from %s\n", ip_address);
    close(sockfd);
    close(newfd);
    closelog();
    remove(DATA_FILE);
    return(SUCCESS);
}
