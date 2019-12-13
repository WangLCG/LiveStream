#include <stdio.h>
#include <string.h>
#include <string>

#include "faac.h"
#include "PCMToAAC.h"
#include <sys/time.h>

using namespace std;

void testPCMtoAAC()
{
    PCM_TO_AAC pcm2aacHandler(8000, 2, 16);  /* 8Khz 2ͨ����16bit����λ�� */

    FILE* rfd = fopen("../Audio/pcm8k.pcm", "rb+");
    FILE* wfd = fopen("../Audio/convert_out.aac", "wb+");
    if (rfd && wfd)
    {
        char pcm_buf[4096];  /* �����bffer���ݴ�С�ڴ����б������ 2048 *������λ��16��/8  2048��faacEncOpen���� */
        char aac_buf[4096];
        printf("Begin to cnvert .......\n");
        struct timeval adts_start , adts_end;
        gettimeofday(&adts_start , NULL);

        while (!feof(rfd))
        {
            int ret = fread(pcm_buf, 1, 4096, rfd);
            //printf("ret = %d \n", ret);
            int aac_size = pcm2aacHandler.ConvertProcess(pcm_buf, ret, (unsigned char*)aac_buf);
            //printf("ret = %d aac_size = %d\n", ret, aac_size);
            fwrite(aac_buf, 1, aac_size, wfd);
        }
        gettimeofday(&adts_end , NULL);
        long adts_cost_ms = (adts_end.tv_sec - adts_start.tv_sec) * 1000 * 1000 + (adts_end.tv_usec - adts_start.tv_usec) ;
        printf("----- cost time [%ld] us ------\n", adts_cost_ms);
        fclose(rfd);
        fclose(wfd);
        printf("PCM to AAC Conver Success\n");
    }
}

int main()
{
    testPCMtoAAC();
    printf("##### Enter to exit #####\n");
    getchar();
    return 0;
}