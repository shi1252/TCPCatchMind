#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include "resource.h"
#include <CommCtrl.h>

#define SERVERIP   "127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE    512

struct DrawingData
{
	COLORREF Color;
	int Width;
	POINT MousePos;
};

// 대화상자 프로시저
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
// 편집 컨트롤 출력 함수
void DisplayText(char *fmt, ...);
// 오류 출력 함수
void err_quit(char *msg);
void err_display(char *msg);
// 사용자 정의 데이터 수신 함수
int recvn(SOCKET s, char *buf, int len, int flags);
// 소켓 통신 스레드 함수
DWORD WINAPI ClientMain(LPVOID arg);
DWORD WINAPI RecvClient(LPVOID arg);

SOCKET sock; // 소켓
char readbuf[BUFSIZE+1]; // 데이터 송수신 버퍼
char sendbuf[BUFSIZE + 1];
HWND hSendButton; // 보내기 버튼
HWND hEdit1, hEdit2; // 편집 컨트롤
char nick[20 + 1];
char chat[BUFSIZE - 20 -4 + 1];
char quiz[20 + 1];
char rightAnswer[7 + 20 + 1];
int retval;
bool isPainter = false;
bool isPainting;
HWND hRemove, hNick, hEdit3, hSlider, hRed, hGreen, hBlue, hBlack;
HWND hCreate, hStatic;
HWND hDlgWnd;
HINSTANCE gi;
HPEN pen, oldpen;
int mx, my, sx, sy;
HDC hdc;
DrawingData dd;
COLORREF Color = RGB(0, 0, 0);
int Width;
RECT wndRect;
CRITICAL_SECTION cs;
bool isInRoom = false;
char str[512];

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
	gi = hInstance;
	InitializeCriticalSection(&cs);

	// 소켓 통신 스레드 생성
	CreateThread(NULL, 0, ClientMain, NULL, 0, NULL);

	// 대화상자 생성
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

	// closesocket()
	closesocket(sock);

	// 윈속 종료
	WSACleanup();
	DeleteCriticalSection(&cs);
	return 0;
}

// 대화상자 프로시저
BOOL CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	switch(uMsg){
	case WM_INITDIALOG:
		hDlgWnd = hDlg;
		hEdit1 = GetDlgItem(hDlg, IDC_EDIT1);
		hEdit2 = GetDlgItem(hDlg, IDC_EDIT2);
		hEdit3 = GetDlgItem(hDlg, IDC_EDIT3);
		hSendButton = GetDlgItem(hDlg, IDOK);
		hRemove = GetDlgItem(hDlg, IDC_BUTTON1);
		hNick = GetDlgItem(hDlg, IDC_BUTTON2);
		hSlider = GetDlgItem(hDlg, IDC_SLIDER1);
		hRed = GetDlgItem(hDlg, IDC_RED);
		hGreen = GetDlgItem(hDlg, IDC_GREEN);
		hBlue = GetDlgItem(hDlg, IDC_BLUE);
		hBlack = GetDlgItem(hDlg, IDC_BLACK);
		hStatic = GetDlgItem(hDlg, IDC_STATIC2);
		hCreate = GetDlgItem(hDlg, IDC_CREATE);
		SendMessage(hEdit1, EM_SETLIMITTEXT, BUFSIZE - 20-4, 0);
		SendMessage(hEdit3, EM_SETLIMITTEXT, 20, 0);
		SendMessage(hSlider, TBM_SETRANGE, NULL, MAKELPARAM(1, 10));
		GetWindowRect(hDlg, &wndRect);
		return TRUE;
	case WM_HSCROLL:
		Width = SendMessage(hSlider, TBM_GETPOS, 0, 0);
		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wParam)){
		case IDOK:
			GetDlgItemText(hDlg, IDC_EDIT1, chat, BUFSIZE-20-4+1);
			SetFocus(hEdit1);
			SendMessage(hEdit1, EM_SETSEL, 0, -1);
			if (isInRoom)
			{
				EnterCriticalSection(&cs);
				sendbuf[0] = '0';
				LeaveCriticalSection(&cs);
			}
			return TRUE;
		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		case IDC_BUTTON1:// 지우기 버튼
			InvalidateRect(hDlg, NULL, TRUE);
			EnterCriticalSection(&cs);
			sendbuf[0] = '3';
			LeaveCriticalSection(&cs);
			return TRUE;
		case IDC_BUTTON2:// 닉네임 설정 버튼
			GetDlgItemText(hDlg, IDC_EDIT3, nick, 20 + 1);
			EnableWindow(hEdit3, FALSE);
			EnableWindow(hNick, FALSE);
			return TRUE;
		case IDC_RED:
			Color = RGB(255, 0, 0);
			return TRUE;
		case IDC_BLUE:
			Color = RGB(0, 0, 255);
			return TRUE;
		case IDC_GREEN:
			Color = RGB(0, 255, 0);
			return TRUE;
		case IDC_BLACK:
			Color = RGB(0, 0, 0);
			return TRUE;
		case IDC_CREATE:
			EnterCriticalSection(&cs);
			sendbuf[0] = 0;
			sendbuf[1] = 1;
			LeaveCriticalSection(&cs);
			return TRUE;
		}
		return FALSE;
	case WM_LBUTTONDOWN:
		if (isPainter)
		{
			mx = LOWORD(lParam);
			my = HIWORD(lParam);
			if (mx > wndRect.left+15 && mx < wndRect.right-35 && my >wndRect.top +50 && my < wndRect.bottom-240)
			{
				isPainting = TRUE;
				EnterCriticalSection(&cs);
				sx = mx;
				sy = my;
				sendbuf[0] = '1';
				LeaveCriticalSection(&cs);
			}
		}
		return TRUE;
	case WM_MOUSEMOVE:
		if (isPainting && isPainter)
		{
			mx = LOWORD(lParam);
			my = HIWORD(lParam);
			if (mx > wndRect.left + 15 && mx < wndRect.right - 35 && my >wndRect.top + 50 && my < wndRect.bottom - 240)
			{
				hdc = GetDC(hDlgWnd);
				MoveToEx(hdc, sx, sy, NULL);
				pen = CreatePen(PS_SOLID, Width, Color);
				oldpen = (HPEN)SelectObject(hdc, pen);
				LineTo(hdc, mx, my);
				DeleteObject(SelectObject(hdc, oldpen));
				EnterCriticalSection(&cs);
				sx = mx;
				sy = my;
				sendbuf[0] = '2';
				LeaveCriticalSection(&cs);
				ReleaseDC(hDlgWnd, hdc);
			}
		}
		return TRUE;
	case WM_LBUTTONUP:
		isPainting = FALSE;
		return TRUE;
	case WM_PAINT:
		BeginPaint(hDlg, &ps);
		TextOut(hdc, 10, 10, quiz, strlen(quiz));
		EndPaint(hDlg, &ps);
		return TRUE;
	}
	return FALSE;
}

// 편집 컨트롤 출력 함수
void DisplayText(char *fmt, ...)
{
	va_list arg;
	va_start(arg, fmt);

	char cbuf[BUFSIZE+256];
	vsprintf(cbuf, fmt, arg);

	EnterCriticalSection(&cs);
	int nLength = GetWindowTextLength(hEdit2);
	SendMessage(hEdit2, EM_SETSEL, nLength, nLength);
	SendMessage(hEdit2, EM_REPLACESEL, FALSE, (LPARAM)cbuf);
	LeaveCriticalSection(&cs);

	va_end(arg);
}

// 소켓 함수 오류 출력 후 종료
void err_quit(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

// 소켓 함수 오류 출력
void err_display(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	DisplayText("[%s] %s", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

// 사용자 정의 데이터 수신 함수
int recvn(SOCKET s, char *buf, int len, int flags)
{
	int received;
	char *ptr = buf;
	int left = len;

	while(left > 0){
		received = recv(s, ptr, left, flags);
		if(received == SOCKET_ERROR)
			return SOCKET_ERROR;
		else if(received == 0)
			break;
		left -= received;
		ptr += received;
	}

	return (len - left);
}

// TCP 클라이언트 시작 부분
DWORD WINAPI ClientMain(LPVOID arg)
{
	// 윈속 초기화
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0)
		return 1;

	// socket()
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == INVALID_SOCKET) err_quit("socket()");

	// connect()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(SERVERIP);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = connect(sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if(retval == SOCKET_ERROR) err_quit("connect()");

	CreateThread(NULL, 0, RecvClient, NULL, 0, NULL);

	ZeroMemory(chat, sizeof(chat));

	// 서버와 데이터 통신
	while (1) {
		// 문자열 길이가 0이면 보내지 않음
		/*
		if (strlen(sendbuf) == 0) {
			continue;
		}
		*/

		if (isInRoom == false)
		{
			EnterCriticalSection(&cs);
			if (strlen(chat)!=0)
			{
				sendbuf[0] = 0;
				sendbuf[1] = 2;
				sendbuf[2] = atoi(chat);
			}
			else
			{
				sendbuf[0] = 0;
				if (sendbuf[1] != 1)
					sendbuf[1] = 0;
			}
			retval = send(sock, sendbuf, BUFSIZE, 0);
			if (retval == SOCKET_ERROR) {
				err_display("send()");
				break;
			}
			ZeroMemory(chat, sizeof(chat));
			LeaveCriticalSection(&cs);
			Sleep(1000);
		}
		else
		{
			EnterCriticalSection(&cs);

			if (sendbuf[0] == '0')
			{
				strcat(sendbuf, nick);
				strcat(sendbuf, " : ");
				strcat(sendbuf, chat);
				retval = send(sock, sendbuf, strlen(sendbuf), 0);
				if (retval == SOCKET_ERROR) {
					err_display("send()");
					break;
				}
				DisplayText("%s\r\n", &sendbuf[1]);
			}
			else if (sendbuf[0] == '1' || sendbuf[0] == '2')
			{
				//EnterCriticalSection(&cs);
				dd.Color = Color;
				dd.Width = Width;
				dd.MousePos.x = mx;
				dd.MousePos.y = my;
				memcpy(&sendbuf[1], &dd, sizeof(DrawingData));
				retval = send(sock, sendbuf, 1 + sizeof(DrawingData), 0);
				if (retval == SOCKET_ERROR) {
					err_display("send()");
					break;
				}
				//LeaveCriticalSection(&cs);
			}
			else
			{
				retval = send(sock, sendbuf, strlen(sendbuf), 0);
				if (retval == SOCKET_ERROR) {
					err_display("send()");
					break;
				}
			}

			ZeroMemory(sendbuf, BUFSIZE + 1);

			LeaveCriticalSection(&cs);
		}
	}

	return 0;
}

DWORD WINAPI RecvClient(LPVOID arg)
{
	while (1)
	{
		// 데이터 받기
		retval = recv(sock, readbuf, BUFSIZE, 0);
		if (retval == SOCKET_ERROR) {
			err_display("recv()");
			break;
		}
		//else if (retval == 0)
		//	continue;

		if (isInRoom == false)
		{
			if (readbuf[0] == 0)
			{
				int i, nLength;
				if (readbuf[1] == 0)
				{
					strcpy(str, "- 방 목록 -\n");
					for (int q = 0; q < readbuf[2]; q++)
					{
						if (readbuf[q+3] != 0)
							sprintf(str, "%s%d번 방 [%d/6]\n", str, q, readbuf[q + 3]);
					}
					EnterCriticalSection(&cs);
					SetDlgItemText(hDlgWnd, IDC_STATIC2, str);
					LeaveCriticalSection(&cs);
				}
				else
				{
					if (readbuf[2] == 1)
					{
						//sprintf(str, "%d번 방에 접속하셨습니다.", readbuf[3]);
						DisplayText("%d번 방에 접속하셨습니다.\n", readbuf[3]);
						isInRoom = true;
						ShowWindow(hStatic, 0);
						ShowWindow(hCreate, 0);
					}
					else
					{
						if (readbuf[1] == 1)
							DisplayText("방 생성에 실패하였습니다.\n");
						else
							DisplayText("방 접속에 실패하였습니다.\n");
					}
				}
			}
		}
		else
		{
			if (readbuf[0] == 0)
			{
				for (int i = 0; i < 4; i++)
					DisplayText("%d", readbuf[i]);
			}
			else if (readbuf[0] == '0')
			{
				readbuf[retval] = '\0';
				DisplayText("%s\r\n", &readbuf[1]);
			}
			else if (readbuf[0] == '1')
			{
				EnterCriticalSection(&cs);
				memcpy(&dd, &readbuf[1], sizeof(DrawingData));
				sx = dd.MousePos.x;
				sy = dd.MousePos.y;
				LeaveCriticalSection(&cs);
			}
			else if (readbuf[0] == '2')
			{
				hdc = GetDC(hDlgWnd);
				MoveToEx(hdc, sx, sy, NULL);
				EnterCriticalSection(&cs);
				memcpy(&dd, &readbuf[1], sizeof(DrawingData));
				pen = CreatePen(PS_SOLID, dd.Width, dd.Color);
				oldpen = (HPEN)SelectObject(hdc, pen);
				LineTo(hdc, dd.MousePos.x, dd.MousePos.y);
				DeleteObject(SelectObject(hdc, oldpen));
				sx = dd.MousePos.x;
				sy = dd.MousePos.y;
				LeaveCriticalSection(&cs);
				ReleaseDC(hDlgWnd, hdc);
			}
			else if (readbuf[0] == '3')
			{
				InvalidateRect(hDlgWnd, NULL, TRUE);
			}
			else if (readbuf[0] == '4')
			{
				if (readbuf[1] == '1')
				{
					isPainter = TRUE;
					strcpy(quiz, &readbuf[2]);
					sprintf(rightAnswer, "정답 : %s", quiz);
					SetDlgItemText(hDlgWnd, IDC_EDIT1, rightAnswer);
					EnableWindow(hEdit1, FALSE);
				}
				else
				{
					isPainter = FALSE;
					isPainting = FALSE;
					ZeroMemory(quiz, 21);
					SetDlgItemText(hDlgWnd, IDC_EDIT1, quiz);
					EnableWindow(hEdit1, TRUE);
				}
			}

			if (isPainter)
			{
				EnableWindow(hRemove, TRUE);
				EnableWindow(hSlider, TRUE);
				EnableWindow(hRed, TRUE);
				EnableWindow(hBlue, TRUE);
				EnableWindow(hGreen, TRUE);
				EnableWindow(hBlack, TRUE);
			}
			else
			{
				EnableWindow(hRemove, FALSE);
				EnableWindow(hSlider, FALSE);
				EnableWindow(hRed, FALSE);
				EnableWindow(hBlue, FALSE);
				EnableWindow(hGreen, FALSE);
				EnableWindow(hBlack, FALSE);
			}

			ZeroMemory(readbuf, BUFSIZE + 1);
		}
	}

	return 0;
}