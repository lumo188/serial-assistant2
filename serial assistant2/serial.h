#pragma once

#include<stdio.h>
#include<string.h>
#include<windows.h>
#include<conio.h>
#include<stdlib.h>
#include<ctype.h>

#define FRAME_HEADER1 0xAA	//帧头1
#define FRAME_HEADER2 0x55	//帧头2
#define ACK 0x06
#define NAK 0x15
#define END_FRAME 0xFFFF	//16位结束序号
#define RETRIES 100000000			//重试次数
#define ACK_TIMEOUT 3000			//3秒
#define BLOCK_SIZE 1024		//每块数据大小





typedef struct {
	char portname[10];		//串口名
	int baurdrate;			//波特率
	int databits;			//数据位
	int stopbits;			//停止位
	char parity;			//校验位
	int showHex;            // 是否以HEX格式显示
	int sendHex;            // 是否以HEX格式发送

}serial;

//函数声明
void Checkconfig();		//默认串口配置
int usable();			//检测可用串口
void serialmenu();		//串口菜单界面
void closeserial();		//关闭串口
void ConfigureSerial();	//配置串口参数
void back();			//返回菜单
void openserial();		//打开串口
void Checkconfig();		//查看当前串口配置
void senddata();		//发送数据

unsigned short crc16(unsigned char* data, int len);
int waitack();
void sendimage();
void receiveimage();

//全局变量
extern serial config;
extern HANDLE hserial; // 串口句柄
extern int running;


