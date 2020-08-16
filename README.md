CS 372 Intro to Computer Networks Program 2
Programmer: Andrew Sunderland
Last Modified: 3/8/2020

Compile Instructions:
To compile the chatclient, simply use GCC with the following command:
gcc ftserver.c -o ftserver


Execute Instructions:
To run the server, use the following command:
ftserver [PORT NUMBER]
Example: ftserver 46444

To run the client, use the following command:
For List:
python3 ftclient.py [HOST NAME] [COMMAND PORT NUMBER] [COMMAND] [DATA PORT NUMBER]
For Get:
python3 ftclient.py [HOST NAME] [COMMAND PORT NUMBER] [COMMAND] [FILENAME] [DATA PORT NUMBER]
Example: python3 ftclient.py flip2.engr.oregonstate.edu 46444 -l 47442
Example: python3 ftclient.py flip2.engr.oregonstate.edu 46444 -g test.txt 47443

For the host name make sure to use the correct hostname that the ftserver is running on.  For testing, I had ftserver running on flip2 and ftclient on flip1, so I used flip2.engr.oregonstate.edu for the host name and I used port 46444.  Make sure that the port numbers on the client and server match.


Running Instructions:
At startup, the client will connect to the server.  Afterword the client will send the command, filename(if relevant) and data port to the server.  Once the server has verified the message, it will either send a directory listing or file back to the client.  If invalid input is received, it will let the client know with an error.  After the client receives the directory, it will print it to the screen, if it receives a file, it will check to see if the file already exists and if it does it will rename the incoming file by adding '_copy' to the end of it.  Either way, it will create a new file and copy the data from the message sent by the server.
To quit on the server send an SIGINT signal.

Extra Credit:
The server and client are set up to send a file using binary, so any file type should be supported for sending as long as it's under 2GB.
