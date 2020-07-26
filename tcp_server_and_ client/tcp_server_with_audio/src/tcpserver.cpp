/**************************************************************************
* @ file    : tcp_server.cpp
* @ author  : syc
* @ version : 1.0
* @ date    : 2020.7.12
* @ brief   : tcp_server
***************************************************************************/
#include "audio_capture.h"   
#include "pthread_define.h"
#include "protocol_data.h"
#include "tcpserver.h"

//#include "audio_capture.h"

//构造
TCPServer::TCPServer()
{
	//初始化一些全局变量
	m_nListenSockfd = -1;
	m_nBufferLength = 1024;
	m_pDataRecv = (char*)malloc(sizeof(char) * m_nBufferLength);
	memset(m_pDataRecv, 0, m_nBufferLength);
	m_bWorkFlag = true;
	//初始化互斥锁
    pthread_mutex_init(&m_cRecv_mutex, NULL);
    pthread_mutex_init(&m_cSend_mutex, NULL);
    pthread_mutex_init(&m_adRecv_mutex, NULL);
    pthread_mutex_init(&m_adSend_mutex, NULL);
}

//析构
TCPServer::~TCPServer()
{
	free(m_pDataRecv);
	m_clientLists.clear();
	if(&m_cRecv_mutex != NULL)
      pthread_mutex_destroy(&m_cRecv_mutex);
    if(&m_cSend_mutex != NULL)
      pthread_mutex_destroy(&m_cSend_mutex);
    if(&m_adRecv_mutex != NULL)
      pthread_mutex_destroy(&m_adRecv_mutex);
    if(&m_adSend_mutex != NULL)
      pthread_mutex_destroy(&m_adSend_mutex);
}

//启动服务
int TCPServer:: StartServer(int port,int connNum)
{
	int ret = 0;
	ret = initServer(port);
	
	if(ret)
	{
		printf("init_server_error \n");
		return -1;
	}
	printf("after initserver\n");
	ret = startListen(connNum);
	if(ret)
	{
		printf("start_listen_error \n");
		return -1;
	}
	return ret;
}

//停止服务
int TCPServer:: StopServer()
{
	m_bWorkFlag = true;
	close(m_nListenSockfd);
	return 0;
}

//初始化socket
int TCPServer:: initServer(int port)
{
	int ret;
	//server地址
	struct sockaddr_in server_addr;
	socklen_t server_addrlen;
	int opton= 1;
	//创建TCPsocket，返回socket描述符
	m_nListenSockfd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	//m_nListenSockfd = socket(AF_INET,SOCK_STREAM,0);

	if(-1 == m_nListenSockfd)
	{
		printf("socket error");
	 	ret = -1;
	 	return ret;
	}
	//启用端口复用选项功能，一个端口释放后等待两分钟才能再被使用，SO_REUSEADDR是让端口释放立即就可以被再次使用
	ret = setsockopt(m_nListenSockfd,SOL_SOCKET,SO_REUSEADDR,&opton,sizeof(opton));
	//将地址空间清零
	bzero(&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	printf("listen %d\n",port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addrlen = sizeof(struct sockaddr);
	printf("before bind ret:%d\n",ret);
	ret = bind(m_nListenSockfd,(const struct sockaddr*)&server_addr,server_addrlen);
	printf("bind ret:%d\n",ret);
	if(-1 == ret)
	{
		printf("bind error");
		ret = -1;
		return ret;
	}
	return ret;
}

//开始监听
int TCPServer:: startListen(int connNum)
{
	int ret;
	struct sockaddr_in client_addr;
	socklen_t client_addrlen;
	int client_sockfd;
	/*3,listen the socket*/
	m_nClientMaxcount = connNum;
	printf("server before listen\n");
	ret = listen(m_nListenSockfd,connNum);
	printf("server after listen\n");
	if(-1 == ret)
	{
		printf("listen error\n");
		ret = -1;
		return ret;
	}
	while(m_bWorkFlag)
	{
		/*4,accept the data from the client*/
		printf("accept\n");
		client_addrlen = sizeof(struct sockaddr);
		client_sockfd = accept(m_nListenSockfd,(struct sockaddr *)&client_addr,&client_addrlen);
		if(client_sockfd > 0)
		{    
		    	printf("Client IP is: %s,Port is:%d , sockfd is %d\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port),client_sockfd);
				//用全局的变量接住                                                 
				m_nClientSockfd = client_sockfd;
				m_clientLists.push_back(m_nClientSockfd);
		    	ret = detach_thread_create(NULL,(void *)sockProcessThread,(void *)this);
		}
		usleep(1*1000);
	}
	return ret; 
}

//处理客户端请求线程
void*  TCPServer::sockProcessThread(void* arg)
{
	((TCPServer *)arg)->receiveDataProcess();
	return 0 ;
}

//开始接收客户端通信的数据
void TCPServer:: receiveDataProcess()
{
	fd_set 		read_set;
	struct timeval 	tmval; 
	tmval.tv_sec = 10;//10s,10s 超时不发则断开连接
    tmval.tv_usec = 0;
	int clientsockfd = m_nClientSockfd;
	FD_ZERO(&read_set);
    FD_SET(clientsockfd,&read_set);
	int ret;
	m_nRevDataSize = 0;
	//先接收协议头，再按照协议头的命令码及接受长度接收数据实体内容
	MsgHeader* header;
	//如果客户端连接上后10S 不发送数据，或者发送错误的数据，则断开连接
	//int recvcount = 0;
	bool sockerror = false;
	//1、先接协议头，再接数据体，协议头中包含后面需要接受的数据的长度

	//先判断描述符中有没有数据可读
	ret = select(clientsockfd+1,&read_set, NULL, NULL,&tmval);
	//<0SOCKET_ERROR错误,有可能网络断开,退出轮询，关闭连接
	//==0等待超时，没有可读写或错误的文件
	if(ret<=0)
	{
		printf("select sock error\n");
		sockerror = true;
	}
	else//大于零的情况，才可以进行读取
	{
		if(FD_ISSET( clientsockfd, &read_set))
		{
			int revLen = recv(clientsockfd, m_pDataRecv, sizeof(MsgHeader), 0);
			if(revLen<=0)//SOCKET_ERRORselect错误,有可能网络断开
			{
				printf("recv sockerror\n");
				sockerror = true;
			}
			else
			{
				m_nRevDataSize +=revLen;
				if(m_nRevDataSize >= sizeof(MsgHeader))
				{
					printf("m_nRevDataSize >= sizeof(MsgHeader) : %d \n",revLen);
				}
			}
		}
	}
	
	if(sockerror)
	{
		close(clientsockfd);
		clientsockfd = 0;
		return;
	}
	
	//将缓冲区中的数转换成协议头格式
	header = (MsgHeader*)m_pDataRecv;
	int revLen = 0;
    switch(header->cmdCode)
    {
		
        case CMD_LOGINREQUEST://登录验证请求
			printf("enter CMD_LOGINREQUEST brench\n");
        	revLen = recv(clientsockfd, m_pDataRecv+m_nRevDataSize, header->contentLength, 0);
			if( revLen == header->contentLength)
			{
				m_nRevDataSize +=  header->contentLength;
				//校验结构
				VerifyUserInfoContent* userinfo = (VerifyUserInfoContent*)(m_pDataRecv+sizeof(MsgHeader));
				loginProcess(&clientsockfd,userinfo);
			}
			break;
        case CMD_AUDIOTRANSLATION_REQUEST:
			printf("enter CMD_AUDIOTRANSLATION_REQUEST\n");
			m_adSockfd = clientsockfd;
			revLen = recv(m_adSockfd, m_pDataRecv+m_nRevDataSize, header->contentLength, 0);
			if( revLen == header->contentLength)
			{
				m_nRevDataSize +=  header->contentLength;
				//校验结构
				AudioTranslationRequest* audioinfo = (AudioTranslationRequest*)(m_pDataRecv + sizeof(MsgHeader));
				if(audioinfo->userID ==1)//校验通过，能够传输音频
				{
					audioRequestProcess(&m_adSockfd,audioinfo);
				}
			}
			break;
		// case CMD_VIDEOTRANSLATION_REQUEST:
		// 	break;
	}

	// failed:
	// {
	// 	printf(" go to sock failed");
	// 	close(clientsockfd);
    //     clientsockfd = 0;
	// }
}

//登录,arg 验证信息结构体指针
int TCPServer::	loginProcess(int* psockfd,void* arg)
{
	char replyBuf[100];//足够
	memset(replyBuf, 0, sizeof(replyBuf));
	int clientsockfd = *psockfd;
	LoginRequestReply *replybody = (LoginRequestReply *)(replyBuf + sizeof(MsgHeader));
	if(!validateAuthentication(arg))//验证通过，回复登录
	{
		replybody->result = VERIFYREPLYRETURNVALUE_OK;//校验正确，允许登录
		//分配userID，是个全局token，后面的通讯将利用UserID进行全程的校验
		replybody->userID = 1;
		m_nUserID = 1;
	}
	else
	{
		replybody->result = 2;//用户名或密码错误
	}

	MsgHeader  *replyHeader;
	memset(replyHeader, 0, sizeof(replyHeader));
	strncpy((char*)replyHeader->messageHeader, "CEVS",4);//协议头
	replyHeader->cmdCode = CMD_LOGINREPLY;
	replyHeader->contentLength = sizeof(LoginRequestReply);
	ResponseLogin(psockfd,replyHeader,replybody);
	//校验正确，开启接收信令线程不断接收信令
	if(replybody->result== VERIFYREPLYRETURNVALUE_OK)
	{
		//todo，开线程，不断接收数据处理心跳等信令数据
	}
}

//登录回复
void TCPServer:: ResponseLogin(int *psockfd,void *pheader,void * preplybody)
{
	sendData(*psockfd,&m_cSend_mutex,(char*)pheader,sizeof(MsgHeader));
	sendData(*psockfd,&m_cSend_mutex,(char*)preplybody,sizeof(LoginRequestReply));
}

//发送数据
int TCPServer:: sendData(int sock,pthread_mutex_t *mutex, char *pBuf, int aLength)
{
    signal(SIGPIPE, SIG_IGN);
    //共享socket之间需要加锁来处理同时访问的问题
    pthread_mutex_lock(mutex);

    int sendLen=0;
    int nRet=0;

    while(sendLen<aLength)
    {
        if(sock>0)
        {
            nRet=send(sock,pBuf,aLength-sendLen,0);

            if(nRet<0)
            {
                if(errno==EAGAIN || errno == EINTR ||errno == EWOULDBLOCK){
                    usleep(10*1000);
                    continue;
                }
            }

            if(-1==nRet || 0==nRet)
            {
                pthread_mutex_unlock(mutex);
                printf("Socket send error nRet%d\n",nRet);
				printf("errno:%s\n", strerror(errno)); 
                return false;
            }

            sendLen+=nRet;
            pBuf+=nRet;

            printf("SEND LEN: %d %d\n",aLength,sendLen);
        }
        else
        {
            printf("Socket fd error %d\n",sock);
            pthread_mutex_unlock(mutex);
            return sendLen;
        }

    }

    pthread_mutex_unlock(mutex);

    return sendLen;
}

//接收数据
int TCPServer:: recvData(int sock, pthread_mutex_t* mutex,char *pBuf, int aLength)
{
    signal(SIGPIPE, SIG_IGN);

    pthread_mutex_lock(mutex);

    int recvLen=0;
    int nRet=0;

    while(recvLen<aLength)
    {
        nRet=recv(sock,pBuf,aLength-recvLen,0);

        if(nRet<0)
        {
            if(errno==EAGAIN || errno == EINTR ||errno == EWOULDBLOCK){
                usleep(10*1000);
                continue;
            }
            else 
            {
                pthread_mutex_unlock(mutex);
                printf("Socket recv error\n");
                return false;
            }
        }
        if( 0==nRet)//对方关闭连接
        {
            pthread_mutex_unlock(mutex);
            printf("Socket recv error\n");
            return false;
        }
        recvLen+=nRet;
        pBuf+=nRet;
    }

    pthread_mutex_unlock(mutex);
    return recvLen;
}

//鉴权0 正确，非零错误
int TCPServer::	validateAuthentication(void* arg)
{
	VerifyUserInfoContent* userinfo =(VerifyUserInfoContent*)arg;
	int usertoken = strncmp((char*)userinfo->userName, "admin", 5);
	int pwdtoken = strncmp((char*)userinfo->password, "abcd1234", 8);
	if(!usertoken&&!pwdtoken)
	  return 0;
	return -1;
}

//audio request
void TCPServer:: audioRequestProcess(int* psockfd,void* arg)
{
	AudioTranslationRequest* audioTransReq =(AudioTranslationRequest* )arg;
	if(audioTransReq-> audioID ==1)
	{
		detach_thread_create(NULL,(void *)audioCaptureThread,(void *)this);
	}
}

//audio capture thread
void*  TCPServer::audioCaptureThread(void* arg)
{
	((TCPServer *)arg)->audioCapture();
}		

//audio capture
void  TCPServer::audioCapture()
 {
	printf("enter audioCapture m_adSockfd %d\n",m_adSockfd);
	sleep(1);
	
    m_tAudioPcm = (audio_pcm_t*)malloc(sizeof(audio_pcm_t));
    m_tAudioPcm->frames = 32;
    m_tAudioPcm->val = 44100;
    m_tAudioPcm->channels = 2;
	if (initAudioCapture(m_tAudioPcm)==0)
	{
		m_nPcmBufSize  = (m_tAudioPcm->frames)*4;
		m_pPcmBuffer =  (char *)malloc(m_nPcmBufSize*2);
		printf("m_nPcmBufSize %d\n",m_nPcmBufSize*2);
		int capturesize;
		MsgHeader *replyHeader = (MsgHeader  *)malloc(sizeof(MsgHeader));
		(char *)malloc(m_nPcmBufSize*2);
	    memset(replyHeader, 0, sizeof(replyHeader));
		printf("after memset\n");
	    strncpy((char*)replyHeader->messageHeader, "CEVS",4);//协议头
		printf("after strncpy\n");
	    replyHeader->cmdCode = CMD_AUDIOTRANSLATION_REPLY;
		m_bAudioTransFlag = true;
		
		while(m_bAudioTransFlag)
		{
			capturesize = captureAudio(m_tAudioPcm,m_pPcmBuffer);
			if(capturesize>0)
			{
				replyHeader->contentLength = sizeof(capturesize);
				//传输protocol header
				sendData(m_adSockfd, &m_adSend_mutex,(char *)replyHeader, sizeof(MsgHeader));
				//传输音频
				printf("after send  MsgHeader m_adSockfd :%d\n",m_adSockfd);
                sendData(m_adSockfd, &m_adSend_mutex,m_pPcmBuffer, capturesize);
				printf("after send  capturesize m_adSockfd :%d,capturesize:%d\n",m_adSockfd,capturesize);
				break;
			}
		}
	}
 }

