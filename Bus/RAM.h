#ifndef RAM_H
#define RAM_H
#include "../Utils/handler.h"


class RAM
{
    public:
        RAM();
        BYTE readFromMemory(ADDRESS address) const;
        void writeToMemory(ADDRESS address,BYTE value);
        void clearMemoryBlock(ADDRESS start,ADDRESS end);
        void print(int end = 20);
        friend class CPU;
    private:
        BYTE memory[64 * 1024]; // main memory will be allocated in construction
};


#endif