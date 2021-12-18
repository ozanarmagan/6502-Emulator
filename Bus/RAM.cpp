#include "RAM.h"
#include <iostream>
#include <iomanip>

RAM::RAM()
{
    //memory = new uint8_t[64 * 1024](); // Allocate 64KB memory for RAM of NES 
}

BYTE RAM::readFromMemory(ADDRESS address) const
{
    return memory[address];
}

void RAM::writeToMemory(ADDRESS address,BYTE value) 
{
    memory[address] = value;
}

void RAM::clearMemoryBlock(ADDRESS start,ADDRESS end)
{
    for(ADDRESS i = start;i < end + 1;i++)
        memory[i] = 0x00;
}

void RAM::print(int end)
{
    std::cout << "RAM: " << std::endl;
    for(int i = 0;i < end;i++)
    {
        std::cout << std::hex << std::setfill('0') << std::setw(2)  << memory[i] << " ";
        if(i % 8 == 0)
            std::cout  << std::endl;
    }
    std::cout << std::endl;
}