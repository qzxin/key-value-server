/*
 *  server.cpp
 *  
 *  Author: quinn
 *  Date : 2015/06/12
*/
#include <unordered_map>
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <iostream>
#include "node.h"
#include "convert.h"
#include "hash_cache.h"
using namespace std;
#define DEFAULT_PORT  8000
#define MAX_LINE 4096
//#define CACHE_SIZE 1000
#define LEN (KEY_SIZE+VALUE_SIZE+2)
static mutex put_mutex, get_mutex;
static class HashCache cache;
static void handle_client_requests(int connected_fd); // thread function
int main(int argc, char* argv[])
{
	int socket_fd, connected_fd;
	char buff[MAX_LINE];
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	int sin_addr_size = sizeof(server_addr);
	// creat socket
	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("Creat a socket error: %s (errno: %d)\n", strerror(errno), errno);
		exit(0);
	}
	// init socket
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	/*
	  htonl()--"Host to Network Long int"     32Bytes
      ntohl()--"Network to Host Long int"     32Bytes
      htons()--"Host to Network Short int"   16Bytes
      ntohs()--"Network to Host Short int"   16Bytes
     */
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
	server_addr.sin_port = htons(DEFAULT_PORT); 
	//
	int on = 1;
	if ((setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) < 0) {
		perror("setsockopt error\n");
		exit(0);
	}
	//bind
	if (bind(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
		printf("bind  socket error: %s (errno: %d)\n", strerror(errno), errno);
		exit(0);
	}
	if (listen(socket_fd, 10) == -1) {
		printf("listen socket error: %s (errno: %d)\n", strerror(errno), errno);
		exit(0);
	}
	printf("=====Waiting for clients's request=====\n");
	while(true) {
		//accept默认会阻塞进程，直到有一个客户连接建立后返回，它返回的是一个新可用的套接字
		if ((connected_fd = 
			accept(socket_fd, (struct sockaddr*)&client_addr, (socklen_t*)&sin_addr_size)) == -1) {
			printf("accept socket error: %s (errno: %d)\n", strerror(errno), errno);
			continue;
		}
		cout << "client " << connected_fd - 3 << " is connecting." << endl; // 0~2 is file_thread; 3 is main_thread;4 is the first client
		cout << "IP: " << inet_ntoa(client_addr.sin_addr) << "  " << "Port: " << htons(client_addr.sin_port) << endl;
		thread client(handle_client_requests, connected_fd); // new a thread to handle current client's requests
		client.detach(); // detach this client thread, or you must wait it ( client.join() ).
	}
	close(socket_fd);
	cout << "server finish" << endl;
	return 0;
}

static void handle_client_requests(int connected_fd) 
{
	cout << "client " << connected_fd - 3 << " connected success" << endl; // 0~3 thread are default; 4 is the first client 
	char buff[MAX_LINE];
	memset(buff, 0, MAX_LINE);
	buff[0] = '\0';
	//close(socket_fd); // close listen fd
	while (true) { // for 循环，和单个client持续通信
	// receive from client
		int len = recv(connected_fd, buff, MAX_LINE, 0);  // 0 is a flag
		if (len == 0)
			continue;
		vector<string> recv_vector = split_str(buff, " ;:");
		if (recv_vector[0] == "exit") {  // 结束和当前客户端的通信
			break;
		}
		if (recv_vector[0] == "save") {  // save all cache data
			cache.save_cache();
			static const char* save_info = "All cache has saved to files. You can shutdown the server safely.";
			if(send(connected_fd, save_info, MAX_LINE, 0) == -1) {
                perror("send error");
			}
			cout << save_info << endl;
			continue;
		}
		memset(buff, 0, MAX_LINE);
		if (recv_vector.size() == 2 && recv_vector[0] == "get"){
			//when get: forbid put, allow get
			put_mutex.lock();
			string value = cache.get(recv_vector[1]);
			put_mutex.unlock();
			if(send(connected_fd, ("get-"+value).c_str(), MAX_LINE, 0) == -1) {
                perror("send error");
			}
			if (!value.empty()) {
				cout << "search success, key = " << recv_vector[1] << " and value = " << value << endl;
			} else {
				cout << "search failed, there is no key = " <<  recv_vector[1] << endl;
			}
			
		} else if (recv_vector.size() == 3 && recv_vector[0] == "put") {
			// when put : forbid get and put
			put_mutex.lock();
			get_mutex.lock();
			cache.put(recv_vector[1], recv_vector[2]);
			get_mutex.unlock();
			put_mutex.unlock();
			if(send(connected_fd, "put-success", MAX_LINE, 0) == -1) {
                perror("send error");
			}
			cout << "put success" << endl;
		} else {
			static const char* input_error_info = "Input command error, try again. You can enter [help] to view help information";
			if(send(connected_fd, input_error_info, MAX_LINE, 0) == -1)
                perror("send error");
			cout << input_error_info << endl;
		}
		// 处理接受到的信息
		printf("==================================================\n");
	}
	cout << "client " << connected_fd - 3 << " has exited" << endl;
	close(connected_fd);
	// exit(0);
}
