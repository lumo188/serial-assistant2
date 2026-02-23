#define _CRT_SECURE_NO_WARNINGS  // 禁用安全警告
#define _CRT_SECURE_NO_WARNINGS_GLOBALS  // 禁用全局安全警告

#include"serial.h";


//配置串口参数
void ConfigureSerial() {
	if (hserial != INVALID_HANDLE_VALUE) {
		printf("串口已打开，请先关闭串口再配置\n");
		return;
	}
	int ports = usable();
	if (ports != 0) {
		printf("请输入串口号:");
	}
	char input[20];
	fgets(input, sizeof(input), stdin);
	input[strcspn(input, "\n")] = '\0';
	strcpy(config.portname, input);

	char serial[20];
	sprintf(serial, "\\\\.\\%s", config.portname);
	// 验证串口号
	hserial = CreateFileA(
		serial,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hserial == INVALID_HANDLE_VALUE) {
		printf("请输入有效串口\n");
		return;
	}
	else {
		CloseHandle(hserial);
		hserial = INVALID_HANDLE_VALUE;
	}

	//配置波特率
	printf("波特率(常用波特率：9600, 19200, 38400, 57600, 115200)：");
	fgets(input, sizeof(input), stdin);
	input[strcspn(input, "\n")] = '\0';
	config.baurdrate = atoi(input);

	if (config.baurdrate != 9600 &&
		config.baurdrate != 19200 &&
		config.baurdrate != 38400 &&
		config.baurdrate != 57600 &&
		config.baurdrate != 115200) {
		printf("请输入有效的波特率\n");
		return;
	}
	//配置数据位
	printf("数据位（5，6，7，8）：");
	fgets(input, sizeof(input), stdin);//读取键盘输入，赋值给input
	input[strcspn(input, "\n")] = '\0';//去掉换行符。strcspn找第二个字符集里的字符与第一个字符集的第一个不同字符的位置
	config.databits = atoi(input);//atoi，将由键盘输入的字符串转换为数字

	if (config.databits < 5 || config.databits>8) {
		printf("请输入有效的数据位\n");
		return;
	}
	//配置停止位
	printf("停止位（1，2）：");
	fgets(input, sizeof(input), stdin);
	input[strcspn(input, "\n")] = '\0';
	config.stopbits = atoi(input);

	if (config.stopbits != 1 &&
		config.stopbits != 2) {
		printf("请输入有效的停止位\n");
		return;
	}

	//配置校验位
	printf("校验位（N(无), O(奇), E(偶)）：");
	fgets(input, sizeof(input), stdin);
	input[strcspn(input, "\n")] = '\0';
	config.parity = toupper(input[0]);//小写字母转换为大写字母

	if (config.parity != 'N' &&
		config.parity != 'O' &&
		config.parity != 'E') {
		printf("请输入有效的校验位\n");
		return;
	}
}

//关闭串口
void closeserial()
{
	if (hserial == INVALID_HANDLE_VALUE)
	{
		printf("串口未打开\n");
	}
	else
	{
		CloseHandle(hserial);
		hserial = INVALID_HANDLE_VALUE;
		printf("串口已关闭\n");
	}

}

//打开串口
void openserial() {
	//判断串口是否打开
	if (hserial != INVALID_HANDLE_VALUE) {
		closeserial();
	}

	//打开串口
	char fullportname[20];
	sprintf(fullportname, "\\\\.\\%s", config.portname);
	hserial = CreateFileA(
		fullportname,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);


	if (hserial == INVALID_HANDLE_VALUE) {
		printf("串口打开失败\n");
		return;
	}
	//配置结构体大小
	DCB SerialPortParameters = { 0 };
	SerialPortParameters.DCBlength = sizeof(SerialPortParameters);

	if (!GetCommState(hserial, &SerialPortParameters)) {
		CloseHandle(hserial);
		hserial = INVALID_HANDLE_VALUE;
		printf("无法获取串口状态\n");
		return;
	}
	//设置波特率
	SerialPortParameters.BaudRate = config.baurdrate;
	//设置数据位
	SerialPortParameters.ByteSize = config.databits;
	//设置停止位
	switch (config.stopbits) {
	case 1:
		SerialPortParameters.StopBits = ONESTOPBIT;
		break;
	case 2:
		SerialPortParameters.StopBits = TWOSTOPBITS;
		break;
	}
	//设置校验位
	switch (config.parity) {
	case 'N':
		SerialPortParameters.Parity = NOPARITY;
		break;
	case 'O':
		SerialPortParameters.Parity = ODDPARITY;
		break;
	case'E':
		SerialPortParameters.Parity = EVENPARITY;
		break;
	}

	if (!SetCommState(hserial, &SerialPortParameters)) {
		CloseHandle(hserial);
		hserial = INVALID_HANDLE_VALUE;
		printf("无法设置串口参数\n");
		return;
	}
	COMMTIMEOUTS timeout = { 0 };

	timeout.ReadIntervalTimeout = 50;
	timeout.ReadTotalTimeoutConstant = 50;
	timeout.ReadTotalTimeoutMultiplier = 50;
	timeout.WriteTotalTimeoutConstant = 50;
	timeout.WriteTotalTimeoutMultiplier = 50;

	if (!SetCommTimeouts(hserial, &timeout)) {
		CloseHandle(hserial);
		hserial = INVALID_HANDLE_VALUE;
		printf("无法设置超时\n");
		return;
	}

	PurgeComm(hserial, PURGE_RXCLEAR | PURGE_TXCLEAR); // 清空缓冲区
	printf("%s打开成功\n", config.portname);

}



//发送数据
void senddata() {
	if (hserial == INVALID_HANDLE_VALUE) {
		printf("串口未打开\n");
		back();
		return;
	}
	while (1) {
		printf("请输入要发送的数据（按Esc返回菜单）：");
		//获取字符输入
		char input[256];
		int count = 0;
		while (1) {
			char ch = _getch();
			//27=esc
			if (ch == 27) {
				printf("\n");
				return;
			}
			//13=回车
			else if (ch == 13) {
				input[count] = '\0';
				printf("\n");
				break;
			}
			//8=退格
			else if (ch == 8) {

				if (count > 0) {
					count--;
					printf("\b \b");// \b 退格字符，添加空格代替退格符号后的字符，然后再退格
				}
			}
			else if (count < sizeof(input) - 1) {
				input[count] = ch;
				count++;
				printf("%c", ch);
			}



			if (count == 0) {
				continue;
			}
		}



		unsigned char send[512];
		DWORD sendbit = 0;
		DWORD writebits;
		if (config.sendHex) {
			char* token = strtok(input, " ");//00 01 02 03,将01前的空格替换为‘\0’，返回00的地址
			while (token != NULL && sendbit < sizeof(send)) {
				unsigned int data;
				sscanf(token, "%02X", &data);//将token解释为16进制数给到data
				send[sendbit] = (unsigned char)data;
				sendbit++;
				token = strtok(NULL, " ");//strtok会保存上次分割位置，所以00 01 02 03,将02前的空格替换为‘\0’，返回01的地址，循环直到返回NULL
			}
		}
		//ASCALL发送
		else {
			strcpy(send, input);
			sendbit = strlen(input);
		}
		//发送数据
		if (!WriteFile(hserial, send, sendbit, &writebits, NULL)) {
			printf("发送失败\n");
			back();
			return;
		}
		//显示发送数据
		printf("发送：");
		if (config.showHex) {
			for (int i = 0; i < writebits; i++) {
				printf("%02X ", send[i]);
			}
		}
		else {
			for (int i = 0; i < writebits; i++) {
				//筛选可打印ASCALL字符
				if (send[i] >= 32 && send[i] < 127) {
					printf("%c", send[i]);
				}
				else {
					printf(".");
				}

			}
		}
		printf("\n");


		//接收数据

		unsigned char receive[1024];
		DWORD receivebit;
		DWORD errors;
		COMSTAT comstat;
		//清除错误并获取当前状态
		ClearCommError(hserial, &errors, &comstat);

		//comstat.cbInQue接收缓冲区中的字节数
		if (comstat.cbInQue > 0) {
			DWORD read = comstat.cbInQue;

			if (ReadFile(hserial, receive, read, &receivebit, NULL) && receivebit > 0) {   //receive输入缓存数据，read准备读取的字节数，&receivebit存入实际读取的字节数
				printf("接收:");
				if (config.showHex) {
					for (int i = 0; i < receivebit; i++) {
						printf("%02x", receive[i]);
					}
				}
				else {
					for (int i = 0; i < receivebit; i++) {
						if (receive[i] > 31 && receive[i] < 127) {
							printf("%c", receive[i]);
						}
						else {
							printf(".");
						}
					}
				}
				printf("\n");
			}

		}
	}
}