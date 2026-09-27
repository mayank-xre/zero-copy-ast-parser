#include "Synthesize.hpp"
#include "ExprStructs.hpp"
#include "Lexer.hpp"
#include "SPSC.hpp"
#include <iostream>
#include <chrono>

std::string e_to_s(int type){
    switch(type){
        case 0 : return "GENERAL"; 
        case 1 : return "NESTED";
        case 2 : return "SHORT";
        case 3 : return "LONG";
        case 4 : return "MIXED"; 
    }
    return "";
}

void print_benchmarks(Workload type){
    int UNIQUE_COUNT=100'000;
    int NUM_MESSAGES=10'000'000;
    PayloadSynthesizer ps;
    std::vector<std::string> messages=ps.generate(UNIQUE_COUNT,type);
    long proc_data_sz=0;
    SPSC<>queue;
    for(int i=0;i<NUM_MESSAGES;i++){
        proc_data_sz+=messages[i%UNIQUE_COUNT].size();
    }
    auto start_time = std::chrono::high_resolution_clock::now();
    for(int i=0;i<NUM_MESSAGES;i++){
        const auto& msg=messages[i%UNIQUE_COUNT];
        queue.insert(msg.data(),static_cast<int>(msg.size()));
    }
    queue.done();
    auto end_time = std::chrono::high_resolution_clock::now();
    auto elapsed_ns=std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
    double elapsed_sec = static_cast<double>(elapsed_ns) / 1e9;
    double msg_per_sec = static_cast<double>(NUM_MESSAGES) / elapsed_sec;
    double avg_pipeline_ns = static_cast<double>(elapsed_ns) / NUM_MESSAGES;
    double GB_proc=(static_cast<double>(proc_data_sz)/1e9)/elapsed_sec;
    std::cout << "\n================ "<< e_to_s(static_cast<int>(type)) <<" PAYLOAD BENCHMARK RESULTS ================\n";
    std::cout << " Total Processed   : " << NUM_MESSAGES << " messages\n";
    std::cout << " Unique Payloads   : " << UNIQUE_COUNT << " strings\n";
    std::cout << " Total Time        : " << elapsed_sec << " seconds\n";
    std::cout << " Throughput        : " << (msg_per_sec / 1e6) << " Million msgs/sec\n";
    std::cout << " GB Throughput     : " << GB_proc <<" GB/s\n";
    std::cout << " Avg Per Message   : " << avg_pipeline_ns << " ns\n";
    std::cout << " Verification Hash : " << queue.get_hash() << "\n";
    std::cout << "===============================================================================\n";
}
int main(){
    print_benchmarks(Workload::MIXED);
    print_benchmarks(Workload::GENERAL);
    print_benchmarks(Workload::SHORT_MESSAGES);
    print_benchmarks(Workload::NESTED);
    print_benchmarks(Workload::LARGE_MESSAGES);
    return 0;
}