/**************************************************************************
  * @ file    : tcp_server.h
  * @ author  : syc
  * @ version : 1.0
  * @ date    : 2020.7.18
  * @ brief   : tcp_server_socket
***************************************************************************/
#ifndef  _TCPCLIENT_H_
#define _TCPCLIENT_H_

#include "protocol_client.h"
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


class TCPClient
{
public:
	//构造
	TCPClient();

	//析构
	virtual ~TCPClient();
	
	//开始连接
	bool StartConnection(NetConnectInfo  *connInfo);

	//退出客户端
	void StopConnection();

//私有方法
private:
	bool initSockConnetServer(int * psockfd,sockaddr_in * psokaddr);
	//通信线程
	static void communicationThread(void * arg);

	//通信处理
	void communicationProcess();

	//请求登录
	bool loginRequest();

	//登录回复
	bool loginReply();

	//请求音频
	bool audioRequest();

	//发送数据
	int  sendData(int sock,pthread_mutex_t* mutex, char *buf, int len);

	//接收数据
	int  recvData(int sock, pthread_mutex_t* mutex,char *buf, int len);

	//接受流媒体数据线程
	static void recvAudioDataThread(void * arg);

	//启动接收信令线程
	static void recvCommandThread(void * arg);

	//接收流媒体数据,音频数据
	void recvAudioData();

	//
	bool requestAudioData();

	//接收信令,心跳等等
	void recCommand();

//私有字段
private:
	//客户套接字.
    NetConnectInfo m_NetInfo ; 

	//申请的视频ID
	int             m_videoID;      //视频ID

	//申请的音频ID
	int             m_audioID;      //音频ID

	//用户全局ID
	int             m_userID;      //用户登录ID

	//服务端信令套接字地址.
	struct sockaddr_in m_csvr_sockaddr;
	//信令套接字.
	int  m_cSockfd; 

	//服务套接字地址.
	struct sockaddr_in m_cSockaddr;

	//音频流媒体套接字.
	int  m_adSockfd; 
	//
	struct sockaddr_in m_adSockaddr;

	//共享锁
	pthread_mutex_t m_cRecv_mutex;
	pthread_mutex_t m_cSend_mutex;
	pthread_mutex_t m_adRecv_mutex;
	pthread_mutex_t m_adSend_mutex;

	//服务器端返回的ID
	int  m_UserID ;

	//接收缓冲区指针
    char* m_pDataRecv;
	
	//接收缓冲区长度
	int m_nBufferLength;

	//接收的数据长度
	int  m_nRevDataSize;
	
	//线程退出信号
	bool m_bWorkFlag ;
	
	//音频数据接收开关
	bool m_bAudioTransFlag;

    //音频数据缓存
	char* m_pAudioDataRecvBuf;

    //音频数据缓存长度
	int m_nAudioBufferlenght;
};
#endif // _TCP_CLIENT_H_
