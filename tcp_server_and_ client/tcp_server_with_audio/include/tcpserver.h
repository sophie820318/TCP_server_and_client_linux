/**************************************************************************
  * @ file    : tcp_server.h
  * @ author  : syc
  * @ version : 1.0
  * @ date    : 2020.7.18
  * @ brief   : tcp_server_socket
***************************************************************************/
#ifndef _TCPSERVER_H_
#define _TCPSERVER_H_

extern "C"
{
	#include "audio_capture.h"   
}
 
#include<sys/ioctl.h>
#include <unistd.h> 
#include <stdio.h>                                
#include <stdlib.h>                                
#include <string.h>                                
#include <sys/socket.h>                                                   
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <vector>
#include <signal.h>


typedef struct clientSockInformation
{
	int  Sockfd; 
	struct sockaddr_in Sockaddr;
}clientSockInfo;


class TCPServer
{
public:
	//构造
	TCPServer();

	//析构
	virtual ~TCPServer();
	
	//启动服务
	int StartServer(int port,int connNum);

	//停止服务
	int StopServer();

//私有方法
private:
	//初始化
	int initServer(int port);
	
	//开始监听
	int startListen(int connNum);

	//处理客户端业务逻辑的代理函数，负责转接到对应业务函数
	static void* sockProcessThread(void* arg);

	//客户端通信处理转接函数
	void sockProcess();
	
	//服务端socket接收数据业务处理
	void receiveDataProcess();
	
	//登录请求处理
	int loginProcess(int* psockfd,void* arg);

	//登录回复
	void ResponseLogin(int *psockfd,void *pheader,void * preply);

	//recvData
    int recvData(int sock, pthread_mutex_t* mutex,char *pBuf, int aLength);

	//sendData
	int sendData(int sock,pthread_mutex_t *mutex, char *pBuf, int aLength);

	//鉴权
	int validateAuthentication(void* arg);
	
	//音频请求处理
	void audioRequestProcess(int* psockfd,void* arg);

	//传输音频
	//void transAudioData(void* arg);

	//音频采集线程入口
	static void* audioCaptureThread(void* arg);

	//音频采集
	void  audioCapture();


//私有字段
private:
	//服务命令套接字.
    int  m_nListenSockfd; 

	//客户端信令套接字.
	int  m_nClientSockfd; 

	//audio data sockfd.
	int  m_adSockfd; 

	//分配的
	int m_nUserID ;

	//接收缓冲区指针
    char* m_pDataRecv;
	
	//接收缓冲区长度
	int m_nBufferLength;

	//接收的数据长度
	int  m_nRevDataSize;
	
	//统计客户端连接集合
	std::vector<int> m_clientLists;

	//mutex
	pthread_mutex_t m_cRecv_mutex;
	pthread_mutex_t m_cSend_mutex;
	pthread_mutex_t m_adRecv_mutex;
	pthread_mutex_t m_adSend_mutex;

	//客户端最大连接数量
	int m_nClientMaxcount;

	//线程退出信号
	bool m_bWorkFlag ;

	//音频传输终止退出信号
	bool m_bAudioTransFlag ;
	
	//音频结构体信息
	audio_pcm_t*  m_tAudioPcm;

	//音频采集缓冲区
	char* m_pPcmBuffer;

	//音频缓冲区大小
	int m_nPcmBufSize;

};
#endif // _TCP_SERVER_H_
