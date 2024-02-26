/*
 * Filename   : aesdsocket.c
 *
 * Description: C Code that writes a socket serverr application
 *            : Code Flow:
 *            : 1) Enable syslog with openlog
 *            : 2) Check if "-d" argument present, to run aesdsocket application in daemon mode
 *            : 3) Handle SIGNINT and SIGTERM, cleanup and exit program
 *            : 4) Load up address structs with getaddrinfo(), store addrinfo struc in res arg passed and handle errors
 *            : 5) Create socket and set socket options for reusing of local addr
 *            : 6) Bind to port 9000
 *            : 7) When in daemon mode, fork
 *            : 8) Listens for for connections on a socket
 *            : 9) Accepting connections from new clients forever in a loop until SIGINT or SIGTERM is received
 *            : 10) Syslog accepted connection from ip address using inet_ntoa and sockaddr_in struct
 *            : 11) Receive data over the connection and append to file /var/tmp/aesdsocketdata while checking for '\n' in the received data indicating end of a data packet
 *            : 12) Send the full content of /var/tmp/aesdsocketdata to the client as soon as the received data packet completes.
 *            : 13) Syslog closed connection from ip address
 *            : 14) Cleanup and exit program
 *    
 * Author     : Swathi Venkatachalam
 *
 * Reference  : Man pages and beej guide
 *            : [1] Socket server - https://beej.us/guide/bgnet/html/
 *            : [2] openlog       - https://www.man7.org/linux/man-pages/man3/syslog.3.html
 *            : [3] signal        - https://www.man7.org/linux/man-pages/man2/signal.2.html            
 *            : [4] getaddreinfo  - https://man7.org/linux/man-pages/man3/getaddrinfo.3.html
 *            : [5] socket        - https://www.man7.org/linux/man-pages/man2/socket.2.html
 *            : [6] setsockopt    - https://www.man7.org/linux/man-pages/man3/setsockopt.3p.html
 *            : [7] bind          - https://www.man7.org/linux/man-pages/man2/bind.2.html
 *            : [8] listen        - https://www.man7.org/linux/man-pages/man2/listen.2.html
 *            : [9] accept        - https://www.man7.org/linux/man-pages/man2/accept.2.html
 *            : [10] inet_ntoa    - https://linux.die.net/man/3/inet_ntoa
 *            : [11] sockaddr_in  - https://www.man7.org/linux/man-pages/man3/sockaddr.3type.html 
 *            : [12] recv         - https://www.man7.org/linux/man-pages/man2/recv.2.html
 *            : [13] fwrite       - https://www.man7.org/linux/man-pages/man3/fwrite.3p.html
 *            : [14] memchr       - https://www.man7.org/linux/man-pages/man3/memchr.3.html
 *            : [15] fread        - https://manpages.courier-mta.org/htmlman3/fread.3.html
 *            : [16] send         - https://www.man7.org/linux/man-pages/man2/send.2.html
 */

/*************************************************************************
 *                            Header Files                               *
 *************************************************************************/
 
#include <stdio.h>                               // Standard input output library
#include <stdlib.h>                              // General purpose utility functions
#include <syslog.h>                              // System logging
#include <string.h>                              // String manipulations

#include <sys/types.h>                           // Data types used in system calls
#include <sys/socket.h>                          // Socket programming

#include <unistd.h>                              // POSIX API; fork
#include <signal.h>                              // User interrupts (SIGINT), segmentation faults (SIGSEGV)
#include <netinet/in.h>                          // IP struct; struct sockaddr_in
#include <arpa/inet.h>                           // Internet addresses to text representations; inet_ntoa
#include <netdb.h>                               // Network database ops; getaddrinfo

/*************************************************************************
 *                            Macros                                     *
 *************************************************************************/
 
#define RET_FAILURE (-1)                          // Return value check
#define FAILURE     (1)
#define SUCCESS     (0)

#define PORT        ("9000")                      // For Opening a stream socket bound to port 9000
#define DATA_FILE   ("/var/tmp/aesdsocketdata")   // Receives data over the connection and appends to this file 
#define BUFFER_SIZE (128)

/*************************************************************************
 *                  Global Variables                                     *
 *************************************************************************/
 
int sockfd;                                       // Socket function return val
FILE *file_ptr;                                   // fopen return val
char* ip_address;                                 // inet_ntoa return val

/*************************************************************************
 *             Signal Handler Function                                   *
 *************************************************************************/
 
void signalhandler (int signal)
{
    // Logs message to the syslog “Caught signal, exiting” when SIGINT or SIGTERM is received.
    if (signal == SIGINT || signal == SIGTERM )
    {   
        // Gracefully exits when SIGINT or SIGTERM is received, completing any open connection operations, closing any open sockets, and deleting the file /var/tmp/aesdsocketdata
        close(sockfd);
        remove(DATA_FILE);
        
        syslog(LOG_INFO, "Caught signal, exiting");
                
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
     
    // Ref: [2] man page
    // Opens a connection to the system logger for a program
    // openlog(ident, option, facility)
    openlog(NULL, LOG_PID, LOG_USER); // Program name, Caller's PID, default LOG_USER - generic user-level messages
    syslog(LOG_INFO,"Success: Starting log...\n");

    /*************************************************************************
     *                            Daemon mode                                *
     *************************************************************************/ 
     
     // Modify your program to support a -d argument which runs the aesdsocket application as a daemon
     int daemon = 0;
     if((argc > 1) && (strcmp(argv[1], "-d") == 0))
     {
     	daemon = 1; 
     	syslog(LOG_INFO,"Success: Running in daemon mode...\n");     	
     }
    
    /*************************************************************************
     *                     Signal Handler                                    *
     *************************************************************************/  
     
     // Ref: [3] man page
     // Error handling for signals  
     // sighandler_t signal(int signum, sighandler_t handler);
    if (signal(SIGINT, signalhandler) == SIG_ERR)
    {
        syslog(LOG_ERR, "Error signal handler for SIGNINT\n");
        closelog();
        exit(FAILURE);
    }
    
    if (signal(SIGTERM, signalhandler) == SIG_ERR)
    {
        syslog(LOG_ERR, "Error signal handler for SIGNTERM\n");
        closelog();
        exit(FAILURE);
    }

    /*************************************************************************
     *                            Get address info                           *
     *************************************************************************/
     
    // Ref: [1] beej guide
    struct addrinfo hints, *res;
    struct sockaddr_in clientaddr;
    socklen_t clientaddrlen;
    
    // Ref: [4] man page
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

    // Ref: [1] beej guide
    // Load up address structs with getaddrinfo()
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;        // IPv4
    hints.ai_socktype = SOCK_STREAM;    // stream sockets
    hints.ai_flags    = AI_PASSIVE;     // fill in IP

    // Ref: [4] man page
    // getaddrinfo() returns addrinfo structure and stores it in res arg passed
    // int getaddrinfo(const char *restrict node, const char *restrict service, const struct addrinfo *restrict hints, struct addrinfo **restrict res);
    int rc = getaddrinfo(NULL, PORT, &hints, &res);
    if (rc != SUCCESS)
    {
        syslog(LOG_ERR,"Error getting address info; getaddrinfo() failure\n"); //syslog error
        printf("Error! getaddrinfo() failure\n");                              //prints error
        freeaddrinfo(res);
        closelog();
        exit(FAILURE);       
    }
    syslog(LOG_INFO,"Success: getaddrinfo()\n");
    
    // Check if addrinfo stored in res from getaddrinfo
    if (res == NULL)
    {
        syslog(LOG_ERR,"Error storing info in res; malloc() failure\n"); //syslog error
        printf("Error! malloc() failure\n");                             //prints error
        freeaddrinfo(res);
        closelog();
        exit(FAILURE);    
    }
    
    /*************************************************************************
     *                            Create Socket                              *
     *************************************************************************/    
     
    // Ref: [5] man page
    // socket: creates endpoint for communication; returns a fd that refers to that endpoint
    // int socket(int domain, int type, int protocol);
    sockfd = socket(PF_INET, SOCK_STREAM, 0);                           //IPv4, stream, TCP
    int newfd;                                                          //for client after accepting connection from server
    if (sockfd == RET_FAILURE) 
    {
        syslog(LOG_ERR,"Error creating socket; socket() failure\n");    //syslog error
        printf("Error! socket() failure\n");                            //prints error
        freeaddrinfo(res);
        closelog();
        close(sockfd);
        exit(FAILURE);
    }
    syslog(LOG_INFO,"Success: socket()\n");
    
    // Ref: [6] man page, [1] beej guide
    // setsockopt: set socket options
    // int setsockopt(int socket, int level, int option_name, const void *option_value, socklen_t option_len);
    int yes=1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)  //socket level, SO_REUSEADDR: allows reuse of local addr
    {
        syslog(LOG_ERR,"Error setting socket options; setsockopt() failure\n"); //syslog error
        printf("Error! setsockopt() failure\n");                                //prints error
        freeaddrinfo(res);
        closelog();
        close(sockfd);
    	exit(FAILURE);
    }
    syslog(LOG_INFO,"Success: setsockopt()\n");
    
     /*************************************************************************
      *                            Bind                                       *
      *************************************************************************/ 
      
    //Ref: [7] man page, [1] beej guide
    // bind: bind a name to a socket
    // int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
    rc = bind(sockfd, res->ai_addr, res->ai_addrlen);
    if (rc == RET_FAILURE)
    {
        syslog(LOG_ERR,"Error binding to the port we passed in to getaddrinfo(); bind() failure\n"); //syslog error
        printf("Error! bind() failure\n");                                                           //prints error
        freeaddrinfo(res);
        closelog();
        close(sockfd);
        exit(FAILURE);       
    }
    syslog(LOG_INFO,"Success: bind()\n");
    freeaddrinfo(res);
    
    /*************************************************************************
     *                           Daemon enabled                              *
     *************************************************************************/
     
    // When in daemon mode the program should fork after ensuring it can bind to port 9000.
    if (daemon)
    {
        pid_t pid = fork();      // Create child process, sets pid = 0; in parent process pid stores process id of child 

    	if (pid == RET_FAILURE)
    	{
        	syslog(LOG_ERR, "Error forking: %m");
        	close(sockfd);
    		closelog();
        	exit(FAILURE);
    	} 	
    	else if (pid != 0)
    	{
    	    syslog(LOG_INFO,"Success: fork()\n");
    	    close(sockfd);
    		closelog();
        	exit(SUCCESS); // Parent process exits, leaving child process running as daemon
    	}
    }
    
     /*************************************************************************
      *                            Listen                                     *
      *************************************************************************/ 
      
    // Ref: [8] man page, [1] beej guide
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

    // Restarts accepting connections from new clients forever in a loop until SIGINT or SIGTERM is received
    while(1)
    {
     /*************************************************************************
      *                            Accept                                     *
      *************************************************************************/ 
      
        //Ref: [9] man page, [1] beej guide
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
        // Ref: [10], [11] man pages
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
        
     /*************************************************************************
      *                            Receive                                    *
      *************************************************************************/ 
      
        // Receives data over the connection and appends to file /var/tmp/aesdsocketdata, creating this file if it doesn’t exist.
        if ((file_ptr = fopen(DATA_FILE, "a+")) == NULL) //opens file in append and update mode and checks if error
		{
        	syslog(LOG_ERR,"Error while opening given file; fopen() failure\n"); //syslog error
			printf("Error! fopen() failure\n"); //prints error
			return FAILURE; //Exits with value 1 error
		}
    	char buffer[BUFFER_SIZE];
    	
        while(1)
        {
            // Ref: [12] man page
            // receive - returns the number of bytes actually read into the buffer
        	// int recv(int sockfd, void *buf, int len, int flags);
        	int num_bytes = recv(newfd, buffer, sizeof(buffer), 0); // Receive data from the newfd socket into the buffer. It reads up to sizeof(buffer) bytes at a time.
        	if (num_bytes == RET_FAILURE)
        	{
        		syslog(LOG_ERR,"Error while receiving; recv() failure\n"); //syslog error
			    printf("Error! recv() failure/n");                           //prints error	
			    break;
        	}
        	
        	// Ref: [13] man page
        	// Received data written to file
        	// size_t fwrite(const void *ptr, size_t size, size_t count, FILE *stream);
        	fwrite(buffer, 1, num_bytes, file_ptr);
        	
        	// Ref: [14] man page
        	// memchr - Scan memory for a character
        	// void *memchr(const void s[.n], int c, size_t n);
        	if (memchr(buffer, '\n', num_bytes) != NULL) // check '\n' in the received data. If so break out of loop, indicating end of a data packet
            	break;
        }
        
        fclose(file_ptr);

     /*************************************************************************
      *                            Send                                       *
      *************************************************************************/ 
        
        // Returns the full content of /var/tmp/aesdsocketdata to the client as soon as the received data packet completes.
        if ((file_ptr = fopen(DATA_FILE, "r")) == NULL) //opens file in read mode and checks if error
		{
        	syslog(LOG_ERR,"Error while opening given file; fopen() failure\n"); //syslog error
			printf("Error! fopen() failure\n"); //prints error
			return FAILURE; //Exits with value 1 errorr
		}
        
    	while (!feof(file_ptr))
    	{
    	    // Ref: [15] man page
        	// Reads data from file
        	// size_t fread(const void *ptr, size_t nmemb, size_t count, FILE *stream);
        	ssize_t read_bytes = fread(buffer, 1, sizeof(buffer), file_ptr);
        	if (read_bytes <= 0) 
            		break;

        	// Ref: [16] man page
        	// Returns data to client newfd
        	// ssize_t send(int sockfd, const void buf[.len], size_t len, int flags);
        	send(newfd, buffer, read_bytes, 0);
    	}
    	
    	fclose(file_ptr); 
    }
    
    /*************************************************************************
     *                 Close Socket and cleanup                              *
     *************************************************************************/
     
    // Logs message to the syslog “Closed connection from XXX” where XXX is the IP address of the connected client.
    syslog(LOG_USER, "Closed connection from %s\n", ip_address);
    
    // cleanup before exiting program
    close(sockfd);
    close(newfd);
    
    syslog(LOG_USER, "Program successfully completed!\n");
    closelog();
    remove(DATA_FILE);
    return(SUCCESS);
}
