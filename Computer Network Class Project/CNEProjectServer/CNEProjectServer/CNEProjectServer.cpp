
#define _CRT_SECURE_NO_WARNINGS                 
#define _WINSOCK_DEPRECATED_NO_WARNINGS         
#include <winsock2.h>
#include <winsock.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string>
#include <Windows.h>
#include <ws2tcpip.h>
#include <map>

#undef max
#define s_addr S_un.S_addr
#pragma comment(lib,"Ws2_32.lib")
using namespace std;

enum color { RED = 4, GREEN = 10, YELLOW = 14, WHITE = 15 };

void serverCode();
int recvWhole(SOCKET s, char* buffer, int length);
string receiveMessage(SOCKET comSocket);
int sendMessage(SOCKET s, string input);
void closeSocket(SOCKET comSocket);
void saveLog( FILE *stream, const char* text);
void colorWord(std::string word, enum::color color);
void errorColor(std::string error);

int main()
{
	WSADATA wsadata;
	WSAStartup(WINSOCK_VERSION, &wsadata);

	serverCode();

	return WSACleanup();
}

void serverCode() 
{
	colorWord("+----------+\n", YELLOW);
	colorWord("|  ", YELLOW);
	colorWord("SERVER", WHITE);
	colorWord("  |\n", YELLOW);
	colorWord("+----------+\n", YELLOW);


	// Creating log file
	FILE *logFile;
	logFile = fopen("ChatLog.txt", "w");

	// Creating UDP socket (phase 2)
	std::cout << "\nCreating UDP Broadcats socket..." << endl;
	SOCKET broadcastSocket;

	broadcastSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (broadcastSocket == INVALID_SOCKET)
	{
		errorColor("creating the Broadcast socket.");
		return;
	}
	else
	{
		colorWord("UDP Broadcast socket created.\n", GREEN);
	}

	// Changing broadcast socket SO_BROADCAST option (phase 2)
	int a = 1;
	
	if (setsockopt(broadcastSocket, SOL_SOCKET, SO_BROADCAST, (const char*)&a, sizeof(a)) != 0)
	{
		colorWord("Unable to change Broadcast socket broadcast option.\n", RED);
		return;
	}
	else
	{
		colorWord("Broadcast socket broadcats option changed.\n", GREEN);
	}

	// Changing broadcast socket SO_REUSEADDR option (phase 2)
	if (setsockopt(broadcastSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&a, sizeof(a)) != 0)
	{
		colorWord("Unable to change Broadcast socket address reuse option.\n", RED);
		return;
	}
	else
	{
		colorWord("Broadcast socket address reuse option changed.\n", GREEN);
	}

	// Creating listening socket
	std::cout << "\nCreating TCP socket..." << endl;
	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET)
	{
		errorColor("creating the Listening socket.");
		return;
	}
	else
	{
		colorWord("Listening socket created.\n", GREEN);
	}

	
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	serverAddr.sin_port = htons(31337);

	// Binding listening socket
	int result = bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (result == SOCKET_ERROR)
	{
		errorColor("binding Listening socket.");
		return;
	}
	else
	{
		colorWord("Listening socket successfuly bounded.\n", GREEN);
	}

	// Binding broadcast socket (phase 2)
	result = bind(broadcastSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (result == SOCKET_ERROR)
	{
		errorColor("binding Broadcast socket.");
		return;
	}
	

	//Listening
	result = listen(listenSocket, 1);
	if (result == SOCKET_ERROR)
	{
		errorColor("listening.");
		return;
	}
	else
	{
		colorWord("Broadcast socket successfuly bounded.\n", GREEN);
		colorWord("Broadcasting >)))...\n", YELLOW);

		colorWord("\n****", RED);
		colorWord(" CHAT IS OPEN ", GREEN);
		colorWord("****\n", RED);
	}

	// Creating sets for multiplexing
	fd_set master;
	fd_set ready;
	timeval timeout;

	FD_ZERO(&master);
	FD_ZERO(&ready);
	timeout.tv_sec = 1;

	// Keeping track of subscribed Users
	typedef struct User
	{
		string userName = "";
		int socket = 0;
	};

	// List of Users in the chat
	typedef map<int, string> Users;
	Users users;
	int activeUsers = 0;

	// Adding listening socket to the master set
	FD_SET(listenSocket, &master);

	// Adding broadcast socket to master set (phase 2)
	FD_SET(broadcastSocket, &master);

	sockaddr_in broadcastAddr;
	broadcastAddr.sin_family = AF_INET;
	broadcastAddr.sin_addr.S_un.S_addr = INADDR_BROADCAST;
	broadcastAddr.sin_port = htons(31330);

	// Creating server loop
	while (true)
	{
		// Log file 
		FILE *chatLog = freopen("ChatLog.txt", "a", logFile);

		// Cheking to see if any socket is ready
		ready = master;
		int rc = select(0, &ready, NULL, NULL, &timeout);


		std::string addrMessage = "127.0.0.1:31337:";
		addrMessage = addrMessage.insert(0, 1, addrMessage.size());
		int size = addrMessage.size();
		int result = sendto(broadcastSocket, addrMessage.c_str(), size, 0, (SOCKADDR*)&broadcastAddr, sizeof(broadcastAddr));
		if (result == SOCKET_ERROR)
		{
			errorColor("sending server ip and port.");
		}

		// Checking for new incoming conections
		if (FD_ISSET(listenSocket, &ready))
		{
			// Accepting	
			SOCKET comSocket = accept(listenSocket, NULL, NULL);
			if (comSocket == INVALID_SOCKET)
			{
				errorColor("while creating comSocket.");
				closeSocket(comSocket);
				continue;
			}
			else if (comSocket == SOCKET_ERROR)
			{
				errorColor("while creating comSocket.");
				continue;
			}

			// Adding potential user to the users list
			User user;
			user.socket = comSocket;
			users.emplace(user.socket, "noname");

			
			// Adding socket to master
			FD_SET(comSocket, &master);

			// Removing listening socket from ready set
			FD_CLR(listenSocket, &ready);
		}

		if (ready.fd_count > 0)
		{
			// Looping through the rest of the ready sockets
			for (int i = 0; i < ready.fd_count; i++)
			{
				SOCKET currSocket = ready.fd_array[i];

				// Receiving from sockets that are ready
				string message = receiveMessage(currSocket);
				

				if (message == "SHUTDOWN" || message == "SOCKET_ERROR")
				{
					if (users.find(currSocket)->second != "noname")
					{

						string announcement = "SERVER>>> " + users.find(currSocket)->second + " has left the chat.\n";
						const char* text = announcement.c_str();

						// Saving to log
						saveLog(chatLog, text);

						// To console
						colorWord("SERVER", YELLOW);
						std::cout << ">>> ";
						colorWord(users.find(currSocket)->second, GREEN);
						std::cout <<  + " has left the chat.\n";

						// Decreasing active user count
						activeUsers -= 1;

						// Removing user
						users.erase(users.find(currSocket)->first);

					}
					else
					{
						// Removing unregistered user
						users.erase(users.find(currSocket)->first);
					}
					// Exit procedure
					closeSocket(currSocket);
					FD_CLR(currSocket, &master);
					FD_CLR(currSocket, &ready);
					
				}
				else if (message[0] == '$')
				{
					string userCommand = users.find(currSocket)->second + ">>> " + message + "\n";

					// Saving to log
					saveLog(chatLog, userCommand.c_str());

					string command;
					// Accepting commands
					for (int j = 0; j < message.size(); j++)
					{
						if (message[j] != ' ')
						{
							command += message[j];
						}
						else
						{
							break;
						}
					}

					if (command == "$register")
					{
						if (activeUsers < 3 && users.find(currSocket)->second == "noname")
						{
							// Errasing noname
							users.find(currSocket)->second = "";
							// Adding username to the user
							for (int k = 0; k < message.size(); k++)
							{
								if (message[k] == '<')
								{
									k++;
									while (message[k] != '>')
									{
										users.find(currSocket)->second += message[k];
										k++;
									}
								}
							}
							// Announcing
							colorWord("SERVER", YELLOW);
							std::cout << ">>> ";
							colorWord(users.find(currSocket)->second, GREEN);
							std::cout << +" has joined the chat.\n";

							string announcement = "SERVER>>> " + users.find(currSocket)->second + " has joined the chat.\n";
							const char* text = announcement.c_str();

							// Saving to log
							saveLog(chatLog, text);

							// Adding 1 to active users count
							activeUsers += 1;
							// Replying to user
							sendMessage(currSocket, "SV_SUCCESS");

						}
						else if (users.find(currSocket)->second != "noname")
						{
							// Sending already registered message
							int sendResult = sendMessage(currSocket, "You are already register.\n");
							if (sendResult == SOCKET_ERROR)
							{
								errorColor("while sending already registered message.");
							}
						}
						else
						{
							// Not enough space in the chat
							int sendResult = sendMessage(currSocket, "SV_FULL");
							if (sendResult == SOCKET_ERROR) 
							{
								errorColor("while sending chat is full message.");
							}

							closeSocket(currSocket);
							FD_CLR(currSocket, &master);
							FD_CLR(currSocket, &ready);

							// Announcing
							colorWord("\nSERVER", YELLOW);
							colorWord(">>> Incoming connection ", WHITE);
							colorWord("rejected. ", RED);
							colorWord("Chat is full.\n", WHITE);
							
							// Saving to log
							saveLog(chatLog, "SERVER>>> Incoming connection rejected. Chat is full.\n");

							// Removing unregistered user
							users.erase(users.find(currSocket)->first);
						}

					}
					else if (command == "$exit")
					{
						// Exit procedure
						closeSocket(currSocket);
						FD_CLR(currSocket, &master);
						FD_CLR(currSocket, &ready);

						string announcement = "SERVER>>> " + users.find(currSocket)->second + " has left the chat.\n";
						const char *text = announcement.c_str();

						// Saving to log
						saveLog(chatLog, text);

						// Announcing
						colorWord("SERVER", YELLOW);
						std::cout << ">>> ";
						colorWord(users.find(currSocket)->second, GREEN);
						std::cout << +" has left the chat.\n";

						activeUsers -= 1;

						users.erase(users.find(currSocket)->first);
						
					}
					else if (command == "$getlist")
					{
						string usersList;
						if (activeUsers > 0)
						{
							// Adding first user to the list
							usersList += users.begin()->second;

							// Adding the rest of the users
							for (Users::const_iterator it = ++users.begin(); it != users.end(); ++it) {
								//++it;
								usersList = usersList + ", " + it->second;
							}

							if (activeUsers == 1)
							{
								int sendResult = sendMessage(currSocket, usersList + " is connected.\n");
								if (sendResult == SOCKET_ERROR) {
									errorColor("while sending users list message.");
								}
							}
							else
							{
								int sendResult = sendMessage(currSocket, usersList + " are connected.\n");
								if (sendResult == SOCKET_ERROR) {
									errorColor("while sending users list message.");
								}
							}
						}
						else
						{
							int sendResult = sendMessage(currSocket, "There is no one connected right now.");
							if (sendResult == SOCKET_ERROR) {
								errorColor("while sending users list message.");
							}
						}
					}
					else if (command == "$getlog")
					{
						int c;
						string buffer;
						// Opening Log file 
						FILE* outFile = freopen("ChatLog.txt", "r", logFile);
						if (outFile != NULL)
						{
							// Sending size

							while (true)
							{
								c = fgetc(outFile);
								if (feof(outFile))
								{
									int sendResult = sendMessage(currSocket, "***");
									if (sendResult == SOCKET_ERROR) {
										errorColor("while sending logfile end signal.");
									}
									break;
								}

								if (c == 10)
								{
									int sendResult = sendMessage(currSocket, buffer + "\n");
									if (sendResult == SOCKET_ERROR) {
										errorColor("while sending logfile line.");
									}
									buffer.clear();
									c = NULL;
								}
								else
								{
									buffer += c;
								}
							}
						}
					}
					else
					{
						string help2 = "\n$register -- It signs you into the chat. Ex. &register <user_name>\n";
						string help3 = "$exit ------ It exits the chat\n";
						string help4 = "$getlist --- It shows you who is currently in the chat\n";
						string help5 = "$getlog ---- It gets you a copy of the entire chat\n";

						string help = help2 + help3 + help4 + help5;
						int sendResult = sendMessage(currSocket, help);
						if (sendResult == SOCKET_ERROR) {
							errorColor("while sending help message.");
						}
					}
				}
				else
				{
					if (users.find(currSocket)->second != "noname")
					{
						string str = users.find(currSocket)->second + ">>> " + message + "\n";
						const char* text = str.c_str();

						colorWord(users.find(currSocket)->second, GREEN);
						std::cout << ">>> " << message << endl;

						// Saving to log
						saveLog(chatLog, text);
					}
					else
					{
						int sendResult = sendMessage(currSocket, "You have not register");
						if (sendResult == SOCKET_ERROR) {
							errorColor("while sending register message.");
						}
						sendResult = sendMessage(currSocket, "Use $register <your_username> to register.");
						if (sendResult == SOCKET_ERROR) {
							errorColor("while sending register message.");
						}
					}

					FD_CLR(currSocket, &ready);
				}
			}

		}
		


	}

	closesocket(listenSocket);
	closesocket(broadcastSocket);
	std::fclose(logFile);

}

void closeSocket(SOCKET s) {
	shutdown(s, SD_BOTH);
	closesocket(s);
}

void saveLog(FILE* stream, const char* text)
{
	if (stream != NULL)
	{
		fprintf(stream, text);
	}
}

int recvWhole(SOCKET s, char* buffer, int length)
{
	int total = 0;

	do
	{
		int ret = recv(s, buffer + total, length - total, 0);
		if (ret < 1)
			return ret;
		else
			total += ret;

	} while (total < length);

	return total;
}

string receiveMessage(SOCKET comSocket)
{
	int size = 0;
	string message;

	int recvdResult = recvWhole(comSocket, (char*)&size, 1);
	if ((recvdResult == SOCKET_ERROR))
	{
		message = "SOCKET_ERROR";
		return message;
	}
	else if (recvdResult == 0)
	{
		message = "SHUTDOWN";
		return message;
	}

	char* buffer = new char[size];

	recvdResult = recvWhole(comSocket, (char*)buffer, size);
	if ((recvdResult == SOCKET_ERROR))
	{
		message = "SOCKET_ERROR";
	}
	else if (recvdResult > 0)
	{
		message = string(buffer, 0, recvdResult);
	}

	delete[] buffer;

	return message;
}


int sendMessage(SOCKET comSocket, string userInput)
{
	string data = userInput.insert(0, 1, userInput.size());
	const char* text = data.c_str();

	int sendResult = send(comSocket, text, data.size(), 0);

	return sendResult;

}

void colorWord(std::string word, enum::color color)
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	switch (color)
	{
	case RED:
		SetConsoleTextAttribute(hConsole, RED);
		break;
	case GREEN:
		SetConsoleTextAttribute(hConsole, GREEN);
		break;
	case YELLOW:
		SetConsoleTextAttribute(hConsole, YELLOW);
		break;
	default:
		break;
	}

	std::cout << word;
	SetConsoleTextAttribute(hConsole, WHITE);
}

void errorColor(std::string error)
{
	std::cout << "There was an ";
	colorWord("ERROR", RED);
	std::cout << " " << error << std::endl;
}