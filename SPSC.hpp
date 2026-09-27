#pragma once
#include <atomic>
#include <thread>
#include <algorithm>
#include <cstring>
#include "Lexer.hpp"
// BUF_SZ = 2^N this is so that modulus can be calculated very easily by a single and operation
template<size_t BUF_SZ=8192, size_t C_SZ=256, size_t B_SZ=32> 
class SPSC{
    long long checksum{0};
    size_t cap=BUF_SZ;  // Capacity of the circualar ring, dividend of the modulus
    size_t mask=BUF_SZ-1;  // Divisor of the modulus
    alignas(128) struct{    // alignas(128) to make sure that the cpu doesnt suffer from false sharing   
        std::atomic<size_t>head{0};
        std::atomic<bool>stop{false};
    }c;
    // alignas(128) to make sure that the producer and consumer struct dont end up in the same cache line, leading to false sharing  
    alignas(128) struct{   
        size_t local_tail{0};
        size_t producer_limit=BUF_SZ;
    }p;
    alignas(128) struct{
        std::atomic<size_t>tail{0};
    }p_pub;
    // Aligned seprately from the cache line on which consumer, producer struct exist.
    alignas(128) Expression buffer[BUF_SZ]; 
    std::thread consumer;
    void cons_proc(){
        Lexer outp;  //Creating the lexer on the heap
        size_t current_head=c.head.load(std::memory_order_acquire);
        size_t publish_head=current_head;
        while(true){
            size_t current_tail=p_pub.tail.load(std::memory_order_acquire);
            while(current_head<current_tail){
                size_t head_idx=current_head&mask;  // Modulus to find the index of the buffer
                size_t cou=std::min(current_tail-current_head,cap-head_idx); // Min to make sure that circular buffer warping doesnt lead to out of bound error
                const Expression* ptr=&buffer[head_idx];
                // Data processing loop
                for(int i=0;i<cou;i++){
                    outp.lex(ptr[i].exp,ptr[i].len);
                    checksum+=outp.TreeParse();
                }
                current_head+=cou;
                if(current_head-publish_head>=C_SZ){ // Notifying the producer that data has been processed, freeing up space
                    c.head.store(current_head,std::memory_order_release);
                    publish_head=current_head;
                }
            }
            if(current_head==current_tail){ // Making sure that stalling doesnt occur on edge cases
                c.head.store(current_head,std::memory_order_release);
                publish_head=current_head;
            }
            if(c.stop.load(std::memory_order_acquire)&&(current_head==p_pub.tail.load(std::memory_order_acquire))){
                return;
            }
        }
    }
    public:
        SPSC(){
            consumer=std::thread(&SPSC::cons_proc,this);
        }
        inline void insert(const char exp[],int len){
            if(__builtin_expect(p.local_tail==p.producer_limit,0)){
                size_t curr_head=c.head.load(std::memory_order_acquire);
                p.producer_limit=curr_head+cap;
                while(p.local_tail==p.producer_limit){ //Waiting for the consumer to have free up some space on the buffer 
                    curr_head=c.head.load(std::memory_order_acquire);
                    p.producer_limit=curr_head+cap;
                }
            }
            auto& slot=buffer[p.local_tail&mask]; // & Here acts as a modulo operator
            // Setting the pointers and length of the Expression Object
            slot.exp=exp;
            slot.len=len;
            size_t prev_tail=p.local_tail;
            p.local_tail++;
            // Prevent a deadlock where consumer has nothing to consume and only lesser than B_SZ items are left 
            bool was_empty=(prev_tail==p_pub.tail.load(std::memory_order_relaxed));
            if(!(p.local_tail&(B_SZ-1))||was_empty){
                // Notifying the consumer about the presence of new data on buffer
                p_pub.tail.store(p.local_tail,std::memory_order_release); 
            }
        }
        long long get_hash(){
            return checksum;   //Verification hash for preventing dead code elimination
        }
        void done(){
            if(p.local_tail!=p_pub.tail.load(std::memory_order_relaxed)){
                // Notify the consumer thread that all data has been processed, time to end the queue
                p_pub.tail.store(p.local_tail,std::memory_order_release); 
            }
            c.stop.store(true,std::memory_order_release);
            if(consumer.joinable()){
                consumer.join(); //Consume any leftover data on the buffer
            }
        }
        ~SPSC(){
            done();
        }
};