/******************************* 
接海康摄像头ok，需要注意有些IPC是旧版本(2019年之后的IPC应该是没问题的)的onvif是不支持media2，接旧版本IPC会提示“method not implemented”
********************************/
#include "stdsoap2.h"
#include "soapH.h"
#include "soapStub.h"
#include "wsseapi.h"
#include "namespace.h"

#include <openssl/sha.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <vector>
#include <iostream>
#include <sstream>

using namespace std;

int ret = 0;
char* user_name = "admin";
char* passwd    = "admin";

static void split(string src, char delim, vector<string>& array)
{
    stringstream tmp_strream(src);
    string tmp;
    while(getline(tmp_strream, tmp, delim))
    {
        array.push_back(tmp);
    }
}

static struct soap* ONVIF_Initsoap(struct SOAP_ENV__Header *header, const char *was_To, const char *was_Action, int timeout)
{
    struct soap *soap = NULL;    // soap环境变量
    unsigned char macaddr[6];
    char _HwId[1024];
    unsigned int Flagrand;
 
    soap = soap_new();
    if(soap == NULL)
    {
        printf("[%d]soap = NULL\n", __LINE__);
        return NULL;
    }
 
    soap_set_namespaces(soap, namespaces);   // 设置soap的namespaces，即设置命名空间
 
    // 设置超时（超过指定时间没有数据就退出）
    if(timeout > 0)
    {
        soap->recv_timeout = timeout;
        soap->send_timeout = timeout;
        soap->connect_timeout = timeout;
    }
    else
    {
        //Maximum waittime : 20s
        soap->recv_timeout  = 20;
        soap->send_timeout  = 20;
        soap->connect_timeout = 20;
    }
 
    soap_default_SOAP_ENV__Header(soap, header);
 
    //Create SessionID randomly,生成uuid(windows下叫guid，linux下叫uuid)，格式为urn:uuid:8-4-4-4-12，由系统随机产生
    srand((int)time(0));
    Flagrand = rand()%9000 + 8888;
    macaddr[0] = 0x1;
    macaddr[1] = 0x2;
    macaddr[2] = 0x3;
    macaddr[3] = 0x4;
    macaddr[4] = 0x5;
    macaddr[5] = 0x6;
    sprintf(_HwId, "urn:uuid:%ud68a-1dd2-11b2-a105-%02X%02X%02X%02X%02X%02X", Flagrand, macaddr[0], macaddr[1], macaddr[2],macaddr[3],macaddr[4],macaddr[5]);
    header->wsa__MessageID = (char *)malloc(100);  
    memset(header->wsa__MessageID, 0, 100);
    strncpy(header->wsa__MessageID, _HwId, strlen(_HwId));    //wsa__MessageID存放的是uuid
 
    if(was_Action != NULL)
    {
        header->wsa__Action = (char*)malloc(1024);
        memset(header->wsa__Action, '\0', 1024);
        strncpy(header->wsa__Action, was_Action, 1024); //
    }
    if(was_To != NULL)
    {
        header->wsa__To = (char *)malloc(1024);
        memset(header->wsa__To, '\0', 1024);
        strncpy(header->wsa__To, was_To, 1024);//"urn:schemas-xmlsoap-org:ws:2005:04:discovery";
    }
    soap->header = header;
    return soap;
}

//释放函数
void ONVIF_soap_delete(struct soap *soap)
{
    soap_destroy(soap);                      // remove deserialized class instances (C++ only)
    soap_end(soap);                          // Clean up deserialized data (except class instances) and temporary data
    soap_free(soap);                         // Reset and deallocate the context created with soap_new or soap_copy
}

/* 鉴权 */
static int ONVIF_SetAuthInfo(struct soap *soap, const char *username, const char *password)
{
    int result = 0;
    if((NULL != username) || (NULL != password))
    {
        soap_wsse_add_UsernameTokenDigest(soap, NULL, username, password);
    }
    else
    {
        printf("un etAuth\n");
        result = -1;
    }
 
    return result;
}

void test_discover()
{
    /* 
       添加ens33网卡的路由信息，否则probe时会返回soap_send___wsdd__Probe -1 
       参考：https://blog.csdn.net/elvia7/article/details/69524972
    */
    system("route add -host 239.255.255.250 dev ens33");

    struct __wsdd__ProbeMatches resp;        // 消息应答
    struct wsdd__ScopesType scopes;          // 描述查找哪类的Web服务
    struct wsdd__ProbeType req;                  // 请求消息
    char soap_endpoint[64] = {0};

    struct SOAP_ENV__Header header;
    char* To = "urn:schemas-xmlsoap-org:ws:2005:04:discovery";
    char* action = "http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe";
    struct soap* pSoap = ONVIF_Initsoap(&header, To, action, 5);
    pSoap->recv_timeout = 2;

    //soap_default_wsdd__ScopesType(pSoap&)
    soap_default_wsdd__ScopesType(pSoap, &scopes);
    scopes.__item = NULL;//"onvif://www.onvif.org";
    soap_default_wsdd__ProbeType(pSoap, &req);
    req.Scopes = &scopes;
    char types[] = "dn:NetworkVideoTransmitter";
    req.Types = types;

    string localIp = "172.16.41.221";  /* 本机器IP */
    string mulip = "239.255.255.250";  /* 也可以替换成一个IPC的IP，这样子就会只探测一个设备 */
    sprintf(soap_endpoint, "soap.udp://%s:3702/", mulip.c_str());

    cout << " soap_endpoint " << soap_endpoint;
    struct in_addr if_req;
    if_req.s_addr = inet_addr(localIp.c_str());  // eternet
    pSoap->ipv4_multicast_if = (char*)soap_malloc(pSoap, sizeof(in_addr));
    memset(pSoap->ipv4_multicast_if, 0, sizeof(in_addr));
    memcpy(pSoap->ipv4_multicast_if, (char*)&if_req, sizeof(if_req));

    int num = 0;
    int ret = soap_send___wsdd__Probe(pSoap, soap_endpoint, NULL, &req);
    while(SOAP_OK == ret)
    {
        ret= soap_recv___wsdd__ProbeMatches(pSoap, &resp);
        if(ret == SOAP_OK && !pSoap->error)
        {
            string ipaddress,uuid;int port=0;
            string xAddress=string(resp.wsdd__ProbeMatches->ProbeMatch->XAddrs);
            cout << "xAddress is "<< xAddress << endl;

            std::vector<std::string> vecSegTag;
            split(xAddress,'/', vecSegTag);
            for(int i=0;i<vecSegTag.size();i++)
            {
                cout<<" spilt value is "<<vecSegTag[i] << endl;
            }
        }
        else
        {
            printf("soap_recv___wsdd__ProbeMatches soap error: %d, %s, %s\n", pSoap->error, *soap_faultcode(pSoap), *soap_faultstring(pSoap));
        }
        
    }
    
    if(ret != SOAP_OK)
    {
        cout << "soap_send___wsdd__Probe faile " << pSoap->error << endl;
        printf("soap_send___wsdd__Probe soap error: %d, %s, %s\n", pSoap->error, *soap_faultcode(pSoap), *soap_faultstring(pSoap));
    }
    
}

/* 获取Services */
void GetServices()
{
    int i = 0;
    int ret = 0;
    char secvre_addr[] = "http://172.16.41.64/onvif/device_service"; //设备搜索获取得到的服务地址
    struct SOAP_ENV__Header header;
    struct _tds__GetServices tds__GetServices;
    struct _tds__GetServicesResponse tds__GetServicesResponse;
 
    struct soap* soap = ONVIF_Initsoap(&header, NULL, NULL, 5);
    soap->recv_timeout = 2;

    tds__GetServices.IncludeCapability = 0;
 
    ONVIF_SetAuthInfo(soap,user_name,passwd);  //鉴权
    soap_call___tds__GetServices(soap,secvre_addr,NULL, &tds__GetServices, &tds__GetServicesResponse);
    if(soap->error)
    {
        ret = -1;
        printf("soap_call___tds__GetServices soap error: %d, %s, %s\n", soap->error, *soap_faultcode(soap), *soap_faultstring(soap));
        return ret;
    }
    else
    {
        if (tds__GetServicesResponse.Service[i].Namespace != NULL )
        {
            for(i=0; i<tds__GetServicesResponse.__sizeService; i++)
            {
                printf("namesapce : %s \n", tds__GetServicesResponse.Service[i].Namespace);
                if(strcmp(tds__GetServicesResponse.Service[i].Namespace, "http://www.onvif.org/ver20/media/wsdl") == 0)
                {
                    printf(" 265 media_addr[%d] %s\n", i, tds__GetServicesResponse.Service[i].XAddr);
                }
                if(strcmp(tds__GetServicesResponse.Service[i].Namespace, "http://www.onvif.org/ver10/media/wsdl") == 0)
                {
                    printf(" 264 media_addr->XAddr[%d] %s\n", i, tds__GetServicesResponse.Service[i].XAddr);
                   
                }
            }
 
        }
    }
 
    ONVIF_soap_delete(soap);
}

void GetProfileUsedMedia()
{
    char media_addr2[] = "http://172.16.41.64/onvif/Media"; //GetServices得到的地址
    struct SOAP_ENV__Header header;  
    struct soap* soap = ONVIF_Initsoap(&header, NULL, NULL, 5);

    struct _trt__GetProfiles getProfilesReq;                 //用于发送消息描述
    struct _trt__GetProfilesResponse getProfilesResponse;    //请求消息的回应
    
    ONVIF_SetAuthInfo(soap,user_name,passwd);  //鉴权
    soap_call___trt__GetProfiles(soap, media_addr2, NULL, &getProfilesReq, &getProfilesResponse); 
    if(soap->error)
    {
        //cout << "soap_call___trt__GetProfiles main soap error:"<< soap->error << *soap_faultcode(soap) <<
        //            *soap_faultstring(soap) << endl;
    }
    else
    {
        do
        {
        //LOG_DEBUG<<"GetProfiles soap error:"<<pSoap->error<<" , "<<*soap_faultcode(pSoap)<<" , "<<*soap_faultstring(pSoap);
            if(getProfilesResponse.Profiles == NULL){
                ret = -2;
                break;
            }
            //循环输出文件信息
            for(int i = 0;i < getProfilesResponse.__sizeProfiles;i++,getProfilesResponse.Profiles++)
            {

                if(strstr(getProfilesResponse.Profiles->Name,"test"))
                    continue;

                string tmp;

                if(getProfilesResponse.Profiles->VideoEncoderConfiguration!=NULL)
                {
                    cout <<"get video encoder configuretion "<< getProfilesResponse.Profiles->VideoEncoderConfiguration->Encoding << endl;
                    tmp="h264";
                }
                else
                {
                    tmp="h265";
                }

                //cout << "GET PROFILE ENCODE TYPE IS "<< tmp << endl;
                cout << "===== 264 profile name " << getProfilesResponse.Profiles->Name << " token " << getProfilesResponse.Profiles->token <<  endl;
            }

    }while(0);

    }

    ONVIF_soap_delete(soap);
}

void GetProfileUsedMedia2()
{
    char media_addr2[] = "http://172.16.41.64/onvif/Media2"; //GetServices得到的地址
    struct SOAP_ENV__Header header;  
    struct soap* soap = ONVIF_Initsoap(&header, NULL, NULL, 5);

    struct _tr2__GetProfiles tr2__GetProfiles;
    struct _tr2__GetProfilesResponse tr2__GetProfilesResponse;
    
    tr2__GetProfiles.__sizeType = 1;
    tr2__GetProfiles.Token = NULL;

    char* type[1] = {"VideoEncoder"};
    tr2__GetProfiles.Type = type;

    ONVIF_SetAuthInfo(soap,user_name,passwd);  //鉴权
    soap_call___tr2__GetProfiles(soap, media_addr2, NULL, &tr2__GetProfiles, &tr2__GetProfilesResponse); 
    if(soap->error)
    {
            cout << "soap_call___tr2__GetProfiles main soap error:"<< soap->error << *soap_faultcode(soap) <<
                        *soap_faultstring(soap) << endl;
    }
    else
    {
            if(tr2__GetProfilesResponse.Profiles == NULL)
            {
                ret = -2;
                
            }
            else
            {
                //循环输出文件信息
                for(int i = 0;i < tr2__GetProfilesResponse.__sizeProfiles;i++,tr2__GetProfilesResponse.Profiles++)
                {

                    if(strstr(tr2__GetProfilesResponse.Profiles->Name,"test"))
                        continue;

                    string video_type =  tr2__GetProfilesResponse.Profiles->Configurations->VideoEncoder->Encoding;
                    transform(video_type.begin(), video_type.end(), video_type.begin(), ::tolower);
                    cout << "===== 265 profile name " << tr2__GetProfilesResponse.Profiles->Name << " token " << tr2__GetProfilesResponse.Profiles->token 
                    <<  " video type " << video_type << endl;
                
                }
            }

    }

    ONVIF_soap_delete(soap);
}

void GetUrlUsedMedia2()
{
    char media_addr2[] = "http://172.16.41.64/onvif/Media2"; //GetServices得到的地址
    char taken[] = "Profile_1";   //get_profiles获取
    struct SOAP_ENV__Header header;

    struct soap* soap = ONVIF_Initsoap(&header, NULL, NULL, 5);
    struct _tr2__GetStreamUri tr2__GetStreamUri;
    struct _tr2__GetStreamUriResponse tr2__GetStreamUriResponse;
    tr2__GetStreamUri.Protocol = (char *)soap_malloc(soap, 128*sizeof(char));
    if (NULL == tr2__GetStreamUri.Protocol)
    {
        printf("soap_malloc is error\n");
        ret = -1;
    }

    tr2__GetStreamUri.ProfileToken = (char *)soap_malloc(soap, 128*sizeof(char ));
    if (NULL == tr2__GetStreamUri.ProfileToken)
    {
        printf("soap_malloc is error\n");
        ret = -1;
    }

    strcpy(tr2__GetStreamUri.Protocol, "tcp");
    strcpy(tr2__GetStreamUri.ProfileToken, taken);
    ONVIF_SetAuthInfo(soap,user_name,passwd);  //鉴权
    soap_call___tr2__GetStreamUri(soap, media_addr2, NULL, &tr2__GetStreamUri, &tr2__GetStreamUriResponse);
    if(soap->error)
    {
            cout << "soap_call___tr2__GetStreamUri main soap error:"<< soap->error << *soap_faultcode(soap) <<
                        *soap_faultstring(soap) << endl;
    }
    else
    {
            cout << "media2 MediaUri->Uri=" <<  tr2__GetStreamUriResponse.Uri << endl;

    }

    ONVIF_soap_delete(soap);
}

void GetUrlUsedMedia()
{
    char media_addr2[] = "http://172.16.41.64/onvif/Media"; //GetServices得到的地址
    char taken[] = "Profile_1";   //get_profiles获取
    struct SOAP_ENV__Header header;

    struct soap* soap = ONVIF_Initsoap(&header, NULL, NULL, 5);

    struct _trt__GetStreamUri trt__GetStreamUri;
    struct _trt__GetStreamUriResponse response;
    trt__GetStreamUri.StreamSetup = (struct tt__StreamSetup*)soap_malloc(soap, sizeof(struct tt__StreamSetup));
    if (NULL == trt__GetStreamUri.StreamSetup)
    {
        printf("soap_malloc is error\n");
        ret = -1;
    }

    trt__GetStreamUri.StreamSetup->Stream = tt__StreamType__RTP_Unicast;
    trt__GetStreamUri.StreamSetup->Transport = (struct tt__Transport *)soap_malloc(soap, sizeof(struct tt__Transport));
    if (NULL == trt__GetStreamUri.StreamSetup->Transport)
    {
        printf("soap_malloc is error\n");
        ret = -1;
    }

    trt__GetStreamUri.StreamSetup->Transport->Protocol = 1;
    trt__GetStreamUri.StreamSetup->Transport->Tunnel = 0;
    trt__GetStreamUri.StreamSetup->__size = 1;
    trt__GetStreamUri.StreamSetup->__any = NULL;
    trt__GetStreamUri.StreamSetup->__anyAttribute = NULL;

    trt__GetStreamUri.ProfileToken = (char *)soap_malloc(soap, 128*sizeof(char ));//
    if (NULL == trt__GetStreamUri.ProfileToken){
        printf("soap_malloc is error\n");
            ret = -1;
    }
    strcpy(trt__GetStreamUri.ProfileToken, taken);

    ONVIF_SetAuthInfo(soap,user_name,passwd);  //鉴权
    soap_call___trt__GetStreamUri(soap, media_addr2, NULL, &trt__GetStreamUri, &response);
    if(soap->error)
    {
            cout << "GetStreamUri main soap error:"<< soap->error << *soap_faultcode(soap) <<
                        *soap_faultstring(soap) << endl;
    }
    else
    {
            cout << " h264 main MediaUri->Uri=" <<  response.MediaUri->Uri << endl;

    }

    ONVIF_soap_delete(soap);
}


string ptzurl;  /* ptz控制url */
string mediaUrl; /* media service url */
string imagingUrl;
void GetCapability(char* ip, int port)
{
    if(!ip) 
        return -1;
    
    char soap_endpoint[64] = {0};

    struct SOAP_ENV__Header header;
    struct _tds__GetCapabilities capa_req;
    struct _tds__GetCapabilitiesResponse capa_resp;
    //struct SOAP_ENV__Header header;
    struct soap* pSoap = ONVIF_Initsoap(&header, NULL, NULL, 5);
    pSoap->recv_timeout = 2;

    ONVIF_SetAuthInfo(pSoap, user_name, passwd);  //鉴权

    if(0 == port)
    {
        sprintf(soap_endpoint,"http://%s/onvif/device_service",ip);
    }
    else
    {
        sprintf(soap_endpoint,"http://%s:%d/onvif/device_service", ip, port);
    }

    capa_req.Category=(enum tt__CapabilityCategory *)soap_malloc(pSoap, sizeof(int));
    capa_req.__sizeCategory = 1;
    *capa_req.Category=(enum tt__CapabilityCategory)0;
    capa_resp.Capabilities=(struct tt__Capabilities*)soap_malloc(pSoap,sizeof(struct tt__Capabilities)) ;

    int ret=soap_call___tds__GetCapabilities(pSoap,soap_endpoint, NULL, &capa_req, &capa_resp);
    if(pSoap->error)
    {
        //
        cout <<"getCapabilities 2 error:"<< pSoap->error;
        ret = pSoap->error;
    }
    else
    {
        if(capa_resp.Capabilities->Media)
        {
            cout << "get the capabilities is "<< capa_resp.Capabilities->Media->XAddr << endl;
            mediaUrl=string(capa_resp.Capabilities->Media->XAddr);
        }

        if(capa_resp.Capabilities->PTZ)
        {
            cout << "get the capabilities is "<< capa_resp.Capabilities->PTZ->XAddr << endl;
            ptzurl=string(capa_resp.Capabilities->PTZ->XAddr);
        }

        if(capa_resp.Capabilities->Imaging)
        {
            cout << "get the capabilities is "<< capa_resp.Capabilities->Imaging->XAddr << endl;
            imagingUrl = string(capa_resp.Capabilities->Imaging->XAddr);
        }

        ret = SOAP_OK;
    }

    ONVIF_soap_delete(pSoap);

}

typedef enum
{
    PTZ_CMD_LEFT,
    PTZ_CMD_RIGHT,
    PTZ_CMD_UP,
    PTZ_CMD_DOWN,
    PTZ_CMD_ZOOM_NEAR,
    PTZ_CMD_ZOOM_FAR,
}PTZCMD;

void ptzContinueMove(int cmd , int speed)
{
    if(ptzurl.empty())
    {
        printf("ptzContinueMove ptzurl is empty \n");
        return;
    }
    
    struct SOAP_ENV__Header header;
    struct soap* soap = ONVIF_Initsoap(&header, NULL, NULL, 5);
    const int RECV_MAX_TIME = 2;
    soap->recv_timeout = RECV_MAX_TIME;
    soap->send_timeout = RECV_MAX_TIME;
    soap->connect_timeout = RECV_MAX_TIME;

    int speed_x=0;
    int speed_y=0;
    int speed_z=0;

    struct _tptz__ContinuousMove                continuousMove;
    struct _tptz__ContinuousMoveResponse        continuousMoveresponse;

    LONG64 timeout = 2;
    continuousMove.Timeout = &timeout;
    char ProfileToken[32] = {0};
    strncpy(ProfileToken, "MainProfileToken", sizeof(ProfileToken));
    continuousMove.ProfileToken = ProfileToken;

    struct tt__PTZSpeed* velocity = (struct tt__PTZSpeed*)soap_malloc(soap, sizeof(struct tt__PTZSpeed));
    continuousMove.Velocity = velocity;

    struct tt__Vector2D* panTilt = (struct tt__Vector2D*)soap_malloc(soap, sizeof(struct tt__Vector2D));
    continuousMove.Velocity->PanTilt = panTilt;

    continuousMove.Velocity->PanTilt->space = NULL;

    if(cmd >= PTZ_CMD_ZOOM_NEAR)
    {
        struct tt__Vector1D* zoom = (struct tt__Vector1D*)soap_malloc(soap, sizeof(struct tt__Vector1D));
        continuousMove.Velocity->Zoom = zoom;
        continuousMove.Velocity->PanTilt->x = (float)speed_x / 100;
        continuousMove.Velocity->PanTilt->y = (float) speed_y / 100;
        continuousMove.Velocity->Zoom->x = (float)speed_z / 100;

        continuousMove.Velocity->Zoom->space = NULL;
    }

    switch (cmd)
    {
        case PTZ_CMD_LEFT:
            continuousMove.Velocity->PanTilt->x = -((float)speed / 100);
            break;

        case PTZ_CMD_RIGHT:
            continuousMove.Velocity->PanTilt->x = (float)speed / 100;
            //continuousMove.Velocity->PanTilt->y = 0;
            break;
        case PTZ_CMD_UP:
            //continuousMove.Velocity->PanTilt->x = 0;
            continuousMove.Velocity->PanTilt->y = (float)speed / 100;
            break;

        case PTZ_CMD_DOWN:
            //continuousMove.Velocity->PanTilt->x = 0;
            continuousMove.Velocity->PanTilt->y = -((float)speed / 100);
            break;
        
            // 缩小
        case PTZ_CMD_ZOOM_NEAR:
            continuousMove.Velocity->Zoom->x = -((float)speed / 100);
            break;

        // 放大
        case PTZ_CMD_ZOOM_FAR:
            continuousMove.Velocity->Zoom->x = (float)speed / 100;
            break;

        default:
            break;
    }

    char soap_endpoint[64] = {0};
    sprintf(soap_endpoint, "%s", ptzurl.c_str());
    printf("ptz_url: %s \n", soap_endpoint);

    ONVIF_SetAuthInfo(soap, user_name, passwd);  //鉴权
    if(soap_call___tptz__ContinuousMove(soap, soap_endpoint, NULL, &continuousMove, &continuousMoveresponse) == SOAP_OK)
    {
        printf("======SetPTZcontinuous_move is success!!!=======\n");
    }
    else
    {
        printf("======SetPTZcontinuous_move is faile!!!=======\n");
        printf("soap_call___tptz__ContinuousMove soap error: %d, %s, %s\n", soap->error, *soap_faultcode(soap), *soap_faultstring(soap));
    }

    ONVIF_soap_delete(soap);

}

void StopPTZ()
{
    if(ptzurl.empty())
     {
        printf("StopPTZ ptzurl is empty \n");
        return;
    }
    
    struct SOAP_ENV__Header header;
    struct soap* soap = ONVIF_Initsoap(&header, NULL, NULL, 5);
    const int RECV_MAX_TIME = 10;
    soap->recv_timeout = RECV_MAX_TIME;
    soap->send_timeout = RECV_MAX_TIME;
    soap->connect_timeout = RECV_MAX_TIME;

    //ONVIF_SetAuthInfo(soap, user_name, passwd);  //鉴权

    //char ProfileToken[32] = {0};
    //strncpy(ProfileToken, sProfileToken.c_str(), sizeof(ProfileToken));

    _tptz__Stop* stop = (struct _tptz__Stop*)soap_malloc(soap, sizeof(struct _tptz__Stop));
    _tptz__StopResponse* stopResponse = (struct _tptz__StopResponse*)soap_malloc(soap, sizeof(struct _tptz__StopResponse));

    char ProfileToken[32] = {0};
    strncpy(ProfileToken, "MainProfileToken", sizeof(ProfileToken));
    stop->ProfileToken = ProfileToken;

    stop->PanTilt = (xsd__boolean*)soap_malloc(soap, sizeof(xsd__boolean));
    *(stop->PanTilt) = xsd__boolean__true_;
    stop->Zoom = (xsd__boolean*)soap_malloc(soap, sizeof(xsd__boolean));
    *(stop->Zoom) = xsd__boolean__true_;

    char soap_endpoint[64] = {0};
    sprintf(soap_endpoint, "%s", ptzurl.c_str());

    ONVIF_SetAuthInfo(soap, user_name, passwd);  //鉴权
    if (SOAP_OK == soap_call___tptz__Stop(soap, soap_endpoint, NULL, stop, stopResponse))
    {
       
        printf("======ptzProxy->Stop is success!!!=======\n");
    }
    else
    {
        printf("======ptzProxy->Stop is faile!!!=======\n");
        printf("soap_call___tptz__Stop soap error: %d, %s, %s\n", soap->error, *soap_faultcode(soap), *soap_faultstring(soap));
    }

    ONVIF_soap_delete(soap);
}

int main(int argc,char *argv[])
{
    char* ip = "172.16.41.64";
    int port = 0;

    test_discover();
    GetServices();
    GetCapability(ip, port);

    //GetProfileUsedMedia2();
    GetProfileUsedMedia();

    int cmd = PTZ_CMD_UP;
    int speed = 20;
    ptzContinueMove(cmd, speed);

    usleep(150*1000);
    StopPTZ();

    return 0;
}