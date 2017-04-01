//
// Created by Administrator on 2016/11/7.
//
#include "com_xinzhihui_myndkdemo_MyNdk.h"
#include "android/log.h"
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#define HTTP_GET "GET /%s HTTP/1.1\r\nHOST: %s:%d\r\nAccept: */*\r\n\r\n"
#define BUFFER_SIZE 1024
#define HTTP_POST "POST /%s HTTP/1.1\r\nHOST: %s:%d\r\nAccept: */*\r\n"\
    "Content-Type:application/x-www-form-urlencoded\r\nContent-Length: %d\r\n\r\n%s"

static int http_tcpclient_create(const char *host, int port){
    struct hostent *he;
    struct sockaddr_in server_addr;
    int socket_fd;

    if((he = gethostbyname(host))==NULL){
        return -1;
    }

   server_addr.sin_family = AF_INET;
   server_addr.sin_port = htons(port);
   server_addr.sin_addr = *((struct in_addr *)he->h_addr);

    if((socket_fd = socket(AF_INET,SOCK_STREAM,0))==-1){
        return -1;
    }

    if(connect(socket_fd, (struct sockaddr *)&server_addr,sizeof(struct sockaddr)) == -1){
        return -1;
    }

    return socket_fd;
}

static void http_tcpclient_close(int socket){
    close(socket);
}

static int http_parse_url(const char *url,char *host,char *file,int *port)
{
    char *ptr1,*ptr2;
    int len = 0;
    if(!url || !host || !file || !port){
        return -1;
    }

    ptr1 = (char *)url;

    if(!strncmp(ptr1,"http://",strlen("http://"))){
        ptr1 += strlen("http://");
    }else{
        return -1;
    }

    ptr2 = strchr(ptr1,'/');
    if(ptr2){
        len = strlen(ptr1) - strlen(ptr2);
        memcpy(host,ptr1,len);
        host[len] = '\0';
        if(*(ptr2 + 1)){
            memcpy(file,ptr2 + 1,strlen(ptr2) - 1 );
            file[strlen(ptr2) - 1] = '\0';
        }
    }else{
        memcpy(host,ptr1,strlen(ptr1));
        host[strlen(ptr1)] = '\0';
    }
    //get host and ip
    ptr1 = strchr(host,':');
    if(ptr1){
        *ptr1++ = '\0';
        *port = atoi(ptr1);
    }else{
        *port = 80;
    }

    return 0;
}


static int http_tcpclient_recv(int socket,char *lpbuff){
    int recvnum = 0;

    recvnum = recv(socket, lpbuff,BUFFER_SIZE*4,0);

    return recvnum;
}

static int http_tcpclient_send(int socket,char *buff,int size){
    int sent=0,tmpres=0;

    while(sent < size){
        tmpres = send(socket,buff+sent,size-sent,0);
        if(tmpres == -1){
            return -1;
        }
        sent += tmpres;
    }
    return sent;
}

static char *http_parse_result(const char*lpbuf)
{
    char *ptmp = NULL;
    char *response = NULL;
    ptmp = (char*)strstr(lpbuf,"HTTP/1.1");
    if(!ptmp){
        __android_log_print(ANDROID_LOG_INFO,"liang","http/1.1 not faind\n");
        return NULL;
    }
    if(atoi(ptmp + 9)!=200){
    //TODO: 状态码200表示返回成功，不是404 or 500
        __android_log_print(ANDROID_LOG_INFO,"liang","result:\n%s\n",lpbuf);
        return NULL;
    }

    ptmp = (char*)strstr(lpbuf,"\r\n\r\n");
    if(!ptmp){
        __android_log_print(ANDROID_LOG_INFO,"liang","ptmp is NULL\n");
        return NULL;
    }
    response = (char *)malloc(strlen(ptmp)+1);
    if(!response){
        __android_log_print(ANDROID_LOG_INFO,"liang","malloc failed \n");
        return NULL;
    }
    strcpy(response,ptmp+4);
    return response;
}

char * http_post(const char *url,const char *post_str){

    char post[BUFFER_SIZE] = {'\0'};
    int socket_fd = -1;
    char lpbuf[BUFFER_SIZE*4] = {'\0'};
    char *ptmp;
    char host_addr[BUFFER_SIZE] = {'\0'};
    char file[BUFFER_SIZE] = {'\0'};
    int port = 0;
    int len=0;
    char *response = NULL;

    if(!url || !post_str){
        printf("      failed!\n");
        return NULL;
    }

    if(http_parse_url(url,host_addr,file,&port)){
        printf("http_parse_url failed!\n");
        return NULL;
    }
    //printf("host_addr : %s\tfile:%s\t,%d\n",host_addr,file,port);

    socket_fd = http_tcpclient_create(host_addr,port);
    if(socket_fd < 0){
        printf("http_tcpclient_create failed\n");
        return NULL;
    }

    sprintf(lpbuf,HTTP_POST,file,host_addr,port,strlen(post_str),post_str);

    if(http_tcpclient_send(socket_fd,lpbuf,strlen(lpbuf)) < 0){
        printf("http_tcpclient_send failed..\n");
        return NULL;
    }
    //printf("发送请求:\n%s\n",lpbuf);

    /*it's time to recv from server*/
    if(http_tcpclient_recv(socket_fd,lpbuf) <= 0){
        printf("http_tcpclient_recv failed\n");
        return NULL;
    }

    http_tcpclient_close(socket_fd);

    return http_parse_result(lpbuf);
}

char * http_get(const char *url)
{

    char post[BUFFER_SIZE] = {'\0'};
    int socket_fd = -1;
    char lpbuf[BUFFER_SIZE*4] = {'\0'};
    char *ptmp;
    char host_addr[BUFFER_SIZE] = {'\0'};
    char file[BUFFER_SIZE] = {'\0'};
    int port = 0;
    int len=0;

    if(!url){
        printf("      failed!\n");
        return NULL;
    }

    if(http_parse_url(url,host_addr,file,&port)){
        printf("http_parse_url failed!\n");
        __android_log_print(ANDROID_LOG_INFO,"liang","服务器地址:%s\n",host_addr);
        return NULL;
    }else {
    __android_log_print(ANDROID_LOG_INFO,"liang","服务器:%s\n",host_addr);
    }
    //printf("host_addr : %s\tfile:%s\t,%d\n",host_addr,file,port);

    socket_fd =  http_tcpclient_create(host_addr,port);
    if(socket_fd < 0){
        __android_log_print(ANDROID_LOG_INFO,"liang","http_tcpclient_create failed\n");
        return NULL;
    }

    sprintf(lpbuf,HTTP_GET,file,host_addr,port);

    if(http_tcpclient_send(socket_fd,lpbuf,strlen(lpbuf)) < 0){
        __android_log_print(ANDROID_LOG_INFO,"liang","http_tcpclient_send failed..\n");
        return NULL;
    }
//  printf("发送请求:\n%s\n",lpbuf);

    if(http_tcpclient_recv(socket_fd,lpbuf) <= 0){
        __android_log_print(ANDROID_LOG_INFO,"liang","http_tcpclient_recv failed\n");
        return NULL;
    }
    http_tcpclient_close(socket_fd);

    return http_parse_result(lpbuf);
}




JNIEXPORT jstring JNICALL Java_com_xinzhihui_myndkdemo_MyNdk_getString
  (JNIEnv * env, jobject obj){
   return (*env).NewStringUTF("This is mylibrary !!!");
  }

JNIEXPORT jstring JNICALL Java_com_xinzhihui_myndkdemo_MyNdk_socket
  (JNIEnv * pEnv, jobject pThis, jstring pUrl, jstring pHead){
//  char *host ="192.168.0.66";
//  char *url = "http://192.168.0.66";
//  int port = 80;
//
//int socket_handle = socket(AF_INET,SOCK_STREAM,0);
//if(socket_handle<0){
//__android_log_print(ANDROID_LOG_INFO,"liang","建立socket()出错...%s\n",strerror(errno));
//}else{
//__android_log_write(ANDROID_LOG_INFO,"liang","建立socket()成功...\n");
//struct sockaddr_in loc_addr;//本机地址
//loc_addr.sin_family = AF_INET;//协议
//loc_addr.sin_addr.s_addr = htons(INADDR_ANY);
//loc_addr.sin_port = htons(INADDR_ANY);
//
//if(bind(socket_handle , (struct sockaddr *)&loc_addr , sizeof(struct sockaddr_in))<0){
//__android_log_print(ANDROID_LOG_INFO,"liang","bind()出错...%s\n",strerror(errno));
//}else{
//__android_log_write(ANDROID_LOG_INFO,"liang","bind()成功...\n");
//
//struct sockaddr_in serv_add;//服务器地址
//serv_add.sin_family = AF_INET;
//serv_add.sin_addr.s_addr = inet_addr("14.17.105.214");
//serv_add.sin_port = htons(9527);
//
//if(connect(socket_handle,(struct sockaddr *)&serv_add,sizeof(struct sockaddr_in))<0){
//__android_log_print(ANDROID_LOG_INFO,"liang","connect()出错...%s\n",strerror(errno));
//}else{
//__android_log_write(ANDROID_LOG_INFO,"liang","connect()成功...\n");
//
//char *head = "GET http://192.168.0.66/HttpServer/GetTest.php?name=lang&age=20 HTTP/1.1\r\n"
//"Host:192.168.0.66:80\r\n\r\n";
//if(send(socket_handle,head,strlen(head),0)<0){ //发送头部
//__android_log_print(ANDROID_LOG_INFO,"liang","send()出错...%s\n",strerror(errno));
//}else{
//__android_log_write(ANDROID_LOG_INFO,"liang","send()成功...\n");
//char *result = (char*)malloc(sizeof(char));
//char *temp_result = (char*)malloc(sizeof(char));;
//int SIZE = sizeof(char) * 1024;
//char *cache = (char*)malloc(SIZE);
//int len = 0;
//
//memset(result,0x00,sizeof(char));
//memset(temp_result,0x00,sizeof(char));
//memset(cache,0x00,SIZE);
//
////cache = http_get("http://14.17.105.214:9527");
//
//while((len=recv(socket_handle,cache,SIZE,0))>0){//读服务器信息
//__android_log_print(ANDROID_LOG_INFO,"liang","服务器:%d\n",len);
//int tempLen = sizeof(char) * strlen(result) + 1;
//free(temp_result);
//temp_result = (char*)malloc(tempLen);
//memset(temp_result,0x00,tempLen);
//strcpy(temp_result,result);
//
//free(result);
//tempLen += strlen(cache);
//result = (char*)malloc(tempLen);
//memset(result,0x00,tempLen);
//strcpy(result,temp_result);
//strcat(result,cache);
//
//memset(cache,0x00,SIZE);
//}
//__android_log_print(ANDROID_LOG_INFO,"liang","服务器:%s\n",cache);
//__android_log_print(ANDROID_LOG_INFO,"liang","服务器:%s\n",result);
//
//if(result!=NULL)
//free(result);
//if(temp_result!=NULL)
//free(temp_result);
//if(cache!=NULL)
//free(cache);
//}
//}
//close(socket_handle);
//}
//
//}
char *result = (char*)malloc(sizeof(char));
result = http_get("http://dc.apphub.bigbigon.com/devinfo/v1/upgrade?deviceId=181007cc519041040000740000000000&appVersion=01.01.00&appName=R2Update");
__android_log_print(ANDROID_LOG_INFO,"liang","服务器:%s\n",result);
result = http_post("http://trackapi.365car.com.cn:9527/track/VehicleTrack/getLastPoint", "methodName=obd/VehicleTrack/getLastPoint");
__android_log_print(ANDROID_LOG_INFO,"liang","服务器:%s\n",result);

  return NULL;
}
