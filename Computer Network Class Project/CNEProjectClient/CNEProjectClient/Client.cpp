#define _CRT_SECURE_NO_WARNINGS                 
#define _WINSOCK_DEPRECATED_NO_WARNINGS         
#include <winsock2.h>
#include <winsock.h>
#include <iostream>
#include <string>
#include <fstream>
#include <stdio.h>
#include <Windows.h>


#undef max
#define s_addr S_un.S_addr
#pragma comment(lib,"Ws2_32.lib")
//using namespace std;

enum color { RED = 4, GREEN = 10, YELLOW = 14, WHITE = 15 };

std::string gettingUserName();
void clearScreen();
int recvWhole(SOCKET s, char* buffer, int length);
std::string receiveMessage(SOCKET comSocket);
int sendMessage(SOCKET s, std::string input);
void colorWord(std::string word, enum::color color);
void errorColor(std::string error);
void userHeader(std::string name);
void clientCode();


int main() {

	WSADATA wsadata;
	WSAStartup(WINSOCK_VERSION, &wsadata);

	clientCode();

	return WSACleanup();
}

void clientCode() {

	clearScreen();

	std::string ipAddr;
	std::string chatPort;
	std::string listeningPort = "31330";
	std::string userName;
	SOCKET comSocket;
	bool gettingLog;
	FILE* logFile;

	// Listening address
	sockaddr_in listenAddr;
	listenAddr.sin_family = AF_INET;
	listenAddr.sin_addr.s_addr = INADDR_ANY;
	listenAddr.sin_port = htons(atoi(listeningPort.c_str()));

	//The client is not receiving on the UDP socket.
	

	// Creating UDP socket (phase 2)
	std::cout << "\nCreating UDP Listening socket..." << std::endl;
	SOCKET listeningSocket;

	listeningSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (listeningSocket == INVALID_SOCKET)
	{
		errorColor("creating the Listening socket");
		return;
	}
	else
	{
		colorWord("UDP Listening socket created.\n", GREEN);
	}

	// Binding broadcast socket (phase 2)
	int result = bind(listeningSocket, (SOCKADDR*)&listenAddr, sizeof(listenAddr));
	if (result == SOCKET_ERROR)
	{
		errorColor("binding Listening socket");
		return;
	}
	else
	{
		colorWord("Listening socket successfuly bounded.\n", GREEN);
	}

	// Creating sets for multiplexing
	fd_set master;
	fd_set ready;
	timeval timeout;

	FD_ZERO(&master);
	FD_ZERO(&ready);
	timeout.tv_sec = 1;

	// Adding comSocket to master
	FD_SET(listeningSocket, &master);

	// Listening loop
	std::cout << "Listening on port (";
	colorWord(listeningPort, YELLOW);
	std::cout << ") for server broadcast..." << std::endl;
	int timer = 15;
	while (true)
	{
		std::cout << ">> " << timer;
		Sleep(1000);
		printf("\33[2K\r");

		// Getting ip and port
		ready = master;
		int rc = select(0, &ready, NULL, NULL, &timeout);

		if (FD_ISSET(listeningSocket, &ready))
		{
	
			int recvSize = 0;
			int listenAddrSize = sizeof(listenAddr);

			int resutl = recvfrom(listeningSocket, (char*)&recvSize, 1, 0, (struct sockaddr*)&listenAddr, &listenAddrSize);
			if (result == SOCKET_ERROR)
			{
				errorColor("receiving IP and PORT from server.");
			}
			
			int size = (int)recvSize + 1;
			char* recvBuff = new char[size];

			resutl = recvfrom(listeningSocket, recvBuff, size, 0, (struct sockaddr*)&listenAddr, &listenAddrSize);
			if (result == SOCKET_ERROR)
			{
				errorColor("receiving IP and PORT from server.");
			}
			
			// Parsing incoming ip and port in the format (ip:port)
			std::string s = "";
			std::string buff = std::string(recvBuff, 0, size);
			buff.erase(buff.begin());
			for (auto iter : buff)
			{
				if (iter == ':')
				{
					if (ipAddr.empty())
					{
						ipAddr.append(s);
					}
					else
					{
						chatPort.append(s);
					}
					s = "";
				}
				else
				{
					s = s + iter;
				}
			}

			colorWord("IP: ", GREEN);
			colorWord(ipAddr, YELLOW);
			colorWord(" and PORT: ", GREEN);
			colorWord(chatPort, YELLOW);
			colorWord(" received.\n", GREEN);

			// Removing listening socket from sets
			FD_CLR(listeningSocket, &master);
			FD_CLR(listeningSocket, &ready);

			// Closing listening sockets
			shutdown(listeningSocket, SD_BOTH);
			closesocket(listeningSocket);

			delete[] recvBuff;
			break;
		}

		if (timer == 0)
		{
			// Removing listening socket from master set
			FD_CLR(listeningSocket, &master);

			// Closing listening sockets
			shutdown(listeningSocket, SD_BOTH);
			closesocket(listeningSocket);

			colorWord("Sorry no server broadcast detected. Closing program...\n", RED);
			return;
		}

		timer--;
	}

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	if (inet_addr(ipAddr.c_str()) != INADDR_NONE)
		serverAddr.sin_addr.s_addr = inet_addr(ipAddr.c_str());
	serverAddr.sin_port = htons(atoi(chatPort.c_str()));

	// Creating comsocket
	std::cout << "\nCreating TCP socket..." << std::endl;
	comSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (comSocket == INVALID_SOCKET)
	{
		clearScreen();
		errorColor("creating the com socket.");
		return;
	}
	else
	{
		colorWord("TCP socket created.\n", GREEN);
	}

	// Adding comSocket to master
	FD_SET(comSocket, &master);

	// Connecting
	std::cout << "Connecting to the server..." << std::endl;
	result = connect(comSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (result == SOCKET_ERROR)
	{
		clearScreen();
		errorColor("There was an ERROR conecting to to the server. Try again.\n");
	}
	else
	{
		colorWord("Connection to the server stablished.\n", GREEN);
	}


	userName = gettingUserName();
	userHeader(userName);
	//clearScreen();

	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	std::string registering = "$register <" + userName + ">";
	std::cout << ">> $register <" << userName << ">" << std::endl;
	result = sendMessage(comSocket, registering);
	if (result == SOCKET_ERROR) {
		std::cout << "There was a socket ERROR while registering user." << std::endl;
	}
	else
	{
		std::string reply = receiveMessage(comSocket);
		if (reply == "SV_FULL" || reply == "SHUTDOWN" || reply == "SOCKET_ERROR")
		{
			return;
		}
		else if (reply == "SV_SUCCESS")
		{
			std::cout << ">> ";
			colorWord("Registering successful. You can chat now.\n", GREEN);

			// Client loop
			while (true)
			{
				gettingLog = false;
				std::string userInput;

				std::cout << ">> ";
				std::getline(std::cin, userInput);

				int sendResult = sendMessage(comSocket, userInput);
				if (sendResult != SOCKET_ERROR)
				{
					Sleep(10);
					// checking for commands
					if (userInput == "$getlist")
					{
						std::cout << "Getting list...\n";
						Sleep(10);
					}
					else if (userInput == "$exit")
					{
						std::cout << "Terminating chat...\n";
						break;
					}
					else if (userInput == "$getlog")
					{
						gettingLog = true;
						std::cout << "Getting chat transcript...\n";
					}
					// Getting messages sent
					ready = master;
					int rc = select(0, &ready, NULL, NULL, &timeout);


					if (ready.fd_count > 0)
					{
						// Receiving log file
						if (gettingLog)
						{
							// Creating log file
							logFile = fopen("ChatTranscript.txt", "w");

							std::string message;
						
							while (true)
							{
								message = receiveMessage(comSocket);

								if (message == "SHUTDOWN" || message == "SOCKET_ERROR")
								{
									errorColor("getting chat transcript.");
									break;
								}
								else if (message == "***")
								{
									fclose(logFile);
									gettingLog = false;
									colorWord("Chat transcript received.\n", GREEN);
									break;
								}
								else
								{
									const char* text = message.c_str();

									fprintf(logFile, text);
								}
								
								message = "";
							}
						}
						else
						{
							std::string message = receiveMessage(comSocket);

							if (message == "SHUTDOWN" || message == "SOCKET_ERROR")
							{
								break;
							}
							else
							{
								colorWord("SERVER", YELLOW);
								std::cout << ">>> " + message;
							}
						}
					}
				}
				else
				{
					errorColor("while sending a message.\n");
				}
			}
		}
	}

	// close sockets
	shutdown(comSocket, SD_BOTH);
	closesocket(comSocket);

}


void userHeader(std::string name) 
{
	system("cls");
	int size = name.size();
	std::string border;
	for (size_t i = 0; i < size; i++)
	{
		border.append("-");
	}
	colorWord("+--", YELLOW);
	colorWord(border, YELLOW);
	colorWord("--+\n", YELLOW);
	colorWord("|  ", YELLOW);
	for (auto& c : name) c = toupper(c);
	colorWord(name, GREEN);
	colorWord("  |\n", YELLOW);
	colorWord("+--", YELLOW);
	colorWord(border, YELLOW);
	colorWord("--+\n", YELLOW);
}
void clearScreen()
{
	system("cls");
	colorWord("+---------------+\n", YELLOW);
	colorWord("|  ", YELLOW);
	colorWord("CLIENT CHAT", WHITE);
	colorWord("  |\n", YELLOW);
	colorWord("+---------------+\n", YELLOW);
}

std::string gettingUserName()
{
	bool done = false;
	std::string userName;

	std::cout << "\nBefore we start to chat let's creater a user name." << std::endl;
	std::cout << "It can contain letters, numbers and symbols," << std::endl;
	std::cout << "But it can only contain 10 characters." << std::endl;

	// ASking for username	
	while (!done)
	{
		char choice;
		while (true)
		{
			std::cout << "\nPlease enter a user name: ";
			std::cin >> userName;

			if (userName.size() > 10) {
				std::cout << "Sorry but that is too long." << std::endl;
			}
			else
			{
				break;
			}
		}

		while (true)
		{
			std::cout << "You have entered "; 
			colorWord(userName, GREEN);
			std::cout << " as your user name." << std::endl;
			std::cout << "Is this correct? (y/n):";
			std::cin >> choice;
			if (choice == 'y')
			{
				done = true;
				break;
			}
			if (choice == 'n')
			{
				break;
			}
			else
			{
				colorWord("\n<<<Invalid Entry>>>\n", RED);
			}

			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		}
	}

	return userName;
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

std::string receiveMessage(SOCKET comSocket)
{
	int size = 0;
	std::string message;

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
		message = std::string(buffer, 0, recvdResult);
	}

	delete[] buffer;

	return message;
}


int sendMessage(SOCKET comSocket, std::string userInput)
{
	std::string data = userInput.insert(0, 1, userInput.size());

	int sendResult = send(comSocket, data.c_str(), data.size(), 0);

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



