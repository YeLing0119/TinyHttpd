bin=HttpServer
cc=g++
FLAGS=-lpthread -std=c++11
DEBUG=-D_DEBUG_

.PHONY:$(bin)
$(bin):HttpServer.cc
	$(cc) $^ -o $@ $(FLAGS) #$(DEBUG)

.PHONY:clean
clean:
	rm $(bin)
