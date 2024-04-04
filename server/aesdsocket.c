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
 *
 *            : [17] Singly Linked List - https://github.com/stockrt/queue.h/blob/master/sample.c, 
 *                                        https://raw.githubusercontent.com/freebsd/freebsd/stable/10/sys/sys/queue.h
 *            : [18] pthread_create     - https://www.man7.org/linux/man-pages/man3/pthread_create.3.html
 *            : [19] pthread_join       - https://www.man7.org/linux/man-pages/man3/pthread_join.3.html
 *            : [20] timespec           - https://www.man7.org/linux/man-pages/man3/timespec.3type.html
 *            : [21] tm                 - https://www.man7.org/linux/man-pages/man3/tm.3type.html
 *            : [22] strftime           - https://man7.org/linux/man-pages/man3/strftime.3.html
 *            : [23] clock_gettime      - https://www.man7.org/linux/man-pages/man2/clock_gettime.2.html
 *            : [24] localtime_r        - httpshttps://linux.die.net/man/3/localtime_r
 *
 * Update     : Assignment 6 Part 1 
 * Description:  15) Multiple connections are accepted simultaneously using pthread and content is written to DATA_FILE, logged every 10 secs utilizing locks
 *              
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
                             
#include "queue.h"                               // For singly linked list APIs
#include <pthread.h>                             // POSIX threads library
#include <stdbool.h>                             // POSIX threads library

#include "../aesd-char-driver/aesd_ioctl.h"      // Added for A9
#include <fcntl.h>                               // For file ops

/*************************************************************************
 *                            Macros                                     *
 *************************************************************************/
 
#define RET_FAILURE                       (-1)                          // Return value check
#define FAILURE                           (1)
#define SUCCESS                           (0)

#define PORT                              ("9000")                      // For Opening a stream socket bound to port 9000

//#define DATA_FILE                         ("/var/tmp/aesdsocketdata")   // Receives data over the connection and appends to this file 

#define USE_AESD_CHAR_DEVICE              // comment out of not req

#ifdef USE_AESD_CHAR_DEVICE
    #define DATA_FILE                     ("/dev/aesdchar")
#else
    #define DATA_FILE                     ("/var/tmp/aesdsocketdata")
#endif

#define BUFFER_SIZE                       (1024)

// Ref: [22] man page
#define RFC2822_compliant_strftime_format ("%a, %d %b %Y %T %z\n")

#define IOCTL_STRING                      ("AESDCHAR_IOCSEEKTO:")
#define IOCTL_STRING_LENGTH               (19)

/*************************************************************************
 *                  Global Variables                                     *
 *************************************************************************/
 
int sockfd;                                       // Socket function return val
FILE *file_ptr;                                   // fopen return val
char* ip_address;                                 // inet_ntoa return val

pthread_mutex_t lock;                             // For writing to DATA_FILE and timestamp   
bool signal_exit = false;                         // Flag to indicate signal detected
bool timer_exit  = false;                         // Flag to indicate timer end

/*************************************************************************
 *                        Structures                                     *
 *************************************************************************/
// Ref: [20] man page
// timespec - time in seconds and nanoseconds
// Member obj: time_t tv_sec, tv_nsec
struct timespec time_now, time_sleep = {10, 0};  // For timestamp after 10 sec, 0 nanosec

// Ref: [21] man page
// tm - broken-down time
/* struct tm {
	int         tm_sec;    // Seconds          [0, 60] 
    int         tm_min;    // Minutes          [0, 59] 
    int         tm_hour;   // Hour             [0, 23] 
    int         tm_mday;   // Day of the month [1, 31] 
    int         tm_mon;    // Month            [0, 11]  (January = 0)
    int         tm_year;   // Year minus 1900 
    int         tm_wday;   // Day of the week  [0, 6]   (Sunday = 0) 
    int         tm_yday;   // Day of the year  [0, 365] (Jan/01 = 0) 
    int         tm_isdst;  // Daylight savings flag 

    long        tm_gmtoff; // Seconds East of UTC 
    const char *tm_zone;   // Timezone abbreviation 
}; */
struct tm time_info;                                // time info in tm
  
// Ref: [17] sample.c, queue.h
// Singly Linked List
/*
#define	SLIST_ENTRY(type)						\
struct {								        \
	struct type *sle_next;	// next element     \
}*/
typedef struct slist_client_s slist_client_t;
struct slist_client_s 
{
    pthread_t thread_id;                            // Track thread Id
    bool thread_completion_flag;                    // Check for completion
    int newfd;                                      // File descriptor of new client connection
    SLIST_ENTRY(slist_client_s) entries;
};

/* SLIST_HEAD(name, type)
struct name {								   \
	struct type *slh_first;	// first element   \
}*/
SLIST_HEAD(slisthead, slist_client_s) head;        // Head of LL

struct slist_client_s *temp;                       // SLIST_FOREACH_SAFE function arg

/*************************************************************************
 *                    Cleanup Function                                   *
 *************************************************************************/
 
void cleanup()
{
    // Gracefully exits when SIGINT or SIGTERM is received, completing any open connection operations, closing any open sockets, and deleting the file /var/tmp/aesdsocketdata
	#ifndef USE_AESD_CHAR_DEVICE
    remove(DATA_FILE);
    #endif

        
    syslog(LOG_INFO, "Caught signal, exiting");               
    closelog();
    
    struct slist_client_s *new_client_node;
    
    // Ref: [17] queue.h
    /*
    #define	SLIST_FOREACH_SAFE(var, head, field, tvar)			\
	for ((var) = SLIST_FIRST((head));				            \
	    (var) && ((tvar) = SLIST_NEXT((var), field), 1);		\
	    (var) = (tvar))*/
    SLIST_FOREACH_SAFE(new_client_node, &head, entries, temp)
    {
        pthread_join(new_client_node->thread_id, NULL);
        SLIST_REMOVE(&head, new_client_node, slist_client_s, entries);
        free(new_client_node);
    }
    syslog(LOG_INFO, "Program completed successfully!"); 
    printf("Program completed successfully!"); 
    exit(SUCCESS);
}

/*************************************************************************
 *             Signal Handler Function                                   *
 *************************************************************************/
 
void signalhandler (int signal)
{ 

    if (signal == SIGINT || signal == SIGTERM )
    {   
        signal_exit = true;
        
    	shutdown(sockfd, SHUT_RDWR);
    	cleanup();
    }   
}

/*************************************************************************
 *               Time Handler Function                                   *
 *************************************************************************/
#ifndef USE_AESD_CHAR_DEVICE
void timehandler(int signal)
{ 
    if (signal == SIGALRM)
    {          
        timer_exit = true;
    }   
}
#endif

/*************************************************************************
 *                  Multithread_handler Function                         *
 *************************************************************************/
 
void *multithread_handler(void *new_client_node)
{
	struct slist_client_s *thread_param = (struct slist_client_s*)new_client_node;
	
	/*************************************************************************
     *                            Receive                                    *
     *************************************************************************/ 
     int fd;
    //pthread_mutex_lock(&lock);
    //printf("Locked for receive!\n");
    // Receives data over the connection and appends to file /var/tmp/aesdsocketdata, creating this file if it doesn’t exist.
    
    //if ((file_ptr = fopen(DATA_FILE, "a+")) == NULL) //opens file in append and update mode and checks if error
    if ((fd = open(DATA_FILE, O_CREAT | O_RDWR | O_APPEND, 0744)) == -1) //opens file checks if error
    {
        syslog(LOG_ERR,"Error while opening given file; fopen() failure\n"); //syslog error
		printf("Error! fopen() failure\n"); //prints error
		perror("");
		pthread_mutex_unlock(&lock);
		close(thread_param->newfd);
        thread_param->thread_completion_flag = 1;
		return NULL;
    }
    printf("Opened DATA_FILE: %s for receive\n", DATA_FILE);
    char buffer[BUFFER_SIZE];
    struct aesd_seekto seekto;
    off_t offset;
    
 	int num_bytes;

    // Ref: [12] man page
    // receive - returns the number of bytes actually read into the buffer
    // int recv(int sockfd, void *buf, int len, int flags);
    while ((num_bytes  = recv( thread_param->newfd, buffer, sizeof( buffer), 0)) > 0)
    {
    	printf("Recv success!\n");
        pthread_mutex_lock(&lock);
        
        // ioctl check
    	// int strncmp(const char s1[.n], const char s2[.n], size_t n);
    	if ((strncmp( buffer, "AESDCHAR_IOCSEEKTO:", 19 )) == SUCCESS)
    	{
            unsigned int write_cmd, write_cmd_offset;
            sscanf( buffer + 19, "%u,%u", &write_cmd, &write_cmd_offset );
            seekto.write_cmd = write_cmd;
            seekto.write_cmd_offset = write_cmd_offset;

    		if(ioctl(fd, AESDCHAR_IOCSEEKTO, &seekto) == -1)
        	{
        		syslog(LOG_ERR,"Error while ioctl; ioctl failure\n"); //syslog error
				printf("Error! ioctl() failure\n"); //prints error
				pthread_mutex_unlock(&lock);
				perror("");
				return NULL;
        	}
        	printf("IOCTL success!\n");
            offset = lseek( fd, 0, SEEK_CUR );
        }
        else
        {
    	    // Ref: [13] man page
        	// Received data written to file
        	// size_t fwrite(const void *ptr, size_t size, size_t count, FILE *stream);
        	write(fd, buffer, num_bytes);
        	printf("Write success!, received data written to file\n");
        	
        	offset = -1; // flag write occured
        	
        	close(fd);    
    		printf("Closed DATA_FILE: %s after receive\n", DATA_FILE); 
        }

        pthread_mutex_unlock(&lock);
        printf("Unlocked after receive!\n");

        // Ref: [14] man page
        // memchr - Scan memory for a character
        // void *memchr(const void s[.n], int c, size_t n);
        if (memchr(buffer, '\n', num_bytes) != NULL) // check '\n' in the received data. If so break out of loop, indicating end of a data packet
        {
            printf("NULL detected; end of packet!\n");
            break;
        }
    }
     /*************************************************************************
      *                            Send                                       *
      *************************************************************************/ 
    pthread_mutex_lock(&lock);
    printf("Locked for send!\n");
    
    // Returns the full content of DATA_FILE to the client as soon as the received data packet completes.
    if (offset == -1 && (fd = open(DATA_FILE, O_RDWR)) == -1)  //opens file in read mode and checks if error
    {
        syslog(LOG_ERR,"Error while opening given file; fopen() failure\n");      //syslog error
		printf("Error! fopen() failure in send function\n");                                       //prints error
		pthread_mutex_unlock(&lock);
        thread_param->thread_completion_flag = 1;
		return NULL;
    }
    printf("Opened DATA_FILE: %s for send\n", DATA_FILE);
       
    // Ref: [15] man page
    // Reads data from file
    // ssize_t read(int fd, void buf[.count], size_t count); 
    ssize_t read_bytes;
    while ((read_bytes = read(fd, buffer, sizeof(buffer))) > 0)
    {
        // Ref: [16] man page
        // Returns data to client newfd
        // ssize_t send(int sockfd, const void buf[.len], size_t len, int flags);
        send(thread_param->newfd, buffer, read_bytes, 0);
    }
    printf("Send success!\n");
	   
    close(thread_param->newfd);
    close(fd); 
    printf("Closed DATA_FILE: %s after send\n", DATA_FILE); 
    pthread_mutex_unlock(&lock);
    printf("Unlocked after send!\n");
    thread_param->thread_completion_flag = 1;
    return NULL;
}

/*************************************************************************
 *                 timestamp_handler Function                         *
 *************************************************************************/
#ifndef USE_AESD_CHAR_DEVICE
void *timestamp_handler (void *arg)
{
    char buffer[BUFFER_SIZE];                                                           // To store formatted time and date

    while (!timer_exit)
    {
        // Ref: [23] man page
        // clock_gettime(clockid_t clockid, struct timespec *tp);
        clock_gettime(CLOCK_REALTIME, &time_now);                                        // Get current time with nanosec
        
        // Ref: [24] man page
        // struct tm *localtime_r(const time_t *timep, struct tm *result);        
        localtime_r( &time_now.tv_sec, &time_info);                                      // Convert timespec to tm 
        
        // Ref: [22] man page
        // strftime - formats broken-down time tm according to format spec and stores in char arr
        // size_t strftime(char s[.max], size_t max, const char *format, const struct tm *tm);
        strftime(buffer, sizeof(buffer), RFC2822_compliant_strftime_format, &time_info); // Format date and time 

        pthread_mutex_lock(&lock);
        
        if ((file_ptr = fopen(DATA_FILE, "a+")) == NULL)                                 //opens file in append mode and checks if error
		{
        	syslog(LOG_ERR,"Error while opening given file; fopen() failure\n");         //syslog error
			printf("Error! fopen() failure\n");                                          //prints error
			pthread_mutex_unlock(&lock);
			return NULL;
		}
        
        if (fwrite("timestamp:", 1, strlen("timestamp:"), file_ptr) != strlen("timestamp:"))         // Write timestamp string
        {
            syslog( LOG_ERR, "Error while writing to given file; fwrite() failure\n" );
            printf("Error! fwrite() failure\n");                                                     //prints error
            fclose(file_ptr);
            pthread_mutex_unlock(&lock);
            return NULL;
        }

        if (fwrite(buffer, 1, strlen(buffer), file_ptr) != strlen(buffer))              // Write timestamp formatted 
        {
            syslog( LOG_ERR, "Error while writing to given file; fwrite() failure\n" );
            printf("Error! fwrite() failure\n");                                       //prints error
            fclose(file_ptr);
            pthread_mutex_unlock(&lock);
            return NULL;
        }

        fclose(file_ptr);
        
        pthread_mutex_unlock(&lock);
        
        nanosleep(&time_sleep, NULL);                                                   // sleep for 10 sec; time_sleep timespec struct set to 10 sec
    }
    syslog(LOG_INFO,"Success: Timestamp...\n");
    return NULL;
}
#endif
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
    openlog(NULL, LOG_PID, LOG_USER);                                                       // Program name, Caller's PID, default LOG_USER - generic user-level messages
    syslog(LOG_INFO,"Success: Starting log...\n");
    printf("Program starting...\n");

    /*************************************************************************
     *                            Daemon mode                                *
     *************************************************************************/ 
     
     // Modify your program to support a -d argument which runs the aesdsocket application as a daemon
     int daemon = 0;
     if((argc > 1) && (strcmp(argv[1], "-d") == 0))
     {
     	daemon = 1; 
     	syslog(LOG_INFO,"Success: Running in daemon mode...\n"); 
     	printf("Success: Running in daemon mode...\n");    	
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
    
    if (signal(SIGALRM, signalhandler) == SIG_ERR)
    {
        syslog(LOG_ERR, "Error signal handler for SIGALRM\n");
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
    printf("Success: getaddrinfo()\n");
    
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
    printf("Success: socket()\n");
    
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
    printf("Success: setsockopt()\n");
    
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
        perror("");  
        freeaddrinfo(res);
        closelog();
        close(sockfd);
        exit(FAILURE);       
    }
    syslog(LOG_INFO,"Success: bind()\n");
    printf("Success: bind()\n");
    freeaddrinfo(res);
    
    /*************************************************************************
     *                           Daemon enabled                              *
     *************************************************************************/
     
    // When in daemon mode the program should fork after ensuring it can bind to port 9000.
    if (daemon)
    {
        pid_t pid = fork();                                                                      // Create child process, sets pid = 0; in parent process pid stores process id of child 

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
    int backlog = 1; // no of connections allowed on the incoming queue (incoming connections are going to wait in this queue until you accept() them; limit on how many can queue up.
    rc = listen(sockfd, backlog);
    if (rc == RET_FAILURE)
    {
        syslog(LOG_ERR,"Error listening for connections on a socket; listen() failure\n"); //syslog error
        printf("Error! listen() failure\n"); //prints error
        closelog();
        exit(FAILURE);        
    }
    syslog(LOG_INFO,"Success: listen()\n");
    printf("Success: listen()\n");

     /*************************************************************************
      *                          Lock Init                                    *
      *************************************************************************/ 
          
    // int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
    rc = pthread_mutex_init(&lock, NULL);                                            // Initialize a mutex
    if (rc != SUCCESS)                                     // Returns error code on failure
    {
        syslog(LOG_ERR,"Error initializing mutex; pthread_mutex_init() failure\n"); //syslog error
        printf("Error! pthread_mutex_init() failure\n");                         //prints error 
        closelog();
        exit(FAILURE);                
    }
    
     /*************************************************************************
      *                          Timestamp                                   *
      *************************************************************************/ 
   #ifndef USE_AESD_CHAR_DEVICE
    pthread_t timestamp_thread;                       // thread_id
    
    rc = pthread_create(&timestamp_thread,   // Thread ID
                        NULL,                          // Default attr
                        timestamp_handler,           // Handle timestamp
                        NULL);        // No args
        
    if (rc != SUCCESS)                                     // Returns error code on failure
    {
        syslog(LOG_ERR,"Error creating thread; pthread_create() failure\n"); //syslog error
        printf("Error! pthread_create() failure\n");                         //prints error                                          // Free ptr  
        closelog();
        exit(FAILURE);                
    }
    #endif

    // Restarts accepting connections from new clients forever in a loop until SIGINT or SIGTERM is received
    
    // For LL
    // Ref: [17] queue.h
    /*
	#define	SLIST_INIT(head) do {			    \
	SLIST_FIRST((head)) = NULL;					\
    } while (0) */	
    SLIST_INIT(&head);
    printf("Entering loop to accept!\n");
    int debug_count = 0;
    while(!signal_exit)
    {
      debug_count++;
      printf("Debug_count = %d\n", debug_count);
     /*************************************************************************
      *                    Accept multiple                                    *
      *************************************************************************/ 
      
        //Ref: [9] man page, [1] beej guide
        // accept - accept a connection on a socket
        //int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen); 
        clientaddrlen = sizeof(clientaddr);
                printf("Before accept\n");
        newfd =  accept(sockfd, (struct sockaddr *)&clientaddr, &clientaddrlen);
        if (newfd == RET_FAILURE)
        {
            syslog(LOG_ERR,"Error accepting a connection on a socket; accept() failure\n"); //syslog error
            printf("Error! accept() failure\n"); //prints error
            closelog();
            exit(FAILURE);       
        }
        syslog(LOG_INFO,"Success: accept()\n");
        printf("Success: accept()\n");
        
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
        printf("Accepted connection from %s\n", ip_address);
        
        // Ref: [17] sample.c
		// Singly Linked List    
		struct slist_client_s *new_client_node= (struct slist_client_s*) malloc(sizeof(struct slist_client_s)); // Allocate memory for new client node
		new_client_node->newfd = newfd;                       // Load fd value
		new_client_node->thread_completion_flag = 0;          // Init complete flag
		
		// SLIST_INSERT_HEAD(head, elm, field)
		SLIST_INSERT_HEAD(&head, new_client_node, entries);   // Insert element at head
		
		// Ref: [18] man page
        // pthread_create - create a new thread
        // int pthread_create(pthread_t *thread,
        //                    const pthread_attr_t *attr,
        //                    void *(*start_routine)(void *),
        //                    void *arg);
        rc = pthread_create(&new_client_node->thread_id,                     // Thread ID
                            NULL,                                            // Default attr
                            multithread_handler,                             // Handle connection made
                            (void *)new_client_node);                        // Send new client node data as arg
        
        if (rc != SUCCESS)                                                   // Returns error code on failure
        {
            syslog(LOG_ERR,"Error creating thread; pthread_create() failure\n"); //syslog error
            printf("Error! pthread_create() failure\n");                         //prints error
            
            // SLIST_REMOVE(head, elm, type, field)
            SLIST_REMOVE(&head, new_client_node, slist_client_s, entries);  // Remove node from LL if thread creation unsuccessful
            close(new_client_node->newfd);                                  // Close client connection made
            free(new_client_node);                                          // Free ptr  
            closelog();
            exit(FAILURE);                
        }
        syslog(LOG_INFO,"Success: pthread_create()\n");
        printf("Success: pthread_create()\n");
           
        SLIST_FOREACH_SAFE(new_client_node, &head, entries, temp)
        {
             if (new_client_node->thread_completion_flag)
             {
             	 // Ref: [19] man page
                 // pthread_join - join with a terminated thread
                 // pthread_join(pthread_t thread, void **retval);
                 rc = pthread_join(new_client_node->thread_id, NULL);
                 SLIST_REMOVE(&head, new_client_node, slist_client_s, entries);         // Remove node from LL
                 free(new_client_node);
                 if (rc != SUCCESS)   
                 {
                     syslog(LOG_ERR,"Error joining thread; pthread_join() failure\n");  //syslog error
                     printf("Error! pthread_join() failure\n");                         //prints error 
                   
                     closelog();
                     exit(FAILURE);  
                 }
             }
        }  
        // Logs message to the syslog “Closed connection from XXX” where XXX is the IP address of the connected client.
        syslog(LOG_USER, "Closed connection from %s\n", ip_address);  
         printf("Closed connection from %s\n", ip_address);     
    }
}
