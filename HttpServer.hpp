#ifndef __HTTPSERVER_HPP__
#define __HTTPSERVER_HPP__

#include <iostream>
#include <unistd.h>
#include "ProtocolUtil.hpp"
class HttpServer{
	private:
		int listen_sock;
		int port;
	public:
		HttpServer(int _port):port(_port){}

	    void InitServer(){
	    	listen_sock = SocketApi::Socket();
	    	SocketApi::Bind(listen_sock, port);
	    	SocketApi::Listen(listen_sock);	
	    }
		
	    void Start(){
			
			while(1){
				std::string peer_ip;
				int peer_port;
				int *sock_p = new int;
				*sock_p = SocketApi::Accept(listen_sock, peer_ip, peer_port);
				if(*sock_p >= 0){
					std::cout << peer_ip << ":" << peer_port << std::endl;
					pthread_t tid;
					pthread_create(&tid, NULL, Entry::HandlerRequest, (void *)sock_p);
				}
	    	}
	    }
		
		~HttpServer(){
			if(listen_sock > 0){
				close(listen_sock);
			}
		}
};

#endif // __HTTPSERVER_HPP__
