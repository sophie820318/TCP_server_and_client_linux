/**************************************************************************
* @ file    : tcpclient.cpp
* @ author  : syc
* @ version : 1.0
* @ date    : 2020.7.20
* @ brief   : tcp_client
***************************************************************************/

#include "pthread_define.h"
#include "protocol_client.h"
#include "tcpclient.h"
#include <strings.h>
//构造
TCPClient::TCPClient()
{
    //网络通信参数结构体封装初始化
    memset(&m_NetInfo, 0, sizeof(NetConnectInfo));
    //初始化互斥锁
    pthread_mutex_init(&m_cRecv_mutex, NULL);
    pthread_mutex_init(&m_cSend_mutex, NULL);
    pthread_mutex_init(&m_adRecv_mutex, NULL);
    pthread_mutex_init(&m_adSend_mutex, NULL);
    //初始化音频队列放这里
    m_nAudioBufferlenght = 500;
    m_pAudioDataRecvBuf = (char *)malloc(sizeof(m_nAudioBufferlenght));
    printf("malloc  m_nAudioBufferlenght\n");
    memset(&m_pAudioDataRecvBuf, 0, sizeof(m_nAudioBufferlenght));
}

//析构
TCPClient::~TCPClient()
{
    // if(&m_NetInfo!= NULL)
    //     destroy(&m_NetInfo);
    if(&m_cRecv_mutex != NULL)
      pthread_mutex_destroy(&m_cRecv_mutex);
    if(&m_cSend_mutex != NULL)
      pthread_mutex_destroy(&m_cSend_mutex);
    if(&m_adRecv_mutex != NULL)
      pthread_mutex_destroy(&m_adRecv_mutex);
    if(&m_adSend_mutex != NULL)
      pthread_mutex_destroy(&m_adSend_mutex);
}

//开始连接
bool TCPClient::StartConnection(NetConnectInfo  *connInfo)
{
    memcpy(m_NetInfo.serverip,connInfo->serverip,sizeof(m_NetInfo.serverip));
    memcpy(m_NetInfo.username, connInfo->username, sizeof(m_NetInfo.username));
    memcpy(m_NetInfo.password, connInfo->password, sizeof(m_NetInfo.password));
    m_NetInfo.port = connInfo->port;
    if(initSockConnetServer(&m_cSockfd,&m_cSockaddr))
    {
        printf("sock fd :%d\n",m_cSockfd);
        communicationProcess();
    }
}

bool TCPClient:: initSockConnetServer(int * psockfd,sockaddr_in * psokaddr)
{
    *psockfd = socket(AF_INET,SOCK_STREAM,0);
    if(*psockfd<0)
    {
        printf("socket error!\n");
        return false;
    }
    psokaddr->sin_family = AF_INET;
    psokaddr->sin_port = htons(m_NetInfo.port);
    int i = 0;
    while(m_NetInfo.serverip[i]!= '\0')
    {   
        printf("%c",m_NetInfo.serverip[i]);   
        i++;
    }
    printf("\n");
    //将IP地址“点分十进制”和“二进制整数”间转换
    inet_pton(AF_INET,m_NetInfo.serverip,&psokaddr->sin_addr.s_addr);

    //设置为阻塞模式
    int flags1=fcntl(*psockfd, F_GETFL,0);
    fcntl(*psockfd, F_SETFL,flags1 &(~O_NONBLOCK));
   
    //设置发送和接收通信超时时间
    struct timeval timeout={15,0};
    int ret;
    ret=setsockopt(*psockfd , SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(struct timeval));
    if(ret<0)
    {
        printf("setsockopts error!!!\n");
        return false;
    }
    ret=setsockopt(*psockfd , SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(struct timeval));
    if(ret<0)
    {
        printf("setsockoptr error!!!\n");
        return false;
    }
    printf("Start connect:\n");
    //连接服务器
    int connetRet=connect(*psockfd ,(struct sockaddr*)psokaddr,sizeof(struct sockaddr_in));
    if(connetRet<0)
    {
         perror("connect error \n");
         return false;
    }
    printf("socket connect server success\n");
    return true;
}

//登录入口
void TCPClient:: communicationProcess()
{
    fd_set w_set;
    FD_ZERO(&w_set);
    FD_SET(m_cSockfd,&w_set);
    struct timeval timeout={10,0};
    int ret=select(m_cSockfd+1,NULL, &w_set, NULL, &timeout);
    if(ret==-1)//sockerror
    {
        printf("socket error\n");
        return ;
    }
    else if(ret==0)
    {
        printf("socket timeout\n");
        return ;
    }
    else if(ret>0)
    {
        //可写,可通讯
        if(FD_ISSET(m_cSockfd,&w_set))
        {
            if(loginRequest())
            {
                if(loginReply())
                {
                    //建立信令接收线程
                    detachThreadCreate(NULL,(void*)recvCommandThread,(void *)this);
                    //初始化音频通道,连接服务器
                    //申请数据：uID+音频ID
                    if(requestAudioData())
                    {
                        printf("before recvAudioDataThread\n");
                        detachThreadCreate(NULL,(void*)recvAudioDataThread,(void *)this);
                    }
                }
            }
        }
    }
}

//接受流媒体数据线程
void TCPClient::recvAudioDataThread(void * arg)
{
    printf("recvAudioDataThread\n");
    TCPClient * client= (TCPClient *)arg;
    client->recvAudioData();
}

//启动接收信令线程
void TCPClient::recvCommandThread(void * arg)
{
    TCPClient * client= (TCPClient *)arg;
    client->recCommand();
}

//接收流媒体数据,音频数据
void TCPClient::recvAudioData()
{
    printf("enter recvAudioData  branch\n");

    printf("memset  m_nAudioBufferlenght\n");
    m_bAudioTransFlag = true;
    int receivelength ;
    MsgHeader* header;
    while (m_bAudioTransFlag)
    {
        printf("start recv audio data\n");
        receivelength =  recvData(m_adSockfd,&m_adRecv_mutex,m_pAudioDataRecvBuf,sizeof(MsgHeader));
        if(receivelength==sizeof(MsgHeader))
        {
            header = (MsgHeader*)m_pAudioDataRecvBuf;
            if( header!= NULL && header->contentLength>0 )
            {
                receivelength =  recvData(m_adSockfd,&m_adRecv_mutex,m_pAudioDataRecvBuf,sizeof(header->contentLength));
                if(receivelength>0)
                {
                    printf("recv audio lenght %d\n",receivelength);
                    //播放
                }
            }
        }
      usleep(1*1000);
    }
}

//申请音频数据
bool TCPClient::requestAudioData()
{
    if(initSockConnetServer(&m_adSockfd,&m_adSockaddr))
    {
        if(audioRequest())
        {
          printf("audioRequest return true\n");
          return true;
        }
    }
    else
    {
        printf("audioRequest return false\n");
        return false;
        
    }
    printf("audioRequest return false\n");
    return false;
}

//接收信令,心跳等等
void TCPClient::recCommand()
{
    //todo//接收信令,心跳等等处理
}

//音频请求
bool TCPClient::audioRequest()
{
    MsgHeader msgHeader;
    memset(&msgHeader,0,sizeof(msgHeader));
    strncpy((char*)msgHeader.messageHeader, "CEVS",4);//协议头
    msgHeader.cmdCode = CMD_AUDIOTRANSLATION_REQUEST;//8 //音频请求  (信令)
    int contentLength = sizeof(AudioTranslationRequest);
    msgHeader.contentLength = contentLength;
    //先发送头
    int sendLength1 = sendData(m_adSockfd,&m_adSend_mutex,(char *)&msgHeader,sizeof(msgHeader));
    //todo:校验
    AudioTranslationRequest audioRequest;
    memset(&audioRequest,0,contentLength);
    audioRequest.userID = 1;
    audioRequest.audioID = 1;
    //再发送正文内容
    int sendLength2 = sendData(m_adSockfd,&m_adSend_mutex,(char *)&audioRequest,contentLength);
    //todo:校验
    return true;
}

//登录请求
bool TCPClient::loginRequest()
{
    MsgHeader msgHeader;
    memset(&msgHeader,0,sizeof(msgHeader));
    strncpy((char*)msgHeader.messageHeader, "CEVS",sizeof("CEVS"));//协议头
    msgHeader.cmdCode = CMD_LOGINREQUEST;//1代表登录请求
    int contentLength = sizeof(LoginRequestContent);
    msgHeader.contentLength = contentLength;
    //先发送头
    int sendLength1 = sendData(m_cSockfd,&m_cSend_mutex,(char *)&msgHeader,sizeof(msgHeader));
    //todo:校验
    LoginRequestContent userInfo;
    memset(&userInfo,0,contentLength);
    strncpy((char*)userInfo.userName, "admin", sizeof("admin"));
    strncpy((char*)userInfo.passWord, "abcd1234", sizeof("abcd1234"));
    //再发送正文内容
    int sendLength2 = sendData(m_cSockfd,&m_cSend_mutex,(char *)&userInfo,contentLength);
    //todo:校验
    return true;
}

//登录回应
bool TCPClient::loginReply()
{
    LoginRequestReply loginReply;
    memset(&loginReply, 0, sizeof(loginReply));
    int recvLength = recvData(m_cSockfd, &m_cRecv_mutex,(char *)&loginReply,  sizeof(loginReply));
    if( recvLength == sizeof(loginReply))
    {
        if(strncmp((char*)loginReply.msgHeader.messageHeader,"CEVS",4)==0)
        {
            if(loginReply.msgHeader.cmdCode ==CMD_LOGINREPLY&& loginReply.result ==VERIFYREPLYRETURNVALUE_OK)
            {
                printf("登录成功,分配唯一用户ID,IDLoginReply INFO userID:  %d\n",loginReply.userID);
                return true;
            }
        }
    }


}

//发送数据
int TCPClient:: sendData(int sock,pthread_mutex_t *mutex, char *pBuf, int aLength)
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
                printf("Socket send error\n");
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
int TCPClient:: recvData(int sock, pthread_mutex_t* mutex,char *pBuf, int aLength)
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
            if(errno==EAGAIN || errno == EINTR ||errno == EWOULDBLOCK)
            {
                usleep(1*1000);
                printf("Socket recv usleep continue\n");
                continue;
            }
        }
        if(nRet==-1|| 0==nRet)//对方关闭连接
        {
            pthread_mutex_unlock(mutex);
            printf("Socket recv error\n");
            return nRet;
        }
        recvLen+=nRet;
        pBuf+=nRet;
    }

    pthread_mutex_unlock(mutex);
    return recvLen;
}

//结束客户端
void TCPClient::StopConnection()
{

}
