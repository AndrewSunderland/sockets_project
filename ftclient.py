#!/usr/bin/env python3

from socket import *
import sys


# Programmer: Andrew Sunderland
# Program Name: chatserve.py
# Program Description: This is a chat program that can request directory listings and files from a server
# Course Name: CS 372 Intro to Computer Networks
# Last Modified: 3/8/2020


###############################################################################################
# Function: initiateContact
# Description: Sets up the socket for the client attempts to connect to the server
# Parameters: argv
# Pre-Conditions: The command line arguments are checked and valid
# Post-Conditions: The socket is set up and returned for use
###############################################################################################
def initiateContact(argv):
    server, clientPort = argv[1], int(argv[2])
    clientSocket = socket(AF_INET, SOCK_STREAM)  # Set up the socket
    clientSocket.connect((server, clientPort))  # Connecting to the command socket
    return clientSocket


###############################################################################################
# Function: makeRequest
# Description: Sends the command and data port to the server
# Parameters: connectionSocket, argv
# Pre-Conditions: There must be a socket created and initialized already
# Post-Conditions: A command is sent via the socket and the data port is returned from the
# function
###############################################################################################
def makeRequest(connectionSocket, argv):
    command = argv[3]
    if len(argv) == 6:  # For the Get file command
        file = argv[4]
        dataPort = argv[5]
        message = command + "$$" + file + "&&" + dataPort + "@@"
    else:  # For the List command
        dataPort = argv[4]
        message = command + "$$" + dataPort + "@@"

    # Send the command via the command socket
    connectionSocket.send(message.encode())
    statusMessage = connectionSocket.recv(1024).decode()  # Receive status message from server
    if len(statusMessage) > 5:  # For invalid commands or files not found errors
        print(statusMessage)
        exit(2)

    return dataPort


###############################################################################################
# Function: receiveData
# Description: Receives input from the socket print a directory listing or create a file
# Parameters: connectionSocket, argv, dataPort, commandSocket
# Pre-Conditions: There must be two sockets set up and connected, one for command and one for
# data transfer
# Post-Conditions: Either the directory listing is printed to the screen or a file is
# transferred and copied into a newly created file
###############################################################################################
def receiveData(connectionSocket, argv, dataPort, commandSocket):
    command = argv[3]
    serverIP = argv[1]
    filename = argv[4]

    # If the List command was sent
    if command == "-l":
        print("Receiving directory structure from", serverIP, ":", dataPort)
        while 1:
            message = connectionSocket.recv(1024).decode()
            if message.find("@@") != -1:  # Checking for @@ at the end of the message
                print(message[:-3])
                return 0
            print(message)

    # If the Get command was sent
    else:
        print("Receiving", filename, "from", serverIP, ":", dataPort)
        try:  # Checking to see if the filename is already in working folder
            fileD = open(filename)
            filename = filename[:-4] + "_copy" + filename[-4:]  # Adding _copy to end of filename for duplicate
        except IOError:
            pass

        fileD = open(filename, "wb")  # Creating the file to be copied to
        fileSize = commandSocket.recv(256).decode()  # Receiving the file size of incoming file
        fileSizeInt = int(fileSize)

        while fileSizeInt > 0:  # Continue to receive until file is completely sent
            message = connectionSocket.recv(1024)  # Receive file from server
            fileD.write(message)  # Copy file binary to newly created file
            fileSizeInt = fileSizeInt - len(message)  # Update remaining file size to be received

        print("File transfer complete.")


if __name__ == '__main__':
    if len(sys.argv) < 5:
        print("Not enough arguments!")
        exit(2)
    clientSocket = initiateContact(sys.argv)  # Initialize the command socket
    dataPort = makeRequest(clientSocket, sys.argv)  # Send command to server

    # Set up the data socket
    dataSocket = socket(AF_INET, SOCK_STREAM)
    dataSocket.connect((sys.argv[1], int(dataPort)))

    done = receiveData(dataSocket, sys.argv, dataPort, clientSocket)  # Receive data from server over data socket

    # Close out the sockets
    dataSocket.close()
    clientSocket.close()
