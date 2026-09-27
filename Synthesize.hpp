#pragma once
#include <vector>
#include <array>
#include <string>
#include <random>
#include <cstdio>
#include <algorithm>

// Specification of the message type
struct MessageSpec{
    int operation;
    int entity;
    int field_count;
};

enum class Workload {
    GENERAL,
    NESTED,
    SHORT_MESSAGES,
    LARGE_MESSAGES,
    MIXED
    };
//Random generation helpers
inline static struct Generator{
    std::mt19937_64 rng;
    std::uniform_int_distribution<int> id_dist{100000,999999};
    std::uniform_int_distribution<int> time_dist{10,100};
    std::uniform_int_distribution<int> name_dist{0,19};
    Generator():rng(std::random_device{}()){};
    int rand_num(int lo,int hi){
        return std::uniform_int_distribution<int>{lo,hi}(rng);
    }
}gen;

class PayloadSynthesizer{
// String symbols to construct our artificial data
inline static constexpr std::array<std::string_view, 12> OPERATIONS = {
    "CREATE",
    "UPDATE",
    "DELETE",
    "READ",
    "START",
    "STOP",
    "ENABLE",
    "DISABLE",
    "MOVE",
    "COPY",
    "EXECUTE",
    "CANCEL"
};
inline static constexpr std::array<std::string_view, 16> ENTITIES = {
    "USER",
    "TASK",
    "JOB",
    "FILE",
    "DEVICE",
    "SERVICE",
    "ACCOUNT",
    "SESSION",
    "RESOURCE",
    "MESSAGE",
    "PROCESS",
    "ORDER",
    "ITEM",
    "RECORD",
    "CACHE",
    "QUEUE"
};
inline static constexpr std::array<std::string_view, 12> FIELDS = {
    "id", 
    "name", 
    "type", 
    "status",
    "priority",
    "size",
    "count",
    "source",
    "target",
    "value",
    "mode",
    "timeout"
};
inline static constexpr std::array<std::string_view,20> NAMES = {
        "Lorem", "ipsum", "dolor", "sit", "amet", 
        "consectetur", "adipiscing", "elit", "sed", "do",
        "eiusmod", "tempor", "incididunt", "ut", "labore", 
        "et", "dolore", "magna", "aliqua", "VERY_LONG_NAME_TO_STRESS_THE_LEXER"
    };
    public:
    //Simply generates the message according to the spec provided;
        static std::string generate_message(MessageSpec spec){
        std::string s="( ";
        s+=OPERATIONS[spec.operation];
        s+=' ';
        s+=ENTITIES[spec.entity];
        for(int i=0;i<spec.field_count;i++){
            s+=' ';
            s+=FIELDS[i%12];
            s+=" ";
            if(i%12==1||i%12==7||i%12==8){
                s+=NAMES[gen.name_dist(gen.rng)];
            }
            else if(i%12==0){
                s+=std::to_string(gen.id_dist(gen.rng));
            }
            else if(i%12==5||i%12==11){
                s+=std::to_string(gen.time_dist(gen.rng));
            }
            else{
                s+=std::to_string(gen.name_dist(gen.rng));
            }
        }
        s+=")";
        return s;
    }
    // Implements our custom variety strings using the type, essentially a type interpreter
    static std::string generate_custom(Workload type){
        MessageSpec spec;
        std::string str="";
        if(type==Workload::GENERAL){
                spec.operation=gen.rand_num(0,11);
                spec.entity=gen.rand_num(0,15);
                spec.field_count=gen.rand_num(2,12);
                return generate_message(spec);
        }
        else if(type==Workload::SHORT_MESSAGES){
            spec.operation=gen.rand_num(0,11);
            spec.entity=gen.rand_num(0,15);
            spec.field_count=gen.rand_num(1,3);
            return generate_message(spec);
        }
        else if(type==Workload::NESTED){
            int depth=gen.rand_num(1,5); // How deep to nest the commands
            int commands=gen.rand_num(2,6); // How many commands to write
            for(int i=0;i<depth;i++){
                str+="(BATCH ";
            }
            for(int i=0;i<commands;i++){
                spec.operation=gen.rand_num(0,11);
                spec.entity=gen.rand_num(0,15);
                spec.field_count=gen.rand_num(0,6);
                str+=generate_message(spec);
            }
            for(int i=0;i<depth;i++){
                str+=")";
            }
            return str;
        }
        else if(type==Workload::LARGE_MESSAGES){
            for(int i=0;i<10;i++){
                spec.operation=gen.rand_num(0,11);
                spec.entity=gen.rand_num(0,15);
                spec.field_count=12;
                str+=generate_message(spec);
                int spc=gen.rand_num(3,10);
                for(int j=0;j<spc;j++){
                    str+=" ";
                }
            }
            return str;
        }
        else{
            return "";
        }
    }
    static std::vector<std::string> generate(size_t count,Workload type){
        std::vector<std::string>messages;
        if(type==Workload::MIXED){
            for(int i=0;i<count;i++){
                // Deciding which type to insert, randomization helps in preventing similar type strings being grouped together
                int type_r=gen.rand_num(0,100);
                if(type_r<55){
                    messages.push_back(generate_custom(Workload::SHORT_MESSAGES));
                }
                else if(type_r<75){
                    messages.push_back(generate_custom(Workload::GENERAL));
                }
                else if(type_r<90){
                    messages.push_back(generate_custom(Workload::NESTED));
                }
                else{
                    messages.push_back(generate_custom(Workload::LARGE_MESSAGES));
                }
            }
        }
        else{
            for(int i=0;i<count;i++){
                messages.push_back(generate_custom(type));
            }
        }
        return messages;
    }
    
};