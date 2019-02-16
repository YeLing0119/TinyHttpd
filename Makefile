bin=HttpServer
cc=g++
FLAGS=-lpthread
DEBUG=-D_DEBUG_

.PHONY:$(bin)
$(bin):HttpServer.cc
	$(cc) $^ -o $@ $(FLAGS) -static #$(DEBUG)

.PHONY:clean
clean:
	rm $(bin)
