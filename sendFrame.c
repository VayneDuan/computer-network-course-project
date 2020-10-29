#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MIN 46              //数据部分最小长度
#define MAX 1500            //数据部分最大长度
typedef unsigned char byte; //定义帧的单位
typedef int BOOL;
#define TRUE 1
#define FALSE 0
byte DA[6] = {0x00, 0xE0, 0x4C, 0x70, 0x9E, 0x22}; //目的地址
byte SA[6] = {0x00, 0xE0, 0x4C, 0x70, 0x9E, 0x21}; //源地址
byte frame[1518];                                  //帧

byte data[] = "begin hello world hello world hello world hello world hello world hello world hello world hello world end";
/*
byte data[] = {
    //用于测试的数据部分(payload) length = 80
    0x00,
    0xE0,
    0x4C,
    0x70,
    0x9E,
    0x22,
    0x00,
    0xE0,
    0x4C,
    0x70,
    0x9E,
    0x22,
    0x00,
    0xE0,
    0x4C,
    0x70,
    0x70,
    0x9E,
    0x22,
    0x00,
    0xE0,
    0x4C,
    0x70,
    0x70,
    0x9E,
    0x22,
    0x00,
    0xE0,
    0x4C,
    0x70,
    0x70,
    0x9E,
    0x22,
    0x00,
    0xE0,
    0x4C,
    0x70,
    0x70,
    0x9E,
    0x22,
    0x00,
    0xE0,
    0x4C,
    0x70,
    0x70,
    0x9E,
    0x22,
    0x00,
    0xE0,
    0x4C,
    0x70,
    0x70,
    0x9E,
    0x22,
    0x00,
    0xE0,
    0x4C,
    0x70,
    0x70,
    0x9E,
    0x22,
    0x00,
    0xE0,
    0x4C,
    0x70,
    0x70,
    0x9E,
    0x22,
    0x00,
    0xE0,
    0x4C,
    0x70,
    0x70,
    0x9E,
    0x22,
    0x00,
    0xE0,
    0x4C,
    0x70,
    0x9E,
};
*/
byte type[] = {0x08, 0x00}; //Protocol Type

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

unsigned short generateFrame(byte *da, byte *sa, byte *type, byte *payload, int payloadLen, byte *frame) //生成帧
{
    memcpy(&frame[0], da, 6);                //目的地址放入帧
    memcpy(&frame[6], sa, 6);                //源地址放入帧
    memcpy(&frame[12], type, 2);             //Protocol Type放入帧
    memcpy(&frame[14], payload, payloadLen); //数据部分放入帧
    int tail = payloadLen + 14;              //此时数据部分的末端(也是此时帧的末端,帧校验序列之前)
    unsigned int FCS;                        //帧校验序列
    FCS = crc32(frame, tail);                //生成帧校验序列
    memcpy(&frame[tail], &FCS, sizeof(FCS)); //帧校验序列放入帧
    return payloadLen + 18;                  //帧的实际长度
}

void sendFrame(byte *frame, unsigned short len, FILE *fp) //发送帧
{
    fwrite(&len, sizeof(len), 1, fp);     //写入帧的总长度
    fwrite(frame, sizeof(byte), len, fp); //写入帧
    printf("Sended completely.\n");
}

int main()
{
    FILE *fp = fopen("demo.frm", "w");
    int data_len = sizeof(data); //数据部分的长度
    if (data_len < MIN)          //数据部分过小
        printf("Error! The length of the payload must ≥ 46\n");

    unsigned short frameLen; //帧的实际长度

    //帧1
    //完整且正确
    frameLen = generateFrame(DA, SA, type, data, data_len, frame);
    sendFrame(frame, frameLen, fp);

    //帧2
    //某一位出错
    frameLen = generateFrame(DA, SA, type, data, data_len, frame);
    int error_pos = 20;
    frame[error_pos] ^= 1;
    sendFrame(frame, frameLen, fp);

    //帧3
    //目的地址出错
    frameLen = generateFrame(SA, SA, type, data, data_len, frame);
    sendFrame(frame, frameLen, fp);

    //帧4
    //帧校验序列(FCS)传输错误
    frameLen = generateFrame(DA, SA, type, data, data_len, frame);
    error_pos = frameLen - 3;
    frame[error_pos] ^= 1;
    sendFrame(frame, frameLen, fp);

    fclose(fp);
    return 0;
}
