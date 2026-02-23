#define _CRT_SECURE_NO_WARNINGS  // 禁用安全警告
#define _CRT_SECURE_NO_WARNINGS_GLOBALS  // 禁用全局安全警告

#include"serial.h";

//返回菜单
void back() {
	printf("按任意键返回菜单\n");
	_getch();
}



//串口菜单界面
void serialmenu() {
	printf("=================串口助手菜单=================\n");
	printf("0. 查看当前串口配置\n");
	printf("1、检测可用串口\n");
	printf("2、配置串口参数\n");
	printf("3、打开串口\n");
	printf("4、关闭串口\n");
	printf("5、发送数据\n");
	printf("6、设置显示格式（当前：%s）\n", config.showHex ? "Hex" : "ASCALL");//config.showHex为1（真）显示HEX，为0（假）显示ASCLL
	printf("7、设置发送格式（当前：%s）\n", config.sendHex ? "Hex" : "ASCALL");
	printf("8、发送图片文件\n");
	printf("9、接收图片文件\n");
	printf("a、退出程序\n");
	printf("===============================================\n");
	printf("请选择操作：\n");
}


//查看当前串口配置
void Checkconfig() {
	printf("串口号:%s\n", config.portname);
	printf("波特率:%d\n", config.baurdrate);
	printf("数据位:%d\n", config.databits);
	printf("停止位:%d\n", config.stopbits);
	printf("校验位:%c\n", config.parity);

}


//检测可用串口
int usable() {
	printf("正在检测可用串口......\n");
	char portname[20];
	int count = 0;
	for (int i = 1; i < 20; i++) {
		sprintf(portname, "\\\\.\\COM%d", i);
		HANDLE open = CreateFileA(
			portname,
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		if (open != INVALID_HANDLE_VALUE) {
			printf("COM%d\n", i);
			count++;
			CloseHandle(open);
		}
	}
	if (count == 0) {
		printf("未检测出可用串口\n");
	}
	return count;
}
