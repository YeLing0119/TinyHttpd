#ifndef __PROTACOLUTIL_HPP__
#define __PROTACOLUTIL_HPP__

#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <string>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <unordered_map>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sstream>
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

#define WWWROOT "wwwroot"
#define HOMEPAGE "index.html"

const char *errorstr[] = {
	"Normal",
	"Warning",
	"Fatal Error"
};

void Logger(std::string msg, int level, std::string file, int line){
		std::cout<< "[" << file << ":" << line << "] " << msg << " [" << errorstr[level] << "]"<< std::endl;
}

#define  LOG(msg, level) Logger(msg, level, __FILE__, __LINE__)

class Util{
    public:
        static std::string IntToString(int num){
            std::stringstream ss;
            std::string line;
            ss <<  num;
            ss >> line;
            return line;
        }

        static int StringToInt(std::string line){
            std::stringstream ss(line);
            int num = -1;

            ss >> num;
            return num;
        }

        static std::string StatusCodeToStatusInfo(int status_code){
            switch(status_code){
                case 200: return "OK"; break; 
                case 400: return "Bad Request"; break;
                case 404: return "Not Find"; break;
                    default: return "UnKnown"; break;
            }

            return "UnKnown";
        }

        static std::string SuffixToType(std::string path){
            std::string suffix = path.substr(path.rfind('.'));

            if(suffix == ".html" || suffix == ".htm"){
                return "text/html";
            }

            if(suffix == ".js"){
                return "application/x-javascript";    
            }

            if(suffix == ".css"){
                return "text/css";
            }

            if(suffix == ".jpg"){
                return "application/x-jpg";
            }

            if(suffix == ".png"){
                return "application/x-png";
            }

            return "text/html";
        }
};


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

class HttpRequest{
	private:
		std::string method;
        std::string uri;
		std::string request_path;
        std::string request_parm;
		std::string version;
        bool cgi;
        int status_code;
        int resource_size;
	public:
		std::string request_line;
		std::unordered_map<std::string, std::string> request_header;
		std::string request_text;

	public:
		HttpRequest():request_path(WWWROOT),cgi(false),status_code(200){};

		void RequestLinePrase(){
			std::stringstream ss(request_line);
			ss >> method >> uri >> version;	
			transform(method.begin(), method.end(), method.begin(), ::toupper);
		}

		int IsMethodLegal(){
			if(method != "GET" && method != "POST"){
				return 400;
			}
			return 200;
		}

        void UriParse(){
            if(method == "GET"){
                size_t pos = uri.find('?');
                if(pos != std::string::npos){
                    // GET 带参
                    request_path += uri.substr(0, pos);
                    request_parm = uri.substr(pos + 1);
                }else{
                    // GET 不带参
                    request_path += uri;
                }
            }else{
                // POST 方法
               request_line += uri; 
            }
        }
        
        int IsRequestPathLegal(){
            struct stat file_stat;
            int ret = stat(request_path.c_str(), &file_stat);
            if(ret < 0){
                return 404;
            }

            if(S_ISDIR(file_stat.st_mode)){
                request_path += HOMEPAGE;
                struct stat home_page;
                stat(request_path.c_str(), &home_page);
                resource_size = home_page.st_size;
                LOG(request_path, NORMAL);
                return 200;
            }

            if((file_stat.st_mode & S_IXUSR) | 
               (file_stat.st_mode & S_IXGRP) |
               (file_stat.st_mode & S_IXOTH) ){
               cgi = true;
            }

            resource_size = file_stat.st_size;
            LOG(request_path, NORMAL);
            return 200;
        }

        void SetStatusCode(int num){
            status_code = num;
        }

        int GetStatusCode(){
            return status_code;
        }

        void SetResourceSize(int size){
            resource_size = size;
        }

        int GetResourceSize(){
            return resource_size;
        }

        std::string GetRequestPath(){
            return request_path;
        }

        void SetRequestPath(std::string path){
            request_path = path;
        }

        std::string GetRequestMethod(){
            return method;
        }

        bool IsCgi(){
            return cgi == true;
        }

		~HttpRequest(){};
		
};

class HttpResponse{
    public:
        std::string response_line;
        std::string version;
        std::string response_header;
        std::string response_text;
    public:
        HttpResponse():version("HTTP/1.0"){}

        void MakeResponseLine(HttpRequest* httpreq){
            response_line += version;
            response_line += ' ';
            response_line += Util::IntToString(httpreq->GetStatusCode());
            response_line += ' ';
            response_line +=Util::StatusCodeToStatusInfo(httpreq->GetStatusCode());
            response_line += '\n';
        }
        void MakeResponseHeader(HttpRequest* httpreq){
            std::string key = "Content-Lenght: ";
            std::string value = Util::IntToString(httpreq->GetResourceSize());

            response_header += key;
            response_header += value;
            response_header += '\n';

            key = "Content-Type: ";
            value = Util::SuffixToType(httpreq->GetRequestPath());
            
            response_header += key;
            response_header += value;
            response_header += '\n';
            
            response_header += '\n';
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
            index--;        // 去除后面的\n
			buff[index] = 0;

			line = buff;
			return index;
		}

        void GetRequestHeader(HttpRequest* httpreq){
            std::string line;
            std::string key;
            std::string value;
            while(true){
                GetOneLine(line);
                if(line == ""){
                    break;
                }
                int pos = line.find(": ");
                key = line.substr(0, pos);
                value = line.substr(pos + 2);
                httpreq->request_header.insert(std::pair<std::string, std::string>(key, value));
            }
        }

        void GetRequestText(HttpRequest* httpreq){
            int size = Util::StringToInt(httpreq->request_header["Content-Length"]);
            char buff[1024]; 

            read(sock, buff, size);
        }

        void SendResponse(HttpRequest* httpreq, HttpResponse* httprsp){
            write(sock, httprsp->response_line.c_str(), httprsp->response_line.size());  
            write(sock, httprsp->response_header.c_str(), httprsp->response_header.size());

            int sendfd = open(httpreq->GetRequestPath().c_str(), O_RDONLY);
            if(sendfd < 0){
                LOG("Open Source File Error", WARNING);
            }

            sendfile(sock, sendfd, NULL, httpreq->GetResourceSize());

            LOG("Send Response Success", NORMAL);
        }

        ~Connect(){
            close(sock);
        }
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
	#else
			std::string line;	
			Connect *conn = new Connect(sock);
			HttpRequest *httpreq = new HttpRequest;
            HttpResponse *httprsp = new HttpResponse;
        // 分析请求
            // 获取请求行，并且对请求行进行分析
			conn->GetOneLine(httpreq->request_line);
            std::cout << httpreq->request_line << std::endl;
            // 对请求行进行拆分
			httpreq->RequestLinePrase();
			
            // 分析请求方式是否合法
            if(httpreq->IsMethodLegal() == 400){
                httpreq->SetStatusCode(400);
                LOG("Request Method Is not legal", WARNING);
                goto end;
            }
            // 解析URI
            httpreq->UriParse();

            // 分析 request_path 是否合法
            if(httpreq->IsRequestPathLegal() == 404){
                httpreq->SetStatusCode(404);
                LOG("Request Path Is Not Legal", WARNING);
                goto end;
            }
            // 获取请求报头    
            conn->GetRequestHeader(httpreq);

            if(httpreq->GetRequestMethod() == "POST"){
                conn->GetRequestText(httpreq);
            }
        // 构建响应
            ProcessResponse(httpreq, httprsp, conn);
			
	#endif

    end:
            if(httpreq->GetStatusCode() == 400){
                httpreq->SetRequestPath("wwwroot/404.html");
                struct stat error_stat;
                stat(httpreq->GetRequestPath().c_str(), &error_stat);
                httpreq->SetResourceSize(error_stat.st_size);
                ProcessNoneCgi(httpreq, httprsp, conn);
            }else if(httpreq->GetStatusCode() == 404){
                httpreq->SetRequestPath("wwwroot/404.html");
                struct stat error_stat;
                stat(httpreq->GetRequestPath().c_str(), &error_stat);
                httpreq->SetResourceSize(error_stat.st_size);
                ProcessNoneCgi(httpreq, httprsp, conn);
            }

            delete httpreq;
            delete httprsp;
            delete conn;

            return (void *)0;
		}

        static void ProcessResponse(HttpRequest* httpreq, HttpResponse* httprsp, Connect* conn){
            if(httpreq->IsCgi()){
                LOG("Process With Cgi !",NORMAL);
                //ProcessWithCgi(httpreq, httprsp, conn);
            }else{
                LOG("Process Without Cgi", NORMAL);
                ProcessNoneCgi(httpreq, httprsp, conn);
            }
        }

        static void ProcessNoneCgi(HttpRequest* httpreq, HttpResponse* httprsp, Connect* conn){
            httprsp->MakeResponseLine(httpreq);
            httprsp->MakeResponseHeader(httpreq);
            conn->SendResponse(httpreq, httprsp);
        }
};



#endif // __PROTACOLUTIL_HPP__
