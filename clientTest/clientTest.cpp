// clientTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#undef UNICODE
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <thread>

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512            
#define IP_ADDRESS "192.168.56.1"
#define DEFAULT_PORT "3504"

struct client_type
{
	SOCKET socket;
	int id;
	char received_message[DEFAULT_BUFLEN];
};

int process_client(client_type &new_client);
int main();

int process_client(client_type &new_client)
{
	
	while (true)
	{
		memset(new_client.received_message, 0, DEFAULT_BUFLEN);

		if (new_client.socket != 0)
		{
			const auto i_result = recv(new_client.socket, new_client.received_message, DEFAULT_BUFLEN, 0);

			// Print messages from server comming from other clients
			if (i_result != SOCKET_ERROR) {
				std::cout << "\r";
				std::cout << new_client.received_message << std::endl;
				std::cout << "me: ";
			} else
			{
				std::cout << "recv() failed: " << WSAGetLastError() << std::endl;
				break;
			}
		}
	}

	if (WSAGetLastError() == WSAECONNRESET)
		std::cout << "The server has disconnected" << std::endl;

	return 0;
}

int main()
{
	WSAData wsa_data{};
	struct addrinfo *result = nullptr, *ptr = nullptr, hints{};
	std::string sent_message;
	std::string pseudo;
	client_type client = { INVALID_SOCKET, -1, "" };
	auto i_result = 0;

	std::cout << "Starting Client...\n";

	// Initialize Winsock
	i_result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (i_result != 0) {
		std::cout << "WSAStartup() failed with error: " << i_result << std::endl;
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	std::cout << "Connecting...\n";

	// Resolve the server address and port
	i_result = getaddrinfo(static_cast<LPCTSTR>(IP_ADDRESS), DEFAULT_PORT, &hints, &result);
	if (i_result != 0) {
		std::cout << "getaddrinfo() failed with error: " << i_result << std::endl;
		WSACleanup();
		std::system("pause");
		return 1;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != nullptr; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		client.socket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (client.socket == INVALID_SOCKET) {
			std::cout << "socket() failed with error: " << WSAGetLastError() << std::endl;
			WSACleanup();
			std::system("pause");
			return 1;
		}

		// Connect to server.
		i_result = connect(client.socket, ptr->ai_addr, static_cast<int>(ptr->ai_addrlen));
		if (i_result == SOCKET_ERROR) {
			closesocket(client.socket);
			client.socket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (client.socket == INVALID_SOCKET) {
		std::cout << "Unable to connect to server!" << std::endl;
		WSACleanup();
		std::system("pause");
		return 1;
	}

	std::cout << "Successfully Connected" << std::endl;

	//Obtain id from server for this client;
	recv(client.socket, client.received_message, DEFAULT_BUFLEN, 0);
	const std::string message = client.received_message;

	if (message != "Server is full")
	{
		char * end;
		client.id = strtol(client.received_message, &end, 10);
		if (*end) std::cout << "Convertion went wrong" << std::endl;

		std::cout << "Please chose your pseudonyme" << std::endl;
		getline(std::cin, pseudo);
		while(strcmp("", pseudo.c_str()) ==0)
		{
			std::cout << "You can't leave it blank, please enter your pseudonyme: " << std::endl;
			getline(std::cin, pseudo);
		}
		if(strcmp(pseudo.c_str(), "") == 0) std::cout << "Welcome to this chatroom Client #" << client.id;
		else {
			std::cout << "Welcome to this chatroom " << pseudo;
			send(client.socket, (pseudo).c_str(), strlen(pseudo.c_str()), 0);
		}
		std::cout << ". You can now chat with the other persons in this room" << std::endl;
		std::cout << "____________________________________________________________________________________\n" << std::endl;

		std::thread my_thread(process_client, client);

		while (true)
		{
			std::cout << "me: ";
			getline(std::cin, sent_message);
			i_result = send(client.socket, (sent_message).c_str(), strlen(sent_message.c_str()), 0);

			if (i_result < 0)
			{
				std::cout << "send() failed: " << WSAGetLastError() << std::endl;
				break;
			}
		}

		//Shutdown the connection since no more data will be sent
		my_thread.detach();
	}
	else {
		std::cout << client.received_message << std::endl;
	}

	std::cout << "Shutting down socket..." << std::endl;
	i_result = shutdown(client.socket, SD_SEND);
	if (i_result == SOCKET_ERROR) {
		std::cout << "shutdown() failed with error: " << WSAGetLastError() << std::endl;
		closesocket(client.socket);
		WSACleanup();
		std::system("pause");
		return 1;
	}

	closesocket(client.socket);
	WSACleanup();
	std::system("pause");
	return 0;
}

