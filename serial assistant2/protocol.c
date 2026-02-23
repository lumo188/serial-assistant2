#define _CRT_SECURE_NO_WARNINGS  // 禁用安全警告
#define _CRT_SECURE_NO_WARNINGS_GLOBALS  // 禁用全局安全警告
#include"serial.h";

//crc16计算函数
unsigned short crc16(unsigned char* data, int len) {
	unsigned short crc = 0xFFFF;
	for (int i = 0; i < len; i++) {
		crc = crc ^ data[i];
		for (int j = 0; j < 8; j++) {
			if (crc & 1) {					//检验最低位是否为1
				crc = (crc >> 1) ^ 0xA001;  //右移一位做异或运算
			}
			else {
				crc >>= 1;
			}
		}
	}
	return crc;
}

//等待ACK函数
int waitack() {
	unsigned char buf;
	DWORD read;
	DWORD start = GetTickCount();
	while (GetTickCount() - start < ACK_TIMEOUT) {
		if (ReadFile(hserial, &buf, 1, &read, NULL) && read == 1) {
			if (buf == ACK) {
				return 1;
}
			else if(buf == NAK){
				return -1;
			}
			else {
				printf("收到未知字节：0x%02X\n", buf);
			}
		}
		Sleep(10);//线程休眠10ms
	}
	printf("等待超时\n");
	return 0;
}

//发送图片文件
void sendimage() {
	if (hserial == INVALID_HANDLE_VALUE) {
		printf("串口未打开\n");
		back();
		return;
	}
	char filepath[256];
	printf("请输入要发送的图片文件路径(例 D:\\图片\\image.jpg）：");
	fgets(filepath, sizeof(filepath), stdin);
	filepath[strcspn(filepath, "\n")] = '\0';

	//检查路径
	int len = (int)strlen(filepath);
	if (len == 0) {
		printf("路径不能未空\n");
		back();
		return;
	}

	FILE* fp = fopen(filepath, "rb");	//以二进制的形式打开文件
	if (!fp) {
		printf("无法打开文件\n");
		back();
		return;
	}
	fseek(fp, 0, SEEK_END);	//将fp指针定位到文件末尾
	long filesize = ftell(fp);//返回当前文件读写位置相对于文件开头的偏移量
	fseek(fp, 0, SEEK_SET);
	printf("文件大小：%ld字节\n", filesize);
	int totalblocks = (int)((filesize + BLOCK_SIZE - 1) / BLOCK_SIZE);
	printf("总块数：%d\n", totalblocks);

	//帧缓冲区:帧头2字节+序号2字节+长度2字节+数据+crc校验2字节
	unsigned char sendbuf[BLOCK_SIZE + 8];
	DWORD written;
	int seq = 0;
	int retry;

	for (int i = 0; i < totalblocks; i++) {
		int datalen = (int)fread(sendbuf + 6, 1, BLOCK_SIZE, fp);//sendbuf + 6偏移6位，在fp文件中按1字节读取，存放在sendbuf + 6

		//构建帧
		sendbuf[0] = FRAME_HEADER1;
		sendbuf[1] = FRAME_HEADER2;
		sendbuf[2] = (unsigned char)((seq >> 8) & 0xFF);//保留高8位
		sendbuf[3] = (unsigned char)(seq  & 0xFF);//保留低8位
		sendbuf[4] = (unsigned char)((datalen >> 8) & 0xFF);
		sendbuf[5] = (unsigned char)(datalen & 0xFF);

		// 计算CRC（从序号开始到数据结束）
		unsigned short crc = crc16(sendbuf + 2, datalen + 4); // 序号2 + 长度2 + 数据
		sendbuf[6 + datalen] = (unsigned char)((crc >> 8) & 0xFF);
		sendbuf[6 + datalen + 1] = (unsigned char)(crc & 0xFF);
		int frame_len = 6 + datalen + 2;//发送帧总长度

		retry = 0;
		while (retry < RETRIES) {
			PurgeComm(hserial, PURGE_RXCLEAR);
			if (!WriteFile(hserial, sendbuf, frame_len, &written, NULL) || written != (DWORD)frame_len) {
				printf("发送块 %d 失败\n", seq);
				fclose(fp);
				back();
				return;
			}

			FlushFileBuffers(hserial);
			printf("已发送块 %d，等待确认...\n", seq);
			int ack = waitack();
			printf("waitack返回: %d\n", ack);
			if (ack == 1) {
				printf("块 %d 确认\n", seq);
				break;
			}
			else if (ack == -1) {
				printf("块 %d 校验错误，重发...\n", seq);
				retry++;
			}
			else {
				printf("块 %d 超时，重发...\n", seq);
				retry++;
			}
		}
		if (retry == RETRIES) {
			printf("块 %d 发送失败，退出\n", seq);
			fclose(fp);
			back();
			return;
		}
		seq++;
		}

	// 发送结束帧
	unsigned char endframe[8] = { 0 };
	endframe[0] = FRAME_HEADER1;
	endframe[1] = FRAME_HEADER2;
	endframe[2] = (END_FRAME >> 8) & 0xFF;
	endframe[3] = END_FRAME & 0xFF;
	endframe[4] = 0;
	endframe[5] = 0;
	unsigned short crc = crc16(endframe + 2, 4); // 序号2 + 长度2
	endframe[6] = (unsigned char)((crc >> 8) & 0xFF);
	endframe[7] = (unsigned char)(crc & 0xFF);
	if (!WriteFile(hserial, endframe, 8, &written, NULL) || written != 8) {
		printf("发送结束帧失败\n");
	}
	else {
		FlushFileBuffers(hserial);//将缓存区中的数据写到硬件中
	}
	printf("发送完成\n");
	fclose(fp);
	back();
	}

	// 接收图片文件
	void receiveimage() {
		if (hserial == INVALID_HANDLE_VALUE) {
			printf("串口未打开\n");
			back();
			return;
		}
		// 清空缓冲区
		PurgeComm(hserial, PURGE_RXCLEAR | PURGE_TXCLEAR);

		char filepath[256];
		printf("请输入要保存的图片文件路径(例 D:\\图片\\image.jpg）：");
		fgets(filepath, sizeof(filepath), stdin);
		filepath[strcspn(filepath, "\n")] = '\0';

		int len = (int)strlen(filepath);
		if (len == 0) {
			printf("路径不能为空\n");
			back();
			return;
		}
		if (filepath[len - 1] == '\\' || filepath[len - 1] == '/') {
			printf("错误：路径结尾不能是目录分隔符，请包含文件名。\n");
			back();
			return;
		}
		FILE* fp = fopen(filepath, "wb");
		if (!fp) {
			printf("无法创建文件，错误：%s\n", strerror(errno));
			back();
			return;
		}
		printf("等待接收图片...\n");

		unsigned char buf[4096];
		DWORD read;
		int seq = 0;

		while (1) {
			// 同步帧头
			unsigned char h1, h2;
			while (1) {
				if (!ReadFile(hserial, &h1, 1, &read, NULL) || read != 1) {
					Sleep(10);
					continue;
				}
				if (h1 == FRAME_HEADER1) {
					if (!ReadFile(hserial, &h2, 1, &read, NULL) || read != 1) {
						continue;
					}
					if (h2 == FRAME_HEADER2) {
						break;
					}
				}
			}
			// 读取序号（2字节）
			unsigned char seq_bytes[2];
			if (!ReadFile(hserial, seq_bytes, 2, &read, NULL) || read != 2) {
				printf("读取序号失败\n");
				break;
			}
			int frame_seq = (seq_bytes[0] << 8) | seq_bytes[1];//将高8位左移8位与低八位进行或运算得到完整序号

			// 读取长度（2字节）
			unsigned char len_bytes[2];
			if (!ReadFile(hserial, len_bytes, 2, &read, NULL) || read != 2) {
				printf("读取长度失败\n");
				break;
			}
			int datalen = (len_bytes[0] << 8) | len_bytes[1];
			

			// 读取数据
			unsigned char data[4096];
			DWORD total_read = 0;
			while (total_read < (DWORD)datalen) {
				DWORD toread = (DWORD)datalen - total_read;
				if (!ReadFile(hserial, data + total_read, toread, &read, NULL) || read == 0) {
					printf("读取数据失败\n");
					break;
				}
				total_read += read;
			}
			if (total_read != (DWORD)datalen) {
				printf("数据读取不完整\n");
				unsigned char nak = NAK;
				WriteFile(hserial, &nak, 1, &read, NULL);
				FlushFileBuffers(hserial);
				continue;
			}

			// 读取CRC
			unsigned char crc_bytes[2];
			if (!ReadFile(hserial, crc_bytes, 2, &read, NULL) || read != 2) {
				printf("读取CRC失败\n");
				break;
			}
			unsigned short recv_crc = (crc_bytes[0] << 8) | crc_bytes[1];

			// 计算CRC（从序号开始到数据结束）
			unsigned char check_buf[BLOCK_SIZE + 4]; // 序号2 + 长度2 + 数据
			check_buf[0] = seq_bytes[0];
			check_buf[1] = seq_bytes[1];
			check_buf[2] = len_bytes[0];
			check_buf[3] = len_bytes[1];
			memcpy(check_buf + 4, data, datalen);//将接收到的图片数据复制到临时缓冲区 check_buf 的指定位置
			unsigned short calc_crc = crc16(check_buf, 4 + datalen);

			if (calc_crc != recv_crc) {
				printf("CRC校验失败，请求重传\n");
				unsigned char nak = NAK;
				WriteFile(hserial, &nak, 1, &read, NULL);
				FlushFileBuffers(hserial);
				continue;
			}

			// 检查是否为结束帧
			if (frame_seq == END_FRAME && datalen == 0) {
				printf("接收到结束帧，传输完成\n");
				break;
			}

			// 检查序号
			if (frame_seq != seq) {
				printf("序号错误，期望 %d 收到 %d，请求重传\n", seq, frame_seq);
				unsigned char nak = NAK;
				WriteFile(hserial, &nak, 1, &read, NULL);
				FlushFileBuffers(hserial);
				continue;
			}

			// 写入文件
			fwrite(data, 1, datalen, fp);
			fflush(fp); //将缓冲区中的数据写入磁盘

			// 发送ACK
			unsigned char ack = ACK;
			if (!WriteFile(hserial, &ack, 1, &read, NULL) || read != 1) {
				printf("发送ACK失败\n");
			}
			FlushFileBuffers(hserial);
			printf("已接收块 %d\n", seq);
			seq++;
		}
		fclose(fp);
		printf("图片接收完成，保存至 %s\n", filepath);
		back();
	}
