#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <dirent.h>
#include <arpa/inet.h>



/*
Programmer: Andrew Sunderland
Program Name: ftserver
Program Description: This is a server program that listens for clients to connect.  Once connected the client
can ask the server for a directory listing or a file to be sent
Course Name: CS 372 Intro to Computer Networks
Last Modified: 3/8/2020
*/



/*
# Function: startUp
# Description : Sets up the socket for the server and starts listening for connections
# Parameters : char* portNum
# Pre - Conditions: A port number must be provided for the server to use
# Post - Conditions: The socket is set up and returned for use
*/
int startUp(char* portNum)
{
	int socketFD, portNumber;
	struct sockaddr_in serverAddress;

	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(portNum); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) fprintf(stderr, "CLIENT: ERROR opening socket");

	// Bind and start listening on the socket
	if (bind(socketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
	{
		fprintf(stderr, "ERROR on binding\n", 18);
		return -1;
	}
		
	listen(socketFD, 5); // Flip the socket on so it can now receive up to 5 connections

	return socketFD;
}


/*
Function: handleRequest
Description: Receives input from the command socket to either send a directory listing or a file to be sent 
on a newly created data socket
Parameters: int socketFD
Pre-Conditions: The command socket must be created and initialized
Post-Conditions: A directory listing or a binary file is sent over a data socket 
*/
void handleRequest(int socketFD)
{
	int charsRead, charsWritten, terminalLocation, dataPortLocation, fileLocation, dataPort;
	static char buffer[500], command[500], dataPortstr[500], filestr[500], clientIP[20], directorystr[5000];
	char * invalidCommand = "Invalid command recieved closing connection.";
	char * fileNotFound = "FILE NOT FOUND";
	void * fileBuffer = malloc(2000000000);  // Allocating enough memory for a file to be stored completely in memory

	// Getting client's ip for later use
	struct sockaddr_in clientaddr;
	socklen_t clientSize = sizeof(clientaddr);
	int result = getpeername(socketFD, (struct sockaddr *)&clientaddr, &clientSize);
	memset(clientIP, '\0', sizeof(clientIP));
	strcpy(clientIP, inet_ntoa(clientaddr.sin_addr));

	// Receiving message from server
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
	charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); // Read data from the socket, leaving \0 at end
	if (charsRead < 0) error("CLIENT: ERROR reading from socket");

	// Parse through message for commands
	int commandLocation = strstr(buffer, "$$") - buffer;
	memset(command, '\0', sizeof(command));
	strcpy(command, buffer);
	command[commandLocation] = '\0';

	// If no valid command is received
	if (strcmp(command, "-l") != 0 && strcmp(command, "-g") != 0) 
	{
		printf("Invalid message received!\n");
		charsWritten = send(socketFD, invalidCommand, strlen(invalidCommand), 0);  // Sending invalid command error to client
		if (charsWritten < 0) fprintf(stderr, "SERVER: ERROR writing to socket", 32);
		if (charsWritten < strlen(invalidCommand)) printf("SERVER: WARNING: Not all data written to socket!\n");
	}

	// If a valid command is received
	else
	{
		memset(dataPortstr, '\0', sizeof(dataPortstr));
		
		// Received the GET command
		if (strstr(buffer, "&&") != NULL) 
		{
			// Separating the file name from the message
			fileLocation = strstr(buffer, "$$") - buffer;
			strncpy(filestr, buffer + fileLocation + 2, strlen(buffer));
			terminalLocation = strstr(filestr, "&&") - filestr;
			filestr[terminalLocation] = '\0';

			// Separating the data port from the message
			dataPortLocation = strstr(buffer, "&&") - buffer;
			strncpy(dataPortstr, buffer + dataPortLocation + 2, strlen(buffer));
			terminalLocation = strstr(dataPortstr, "@@") - dataPortstr;
			dataPortstr[terminalLocation] = '\0';
			dataPort = atoi(dataPortstr);  // Converting data port string to integer


			// Locating file to send
			FILE * fileD = fopen(filestr, "rb");
			printf("File %s is requested on port %d.\n", filestr, dataPort);
			if (fileD == NULL)  // If the file can't be located
			{
				printf("File not found. Sending error message to %s:%d\n", clientIP, dataPort);
				charsWritten = send(socketFD, fileNotFound, strlen(fileNotFound), 0);  // Sending file not found to client
				if (charsWritten < 0) fprintf(stderr, "SERVER: ERROR writing to socket", 32);
				if (charsWritten < strlen(fileNotFound)) printf("SERVER: WARNING: Not all data written to socket!\n");
				free(fileBuffer);
				return;
			}
			// File is located and sent
			else
			{
				charsWritten = send(socketFD, "Good", 4, 0); // Valid command received
				if (charsWritten < 0) fprintf(stderr, "SERVER: ERROR writing to socket", 32);
				if (charsWritten < 4) printf("SERVER: WARNING: Not all data written to socket!\n");

				printf("Sending %s to %s:%d\n", filestr, clientIP, dataPort);

				// Opening Data Socket
				int listenDataFD = startUp(dataPortstr);
				if (listenDataFD == -1)
				{
					free(fileBuffer);
					return;
				}
				int dataFD = accept(listenDataFD, (struct sockaddr *)&clientaddr, &clientSize);
				if (dataFD < 0) fprintf(stderr, "ERROR on accept\n", 17);

				// Preparing the file to be sent
				fseek(fileD, 0, SEEK_END);
				long fileSize = ftell(fileD);  // Getting the file size
				size_t bytesRead;
				fseek(fileD, 0, SEEK_SET);
				bytesRead = fread(fileBuffer, fileSize, 1, fileD);  // Copying the file into memory

				char fsize[256];
				sprintf(fsize, "%ld", fileSize);  // Converting the file size to an integer

				// Sending the file size to the client
				charsWritten = send(socketFD, fsize, strlen(fsize), 0);
				if (charsWritten < 0) fprintf(stderr, "SERVER: ERROR writing to socket", 32);

				
				// Sending the file to the client
				while (fileSize > 0)
				{
					charsWritten = send(dataFD, fileBuffer, fileSize, 0);
					if (charsWritten < 0) fprintf(stderr, "SERVER: ERROR writing to socket", 32);
					fileSize = fileSize - charsWritten;
				} 

				close(dataFD);  // Close out the data socket
			}
			free(fileBuffer);  // Free up the file memory
			fclose(fileD);  // Close out the file stream
		}

		// Received the LIST command
		else 
		{
			charsWritten = send(socketFD, "Good", 4, 0); // Valid command received
			if (charsWritten < 0) fprintf(stderr, "SERVER: ERROR writing to socket", 32);
			if (charsWritten < 4) printf("SERVER: WARNING: Not all data written to socket!\n");

			// Separating the data port from the message
			dataPortLocation = strstr(buffer, "$$") - buffer;
			strncpy(dataPortstr, buffer + dataPortLocation + 2, strlen(buffer));
			terminalLocation = strstr(dataPortstr, "@@") - dataPortstr;
			dataPortstr[terminalLocation] = '\0';
			dataPort = atoi(dataPortstr);  // Converting data port string to integer.

			// Opening Data Socket
			int listenDataFD = startUp(dataPortstr);
			int dataFD = accept(listenDataFD, (struct sockaddr *)&clientaddr, &clientSize);
			if (dataFD < 0) fprintf(stderr, "ERROR on accept\n", 17);

			printf("List directory requested on port %d.\n", dataPort);
			printf("Sending directory contents to %s : %d\n", clientIP, dataPort);

			// Getting working directory
			DIR* currentDir;
			struct dirent *currentFile;
			currentDir = opendir("./");
			memset(directorystr, '\0', sizeof(directorystr));

			// Reading through files in CWD and copying to a string
			while ((currentFile = readdir(currentDir)) != NULL) 
			{
				memset(buffer, '\0', sizeof(buffer));
				strcpy(buffer, currentFile->d_name);
				buffer[strlen(buffer)] = '\n';
				strcat(directorystr, buffer);

			}
			strcat(directorystr, "@@"); // Adding terminator for client

			// Sending directory to client
			charsWritten = send(dataFD, directorystr, strlen(directorystr), 0);
			if (charsWritten < 0) fprintf(stderr, "SERVER: ERROR writing to socket", 32);
			if (charsWritten < strlen(directorystr)) printf("SERVER: WARNING: Not all data written to socket!\n");

			close(dataFD);  // Close out the data socket
		}
	}

}


int main(int argc, char *argv[])
{
	int listenFD, charsWritten, charsRead, connectedFD;
	char message[510];
	socklen_t clientSize;
	struct sockaddr_in clientIP;
	
	if (argc < 2) { fprintf(stderr, "USAGE: %s port\n", argv[0]); exit(0); } // Check usage & args

	listenFD = startUp(argv[1]); // Start up the server
	if (listenFD == -1) return 2;
	printf("Server open on %s\n", argv[1]);

	// Loop for continual acceptance
	while (1)
	{

		// Accept the incoming connection
		clientSize = sizeof(clientIP); // Get the address size of connecting client
		connectedFD = accept(listenFD, (struct sockaddr *)&clientIP, &clientSize);
		if (connectedFD < 0) fprintf(stderr, "ERROR on accept\n", 17);

		// Handle the incoming request
		handleRequest(connectedFD);
		
		close(connectedFD);  // Close out the command socket
	}

	close(listenFD); // Close the listen socket
	return 0;
}
