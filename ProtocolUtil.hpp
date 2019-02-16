#ifndef __PROTACOLUTIL_HPP__
#define __PROTACOLUTIL_HPP__

#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <string>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <algorithm>


#define NORMAL 0
#define WARNING 1
#define FATAL 2

#define BUFF_NUM 1024

#define BACKLOG 5

const char *errorstr[] = {
	"Normal",
	"Warning",
	"Fatal Error"
};

void Logger(std::string msg, int level, std::string file, int line){
		std::cout<< file << ": " << "[" <<line << "] "  << " : "<< msg << "  level : [" << errorstr[level] << "]"<< std::endl;
}

#define  LOG(msg, level) Logger(msg, level, __FILE__, __LINE__)

class SocketApi{
	public:
		static int Socket(){
			int sockfd = socket(AF_INET, SOCK_STREAM, 0);
			if(sockfd < 0){
				LOG("Create Socket Failure", FATAL);
				exit(2);
			}

			int opt = 1;
			setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
			
			return sockfd;
		}

		static void Bind(int sockfd, int port){
			struct sockaddr_in local;
			local.sin_family = AF_INET;
			local.sin_port = htons(port);
			local.sin_addr.s_addr = htonl(INADDR_ANY);

			if(bind(sockfd, (struct sockaddr*)&local, sizeof(local)) < 0){
				LOG("Bind Failure", FATAL);
				exit(3);
			}
		}

		static void Listen(int sockfd){
			if(listen(sockfd, BACKLOG) < 0){
				LOG("Listen Error", FATAL);
				exit(4);
			}
		}

		static int Accept(int listen_sock, std::string &ip, int &port){
			struct sockaddr_in peer;		
			socklen_t len = sizeof(peer);
			int sock = accept(listen_sock, (struct sockaddr *)&peer, &len);
			if(sock < 0){
				LOG("Accept Faliure", WARNING);
				return -1;
			}
			
			port = ntohs(peer.sin_port);
			ip = inet_ntoa(peer.sin_addr);

			return sock;
		}
};

class Connect{
	private:
		int sock;
	public:
		Connect(int sock_):sock(sock_){}

		int GetOneLine(std::string &line){
			char buff[BUFF_NUM];
			char c = 'X';
			int index = 0;
			while(c != '\n' && index < BUFF_NUM - 1){
				ssize_t ret = recv(sock, (void*)&c, 1, 0);
				if(ret > 0){
					if(c == '\r'){
						recv(sock, (void*)&c, 1, MSG_PEEK);		// 从sock中尝试获取一个字符，但是不从sock中移除，相当于数据窥探
						if(c == '\n'){					
							recv(sock, (void*)&c, 1, 0);		// 如果窥探的数据是\n 则该系统的换行是 \r\n
						}else{
							c = '\n';					// 如果不是，则该系统的换行是\r
						}	
					}

					// 走到这里这可能当前字符是正常字符 或者 是\n
					buff[index++] = c;
				}else{
					break;
				}
			}
			buff[index] = 0;

			line = buff;
			return index;
		}

		~Connect(){}
};

class HttpRequest{
	private:
		std::string method;
		std::string path;
		std::string version;

	public:
		std::string request_line;
		std::string request_head;
		std::string blank;
		std::string text;

	public:
		HttpRequest(){};

		void RequestLinePrase(){
			std::stringstream ss(request_line);
			ss >> method >> path >> version;	
			transform(method.begin(), method.end(), method.begin(), ::toupper);
		}

		bool IsMethodLegal(){
			if(method != "GET" && method != "POST"){
				return false;
			}
			return true;
		}

		~HttpRequest(){};
		
};

class Entry{
	public:
		static void *HandlerRequest(void *arg){
			pthread_detach(pthread_self());
			int sock = *(int *)arg;
			delete (int *)arg;

	#ifdef _DEBUG_
			// for test 
			char buff[10240];
			memset(buff, 1, sizeof(buff));

			read(sock, buff, sizeof(buff));
			std::cout << buff << std::endl;
						
			char text[1024] = "HTTP/1.1 400 Bad Request\nServer: stgw/1.3.10.3_1.13.5\nDate: Wed, 13 Feb 2019 10:18:53 GMT\nContent-Type: text/html\nContent-Length: 181\nConnection: close\n\n<html>\n<head><title>400 Bad Request</title></head>\n<body bgcolor=\"white\">\n<center><h1>400 Bad Request</h1></center>\n<hr><center>stgw/1.3.10.3_1.13.5</center>\n</body>\n</html>";
			write(sock, text, sizeof(text));
	#else
			std::string line;	
			Connect *conn = new Connect(sock);
			HttpRequest *httpreq = new HttpRequest;
			// 获取请求行，并且对请求行进行分析
			conn->GetOneLine(httpreq->request_line);
			// 对请求行进行拆分
			httpreq->RequestLinePrase();
			// 分析请求方式是否合法
			httpreq->IsMethodLegal();
			
	#endif

			close(sock);
		}
};



#endif // __PROTACOLUTIL_HPP__
