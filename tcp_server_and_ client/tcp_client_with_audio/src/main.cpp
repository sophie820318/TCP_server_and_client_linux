/**************************************************************************
  * @ file    : main.c
  * @ author  : syc
  * @ version : 1.0
  * @ date    : 2020.7.13
  * @ brief   :验证client工作逻辑
***************************************************************************/
#include "tcpclient.h"

int main()
{
	TCPClient client;
  NetConnectInfo info;
  strncpy((char*)info.serverip, "192.168.3.28",12);
  strncpy((char*)info.password, "abcd1234",8);
   strncpy((char*)info.username, "admin",5);
   info.port = 8000;
	//端口：8000，
	client.StartConnection(&info);

}
