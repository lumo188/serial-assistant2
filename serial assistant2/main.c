#define _CRT_SECURE_NO_WARNINGS  // 禁用安全警告
#define _CRT_SECURE_NO_WARNINGS_GLOBALS  // 禁用全局安全警告

#include"serial.h";


serial config;
HANDLE hserial = INVALID_HANDLE_VALUE; // 串口句柄
int running = 1;

//主函数
int main() {
	//默认配置
	strcpy(config.portname, "COM1");
	config.baurdrate = 9600;
	config.databits = 8;
	config.stopbits = 1;
	config.parity = 'N';
	config.showHex = 1;     // 默认HEX显示
	config.sendHex = 1;     // 默认HEX发送


	while (running) {
		serialmenu();

		char choice = _getch();
		printf("\n\n");
		switch (choice) {
		case'0':
			Checkconfig();
			back();
			break;
		case'1':
			usable();
			back();
			break;
		case'2':
			ConfigureSerial();
			back();
			break;
		case'3':
			openserial();
			back();
			break;
		case'4':
			closeserial();
			back();
			break;
		case'5':
			senddata();
			break;
		case'6':
			config.showHex = !config.showHex;
			printf("显示格式已切换为：%s\n", config.showHex ? "Hex" : "ASCALL");
			back();
			break;
		case'7':
			config.sendHex = !config.sendHex;
			printf("发送格式已切换为：%s\n", config.sendHex ? "Hex" : "ASCALL");
			back();
			break;
		case'8':
			sendimage();
			back();
			break;
		case'9':
			receiveimage();
			back();
			break;
		case'a':
			running = 0;
			printf("退出程序\n");
			break;
		default:
			printf("无效选项，请重新选择\n");
			back();
			break;

		}
	}
	if (hserial != INVALID_HANDLE_VALUE) {
		closeserial();
	}
	return 0;
}
