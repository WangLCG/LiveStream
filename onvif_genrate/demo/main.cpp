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

using namespace std;

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

int main(int argc,char *argv[])
{
    int i = 0;
    int ret = 0;
    char secvre_addr[] = "http://172.16.41.190/onvif/device_service"; //设备搜索获取得到的服务地址
    struct SOAP_ENV__Header header;
    struct _tds__GetServices tds__GetServices;
    struct _tds__GetServicesResponse tds__GetServicesResponse;
 
    struct soap* soap = ONVIF_Initsoap(&header, NULL, NULL, 5);
    soap->recv_timeout = 2;

    //tds__GetServices->IncludeCapability = (enum xsd__boolean *)soap_malloc(soap, sizeof(int));
    //*(tds__GetServices->IncludeCapability) = (enum xsd__boolean)0;
    
    tds__GetServices.IncludeCapability = 0;
 
    ONVIF_SetAuthInfo(soap,"admin","itc123456");  //鉴权
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

    /* 264 GetProfiles */
    if(1)
    {
        char media_addr2[] = "http://172.16.41.190/onvif/Media"; //GetServices得到的地址
        struct SOAP_ENV__Header header;  
        struct soap* soap = ONVIF_Initsoap(&header, NULL, NULL, 5);
    
        struct _trt__GetProfiles getProfilesReq;                 //用于发送消息描述
        struct _trt__GetProfilesResponse getProfilesResponse;    //请求消息的回应
        
        ONVIF_SetAuthInfo(soap,"admin","itc123456");  //鉴权
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

    /* 265 getprofiles */
    {
        char media_addr2[] = "http://172.16.41.190/onvif/Media2"; //GetServices得到的地址
        struct SOAP_ENV__Header header;  
        struct soap* soap = ONVIF_Initsoap(&header, NULL, NULL, 5);
    
        struct _tr2__GetProfiles tr2__GetProfiles;
        struct _tr2__GetProfilesResponse tr2__GetProfilesResponse;
        
        tr2__GetProfiles.__sizeType = 1;
        tr2__GetProfiles.Token = NULL;

        char* type[1] = {"VideoEncoder"};
        tr2__GetProfiles.Type = type;

        ONVIF_SetAuthInfo(soap,"admin","itc123456");  //鉴权
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

    {
        char media_addr2[] = "http://172.16.41.190/onvif/Media2"; //GetServices得到的地址
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
        ONVIF_SetAuthInfo(soap,"admin","itc123456");  //鉴权
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

    {
        char media_addr2[] = "http://172.16.41.190/onvif/Media"; //GetServices得到的地址
        char taken[] = "Profile_1";   //get_profiles获取
        struct SOAP_ENV__Header header;

        struct soap* soap = ONVIF_Initsoap(&header, NULL, NULL, 5);

        struct _trt__GetStreamUri trt__GetStreamUri;
        struct _trt__GetStreamUriResponse response;
        trt__GetStreamUri.StreamSetup = (struct tt__StreamSetup*)soap_malloc(soap, sizeof(struct tt__StreamSetup));
        if (NULL == trt__GetStreamUri.StreamSetup){
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

        ONVIF_SetAuthInfo(soap,"admin","itc123456");  //鉴权
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

    return ret;
}