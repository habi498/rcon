#include <iostream>
#include <winsock2.h>
#include <string>
#include <algorithm>
using namespace std;

#pragma comment(lib,"ws2_32.lib") 
#pragma warning(disable:4996) 
#include "tclap/CmdLine.h"
#define SERVER "127.0.0.1"  // or "localhost" - ip address of UDP server
#define BUFLEN 512  // max length of answer
#define PORT 8888  // the port on which to listen for incoming data

int main(int argc, char** argv)
{
    std::string hostname, password, command;
    bool interactive; int port; bool loggedin = false;

    char reply[1024];
    //some variables inside goto
    short cmdlen; char answer[BUFLEN];
    int slen;
    int answer_length; short msglen;
    char input[512];
    // Wrap everything in a try block.  Do this every time, 
    // because exceptions will be thrown for problems.
    try {

        // Define the command line object, and insert a message
        // that describes the program. The "Command description message" 
        // is printed last in the help text. The second argument is the 
        // delimiter (usually space) and the last one is the version number. 
        // The CmdLine object parses the argv array based on the Arg objects
        // that it contains. 
        TCLAP::CmdLine cmd("RCON client for VCMP 0.4", ' ', "1",false);

        // Define a value argument and add it to the command line.
        // A value arg defines a flag and a type of value that it expects,
        // such as "-n Bishop".
        TCLAP::ValueArg<std::string> hostArg("h", "hostname", "Server IP", true, "127.0.0.1", "string");
        TCLAP::ValueArg<int> portArg("P", "port", "Server Port", true, 8192, "integer");
        TCLAP::ValueArg<std::string> passwordArg("p", "password", "RCON Password", true, "", "string");
        TCLAP::SwitchArg interactiveSwitch("i","interactive", "Run in interactive mode", cmd, false);
        TCLAP::ValueArg<std::string> commandArg("c", "command", "Execute command and exit", false, "", "string");
        // Add the argument nameArg to the CmdLine object. The CmdLine object
        // uses this Arg to parse the command line.
        cmd.add(hostArg);
        cmd.add(portArg);
        cmd.add(passwordArg);
        cmd.add(commandArg);

        
        // Parse the argv array.
        cmd.parse(argc, argv);

        // Get the value parsed by each arg. 
        hostname = hostArg.getValue();
        password = passwordArg.getValue();
        port = portArg.getValue();
        interactive = interactiveSwitch.getValue();
        if (commandArg.isSet() && interactive)
        {
            printf("Error: Do not choose both 'interactive' and 'command'");
            exit(0);
        }
        if (commandArg.isSet() == false && !interactive)
        {
            printf("Error: Atleast one of -i or -c parameter is needed");
            exit(0);
        }
        command = commandArg.getValue();

    }
    catch (TCLAP::ArgException& e)  // catch exceptions
    {
        std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
    }
    WSADATA ws;
    //printf("Initialising Winsock...");
    if (WSAStartup(MAKEWORD(2, 2), &ws) != 0)
    {
        printf("Failed. Error Code: %d", WSAGetLastError());
        return 1;
    }
    //printf("Initialised.\n");

    // create socket
    sockaddr_in server;
    int client_socket;
    if ((client_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR) // <<< UDP socket
    {
        printf("socket() failed with error code: %d", WSAGetLastError());
        return 2;
    }

    // setup address structure
    memset((char*)&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.S_un.S_addr = inet_addr(hostname.c_str());

    // start communication
    while (true)
    {
        char message[BUFLEN];
        sprintf(message, "VCMP");
        char* ip = inet_ntoa(server.sin_addr);
        char* token = strtok(ip, "."); int j = 0;
        while (token != NULL)
        {
            if (j == 4)
            {
                printf("Bad IP Address\n");
                exit(0);
            }
            message[4 + j] = (char)stoi(token);
            token = strtok(NULL, ".");
            j++;
        }
        message[8] = (char)(port & 0xFF);
        message[9] = (char)((port >> 8) & 0xFF);
        message[10] = 'r';
        short passlen = password.length();
        message[11] = (char)((passlen >> 8) & 0xFF);
        message[12] = (char)(passlen & 0xFF);
        memcpy(message + 13, password.c_str(), passlen);
    one:
        if (interactive && loggedin==false)
        {
            message[13 + passlen] = 0;
            message[14 + passlen] = 1;
            message[15 + passlen] = 'L';//Login
            // send the message
            if (sendto(client_socket, message, 15 + passlen + 1, 0, (sockaddr*)&server, sizeof(sockaddr_in)) == SOCKET_ERROR)
            {
                printf("sendto() failed with error code: %d", WSAGetLastError());
                return 3;
            }
        }
        else
        {
           cmdlen= command.length();
           message[13 + passlen] = (char)((cmdlen >> 8) & 0xFF);
           message[14 + passlen] = (char)(cmdlen & 0xFF);
           memcpy(message + 15 + passlen, command.c_str(), cmdlen);
           // send the message
           if (sendto(client_socket, message, 15 + passlen+cmdlen, 0, (sockaddr*)&server, sizeof(sockaddr_in)) == SOCKET_ERROR)
           {
               printf("sendto() failed with error code: %d", WSAGetLastError());
               return 3;
           }
        }
        

        // receive a reply and print it
        // clear the answer by filling null, it might have previously received data
        memset(answer, 0, BUFLEN);
        // try to receive some data, this is a blocking call
        slen = sizeof(sockaddr_in);
        
        answer_length = recvfrom(client_socket, answer, BUFLEN, 0, (sockaddr*)&server, &slen);
        if (answer_length == SOCKET_ERROR)
        {
            printf("recvfrom() failed with error code: %d", WSAGetLastError());
            exit(0);
        }
        if (answer_length < 13)
        {
            printf("Invalid reply"); exit(0);
        }
        msglen = (unsigned char)answer[11] * 256 + (unsigned char)answer[12];
        
        if (answer_length < (13 + msglen))
            exit(0);
        
        memcpy(reply, answer + 13, msglen);
        reply[msglen] = '\0';
        if (interactive )
        {
            if (loggedin==false && strcmp(reply, "OK") == 0)
            {
                //password was correct
                
                loggedin = true; 
                goto readcommand;
            }else if (loggedin==false && strcmp(reply, "NO") == 0)
            {
                //password was incorrect
                printf("RCON password incorrect"); exit(0);
            }
        
            else if (loggedin == true)
            {
                if (command.find("rcon_password ") == 0)
                {
                    if (reply[0] == 'r' && reply[1] == 'c' && reply[2] == 'o' &&
                        reply[3] == 'n' && reply[4] == '_' && reply[5] == 'p' &&
                        reply[6] == 'a' && reply[7] == 's' && reply[8] == 's'
                        && reply[9] == 'w' && reply[10] == 'o' &&
                        reply[11] == 'r' && reply[12] == 'd' && reply[13] == ' ')
                    {
                        if (strlen(reply) > 14)
                        {
                            password = string(reply + 14);
                            passlen = password.length();
                            message[11] = (char)((passlen >> 8) & 0xFF);
                            message[12] = (char)(passlen & 0xFF);
                            memcpy(message + 13, password.c_str(), passlen);
                        }
                    }
                }else
                    cout << reply << "\n"; 
        readcommand:    
                printf(">");
                fgets(input, sizeof(input), stdin);
                if (input[strlen(input) - 1] == 0xa)
                    input[strlen(input) - 1] = 0;
                command = string(input);
                goto one;
            }
        }
        else
        {
            cout << reply << "\n"; 
            exit(0);
        }
        
    }

    closesocket(client_socket);
    WSACleanup();
}