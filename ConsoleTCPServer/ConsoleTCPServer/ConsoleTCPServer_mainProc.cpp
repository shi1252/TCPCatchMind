#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32")
#include <WinSock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>


#define SERVERPORT 9000
#define BUFSIZE 512

enum CLIENT_STATE { SelectRoom, InRoom };

CRITICAL_SECTION ScreenPrintfCS;
void ScreenPrintf(char* str);


DWORD WINAPI ProcessClient(LPVOID arg);
DWORD WINAPI RoomHandler(LPVOID arg);

class CHandleClient
{
private:
	struct Node
	{
		int sId;
		SOCKET *socket;
		Node *prev;
		Node *next;
	};
	Node *head;
	CRITICAL_SECTION cs;
	SOCKET *sockArr[65536];
	int nextId;
	int number;
	Node* FindNode(int _num)
	{
		// first data number = 0
		Node *curNode = head;
		if (_num > number) { printf("CHandleClient::FindNode(int) _num�� ���� number�� ���� �ʰ��߽��ϴ�.\n"); return nullptr; }
		for (int i = 0; i < _num; i++)
		{
			curNode = curNode->next;
		}
		return curNode;
	}
	Node* FindNodeId(int _sId)
	{
		Node *curNode = head;
		while (curNode->next != nullptr)
		{
			if (curNode->sId == _sId)
				return curNode;
			curNode = curNode->next;
		}
		return nullptr;
	}
public:
	CHandleClient();
	int getSocketNumber() { return number; }
	int addSocket(SOCKET sock);
	void deleteSocket(int _sId);
	int getSocketId(int _num);
	SOCKET& getSocket(int _num);
	SOCKET& getSocket_sId(int _sId);
};



class CRoom
{
	int id;
	int MaxSize;
	int CurNumber;
	int *IdArr;
	HANDLE RoomThread;

	bool work;

	CRITICAL_SECTION PainterCS;
	char buf[BUFSIZE];
	int curQuizNumber;
	bool GameStart;
	int PainterId;
	int NextPainterId;
	bool isCorrect;

public:
	CRoom(int _MaxSize, int _id):MaxSize(_MaxSize), CurNumber(0), IdArr(new int [MaxSize]), id(_id), work(true), curQuizNumber(0)
	{
		InitializeCriticalSection(&PainterCS);
		GameStart = false;
		PainterId = -1;
		isCorrect = false;
		printf("[%d ��] : ����\n", id);

		RoomThread = CreateThread(NULL, 0, RoomHandler, (LPVOID)this, 0, NULL);
		if (RoomThread != NULL) { CloseHandle(RoomThread); }
	}
	~CRoom()
	{
		printf("[%d ��] : ����\n", id);
		delete[] IdArr;
		//work = false;
		printf("[%d ��] : ����Ϸ�\n", id);
	}
	int getId() { return id; }
	int getClientNumber() { return CurNumber; }
	int getClientId(int _num) { return IdArr[_num]; }
	bool isFull() { return CurNumber < MaxSize ? false : true; }
	bool addClient(int _sId)
	{
		if (isFull()) return false;
		printf("[%d] �߰�", CurNumber);
		IdArr[CurNumber++] = _sId;
		printf("[%d ��] : �ο�[%d] ���� Ŭ���̾�Ʈ%d\n", id, CurNumber, _sId); return true;
	}
	void subClient(int _sId)
	{
		EnterCriticalSection(&PainterCS);
		CurNumber--;
		printf("[%d ��] : �ο�[%d] ���� Ŭ���̾�Ʈ%d\n", id, CurNumber, _sId);
		bool delStart = false;
		for (int i = 0; i < CurNumber; i++)
		{
			printf("%d\n", i);
			if (IdArr[i] == _sId) delStart = true;
			if (delStart) IdArr[i] = IdArr[i + 1];
		}
		LeaveCriticalSection(&PainterCS);
	}

	void ClassMain();

	int getCurQuizNum() { return curQuizNumber; }
	void solveQuiz(int _id)
	{
		EnterCriticalSection(&PainterCS);
		if (isCorrect == false)
		{
			isCorrect = true;
			NextPainterId = _id;
		}
		LeaveCriticalSection(&PainterCS);
	}
};



class CHandleRoom
{
private:
	int MaxRoom;
	int MaxClient;

	CRoom **roomArr;
public:
	CHandleRoom(int _MaxRoom, int _MaxClient) :MaxRoom(_MaxRoom), MaxClient(_MaxClient), roomArr(new CRoom*[MaxRoom])
	{
		for (int i = 0; i < MaxRoom; i++) roomArr[i] = nullptr;
	}
	~CHandleRoom() { delete[] roomArr; }

	int getRoomCount() { return MaxRoom; }
	CRoom & getRoom(int _num) { return *roomArr[_num]; }

	bool RoomExists(int _num) { return roomArr[_num] == nullptr ? false : true; }
	int RoomAdd()
	{
		for (int i = 0; i < MaxRoom; i++)
		{
			printf("%d", roomArr[i]);
			if (roomArr[i] == nullptr) { roomArr[i] = new CRoom(MaxClient, i); return i; }
		}
		return -1;
	}
	void RoomDelete(int _num)
	{
		printf("[%d]�� ����\n", _num);
		delete roomArr[_num];
		roomArr[_num] = nullptr;
		printf("[%d]�� ���ſϷ�\n", _num);
	}
};




int getQuizNum();




// ���� ���� ��� �Լ�
void err_quit(char*msg);
void err_display(char*msg);

CHandleClient hClient;
CHandleRoom* hpRoom;


char * strQuiz[] = {
	"����", "������", "���ξ���", "�챸", "�ƺ�ī��", "�Ž�", "���", "���ƹ�",
	"�ֺ���Ÿ", "����ũ", "���Ҹ�", "������", "���", "�ٴٻ���", "�ٹ��̴ٶ���", "�ж��", "�ָӴ���", "���ڿ�����", "���ȿ�����",
	"��Ʃ��", "�̼�", "Ʈ��", "���", "���", "���", "���", "���α׷���", "�迵��������", "���ٶ�������",
	"����Ƽ", "�𸮾�", "���Ӹ���Ŀ", "���ڽ�2d", "RPG����Ŀ", "�ո�����",
	"��ũ�ҿ�", "����", "����������", "��Ż", "���������꽺��", "��Ÿũ����Ʈ", "��ƺ��", "ȭ��Ʈ����", "����Ŀ", "��Ÿ", "ī���ͽ�Ʈ����ũ", "ö��", "��Ʋ�׶���",
	"3D�ƽ�", "�ӵ�ڽ�", "���귯��", "���伥", "�Ϸ���Ʈ������", "����������Ʈ", "������", "FLSTUDIO",
	"���ӵ����̳�", "����", "Ŭ���̾�Ʈ", "��ȭ��", "��Ʈ", "3D", "2D",
	"�̸�", "��Ӵ�", "�ƹ���", "����", "�ܼ���", "���Ҿƹ���", "���ҸӴ�",
	"������", "�߰�Ʈ����", "�˺��", "MT", "�����", "���", "����", "�ܹ���", "����Ŀ", "���", "��Ƭ", "�ﰢ��", 
	"��κ�", "�ϰ����", "�������", "�ٰ�Ʈ", "����",
	"�ڰ�����", "���źҸ�", "����ȯ��", "��������", "�������", "�����ұ�", "�ְ�ߵ�", "����ȯ",
	"�����", "�޽�", "�н�", "����", "����", "�ڱ�����",
	"����", "���", "�ھ�", "����", "���", "���", "�̱�����",
	"����", "��ȭ", "����Ŀ", "������", "���ڰ�", "�縮", "��ī", "�����强", "�������", "�����ô�", "��纴", "�������", "��������", "������ī",
	"��", "��ռ�", "����", "��Ű����", "Ÿ��ź", "�����׸�", "������Ʈ", "�޼���Ÿ�̾�", "��ī�����̷ν�", "ũ�γ뽺",
	"��", "��ŷ�", "����", "��", "����", "����", "����", "������", "����", "��", "��", "���۱�",
	"��Ʈ����", "����ȭ��", "��Ȥ", "����", "�ֽ�", "�߱�", "�౸", "�迵",
	"û�����б�", "���Ӱ�", "���α׷���", "ȭ����", "��"
};



int main(int argc, char*argv[])
{
	hpRoom = new CHandleRoom(10, 6);
	InitializeCriticalSection(&ScreenPrintfCS);

	int retval;
	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;
	
	// socket()
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");
	
	// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	// ������ ��ſ� ����� ����
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;
	int socketId;
	HANDLE hThread;

	while (1) {
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR *)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}

		printf("\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ = %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));


		socketId = hClient.addSocket(client_sock);
		// ������ ����
		hThread = CreateThread(NULL, 0, ProcessClient, (LPVOID)socketId, 0, NULL);
		if (hThread == NULL) { closesocket(client_sock); }
		else { CloseHandle(hThread); }

	}

	// closesocket()
	closesocket(listen_sock);

	// ���� ����
	WSACleanup();


	// �Ҵ�����
	delete hpRoom;

	return 0;
}








void ScreenPrintf(char* str)
{
	EnterCriticalSection(&ScreenPrintfCS);
	printf(str);
	LeaveCriticalSection(&ScreenPrintfCS);
}




CHandleClient::CHandleClient():nextId(0), number(0)
{
	head = new Node;
	head->next = nullptr;
	head->prev = nullptr;
	head->socket = nullptr;
	head->sId = NULL;
}

int CHandleClient::addSocket(SOCKET _sock)
{
	Node * addNode = new Node;
	addNode->next = head;
	addNode->prev = head->prev;
	addNode->socket = new SOCKET(_sock);
	addNode->sId = nextId++;
	head->prev = addNode;
	head = addNode;
	number++;
	return addNode->sId;
}

void CHandleClient::deleteSocket(int _sId)
{
	Node * DelNode = FindNodeId(_sId);
	if (DelNode->next != nullptr)
		DelNode->next->prev = DelNode->prev;
	if (DelNode->prev == nullptr)
		head = DelNode->next;
	else
		DelNode->prev->next = DelNode->next;
	delete DelNode->socket;
	delete DelNode;
	number--;
}

int CHandleClient::getSocketId(int _num)
{
	return FindNode(_num)->sId;
}

SOCKET& CHandleClient::getSocket(int _num)
{
	return *FindNode(_num)->socket;
}

SOCKET& CHandleClient::getSocket_sId(int _sId)
{
	return *FindNodeId(_sId)->socket;
}




DWORD WINAPI ProcessClient(LPVOID arg)
{
	int socketId = (int) arg;
	CRoom * roomId = nullptr;
	printf("Ŭ���̾�Ʈ[%d] ����\n", socketId);

	CLIENT_STATE clientState = SelectRoom;


	// ������ ��ſ� ����� ����
	int retval;
	SOCKET &client_sock = hClient.getSocket_sId(socketId);
	SOCKADDR_IN clientaddr;
	int addrlen;
	char buf[BUFSIZE + 1];

	// Ŭ���̾�Ʈ ���� ���
	addrlen = sizeof(clientaddr);
	getpeername(client_sock, (SOCKADDR *)&clientaddr, &addrlen);

	bool work = true;
	while (work) {
		// ������ �ޱ�
		retval = recv(client_sock, buf, BUFSIZE, 0);
		if (retval == SOCKET_ERROR) {
			err_display("recv()");
			break;
		}
		else if (retval == 0) break;


		int _rmid;
		switch (clientState)
		{
		case SelectRoom:
			if (buf[0] != 0) break;
			printf("%d %d %d %d\n", buf[0], buf[1], buf[2], buf[3]);
			switch (buf[1])
			{
			case 0:
				buf[2] = hpRoom->getRoomCount();
				for (int i = 0; i < buf[2]; i++)
				{
					if (hpRoom->RoomExists(i))
						buf[i + 3] = hpRoom->getRoom(i).getClientNumber();
					else
						buf[i + 3] = 0;
				}
				break;
			case 1:
				_rmid = hpRoom->RoomAdd();
				if (_rmid != -1) {
					clientState = InRoom;
					roomId = &(hpRoom->getRoom(_rmid));
					roomId->addClient(socketId);
					buf[2] = 1;
					buf[3] = _rmid;
				}
				else buf[2] = 0;
				break;
			case 2:
				_rmid = buf[2];
				if (hpRoom->RoomExists(_rmid))
				{
					roomId = &hpRoom->getRoom(_rmid);
					if (roomId->addClient(socketId))
					{
						buf[2] = 1;
						buf[3] = _rmid;
						clientState = InRoom;
					}
					else buf[2] = 0;
				}
				else
					buf[2] = 0;
				break;
			}
			printf("%d %d %d %d\n", buf[0], buf[1], buf[2], buf[3]);
			send(client_sock, buf, BUFSIZE, 0);
			break;
		case InRoom:
			// ���� ������ ���
			buf[retval] = '\0';
			if (buf[0] == '0')
			{
				printf("[TCP/%s:%d] %s\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), buf);
				int i;
				for (i = 2; buf[i] != ':' && i < 30; i++);
				printf("%d", roomId->getCurQuizNum());
				// ���� Ȯ��
				if (strcmp(buf + i + 2, strQuiz[roomId->getCurQuizNum()]) == 0)
				{
					roomId->solveQuiz(socketId);
				}
			}
			//������ ������
			printf("�ο� �� %d\n", roomId->getClientNumber());
			for (int i = 0; i < roomId->getClientNumber(); i++)
			{
				int _sId = roomId->getClientId(i);
				printf("%d, %d\n", _sId, i);
				SOCKET &_sSock = hClient.getSocket_sId(_sId);
				if (_sId == socketId) continue;
				retval = send(_sSock, buf, retval, 0); 
				if (retval == SOCKET_ERROR) {
					err_display("send()"); work = false;
				}
				printf("%d, %d\n", _sId, i);
			}
			break;
		}
		
	}

	closesocket(client_sock);
	printf("%d", socketId);
	if (clientState == InRoom) roomId->subClient(socketId);
	hClient.deleteSocket(socketId);
	return 0;
}






DWORD WINAPI RoomHandler(LPVOID arg)
{
	CRoom *CurRoom = (CRoom*)arg;

	CurRoom->ClassMain();

	return 0;
}
void CRoom::ClassMain()
{
	srand(time(NULL));
	int retval;

	while (work)
	{
		Sleep(500);
		if (GameStart == false)
		{
			if (CurNumber > 0)
			{
				Sleep(500);
				EnterCriticalSection(&PainterCS);
				printf("[%d ��] : ���� ����\n", id);
				GameStart = true;
				PainterId = IdArr[0];

				buf[0] = '4';
				buf[1] = '1';


				int _curQuizNumber = getQuizNum();
				printf("%d", curQuizNumber);
				strcpy_s(&buf[2], BUFSIZE, strQuiz[_curQuizNumber]);

				curQuizNumber = _curQuizNumber;


				retval = send(hClient.getSocket_sId(PainterId), buf, BUFSIZE, 0);
				if (retval == SOCKET_ERROR) {
					err_display("send()");
				}
				LeaveCriticalSection(&PainterCS);
				printf("[%d ��] : ������ = Ŭ���̾�Ʈ%d\n", id, PainterId);
			}
		}
		else
		{
			if (CurNumber == 0) { hpRoom->RoomDelete(id); return; }
			if (isCorrect)
			{
				EnterCriticalSection(&PainterCS);
				isCorrect = false;

				buf[0] = '4';
				buf[1] = '3';
				retval = send(hClient.getSocket_sId(PainterId), buf, BUFSIZE, 0);
				if (retval == SOCKET_ERROR) err_display("send()");
				
				PainterId = NextPainterId;
				buf[0] = '4';
				buf[1] = '1';


				int _curQuizNumber = getQuizNum();
				strcpy_s(&buf[2], BUFSIZE, strQuiz[_curQuizNumber]);
				curQuizNumber = _curQuizNumber;


				retval = send(hClient.getSocket_sId(PainterId), buf, BUFSIZE, 0);
				if (retval == SOCKET_ERROR) err_display("send()");
				printf("[%d ��] : ������ = Ŭ���̾�Ʈ%d\n", id, PainterId);

				buf[0] = '3';
				for (int i = 0; i < CurNumber; i++)
					send(hClient.getSocket_sId(IdArr[i]), buf, BUFSIZE, 0);

				LeaveCriticalSection(&PainterCS);
			}
		}
	}
}






int getQuizNum()
{
	
	return rand() % (sizeof(strQuiz) / sizeof(char*));
}

// ���� ���� ��� �Լ�
void err_quit(char*msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}
void err_display(char*msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s]%s", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}