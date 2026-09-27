#pragma once
/* 
* struct Expression
* exp is a pointer for storing the expressions location, so as to facilitate zero copying
* len is 4 byte int for storing the size of the exp char array
* alignas(16) ensures 4 descriptors fit into a standard 64-byte cache line
*/
struct alignas(16) Expression{
    const char * exp;  
    int len;
}; 

/* 
* struct Node
* alignas(16) for making sure that 4 Node objects fit snugly in the 64 byte cache line
* alignment also makes sure that the most number of Node are accessible from a cache read
* The Tree is represented as left child, right sibling.
* Each Node has a left child, and then a index to the right sibling
* type : LIST -> 0, ATOM -> 1
* offset : Index of the string from which the ATOM starts
* size : Size of the ATOM
*/
struct alignas(16) Node {
    int32_t left{0};
    int32_t next{0};
    uint16_t offset{0};
    uint16_t size{0};
    uint8_t type{0};
};