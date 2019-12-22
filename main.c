//#include <windows.h>

#include <TCHAR.h>
#include <stdio.h>
#include <stdlib.h>
#include <Winsock2.h>
#include <string.h>

#pragma comment(lib, "Ws2_32.lib")

// WM_
#define UM_ASYNCSELECTMSG  WM_USER+1

LRESULT CALLBACK WinBackProc(HWND hWnd, UINT msgID, WPARAM wparam, LPARAM lparam);

#define MAX_SOCK_COUNT 1024
SOCKET g_sockALL[MAX_SOCK_COUNT];
int g_count = 0;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInstance, LPSTR lpCmdLine, int nShowCmd)
{
	//创建窗口结构体
	WNDCLASSEX wc;
	wc.cbClsExtra = 0;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.cbWndExtra = 0;
	wc.hbrBackground = NULL;
	wc.hCursor = NULL;
	wc.hIcon = NULL;
	wc.hIconSm = NULL;
	wc.hInstance = hInstance;
	wc.lpfnWndProc = WinBackProc;
	wc.lpszClassName = _T("c3window");
	wc.lpszMenuName = NULL;
	wc.style = CS_HREDRAW | CS_VREDRAW;

	//注册结构体
	RegisterClassEx(&wc);

	//创建窗口
	HWND hWnd = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW, TEXT("c3window"), _T("c3窗口"), WS_OVERLAPPEDWINDOW, 200, 200, 600, 400, NULL, NULL, hInstance, NULL);
	if (NULL == hWnd)
	{
		return 0;
	}

	//显示窗口
	ShowWindow(hWnd, nShowCmd);//1

	//更新窗口
	UpdateWindow(hWnd);

	WORD wdVersion = MAKEWORD(2, 2); //2.1  //22
	//int a = *((char*)&wdVersion);
	//int b = *((char*)&wdVersion+1);
	WSADATA wdScokMsg;
	//LPWSADATA lpw = malloc(sizeof(WSADATA));// WSADATA*
	int nRes = WSAStartup(wdVersion, &wdScokMsg);

	if (0 != nRes)
	{
		switch (nRes)
		{
		case WSASYSNOTREADY:
			printf("重启下电脑试试，或者检查网络库");
			break;
		case WSAVERNOTSUPPORTED:
			printf("请更新网络库");
			break;
		case WSAEINPROGRESS:
			printf("请重新启动");
			break;
		case WSAEPROCLIM:
			printf("请尝试关掉不必要的软件，以为当前网络运行提供充足资源");
			break;
		}
		return  0;
	}

	//校验版本
	if (2 != HIBYTE(wdScokMsg.wVersion) || 2 != LOBYTE(wdScokMsg.wVersion))
	{
		//说明版本不对
		//清理网络库
		WSACleanup();
		return 0;
	}

	SOCKET socketServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//int a = WSAGetLastError();
	if (INVALID_SOCKET == socketServer)
	{
		int a = WSAGetLastError();
		//清理网络库
		WSACleanup();
		return 0;
	}

	struct sockaddr_in si;
	si.sin_family = AF_INET;
	si.sin_port = htons(12345);
	si.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	//int a = ~0;
	if (SOCKET_ERROR == bind(socketServer, (const struct sockaddr *)&si, sizeof(si)))
	{
		//出错了
		int a = WSAGetLastError();
		//释放
		closesocket(socketServer);
		//清理网络库
		WSACleanup();
		return 0;
	}

	if (SOCKET_ERROR == listen(socketServer, SOMAXCONN))
	{
		//出错了
		int a = WSAGetLastError();
		//释放
		closesocket(socketServer);
		//清理网络库
		WSACleanup();
		return 0;
	}

	if (SOCKET_ERROR == WSAAsyncSelect(socketServer, hWnd, UM_ASYNCSELECTMSG, FD_ACCEPT))
	{
		//出错了
		int a = WSAGetLastError();
		//释放
		closesocket(socketServer);
		//清理网络库
		WSACleanup();
		return 0;
	}

	g_sockALL[g_count] = socketServer;
	g_count++;

	//消息循环
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	//关闭socket
	for (int i = 0; i < g_count; i++)
	{
		closesocket(g_sockALL[i]);
	}

	WSACleanup();
	return 0;
}

int x = 0;

LRESULT CALLBACK WinBackProc(HWND hWnd, UINT msgID, WPARAM wparam, LPARAM lparam)
{
	HDC hdc = GetDC(hWnd);
	switch (msgID)
	{
	case UM_ASYNCSELECTMSG:
		{
			//MessageBox(NULL, L"有信号啦", L"提示", MB_YESNO);
			//获取socket
			SOCKET sock = (SOCKET)wparam;
			//获取消息
			if (0 != HIWORD(lparam))
			{
				if (WSAECONNABORTED == HIWORD(lparam))
				{
					TextOut(hdc, 0, x, _T("close"), strlen("close"));
					x += 15;
					//关闭该socket上的消息
					WSAAsyncSelect(sock, hWnd, 0, 0);
					//关闭socket
					closesocket(sock);
					//记录数组中删除该socket
					for (int i = 0; i < g_count; i++)
					{
						if (sock == g_sockALL[i])
						{
							g_sockALL[i] = g_sockALL[g_count - 1];
							g_count--;
							break;
						}
					}
				}
				break;
			}
			//具体消息
			switch (LOWORD(lparam))
			{
			case FD_ACCEPT:
				{
					TextOut(hdc, 0, x, _T("accept"), strlen("accept"));
					x += 15;
					SOCKET socketClient = accept(sock, NULL, NULL);
					if (INVALID_SOCKET == socketClient)
					{
						//出错了
						int a = WSAGetLastError();
						break;
					}
					//将客户端投递给消息队列
					if (SOCKET_ERROR == WSAAsyncSelect(socketClient, hWnd, UM_ASYNCSELECTMSG, FD_READ | FD_WRITE | FD_CLOSE))
					{
						//出错了
						int a = WSAGetLastError();
						//释放
						closesocket(socketClient);
						break;
					}
					//记录
					g_sockALL[g_count] = socketClient;
					g_count++;
				}
				break;
			case FD_READ:
		   		{
					TextOut(hdc, 0, x, _T("read"), strlen("read"));
					char str[1024] = { 0 };
					if (SOCKET_ERROR == recv(sock, str, 1023, 0))
					{
						break;
					}
					TextOut(hdc, 30, x, str, strlen(str));
					x += 15;
				}
				break;
			case FD_WRITE:
				//send 
				TextOut(hdc, 0, x, _T("wrtie"), strlen("wrtie"));
				x += 15;
				break;
			case FD_CLOSE:
				TextOut(hdc, 0, x, _T("close"), strlen("close"));
				x += 15;
				//关闭该socket上的消息
				WSAAsyncSelect(sock, hWnd, 0, 0);
				//关闭socket
				closesocket(sock);
				//记录数组中删除该socket
				for (int i = 0; i < g_count; i++)
				{
					if (sock == g_sockALL[i])
					{
						g_sockALL[i] = g_sockALL[g_count - 1];
						g_count--;
						break;
					}
				}
			}
		}
		break;
	case WM_CREATE: //初始化 只执行一次

		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}

	ReleaseDC(hWnd, hdc);

	return DefWindowProc(hWnd, msgID, wparam, lparam);
}
