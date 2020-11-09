/* 
 * server FTP program
 *
 * NOTE: Starting homework #2, add more comments here describing the overall function
 * performed by server ftp program
 * This includes, the list of ftp commands processed by server ftp.
 *
 * 
 * 		String tokenization
 * 		User cmd
 * 		recv cmd
 * 		send cmd
 *      pass cmd
 * 		rmdir cmd
 * 		mkdir cmd
 * 		Dele cmd
 * 		stat cmd 
 * 		ls cmd
 * 		pwd cmd
 * 		help cmd
 * 		
 */

#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define SERVER_FTP_PORT 2023
#define DATA_CONNECTION_PORT 2024

/* Error and OK codes */
#define OK 0
#define ER_INVALID_HOST_NAME -1
#define ER_CREATE_SOCKET_FAILED -2
#define ER_BIND_FAILED -3
#define ER_CONNECT_FAILED -4
#define ER_SEND_FAILED -5
#define ER_RECEIVE_FAILED -6
#define ER_ACCEPT_FAILED -7


/* Function prototypes */

int svcInitServer(int *s);
int sendMessage (int s, char *msg, int  msgSize);
int receiveMessage(int s, char *buffer, int  bufferSize, int *msgSize);
int clntConnect(char	*serverName, int *s);

/* List of all global variables */

char userCmd[1024];	/* user typed ftp command line received from client */
char cmd[1024];		/* ftp command (without argument) extracted from userCmd */
char argument[1024];	/* argument (without ftp command) extracted from userCmd */
char replyMsg[1024];       /* buffer to send reply message to client */
char buffer[4096];
FILE *tempfile;
char users[1024];
char user[1024];
char pass[1024];
int dataSocket;	//socket for data connection
int listensocket;
char orgCmd[1024];
FILE *tfp;
int bytesread = 800;
char *tmp;

/*
 * main
 *
 * Function to listen for connection request from client
 * Receive ftp command one at a time from client
 * Process received command
 * Send a reply message to the client after processing the command with staus of
 * performing (completing) the command
 * On receiving QUIT ftp command, send reply to client and then close all sockets
 *
 * Parameters
 * argc		- Count of number of arguments passed to main (input)
 * argv  	- Array of pointer to input parameters to main (input)
 *		   It is not required to pass any parameter to main
 *		   Can use it if needed.
 *
 * Return status
 *	0			- Successful execution until QUIT command from client 
 *	ER_ACCEPT_FAILED	- Accepting client connection request failed
 *	N			- Failed stauts, value of N depends on the command processed
 */

int main(int argc, char *argv[])
{
    /* List of local varibale */

    int msgSize;        /* Size of msg received in octets (bytes) */
    int listenSocket;   /* listening server ftp socket for client connect request */
    int ccSocket;  /* Control connection socket - to be used in all client communication */
	int dcSocket;
	char bytebuff[100];
    int status;
	char *temp;
	int userIndex = -1;
	char *users[] = {"Alan", "Brian", "Charlie", "Daniel"};
	char *pass[] = {"apass", "bpass", "cpass", "dpass"};
	int loggedIn = -1;


    /*
     * NOTE: without \n at the end of format string in printf,
     * UNIX will buffer (not flush)
     * output to display and you will not see it on monitor.
     */
    printf("Started execution of server ftp\n");


    /*initialize server ftp*/
    printf("Initialize ftp server\n");	/* changed text */

    status=svcInitServer(&listenSocket);
    if(status != 0)
    {
	printf("Exiting server ftp due to svcInitServer returned error\n");
	exit(status);
    }


    printf("ftp server is waiting to accept connection\n");

    /* wait until connection request comes from client ftp */
    ccSocket = accept(listenSocket, NULL, NULL);

    printf("Came out of accept() function \n");

    if(ccSocket < 0)
    {
		perror("cannot accept connection:");
		printf("Server ftp is terminating after closing listen socket.\n");
		close(listenSocket);  /* close listen socket */
		return (ER_ACCEPT_FAILED);  // error exist
    }

    printf("Connected to client, calling receiveMsg to get ftp cmd from client \n");


    /* Receive and process ftp commands from client until quit command.
     * On receiving quit command, send reply to client and 
     * then close the control connection socket "ccSocket". 
     */
    do
    {
		/* Receive client ftp commands until */
		status=receiveMessage(ccSocket, userCmd, sizeof(userCmd), &msgSize);
		if(status < 0)
		{
			printf("Receive message failed. Closing control connection \n");
			printf("Server ftp is terminating.\n");
			break;
		}


		/*
		* Starting Homework#2 program to process all ftp commands must be added here.
		* See Homework#2 for list of ftp commands to implement.
		*/
		/* Separate command and argument from userCmd */
		strcpy(orgCmd, userCmd);
		tmp = strtok(userCmd, " ");
		if(tmp == NULL)
		{
			strcpy(replyMsg, "No cmd found");
		}
		else
		{
			strcpy(cmd, tmp);
			tmp = strtok(NULL, " ");
		}
		if(tmp == NULL)
		{
			strcpy(argument, "");		
		}
		else
		{
			strcpy(argument, tmp);
		}
		
		// Breaks down cmd into multiple commands then those are inserted into and command char arry


		//User cmd, prints usernames of users currently logged into host
		
		if(strcmp(cmd, "user") == 0)
		{
			if (strcmp(argument, "") == 0) // If no argument, return error
			{
				strcpy(replyMsg, "501 Syntax error in parameters or arguments.\n");
			}
			else
			{
				int i = 0; // Loop variable to iterate through users[]
				short valid = 0;
				int size = sizeof(users)/sizeof(users[0]);

				while(i < size) // Loop through all elements in users[]
				{
					if(strcmp(users[i], argument) == 0) // If argument matches any of the valid users, set valid to true, and store i in userIndex for password verification.
					{
						valid = 1;
						userIndex = i;
						break;
					}
					i++;	
				}
				if(valid) // If argument is a valid user, send appropriate replyMsg
				{
					strcpy(replyMsg,"331 User name okay, need password.\n");
				}
				else
				{
					strcpy(replyMsg,"430 Invalid username.\n");
				}
			}
		}

		//pass cmd, check for password validity
		else if (strcmp(cmd, "pass") == 0)
		{
			if (strcmp(argument, "") == 0) // If no argument, return error
			{
				strcpy(replyMsg, "501 Syntax error in parameters or arguments.\n");
			}
			else if (userIndex == -1) // Need to pick user name first.
			{
				strcpy(replyMsg, "332 Need account for login.\n");
			}
			else
			{
				if (strcmp(pass[userIndex], argument) == 0) // Validate user's password using index.
				{
					strcpy(replyMsg, "230 User logged in, proceed.\n");
					loggedIn = 1;
				}
				else
				{
					strcpy(replyMsg, "430 Invalid username or password.\n");
					userIndex = -1;
				}
			}
		}

		//if quit command is issued, log user out
		else if (strcmp(cmd, "quit") == 0)
		{
			memset(replyMsg, '\0', sizeof(replyMsg));
			strcpy(replyMsg, "231 User has logged out and the service is terminated.\n");
		}

		//mkdir cmd, make directories
		else if (strcmp(cmd, "mkdir") == 0)
		{
			if (strcmp(argument, "") == 0) // If no argument, return error
			{
				strcpy(replyMsg, "501 Syntax error in parameters or arguments.\n");
			}
			else if (loggedIn == -1) // If user isn't logged in, return error
			{
				strcpy(replyMsg, "530 Not logged in.\n");
			}
			else
			{
				memset(buffer, '\0', sizeof(buffer));
				char subcommand[1024];
				memset(replyMsg, '\0', sizeof(replyMsg));
				sprintf(subcommand, "mkdir %s", argument);

				status = system(subcommand);
				sprintf(replyMsg, "257 %s created.\n", argument);

				//Flushes cmd and argument
				memset(cmd, '\0', sizeof(cmd));
				memset(argument, '\0', sizeof(argument));
			}
		}

		//rmdir cmd, remove directory entries
		else if (strcmp(cmd, "rmdir") == 0)
		{
			if (strcmp(argument, "") == 0) // If no argument, return error
			{
				strcpy(replyMsg, "501 Syntax error in parameters or arguments.\n");
			}
			else if (loggedIn == -1) // If user isn't logged in, return error
			{
				strcpy(replyMsg, "530 Not logged in.\n");
			}
			else
			{
				//creates subcommand of size 1024 bits
				char subcommand[1024];
				memset(replyMsg, '\0', sizeof(replyMsg));

				sprintf(subcommand, "rmdir %s", argument);

				status = system(subcommand);

				//if status is less than 0, print error
				if (status < 0)
				{
					sprintf(replyMsg, "error\n");
				}

				strcpy(replyMsg, "250 Requested file action okay, completed.\n");
				//sprintf(replyMsg, "cmd 212 successfully removed %s\n", argument);
				memset(cmd, '\0', sizeof(cmd));
				memset(argument, '\0', sizeof(argument));
			}
		}

		//cd command, changes the working directory
		else if (strcmp(cmd, "cd") == 0)
		{
			if (strcmp(argument, "") == 0) // If no argument, return error
			{
				strcpy(replyMsg, "501 Syntax error in parameters or arguments.\n");
			}
			else if (loggedIn == -1) // If user isn't logged in, return error
			{
				strcpy(replyMsg, "530 Not logged in.");
			}
			else
			{
				strcpy(replyMsg, "250 Requested file action okay, completed.\n");
				status = chdir(argument);

				//if the target directory doesnt exist
				if (status < 0)
				{
					sprintf(replyMsg, "550 Requested action not taken. File unavailable (for example, file not found, or no access).\n");
				}
			}
		}

		//dele cmd, deletes a file within the cwd 
		else if (strcmp(cmd, "dele") == 0)
		{
			if (strcmp(argument, "") == 0) // If no argument, return error
			{
				strcpy(replyMsg, "501 Syntax error in parameters or arguments.\n");
			}
			else if (loggedIn == -1) // If user isn't logged in, return error
			{
				strcpy(replyMsg, "530 Not logged in.\n");
			}
			else
			{
				char subcommand[1024];
				memset(replyMsg, '\0', sizeof(replyMsg));

				sprintf(subcommand, "rm %s", argument);

				//Actually deletes said file
				status = system(subcommand);

				//If there is no ok report, report error
				if (status < 0)
				{
					sprintf(replyMsg, "error\n");
				}

				//final report on delete command
				strcpy(replyMsg, "250 Requested file action okay, completed.\n");
				//sprintf(replyMsg, "ok, deleted %s\n", argument);
				memset(cmd, '\0', sizeof(cmd));
				memset(argument, '\0', sizeof(argument));
			}
		}
        

		//"pwd" the present working directory is displayed
		else if(strcmp(cmd, "pwd") == 0)
		{
			if (loggedIn == -1) // If user isn't logged in, return error
			{
				strcpy(replyMsg, "530 Not logged in.\n");
			}
			else
			{
				//Clears buffer
				memset(buffer, '\0', sizeof(buffer));

				system("pwd > /tmp/pwd.txt");

				//Opens pwdfile
				tempfile = fopen("/tmp/pwd.txt", "r");

				//Reads the file to the buffer
				status = fread(buffer, sizeof(buffer), sizeof(char), tempfile);
				sprintf(replyMsg, "250 Requested file action okay, completed. \n%s\n", buffer);

				//Closes pwdfile
				fclose(tempfile);		

				system("rm /tmp/pwd.txt");
			}
		}

		//ls command, list the files and other directories within the directory
		else if (strcmp(cmd, "ls") == 0)
		{
			if (loggedIn == -1) // If user isn't logged in, return error
			{
				strcpy(replyMsg, "530 Not logged in.\n");
			}
			else
			{
				//Clears buffer
				memset(buffer, '\0', sizeof(buffer));

				system("ls > /tmp/ls.txt");

				tempfile = fopen("/tmp/ls.txt", "r");

				//Reads the file to the buffer
				status = fread(buffer, sizeof(buffer), sizeof(char), tempfile);
				sprintf(replyMsg, "250 Requested file action okay, completed. \n%s\n", buffer);

				//Closes the file
				fclose(tempfile);

				system("rm /tmp/ls.txt");
			}
		}

		//stat cmd, inform client that tranfer is in ascii
		else if (strcmp(cmd, "stat") == 0)
		{
			strcpy(replyMsg, "200 The requested action has been successfully completed.\nTransfer mode is ASCII\n");
		}

		//help Command, prints all commands and their uses
		else if (strcmp(cmd, "help") == 0)
		{
		//Prints all of the usable commands in the program
		strcpy(replyMsg, "214 Help message.\n"
			"Commands\t Use \t\t\t Syntax\n"
			"pwd  \t\t print directory   \t pwd\n"
			"cd   \t\t change directory  \t cd dir\n"
			"dele \t\t remove a file     \t dele file\n"
			"stat \t\t print stats       \t stat\n"
			"mkdir\t\t make a directory  \t mkdir dir\n"
			"rmdir\t\t remove directory  \t rmdir dir\n"
			"ls   \t\t print files in dir\t ls\n"
			"pass \t\t log in password   \t password pass\n"
			"user \t\t log in as user    \t username user\n"
			"recv \t\t receives file     \t recv filename\n"
			"send \t\t sends file        \t send filename\n");
		}

		//send cmd, receive messages from the preceding queue
		else if (strcmp(cmd, "send") == 0)
		{
			if (strcmp(argument, "") == 0) // If no argument, return error
			{
				strcpy(replyMsg, "501 Syntax error in parameters or arguments.\n");
			}
			else if (loggedIn == -1) // If user isn't logged in, return error
			{
				strcpy(replyMsg, "530 Not logged in.\n");
			}
			else
			{
				status = clntConnect("127.0.0.1", &dcSocket);
				if (status != 0) {
					printf("Data connection failed and was not created\n");
					strcpy(replyMsg, "Data connection failed and was not created\n");
				}
		    else { 
					printf("Yay data connection successful created data connection\n");
					tfp = fopen(argument, "w+");
					do {
						status = receiveMessage(dcSocket, bytebuff, sizeof(bytebuff), &msgSize);
						fwrite(bytebuff, 1, msgSize, tfp);
					} while ((msgSize > 0) && (status == 0));
					strcpy(replyMsg, "200 cmd OK\n");
					fclose(tfp);
					close(dcSocket);

				}
			}
		}

		//Recv cmd, used to receive messages from another socket
		else if(strcmp(cmd, "recv") == 0)
		{
			if (strcmp(argument, "") == 0) // If no argument, return error
			{
				strcpy(replyMsg, "501 Syntax error in parameters or arguments.\n");
			}
			else if (loggedIn == -1) // If user isn't logged in, return error
			{
				strcpy(replyMsg, "530 Not logged in.\n");
			}
			else
			{
				status = clntConnect("127.0.0.1", &dcSocket);
				if (status != 0) {
					printf("Data connection failed and was not created\n");
					strcpy(replyMsg, "Data connection failed and was not created\n");
				}
				else {
					printf("Yay data connection successful created data connection\n");
					tfp = fopen(argument, "r+");
					while (!feof(tfp)) {
						bytesread = fread(bytebuff, 1, 100, tfp);
						printf("Number of bytes read: %d\n", bytesread);
						sendMessage(dcSocket, bytebuff, bytesread);

					}
					strcpy(replyMsg, "200 cmd Ok\n");
					fclose(tfp);
					close(dcSocket);

				}
			}
			
		}

		//If there is no valid command, print invalid command
		else
			sprintf(replyMsg, "500 Command invalid.\n");

		/*
		* ftp server sends only one reply message to the client for 
		* each command received in this implementation.
		*/
		/*strcpy(replyMsg,"200 cmd okay\n");  [> Should have appropriate reply msg starting HW2 <]*/
		status=sendMessage(ccSocket,replyMsg,strlen(replyMsg) + 1);	/* Added 1 to include NULL character in */
		/* the reply string strlen does not count NULL character */
		if(status < 0)
		{
			break;  /* exit while loop */
		}
    }
    while(strcmp(cmd, "quit") != 0);
 

    printf("Closing control connection socket.\n");
    close (ccSocket);  /* Close client control connection socket */

    printf("Closing listen socket.\n");
    close(listenSocket);  /*close listen socket */

    printf("Exiting from server ftp main \n");

    return (status);
}

/*
 * clntConnect
 *
 * Function to create a socket, bind local client IP address and port to the socket
 * and connect to the server
 *
 * Parameters
 * serverName	- IP address of server in dot notation (input)
 * s		- Control connection socket number (output)
 *
 * Return status
 *	OK			- Successfully connected to the server
 *	ER_INVALID_HOST_NAME	- Invalid server name
 *	ER_CREATE_SOCKET_FAILED	- Cannot create socket
 *	ER_BIND_FAILED		- bind failed
 *	ER_CONNECT_FAILED	- connect failed
 */


int clntConnect (
	char *serverName, /* server IP address in dot notation (input) */
	int *s 		  /* control connection socket number (output) */
	)
{
	int sock;	/* local variable to keep socket number */

	struct sockaddr_in clientAddress;  	/* local client IP address */
	struct sockaddr_in serverAddress;	/* server IP address */
	struct hostent	   *serverIPstructure;	/* host entry having server IP address in binary */


	/* Get IP address os server in binary from server name (IP in dot natation) */
	if((serverIPstructure = gethostbyname(serverName)) == NULL)
	{
		printf("%s is unknown server. \n", serverName);
		return (ER_INVALID_HOST_NAME);  /* error return */
	}

	/* Create control connection socket */
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("cannot create socket ");
		return (ER_CREATE_SOCKET_FAILED);	/* error return */
	}

	/* initialize client address structure memory to zero */
	memset((char *) &clientAddress, 0, sizeof(clientAddress));

	/* Set local client IP address, and port in the address structure */
	clientAddress.sin_family = AF_INET;	/* Internet protocol family */
	clientAddress.sin_addr.s_addr = htonl(INADDR_ANY);  /* INADDR_ANY is 0, which means */
						 /* let the system fill client IP address */
	clientAddress.sin_port = 0;  /* With port set to 0, system will allocate a free port */
			  /* from 1024 to (64K -1) */

	/* Associate the socket with local client IP address and port */
	if(bind(sock,(struct sockaddr *)&clientAddress,sizeof(clientAddress))<0)
	{
		perror("cannot bind");
		close(sock);
		return(ER_BIND_FAILED);	/* bind failed */
	}


	/* Initialize serverAddress memory to 0 */
	memset((char *) &serverAddress, 0, sizeof(serverAddress));

	/* Set ftp server ftp address in serverAddress */
	serverAddress.sin_family = AF_INET;
	memcpy((char *) &serverAddress.sin_addr, serverIPstructure->h_addr, 
			serverIPstructure->h_length);
	serverAddress.sin_port = htons(DATA_CONNECTION_PORT);

	/* Connect to the server */
	if (connect(sock, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0)
	{
		perror("Cannot connect to server ");
		close (sock); 	/* close the control connection socket */
		return(ER_CONNECT_FAILED);  	/* error return */
	}


	/* Store listen socket number to be returned in output parameter 's' */
	*s=sock;

	return(OK); /* successful return */
}  // end of clntConnect() */

/*
 * svcInitServer
 *
 * Came with program
 * 
 * Function to create a socket and to listen for connection request from client
 *    using the created listen socket.
 *
 * Parameters
 * s		- Socket to listen for connection request (output)
 *
 * Return status
 *	OK			- Successfully created listen socket and listening
 *	ER_CREATE_SOCKET_FAILED	- socket creation failed
 */

int svcInitServer (
	int *s 		/*Listen socket number returned from this function */
	)
{
    int sock;
    struct sockaddr_in svcAddr;
    int qlen;

    /*create a socket endpoint */
    if( (sock=socket(AF_INET, SOCK_STREAM,0)) <0)
    {
		perror("cannot create socket");
		return(ER_CREATE_SOCKET_FAILED);
    }

    /*initialize memory of svcAddr structure to zero. */
    memset((char *)&svcAddr,0, sizeof(svcAddr));

    /* initialize svcAddr to have server IP address and server listen port#. */
    svcAddr.sin_family = AF_INET;
    svcAddr.sin_addr.s_addr=htonl(INADDR_ANY);  /* server IP address */
    svcAddr.sin_port=htons(SERVER_FTP_PORT);    /* server listen port # */

    /* bind (associate) the listen socket number with server IP and port#.
     * bind is a socket interface function 
     */
    if(bind(sock,(struct sockaddr *)&svcAddr,sizeof(svcAddr))<0)
    {
		perror("cannot bind");
		close(sock);
		return(ER_BIND_FAILED);	/* bind failed */
    }

    /* 
     * Set listen queue length to 1 outstanding connection request.
     * This allows 1 outstanding connect request from client to wait
     * while processing current connection request, which takes time.
     * It prevents connection request to fail and client to think server is down
     * when in fact server is running and busy processing connection request.
     */
    qlen=1; 


    /* 
     * Listen for connection request to come from client ftp.
     * This is a non-blocking socket interface function call, 
     * meaning, server ftp execution does not block by the 'listen' funcgtion call.
     * Call returns right away so that server can do whatever it wants.
     * The TCP transport layer will continuously listen for request and
     * accept it on behalf of server ftp when the connection requests comes.
     */

    listen(sock,qlen);  /* socket interface function call */

    /* Store listen socket number to be returned in output parameter 's' */
    *s=sock;

    return(OK); /*successful return */
}


/*
 * sendMessage
 *
 * Function to send specified number of octet (bytes) to client ftp
 *
 * Parameters
 * s		- Socket to be used to send msg to client (input)
 * msg  	- Pointer to character arrary containing msg to be sent (input)
 * msgSize	- Number of bytes, including NULL, in the msg to be sent to client (input)
 *
 * Return status
 *	OK		- Msg successfully sent
 *	ER_SEND_FAILED	- Sending msg failed
 */

//NEVER MODIFY SENDMESSAGE
int sendMessage(
	int    s,	/* socket to be used to send msg to client */
	char   *msg, 	/* buffer having the message data */
	int    msgSize 	/* size of the message/data in bytes */
	)
{
    int i;


    /* Print the message to be sent byte by byte as character */
    for(i=0; i<msgSize; i++)
    {
		printf("%c",msg[i]);
    }
    printf("\n");

    if((send(s, msg, msgSize, 0)) < 0) /* socket interface call to transmit */
    {
		perror("unable to send ");
		return(ER_SEND_FAILED);
    }

    return(OK); /* successful send */
}


/*
 * receiveMessage
 *
 * Function to receive message from client ftp
 *
 * Parameters
 * s		- Socket to be used to receive msg from client (input)
 * buffer  	- Pointer to character arrary to store received msg (input/output)
 * bufferSize	- Maximum size of the array, "buffer" in octent/byte (input)
 *		    This is the maximum number of bytes that will be stored in buffer
 * msgSize	- Actual # of bytes received and stored in buffer in octet/byes (output)
 *
 * Return status
 *	OK			- Msg successfully received
 *	R_RECEIVE_FAILED	- Receiving msg failed
 */

//NEVER MODIFY RECEIVEMESSAGE
int receiveMessage (
	int s, 		/* socket */
	char *buffer, 	/* buffer to store received msg */
	int bufferSize, /* how large the buffer is in octet */
	int *msgSize 	/* size of the received msg in octet */
	)
{
    int i;

    *msgSize=recv(s,buffer,bufferSize,0); /* socket interface call to receive msg */

    if(*msgSize<0)
    {
		perror("unable to receive");
		return(ER_RECEIVE_FAILED);
    }

    /* Print the received msg byte by byte */
    for(i=0;i<*msgSize;i++)
    {
		printf("%c", buffer[i]);
    }
    printf("\n");

    return (OK);
}