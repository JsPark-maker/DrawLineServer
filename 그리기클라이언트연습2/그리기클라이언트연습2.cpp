// 그리기클라이언트연습2.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include "framework.h"
#include "그리기클라이언트연습2.h"
#include "CRingBuffer.h"

#define MAX_LOADSTRING 100
#define SERVERPORT 25000
#define WM_MESSAGE (WM_USER + 1)

// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

struct stCLIENT;

bool CloseSocket(SOCKET sock);
void AddSocket(SOCKET sock);
int  SendBroadCast(char* packet, int size);
void SendUniCastForOneClient(stCLIENT* clientPtr);
void RecvEvent(SOCKET sock);
void WriteEvent(stCLIENT* clientPtr);
void Send(stCLIENT* clientPtr, int size);

struct stCLIENT
{
    SOCKET sock;
    CRingBuffer recvRingBuffer;
    CRingBuffer sendRingBuffer;
    stCLIENT* next = nullptr;
    stCLIENT* prev = nullptr;
};

struct stHEADER
{
    unsigned short len;
};

struct stDRAWPACKET
{
    int iStartX;
    int iStartY;
    int iEndX;
    int iEndY;
};


SOCKET g_ListenSock;
stCLIENT* g_ClientList;
HWND hWnd;
bool g_SendFlag = true;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 여기에 코드를 입력합니다.

    // 전역 문자열을 초기화합니다.
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MY2, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 애플리케이션 초기화를 수행합니다:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MY2));

    MSG msg;

    AllocConsole();///////////////////////////
    freopen("CONOUT$", "wt", stdout);///////// 콘솔띄워주는부분
    printf("hello DEBUG\n");//////////////////

    g_ClientList = new stCLIENT; //리스트를 위한 변수할당

    int retVal;

    WSADATA wsa;

    WSAStartup(MAKEWORD(2, 2), &wsa);

    g_ListenSock = socket(AF_INET, SOCK_STREAM, NULL);

    SOCKADDR_IN serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVERPORT);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(g_ListenSock, (SOCKADDR*)&serverAddr, sizeof(serverAddr));

    listen(g_ListenSock, SOMAXCONN);

    WSAAsyncSelect(g_ListenSock, hWnd, WM_MESSAGE, FD_ACCEPT | FD_READ | FD_WRITE);

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MY2));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_MY2);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

   hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_MESSAGE:
        if (WSAGETSELECTERROR(lParam) == SOCKET_ERROR)
        {
            printf("select error \n");
            CloseSocket(wParam); //이건 아직 보류
        }
        else
        {
            switch (WSAGETSELECTEVENT(lParam))
            {
            case FD_ACCEPT:
            {
                //이 메시지가 왔을 경우 클라이언트의 접속 시도가 있었다는 의미가 된다.
                printf("client Income\n");
                SOCKADDR_IN clientAddr;
                int len = sizeof(clientAddr);
                SOCKET sock = accept(g_ListenSock, (SOCKADDR*)&clientAddr, &len);
                if (sock == INVALID_SOCKET)
                {
                    printf("accept error!\n");
                    break;
                }
                AddSocket(sock); //이 함수는 새로들어온 클라이언트를 추가하는 용도로 쓰입니다.
            }
                break;
            case FD_READ:
                printf("FD_READ\n");
                RecvEvent((SOCKET)wParam);
                break;
            case FD_WRITE:
                printf("FD_WRITE\n");
                g_SendFlag = true;
                stCLIENT* clientPtr = g_ClientList;
                while (1)
                {
                    if (clientPtr->sock == wParam)
                    {
                        break;
                    }
                    clientPtr = clientPtr->next;
                }
                WriteEvent(clientPtr);
                break;
            }
        }
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 메뉴 선택을 구문 분석합니다:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: 여기에 hdc를 사용하는 그리기 코드를 추가합니다...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// 정보 대화 상자의 메시지 처리기입니다.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

bool CloseSocket(SOCKET sock)
{
    stCLIENT* clientList = g_ClientList;
    while (1)
    {
        if (clientList->sock == sock)
        {
            clientList->prev->next = clientList->next;
            clientList->next->prev = clientList->prev;
            closesocket(clientList->sock);
            delete clientList;
            return true;
        }
        if (clientList->next == nullptr)
        {
            return false;
        }
    }
}

void AddSocket(SOCKET sock)
{
    stCLIENT* clientList = g_ClientList;
    while(1)
    {
        if (clientList->next == nullptr)
        {
            clientList->next = new stCLIENT;
            clientList->next->prev = clientList;
            clientList->next->sock = sock;
            WSAAsyncSelect(sock, hWnd, WM_MESSAGE, FD_READ | FD_WRITE | FD_CLOSE);
            break; 
        }
        clientList = clientList->next;
    }
}

int SendBroadCast(char* packet, int size)
{
    int retVal;
    char buffer[1000];
    stCLIENT* clientPtr = g_ClientList->next;
    while (1)
    {
        if (clientPtr == nullptr)
            break;

        retVal = send(clientPtr->sock, packet, size, NULL);

        clientPtr = clientPtr->next;
    }
    return 0;
}

void RecvEvent(SOCKET sock)
{
    stCLIENT* clientList = g_ClientList->next;
    int retVal;
    int retVal2;
    while (1)
    {
        if (clientList->sock == sock)
        {
            char buffer[1000];
            retVal = recv(sock, buffer, sizeof(buffer), NULL);
            if (retVal == 0)
            {
                //클라이언트와 접속이 끊겼습니다.
                printf("클라이언트와 접속이 끊겼습니다.\n");
                CloseSocket(sock);
                return;
            }

            retVal2 = clientList->recvRingBuffer.Enqueue(buffer, retVal);
            if (retVal != retVal2)
            {
                printf("이상한 패킷수신받음\n");
                CloseSocket(sock);
                return;
            }

            //만약 아직 패킷이 완성되지 않았다면 Send안에서 리턴하는 구조이다.
            Send(clientList, sizeof(stHEADER) + sizeof(stDRAWPACKET));
            break;
        }
        clientList = clientList->next;
    }
}

void Send(stCLIENT* clientPtr, int size)
{
    int retVal;

    if (clientPtr->recvRingBuffer.GetUseSize() < sizeof(stHEADER))
    {
        printf("아직 해더조차없습니다.\n");
        return;
    }

    stHEADER header;
    stDRAWPACKET drawPacket;
    retVal = clientPtr->recvRingBuffer.Peek((char*)&header, sizeof(header));

    if (clientPtr->recvRingBuffer.GetUseSize() < retVal + header.len)
    {
        printf("헤더는 들어왔지만 아직 데이터가 준비되지않았습니다.\n");
        return;
    }

    clientPtr->recvRingBuffer.MoveFront(sizeof(header)); //헤더는 받았으니 옮기는 작업
    
    retVal = clientPtr->recvRingBuffer.Dequeue((char*)&drawPacket, sizeof(drawPacket));

    if (retVal != sizeof(drawPacket))
    {
        printf("Send파트에서 recvRingBuffer에서 디큐할때 문제가 발생했습니다.\n");
        closesocket(clientPtr->sock);
        return;
    }

    //만약 송신링버퍼의 잔여 크기가 총 메시지의 수보다 작다면 그냥 closesocket때리고 리턴
    if (clientPtr->sendRingBuffer.GetFreeSize() < sizeof(header) + sizeof(drawPacket))
    {
        printf("sendRingBuffer의 GetFreeSize()가 부족합니다 뭔가이상합니다 CloseSocket()처리합니다.\n");
        CloseSocket(clientPtr->sock);
        return;
    }

    clientPtr->sendRingBuffer.Enqueue((char*)&header, sizeof(header));
    clientPtr->sendRingBuffer.Enqueue((char*)&drawPacket, sizeof(drawPacket));

    WriteEvent(clientPtr);
}

void WriteEvent(stCLIENT* clientPtr)
{
    int retVal;
    int retVal2;

    char buffer[1000];

    retVal = clientPtr->sendRingBuffer.Peek(buffer, sizeof(buffer));

    //int retVal2 = send(clientPtr->sock, buffer, retVal, NULL);
    SendBroadCast(buffer, retVal);

    /*if (retVal != retVal2)
    {
        if (WSAGetLastError() == WSAEWOULDBLOCK)
        {
            g_SendFlag == false;
            clientPtr->sendRingBuffer.MoveFront(retVal2);
            return;
        }
        printf("WriteEvent에서 이상한점이 발견되었습니다 삭제합니다\n");
        CloseSocket(clientPtr->sock);
        return;
    }*/
    
    clientPtr->sendRingBuffer.MoveFront(retVal);
}

void SendUniCastForOneClient(stCLIENT* clientPtr)
{

}