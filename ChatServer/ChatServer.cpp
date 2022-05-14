// ChatServer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <stdio.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32")
#pragma warning(disable:4996)

char* ids[64];
SOCKET clients[64];
int numClients;

CRITICAL_SECTION cs;

DWORD WINAPI ClientThread(LPVOID);



int main()
{
	InitializeCriticalSection(&cs);
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);

	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(9000);

	SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	bind(listener, (SOCKADDR*)&addr, sizeof(addr));
	listen(listener, 5);

	// Chap nhan ket noi va truyen nhan du lieu
	numClients = 0;

	while (1)
	{
		printf("Waiting for new client...\n");
		SOCKET client = accept(listener, NULL, NULL);
		printf("New client accepted: %d\n", client);

		CreateThread(0, 0, ClientThread, &client, 0, 0);
		const char* okMsg = "Xin chao hay nhap id theo cu phap Client_id: [your_id]";
		send(client, okMsg, strlen(okMsg), 0);
	}

	DeleteCriticalSection(&cs);
}

DWORD WINAPI ClientThread(LPVOID lpParam)
{
	SOCKET client = *(SOCKET*)lpParam;

	char buf[256];
	int ret;

	char cmd[16], id[32], tmp[32];
	const char* errorMsg = "Sai cu phap. Hay gui lai.\n";

	// xac thuc id client
	while (1)
	{
		ret = recv(client, buf, sizeof(buf), 0);
		if (ret <= 0)
		{
			closesocket(client);
			return 0;
		}

		buf[ret] = 0;
		printf("Received: %s\n", buf);

		// xu ly du lieu
		ret = sscanf(buf, "%s %s %s", cmd, id, tmp);
		if (ret == 2)
		{
			if (strcmp(cmd, "Client_id:") == 0)
			{
				const char* okMsg = "Dung cu phap. Hay gui thong diep.\n";
				send(client, okMsg, strlen(okMsg), 0);

				EnterCriticalSection(&cs);
				ids[numClients] = id;
				clients[numClients] = client;
				numClients++;
				LeaveCriticalSection(&cs);

				break;
			}
			else
			{
				send(client, errorMsg, strlen(errorMsg), 0);
			}
		}
		else
		{
			send(client, errorMsg, strlen(errorMsg), 0);
		}
	}

	// Forwarding messages
	while (1)
	{
		ret = recv(client, buf, sizeof(buf), 0);
		if (ret <= 0)
		{
			closesocket(client);
			return 0;
		}

		buf[ret] = 0;
		printf("receved: %s\n", buf);

		ret = sscanf(buf, "%s", cmd);
		if (ret < 1) continue;



		if (strcmp(cmd, "@all") == 0)
		{
			char sendBuf[256];
			strcpy(sendBuf, id);
			strcpy(sendBuf, ": ");
			strcat(sendBuf, buf + strlen(cmd) + 1);
			for (int i = 0; i < numClients; i++)
				if (clients[i] != client)
					send(clients[i], sendBuf, strlen(sendBuf), 0);
		}
		else
		{
			if (cmd[0] == '@')
			{
				char sendBuf[256];
				strcpy(sendBuf, id);
				strcpy(sendBuf, ": ");
				strcat(sendBuf, buf + strlen(cmd) + 1);
				for (int i = 0; i < numClients; i++)
					if (strcmp(cmd + 1, ids[i]) == 0)
						send(clients[i], sendBuf, strlen(sendBuf), 0);

			}
			else
			{
				send(client, errorMsg, strlen(errorMsg), 0);
			}
		}



	}


}


