/**************************************************************************
  * @ file    : protocal_data.h
  * @ author  : syc
  * @ version : 1.0
  * @ date    : 2020.7.13
  * @ brief   : 协议数据结构
***************************************************************************/
#ifndef _PROTOCOL_DATA_H_
#define _PROTOCOL_DATA_H_

//定义
//数据类型定义
#define    CMD_KEEPALIVECOMMAND            255
#define    CMD_LOGINREQUEST                1    //登陆验证请求 
#define    CMD_LOGINREPLY                  2    //登陆回应  
//校验回复命令
#define     VERIFYREPLYRETURNVALUE_OK          0   //较验正确
#define     VERIFYREPLYRETURNVALUE_ERROR       2   //用户名或密码出

#define    CMD_VIDEOTRANSLATION_REQUEST    4    //视频请求  (信令)
#define    CMD_VIDEOTRANSLATION_REPLY      5    //视频回应  (信令)
#define    CMD_VIDEOTRANSLATION_STOP       6    //视频停止  (信令)

#define    CMD_AUDIOTRANSLATION_REQUEST         8    //音频请求  (信令)
#define    CMD_AUDIOTRANSLATION_REPLY           9    //音频回应  (信令)
#define    CMD_AUDIOTRANSLATION_STOP          10   //停止音频  (信令)

//定义1字节对齐，中间定义结构体
#pragma pack(1)
//协议头
typedef struct MessageHeader
{
	unsigned char           messageHeader[4];    //协议头  起始码，CEVS固定(controleasevideoserver)
	short                   cmdCode;         //指令码，区分同一协议中不同命令.
	int                     contentLength;        //命令中的正文的长度
	unsigned char           reserved[8];         //保留
 }MsgHeader;

//登录请求正文结构体
typedef struct VerifyUserInfoContent
{
	unsigned char   userName[10];      //用户名
	unsigned char   password[10];      //密码
    
}VerifyUserInfoContent;
//登陆响应结构体
typedef struct LoginRequestReply
{
  short           result;   //返回 0 校验正确，OK, 1用户名或密码错误
	int             userID;  //全局userID，
	unsigned char   reserved[8];  //保留
    
}LoginRequestReply;



//发送视频数据请求
typedef struct videoTranslationRequest
{
      int   userID;
      int   videoID;
 }VideoTranslationRequest;

//发送音频数据请求
typedef struct audioTranslationRequest
{
      int   userID;
      int   audioID;
 }AudioTranslationRequest;


//视频正文结构体
typedef struct videoDataContent
{
    unsigned int             timeStamp; //时间戳
    unsigned int             frameTime; //帧采集时间
    unsigned char            reserved;  //保留
    unsigned int             pictureLength;  //图片长度
    
}VideoDataContent;

//音频正文结构体
typedef struct audioDataContent
{
    unsigned int             timeStamp;//时间戳
    unsigned int             packageNumber; //包序号
    unsigned int             collectionTime; //采集时间
    char                     audioFormat;    //音频格式
    unsigned int             dataLength;     //数据长度
    
}AudioDataContent;


#pragma pack()


#endif // _MSG_DATA_DEFINE_H_
