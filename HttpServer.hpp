#ifndef __HTTPSERVER_HPP__
#define __HTTPSERVER_HPP__

#include <iostream>
#include <unistd.h>
#include "ProtocolUtil.hpp"
#include "ThreadPool.hpp"

class HttpServer{
	private:
		int listen_sock;
		int port;
        ThreadPool pool;
	public:
		HttpServer(int port_):listen_sock(-1),port(port_),pool(5){}

	    void InitServer(){
	    	listen_sock = SocketApi::Socket();
	    	SocketApi::Bind(listen_sock, port);
	    	SocketApi::Listen(listen_sock);	
            pool.InitPthreadPool();
	    }
		
	    void Start(){
			
			while(1){
				std::string peer_ip;
				int peer_port;
				int sock = SocketApi::Accept(listen_sock, peer_ip, peer_port);
				if(sock >= 0){
					std::cout << peer_ip << ":" << peer_port << std::endl;
					// pthread_t tid;
					// pthread_create(&tid, NULL, Entry::HandlerRequest, (void *)sock);
                    Task t(sock, Entry::HandlerRequest);
                    pool.PushTask(t);
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
