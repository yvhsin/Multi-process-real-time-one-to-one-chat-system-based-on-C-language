#include <stdio.h>
#include <stdlib.h>
#define HAVE_STRUCT_TIMESPEC	// 防止 winsock2.h 里的某些结构体与其他头文件中的结构体重复定义
#include <pthread.h>
#include <WinSock2.h>

// 引入链接库
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib, "pthreadVC2.lib")

typedef struct sock_with_addr {
	SOCKET socket;
	SOCKADDR_IN socket_addr;
}sock_with_addr;

// 用于接收数据的线程所使用的回调函数
void* recv_thread(void* p) {
	sock_with_addr* client = (sock_with_addr*)p;
	char recv_buf[256];	// 接收缓冲区
	int ret;
	while (1) {
		// 接收
		ret = recv(client->socket, recv_buf, 256, 0);
		// 若未接收到或检测到连接断开时, 退出线程
		if (ret == 0 || ret == -1) {
			printf("Disconected.\n");
			closesocket(client->socket);
			break;
		}
		printf("%s:%d : %s\n", inet_ntoa(client->socket_addr.sin_addr), client->socket_addr.sin_port, recv_buf);
	}
    return NULL;
}

int main() {
	// 初始化 sokcet 库
	WSADATA data;
	WORD dwVersionRequested = MAKEWORD(2, 2);
	WSAStartup(dwVersionRequested, &data);
	// 接受用户输入, 选择连接模式
	int mode;
	printf(" 1. listen\n 2. connect\n");
	scanf("%d", &mode);
	while (mode != 1 && mode != 2) {
		printf("Try again.\n");
		scanf("%d", &mode);
	}
	if (mode == 1) {
		// 初始化监听 socket
		SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (listen_socket == -1) {
			printf("Socket failed.\n");
			exit(0);//正常运行程序并退出程序
		}
		// 为监听 socket 绑定地址
		char addr[15];
		printf("Input the local address : ");	// 接收用户输入的地址
		scanf("%s", addr);
		SOCKADDR_IN sock_addr;
		sock_addr.sin_family = AF_INET; 
		sock_addr.sin_addr.S_un.S_addr = inet_addr(addr);	// 将字符串形式的地址转换为字节
		sock_addr.sin_port = htons(6666);					// 设置端口号, 并转换为字节形式
			//Linux网络编程中主机字节序转换成网络字节序，win是htonl()
		bind(listen_socket, (SOCKADDR*)&sock_addr, sizeof(SOCKADDR));	// 绑定
			//(sockfd 文件标识符 指定地址与哪个socket绑定，addr 指定地址，addlen 地址长度)
		
		// 开始监听
		listen(listen_socket, 5);//
			//(sockfd 文件标识符，backlog 参数涉及网络细节，一般小于30)
		printf("Waiting to be connected.\n");
		// 准备接收连接
		SOCKADDR_IN client_socket_addr;
		int len = sizeof(SOCKADDR);
		SOCKET client_socket = accept(listen_socket, (SOCKADDR*)&client_socket_addr, &len);
		/**
		 * 接受连接
		 *
		 * @param sockfd 上述listen函数指定的监听socket
		 * @param addr   请求连接方（即客户端）地址
		 * @param addrlen 客户端地址长度
		 * @return 函数执行成功返回一个新的连接socket，失败返回-1
		*/
		// 接收到连接之后, 关闭监听 socket
		closesocket(listen_socket);
		printf("Connected.\n");
		sock_with_addr sa;	// 封装接入连接的 socket 和地址以传递给接收线程
		sa.socket = client_socket;
		sa.socket_addr = client_socket_addr;
		// 异步接收和发送
		// 创建并启动线程
		pthread_t recv_th;
		int ret = pthread_create(&recv_th, NULL, recv_thread, (void*)&sa);
		/*
		pthread_create函数用于创建线程。
		第一个参数用于存放指向pthread_t类型的指针（指向该线程tid号的地址）
		第二个参数表示了线程的属性，一般以NULL表示默认属性
		第三个参数是一个函数指针，就是线程执行的函数。这个函数返回值为 void*，形参为 void*
		第四个参数则表示为向线程处理函数传入的参数，若不传入，可用 NULL 填充（第四个参数设计到void*变量的强制转化）

		返回值：成功，返回0；失败，返回一个错误码。可以使用 perror（）函数查看错误原因。
		*/
		if (ret != 0) {
			printf("Thread failed.\n");
			exit(0);
		}
		char send_buf[256];	// 发送缓冲区
		while (1) {
			// 获取用户整行输入
			fgets(send_buf, 256, stdin);
				//相对gets()函数不检查预留存储区是否能够容纳实际输入的数据，可能会发生内存越界，
				//fgets()更安全，但是多了两个参数，会自动根据定义数组的长度截断
			
			// 为发送缓冲区字符串的末尾设置为结束符 \0 ,确定结束
			send_buf[strlen(send_buf) - 1] = '\0';
			if (strlen(send_buf) == 0)
				continue;
			ret = send(client_socket, send_buf, 256, 0);
			// 若未成功发送数据, 则表示连接已断开
			if (ret == 0 || ret == -1) {
				printf("Disconnected.\n");
				break;
			}
		}
		// 终止线程
		pthread_exit(NULL);
		// 关闭接入连接的 socket
		closesocket(client_socket);
	}
	else {
		// 初始化连接 socket
		SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (server_socket == -1) {
			printf("Socket failed.\n");
			exit(0);
		}
		// 指定要连接的地址
		char addr[15];
		printf("Input the server address : ");
		scanf("%s", addr);
		SOCKADDR_IN server_sock_addr;
		server_sock_addr.sin_family = AF_INET;
		server_sock_addr.sin_addr.S_un.S_addr = inet_addr(addr);	// 将字符串形式的地址转换为字节
		server_sock_addr.sin_port = htons(6666);					// 设置端口号, 并转换为字节形式
		// 连接
		int len = sizeof(SOCKADDR);
		int ret = connect(server_socket, (SOCKADDR*)&server_sock_addr, len);
		/*
		connect函数通常用于客户端建立tcp连接。
		参数：
		sockfd：标识一个套接字。
		serv_addr：套接字s想要连接的主机地址和端口号。
		addrlen：name缓冲区的长度。
		返回值：
		成功则返回0，失败返回-1，错误原因存于errno中。
		*/
		printf("Connecting.\n");
		if (ret == -1) {
			printf("Connect failed.\n");
			exit(0);
		}
		printf("Connected.\n");
		sock_with_addr sa;	// 封装接入连接的 socket 和地址 以传递给接收线程
		sa.socket = server_socket;
		sa.socket_addr = server_sock_addr;
		// 异步接收和发送
		// 创建并启动线程
		pthread_t send_th;
		ret = pthread_create(&send_th, NULL, recv_thread, (void*)&sa);
		if (ret != 0) {  //只要不是0就证明创建线程失败了
			printf("Thread failed.\n");
			exit(0);
		}
		char send_buf[256];	// 发送缓冲区
		while (1) {
			// 获取用户整行输入
			fgets(send_buf, 256, stdin);//从标准输入流stdin即缓冲区中读取256个字符保存到内存空间首地址字符数组send_buf中
				//相对gets()函数不检查预留存储区是否能够容纳实际输入的数据，可能会发生内存越界，
				//fgets()更安全，但是多了两个参数，会自动根据定义数组的长度截断
			
			// 为发送缓冲区字符串的末尾设置为结束符 \0 
			send_buf[strlen(send_buf) - 1] = '\0';
			if (strlen(send_buf) == 0)
				continue;
			ret = send(server_socket, send_buf, 256, 0);
			// 若未成功发送数据, 则表示连接已断开
			if (ret == 0 || ret == -1) {
				printf("Disconnected.\n");
				break;
			}
		}
		// 终止线程
		pthread_exit(NULL);
		// 关闭进行连接的 socket
		closesocket(server_socket);
	}

	// 结束使用 socket 库
	WSACleanup();
	return 0;
}