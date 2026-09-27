#pragma once
#include <string>
#include <array>
#include "ExprStructs.hpp"
#include <iostream>
using std::cout;
using std::array;
using std::string_view;
class alignas(64) Lexer{
    alignas(64) array<int,1024> tokens; // Lexing the string into individual tokens
    alignas(64) array<Node,1024> AST;  // Abstract syntax tree of the expression
    int lex_n;
    public:
        int lex(string_view stream,size_t n){
            int idx=0;
            int token_start=-1; // ATOM start index
            for(int i=0;i<n;i++){
                char c=stream[i];
                if(c==' '){
                    if(token_start!=-1){
                        tokens[idx++]=token_start;
                        tokens[idx++]=i-1; // End of the ATOM
                        token_start=-1;
                    }   
                }
                else if(c=='(') {
                    if(token_start!=-1){
                        tokens[idx++]=token_start;
                        tokens[idx++]=i-1; // End of the ATOM
                        token_start=-1;
                    }   
                    tokens[idx++]=-1; // Left Parenthesis
                }
                else if(c==')') {
                    if(token_start!=-1){
                        tokens[idx++]=token_start;
                        tokens[idx++]=i-1; // End of the ATOM
                        token_start=-1;
                    }   
                    tokens[idx++]=-2; // Right Parenthesiss 
                }
                else if(token_start==-1){
                    token_start=i; // Start of an ATOM
                }
            }
            // Using up any token which might not be under parenthesis
            if(token_start!=-1){
                tokens[idx++]=token_start;
                tokens[idx++]=n-1;
            }
            return lex_n=idx;
        }
        int TreeParse(){
            alignas(128) int levels[100]; // A stack like data structure for converting the intuitive recursion to a iterative function
            levels[0]=1;
            int li=0;
            int sz=1;
            AST[1].left=0;
            AST[1].next=0;
            AST[1].type=0;
            int sum=0;
            // i=1, assumption that the first index is ( , the synthesizer makes sure of that
            for(int i=1;i<lex_n;i++){
                if(tokens[i]==-1){
                    if(AST[levels[li]].type==0&&AST[levels[li]].left==0){
                        AST[levels[li]].left=sz+1;
                    }
                    else{
                        AST[levels[li]].next=sz+1;
                    }
                    sz++;
                    // Since the AST structure isnt cleaned every function call, we need to clean it everytime we start using a new Node
                    AST[sz].left=0; 
                    AST[sz].next=0;
                    // Setting up the LIST token
                    AST[sz].type=0;
                    levels[li]=sz;
                    // Emulating the calling of the recursive function
                    li++;
                    levels[li]=sz;
                }
                else if(tokens[i]==-2){
                    // Emulating the end of a recursive call, and going back to the caller
                    li--;
                }
                else{
                    int sl=tokens[i];
                    int sr=tokens[i+1];
                    if(AST[levels[li]].type==0){
                        AST[levels[li]].left=sz+1;
                    }
                    else{
                        AST[levels[li]].next=sz+1;
                    }
                    // Since the AST structure isnt cleaned every function call, we need to clean it everytime we start using a new Node
                    AST[sz+1].left=0; 
                    AST[sz+1].next=0; 
                    // Setting up the ATOM node
                    AST[sz+1].type=1;
                    AST[sz+1].offset=sl;
                    AST[sz+1].size=sr-sl+1;
                    levels[li]=sz+1;
                    sz++;
                    i++;
                    // This sum is used as a hash of sorts, to make sure compiler doesnt remove these functions for optimization
                    sum+=AST[sz].type+AST[sz].offset+AST[sz].size; 
                }
            }
            return sum;
        }
};
