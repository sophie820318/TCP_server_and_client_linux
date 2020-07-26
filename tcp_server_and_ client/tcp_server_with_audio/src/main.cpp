/**************************************************************************
  * @ file    : main.c
  * @ author  : syc
  * @ version : 1.0
  * @ date    : 2020.7.13
  * @ brief   :验证server工作逻辑
***************************************************************************/
#include "tcpserver.h"
//#include "audiocapture.h"
//#include "pthread_define.h"

int main()
{

	TCPServer server;
	//端口：8000，允许连接数量100,本机8000端口连接
	server.StartServer(8000,100);
	
}
