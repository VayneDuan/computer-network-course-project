#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MIN 64              //最小帧长
#define MAX 1518            //最大帧长
typedef unsigned char byte; //定义帧的单位
typedef int BOOL;
#define TRUE 1
#define FALSE 0

byte SA[6] = {0x00, 0xE0, 0x4C, 0x70, 0x9E, 0x22}; //接收方的MAC地址

unsigned int crc32_for_byte(unsigned int r) //用于生成CRC32码表
{
    for (int j = 0; j < 8; ++j)
        r = (r & 1 ? 0 : (unsigned int)0xEDB88320L) ^ r >> 1;
    return r ^ (unsigned int)0xFF000000L;
}

unsigned int crc32(const void *data, int n_bytes) //CRC32校验
{
    unsigned int crc = 0xFFFFFFFF;
    static unsigned int table[0x100];
    if (!*table)
        for (int i = 0; i < 0x100; ++i)
            table[i] = crc32_for_byte(i);
    for (int i = 0; i < n_bytes; ++i)
        crc = table[(byte)crc ^ ((byte *)data)[i]] ^ crc >> 8;
    return crc;
}

byte frame[1518]; //用于存储读取到的帧

BOOL macCheck(byte x[6], byte y[6]) //检查帧的目的地址是否是本机MAC地址
{
    BOOL flag = TRUE;
    for (int i = 0; i < 6; i++)
        if (x[i] != y[i]) //MAC地址不匹配
            flag = FALSE;
    return flag;
}

void macDisplay(byte x[6]) //打印本机MAC地址
{
    for (int i = 0; i < 5; i++)
        printf("%02x:", x[i]);
    printf("%02x", x[5]);
}

//打印帧
void frameDisplay(unsigned short frameLen, byte DA[6], byte SA_recv[6], byte TYPE[2], byte payload[], int payloadLen, byte FCS_frame[4])
{
    printf("\tSize:\t%u\n", frameLen);
    printf("\tDA:\t");
    macDisplay(DA);
    printf("\n\tSA:\t");
    macDisplay(SA_recv);
    printf("\n\tpayload:");
    for (int i = 0; i < payloadLen; i++)
        printf("%c", payload[i]);
    printf("\n\tType:\t%02x%02x", TYPE[0], TYPE[1]);
    printf("\n\tFCS:\t%02x%02x%02x%02x", FCS_frame[0], FCS_frame[1], FCS_frame[2], FCS_frame[3]);
    printf("\n");
}

int main()
{
    FILE *fp = fopen("demo.frm", "r");
    unsigned short frameLen;
    int num = 0;

    while (fread(&frameLen, sizeof(frameLen), 1, fp)) //读取帧的长度
    {
        if (!frameLen)
            break;
        num++;
        fread(frame, sizeof(byte), frameLen, fp); //读取帧
        //检查帧长度
        if (frameLen < MIN || frameLen > MAX)
        {
            printf("Frame %d :\nsize %u error.\n", num, frameLen);
            printf("------------------------------------------\n");
            continue;
        }
        //CRC校验
        unsigned int FCS_int;
        FCS_int = crc32(frame, frameLen - 4);
        byte FCS_cp[4];    //接收方生成的帧校验序列
        byte FCS_frame[4]; //接受到的帧的帧校验序列
        memcpy(FCS_cp, &FCS_int, sizeof(FCS_int));
        memcpy(FCS_frame, &frame[frameLen - 4], 4);
        if (
            FCS_cp[3] != FCS_frame[3] ||
            FCS_cp[2] != FCS_frame[2] ||
            FCS_cp[1] != FCS_frame[1] ||
            FCS_cp[0] != FCS_frame[0])
        {
            printf("Frame %d :\nCRC unmatched.\n", num);
            printf("------------------------------------------\n");
            continue;
        }
        //读取目的地址
        byte DA[6];
        memcpy(&DA, &frame, 6);
        //检查帧的目的地址与本机MAC地址是否匹配
        if (!macCheck(DA, SA))
        {
            printf("Frame %d :\naddress unmatched :\n", num);
            printf("\tDA of the frame received:\t");
            macDisplay(DA);
            printf("\n\tMAC of the machine:\t\t");
            macDisplay(SA);
            printf("\n");
            printf("------------------------------------------\n");
            continue;
        }
        //解析帧
        byte SA_recv[6];
        byte TYPE[2];
        int payloadLen = frameLen - 18;
        byte payload[payloadLen];
        memcpy(&SA_recv, &frame[6], 6);
        memcpy(&TYPE, &frame[12], 2);
        memcpy(&payload, &frame[14], frameLen - 18);

        //打印帧
        printf("Frame %d :\nsize %d received successfully.\n", num, frameLen);
        frameDisplay(frameLen, DA, SA_recv, TYPE, payload, payloadLen, FCS_frame);
        printf("------------------------------------------\n");
    }
    fclose(fp);

    return 0;
}
