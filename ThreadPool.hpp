#ifndef __THREAD_POOL_H_
#define __THREAD_POOL_H_

#include <iostream>
#include <queue>
#include <pthread.h>


typedef void (*handler_t)(int);

class Task{
    private:
        int sock;
        handler_t handler;
    public:
        Task(int sock_, handler_t hander_):sock(sock_),handler(hander_){}
        void Run(){
            handler(sock);
        } 
        ~Task(){}
};

class ThreadPool{
    private:
        int num;        // 线程总数
        int idle_num;   // 休眠线程数
        std::queue<Task> task_queue;    
        pthread_mutex_t lock;
        pthread_cond_t cond;
    public:
        ThreadPool(int num_):num(num_){
            pthread_mutex_init(&lock, NULL);
            pthread_cond_init(&cond, NULL);
        }
        
        void InitPthreadPool(){
            pthread_t tid;
            for(auto i = 0; i < num; i++){
                pthread_create(&tid, NULL, ThreadRoutine, (void*)this);
            }
        }

        void WakeUp(){
            pthread_cond_signal(&cond);
        }

        void LockQueue(){
            pthread_mutex_lock(&lock);
        }

        void UnLockQueue(){
            pthread_mutex_unlock(&lock);
        }

        void Idle(){
            idle_num++;
            pthread_cond_wait(&cond, &lock);
            idle_num--;
        }

        Task PopTask(){
            Task t = task_queue.front();
            task_queue.pop();
            return t;
        }

        void PushTask(Task &t){
            LockQueue();
            task_queue.push(t);
            UnLockQueue();
            WakeUp();
        }

        static void *ThreadRoutine(void* arg){
            pthread_detach(pthread_self());
            ThreadPool *tp = (ThreadPool*)arg;

            while(1){
                tp->LockQueue();
                while(tp->IsTaskQueueEmpty()){
                    tp->Idle(); 
                }

                Task t = tp->PopTask();
                tp->UnLockQueue();
                std::cout << "Handler By: " << pthread_self() << std::endl;
                t.Run();
            }
        }

        bool IsTaskQueueEmpty(){
            return 0 == task_queue.size() ? true : false;
        }

        ~ThreadPool(){
            pthread_mutex_destroy(&lock);
            pthread_cond_destroy(&cond);
        }
};

#endif //__THREAD_POOL_H_

