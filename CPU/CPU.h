#ifndef CPU_H
#define CPU_H

#include "../Utils/handler.h"
#include "../Bus/RAM.h"
#include "../PPU/PPU.h"
#include <functional>

using std::function;

/*  

    This is a solid emulation of 6502 Proccessor.
    
    CPU EMULATION TYPE : Jump Table Based

    EXPLANATION :
    Each addressing mode (ADDRESSING_MODE) and operation (OPEXEC) is a lambda function 
    and they are forming INSTRUCTION struct with cycle count for each OPCODE
    Since there is 256 OPCODE but 6502 using only 151 of them,remaining
    OPCODEs are illegal and they literally do nothing (NOP)

*/








class CPU
{
    public:
        CPU(RAM& mem,PPU& ppu); 

        void setProgramCounter(uint16_t address);

        void moveProgramCounter(uint8_t offset); // move program counter from current location

        void UpdateCurrentCycle(std::uint8_t offset); // add cycle offset

        ADDRESS getProgramCounter() const; 

        ADDRESS getStackPointerAddress() const; // Stack pointer address from RAM
 

        uint64_t getCycleIndex() const; // current cycle index
        
        void tick();

        friend std::ostream& operator<<(std::ostream &out,CPU &cpu); // For logging stuff

    private:
        using OPEXEC = void;
        using OPEXEC_PTR = const std::function<CPU::OPEXEC (ADDRESS)> CPU::*;
        using ADDRESSING_MODE =  const function<ADDRESS ()> CPU::*;
        struct INSTRUCTION 
        {
            ADDRESSING_MODE addr;
            OPEXEC_PTR operation;
            uint8_t cycles;
        };

        /*----------CORE REGISTERS------------*/
        uint8_t A   = 0x00; // Accumulator
        uint8_t X   = 0x00; //
        uint8_t Y   = 0x00; // Index registers
        uint8_t SP  = 0xFF; // Stack Pointer is between 0xFF and 0x00

        FLAG CARRY = 0,OVERFLOWBIT = 0,ZERO = 0,NEGATIVE = 0,BREAK = 0,INTERRUPT_DISABLE = 0,DECIMAL = 0; // Processor flags

        static const ADDRESS IRQVECTOR_H = 0xFFFF;
        static const ADDRESS IRQVECTOR_L = 0xFFFE;
        static const ADDRESS RSTVECTOR_H = 0xFFFD;
        static const ADDRESS RSTVECTOR_L = 0xFFFC;
        static const ADDRESS NMIVECTOR_H = 0xFFFB;
        static const ADDRESS NMIVECTOR_L = 0xFFFA;

        /* PROGRAM COUNTER */
        ADDRESS programCounter = 0x0000;

        /* CYCLE INDEX */
        uint64_t currentCycle = 0x0000000000000000;

        uint8_t currentOpCode;

        INSTRUCTION currentInstruction;

        RAM memory;

        PPU ppu;
        
        void reset(); // CPU to default state

        void push(uint8_t value); // Push value to stack

        uint8_t pop(); // Pop from stack 

        INSTRUCTION table[256];

        void execute();

        /*------------------------OPERATIONS------------------------*/
        const function<OPEXEC(ADDRESS)> ADC = [this](ADDRESS source)
        {
            uint8_t data = memory.readFromMemory(source);
            unsigned int temp = data + A + (CARRY ? 1 : 0);
            ZERO = !(temp & 0xFF);

            if(DECIMAL)
            {
                if(((A & 0xF) + (data & 0xF) + (CARRY ? 1 : 0)) > 9) temp += 6;
                NEGATIVE = temp & 0x80;
                OVERFLOWBIT = !((A ^ data) & 0x80) && ((A ^ temp) & 0x80);
                if(temp > 0x99) temp += 96;
                CARRY = temp > 0x99; 
            }
            else
            {
                NEGATIVE = temp & 0x80;
                OVERFLOWBIT = !((A ^ data) & 0x80) && ((A ^ temp) & 0x80);
                CARRY = temp > 0xFF;
            }

            A = temp & 0xFF;
        };

        const function<OPEXEC(ADDRESS)> AND = [this](ADDRESS source)
        {
            A = A | memory.readFromMemory(source);
            ZERO = !A;
            NEGATIVE = A & 0x80;
        };

        const function<OPEXEC(ADDRESS)> ASL = [this](ADDRESS source)
        {
            uint8_t data = memory.readFromMemory(source);
            CARRY = data & 0x80;
            data <<= 1;
            data &= 0xFF;
            NEGATIVE = data & 0x80;
            ZERO = !data;
            memory.writeToMemory(source,data);
        };

        const function<OPEXEC(ADDRESS)> ASL_ACC = [this](ADDRESS source)
        {
            CARRY = A & 0x80;
            A <<= 1;
            A &= 0xFF;
            NEGATIVE = A & 0x80;
            ZERO = !A;
        };

        const function<OPEXEC(ADDRESS)> BCC = [this](ADDRESS source)
        {
            if(!CARRY)
                programCounter = source;
        };

        const function<OPEXEC(ADDRESS)> BCS = [this](ADDRESS source)
        {
            if(CARRY)
                programCounter = source;
        };

        const function<OPEXEC(ADDRESS)> BEQ = [this](ADDRESS source)
        {
            if(ZERO)
                programCounter = source;
        };

        const function<OPEXEC(ADDRESS)> BIT = [this](ADDRESS source)
        {
            uint16_t data = memory.readFromMemory(source) & A;
            ZERO = !data;
            NEGATIVE = data & 0x80;
            
        };

        const function<OPEXEC(ADDRESS)> BMI = [this](ADDRESS source)
        {
            if(NEGATIVE)
                programCounter = source;
        };

        const function<OPEXEC(ADDRESS)> BNE = [this](ADDRESS source)
        {
            if(!ZERO)
                programCounter = source;
        };

        const function<OPEXEC(ADDRESS)> BPL = [this](ADDRESS source)
        {
            if(!NEGATIVE)
                programCounter = source;
        };

        const function<OPEXEC(ADDRESS)> BRK = [this](ADDRESS source)
        {
            uint8_t flagByte = 0xFF;
            flagByte |= 1UL << BREAK_BIT;
            CARRY ? flagByte |= 1UL << CARRY_BIT : flagByte &= ~(1UL << CARRY_BIT);
            ZERO ? flagByte |= 1UL << ZERO_BIT : flagByte &= ~(1UL << ZERO_BIT);
            INTERRUPT_DISABLE ? flagByte |= 1UL << INTERRUPT_DISABLE_BIT : flagByte &= ~(1UL << INTERRUPT_DISABLE_BIT);
            DECIMAL ? flagByte |= 1UL << DECIMAL_MODE_BIT : flagByte &= ~(1UL << DECIMAL_MODE_BIT);
            OVERFLOWBIT ? flagByte |= 1UL << OVERFLOW_BIT : flagByte &= ~(1UL << OVERFLOW_BIT);
            NEGATIVE ? flagByte |= 1UL << NEGATIVE_BIT : flagByte &= ~(1UL << NEGATIVE_BIT);
            programCounter++;
            push((programCounter >> 8) & 0xFF);
            push(programCounter & 0xFF);
            push(flagByte);

            INTERRUPT_DISABLE = 1;
            programCounter = (memory.readFromMemory(IRQVECTOR_H) << 8) + memory.readFromMemory(IRQVECTOR_L);
        };

        const function<OPEXEC(ADDRESS)> BVC = [this](ADDRESS source)
        {
            if(!OVERFLOWBIT)
                programCounter = source;
        };

        const function<OPEXEC(ADDRESS)> BVS = [this](ADDRESS source)
        {
            if(OVERFLOWBIT)
                programCounter = source;
        };

        const function<OPEXEC(ADDRESS)> CLC = [this](ADDRESS source)
        {
            CARRY = 0;
        };

        const function<OPEXEC(ADDRESS)> CLD = [this](ADDRESS source)
        {
            DECIMAL = 0;
        };

        const function<OPEXEC(ADDRESS)> CLI = [this](ADDRESS source)
        {
            INTERRUPT_DISABLE = 0;
        };

        const function<OPEXEC(ADDRESS)> CLV = [this](ADDRESS source)
        {
            OVERFLOWBIT = 0;
        };

        const function<OPEXEC(ADDRESS)> CMP = [this](ADDRESS source)
        {
            uint8_t data = A - memory.readFromMemory(source);
            CARRY = (0x100 > data) ? 1 : 0;
            NEGATIVE = (0x80 & data) ? 1 : 0;
            ZERO = (data & 0xFF) ? 0 : 1;
        };

        const function<OPEXEC(ADDRESS)> CPX = [this](ADDRESS source)
        {
            uint8_t data = X - memory.readFromMemory(source);
            CARRY = (0x100 > data) ? 1 : 0;
            NEGATIVE = (0x80 & data) ? 1 : 0;
            ZERO = (data & 0xFF) ? 0 : 1;
        };

        const function<OPEXEC(ADDRESS)> CPY = [this](ADDRESS source)
        {
            uint8_t data = Y - memory.readFromMemory(source);
            CARRY = (0x100 > data) ? 1 : 0;
            NEGATIVE = (0x80 & data) ? 1 : 0;
            ZERO = (data & 0xFF) ? 0 : 1;
        };

        const function<OPEXEC(ADDRESS)> DEC = [this](ADDRESS source)
        {
            uint8_t data = memory.readFromMemory(source) - 1;
            NEGATIVE = data & 0x80;
            ZERO = !data;
            memory.writeToMemory(source,data);
        };

        const function<OPEXEC(ADDRESS)> DEX = [this](ADDRESS source)
        {
            uint8_t data = X - 1;
            NEGATIVE = data & 0x80;
            ZERO = !data;
            X = data;
        };

        const function<OPEXEC(ADDRESS)> DEY = [this](ADDRESS source)
        {
            uint8_t data = Y - 1;
            NEGATIVE = data & 0x80;
            ZERO = !data;
            Y = data;
        };

        const function<OPEXEC(ADDRESS)> EOR = [this](ADDRESS source)
        {
            uint8_t data = A ^ memory.readFromMemory(source);
            NEGATIVE = data & 0x80;
            ZERO = data == 0 ? 1 : 0;
            A = data;
        };

        const function<OPEXEC(ADDRESS)> INC = [this](ADDRESS source)
        {
            uint8_t data = (memory.readFromMemory(source) + 1) % 256;
            ZERO = !data;
            NEGATIVE = data & 0x80;
            memory.writeToMemory(source,data);
        };

        const function<OPEXEC(ADDRESS)> INX_OP = [this](ADDRESS source)
        {
            X = (X + 1) % 256;
            ZERO = !X;
            NEGATIVE = X & 0x80;
        };

        const function<OPEXEC(ADDRESS)> INY_OP = [this](ADDRESS source)
        {
            Y = (Y + 1) % 256;
            ZERO = !Y;
            NEGATIVE = Y & 0x80;
        };

        const function<OPEXEC(ADDRESS)> JMP = [this](ADDRESS source)
        {
            programCounter = source;
        };

        const function<OPEXEC(ADDRESS)> JSR = [this](ADDRESS source)
        {
            programCounter--;
            push((programCounter >> 8) &  0xFF);
            push(programCounter & 0xFF);
            programCounter = source;
        };

        const function<OPEXEC(ADDRESS)> LDA = [this](ADDRESS source)
        {
            A = memory.readFromMemory(source);
            ZERO = !A;
            NEGATIVE = 	A & 0x80;
        };

        const function<OPEXEC(ADDRESS)> LDX = [this](ADDRESS source)
        {
            X = memory.readFromMemory(source);
            ZERO = !X;
            NEGATIVE = 	X & 0x80;
        };

        const function<OPEXEC(ADDRESS)> LDY = [this](ADDRESS source)
        {
            Y = memory.readFromMemory(source);
            ZERO = !Y;
            NEGATIVE = 	Y & 0x80;
        };

        const function<OPEXEC(ADDRESS)> LSR = [this](ADDRESS source)
        {
            uint8_t data = memory.readFromMemory(source);
            CARRY = data & 0x01;
            data >>= 1;
            ZERO = !data;
            NEGATIVE = 0;
            memory.writeToMemory(source,data);
        };

        const function<OPEXEC(ADDRESS)> LSR_ACC = [this](ADDRESS source)
        {
            CARRY = A & 0x01;
            A >>= 1;
            ZERO = !A;
            NEGATIVE = 0;
        };

        const function<OPEXEC(ADDRESS)> NOP = [this](ADDRESS source) { };;

        const function<OPEXEC(ADDRESS)> ORA = [this](ADDRESS source)
        {
            A = memory.readFromMemory(source) | A;
            ZERO = !A;
            NEGATIVE = A & 0x80;
        };

        const function<OPEXEC(ADDRESS)> PHA = [this](ADDRESS source)
        {
            push(A);
        };

        const function<OPEXEC(ADDRESS)> PHP = [this](ADDRESS source)
        {
            uint8_t flagByte = 0xFF;
            flagByte |= 1UL << BREAK_BIT;
            CARRY ? flagByte |= 1UL << CARRY_BIT : flagByte &= ~(1UL << CARRY_BIT);
            ZERO ? flagByte |= 1UL << ZERO_BIT : flagByte &= ~(1UL << ZERO_BIT);
            INTERRUPT_DISABLE ? flagByte |= 1UL << INTERRUPT_DISABLE_BIT : flagByte &= ~(1UL << INTERRUPT_DISABLE_BIT);
            DECIMAL ? flagByte |= 1UL << DECIMAL_MODE_BIT : flagByte &= ~(1UL << DECIMAL_MODE_BIT);
            OVERFLOWBIT ? flagByte |= 1UL << OVERFLOW_BIT : flagByte &= ~(1UL << OVERFLOW_BIT);
            NEGATIVE ? flagByte |= 1UL << NEGATIVE_BIT : flagByte &= ~(1UL << NEGATIVE_BIT);

            push(flagByte);	
        };

        const function<OPEXEC(ADDRESS)> PLA = [this](ADDRESS source)
        {
            A = pop();
        };

        const function<OPEXEC(ADDRESS)> PLP = [this](ADDRESS source)
        {
            uint8_t data = pop();
            CARRY = (data >> CARRY_BIT) & 1;
            ZERO = (data >> ZERO_BIT) & 1;
            DECIMAL = (data >> DECIMAL_MODE_BIT) & 1;
            BREAK = (data >> BREAK_BIT) & 1;
            OVERFLOWBIT = (data >> OVERFLOW_BIT) & 1;
            NEGATIVE = (data >> NEGATIVE_BIT) & 1;	
        };

        const function<OPEXEC(ADDRESS)> ROL = [this](ADDRESS source)
        {
            uint8_t data = memory.readFromMemory(source);
            data <<= 1;
            if(CARRY) data |= 0x01;
            CARRY = data > 0xFF;
            data &= 0xFF;
            ZERO = !data;
            NEGATIVE = data & 0x80;
            memory.writeToMemory(source,data);
        };


        const function<OPEXEC(ADDRESS)> ROL_ACC = [this](ADDRESS source)
        {
            A <<= 1;
            if(CARRY) A |= 0x01;
            CARRY = A > 0xFF;
            A &= 0xFF;
            ZERO = !A;
            NEGATIVE = A & 0x80;
        };

        const function<OPEXEC(ADDRESS)> ROR = [this](ADDRESS source)
        {
            uint8_t data = memory.readFromMemory(source);
            if(CARRY) data |= 0x100;
            CARRY = data & 0x01;
            data >>= 1;
            data &= 0xFF;
            NEGATIVE = data & 0x80;
            ZERO = !data;
            memory.writeToMemory(source,data);
        };

        const function<OPEXEC(ADDRESS)> ROR_ACC = [this](ADDRESS source)
        {
            uint8_t data = A;
            if(CARRY) data |= 0x100;
            CARRY = data & 0x01;
            data >>= 1;
            data &= 0xFF;
            NEGATIVE = data & 0x80;
            ZERO = !data;
            A = data;
        };

        const function<OPEXEC(ADDRESS)> RTI = [this](ADDRESS source)
        {
            uint8_t low,high,flagByte;
            flagByte = pop();
            
            CARRY = (flagByte >> CARRY_BIT) & 1;
            ZERO = (flagByte >> ZERO_BIT) & 1;
            DECIMAL = (flagByte >> DECIMAL_MODE_BIT) & 1;
            BREAK = (flagByte >> BREAK_BIT) & 1;
            OVERFLOWBIT = (flagByte >> OVERFLOW_BIT) & 1;
            NEGATIVE = (flagByte >> NEGATIVE_BIT) & 1;	

            low = pop();
            high = pop();

            programCounter = (high << 8) | low; 
        };

        const function<OPEXEC(ADDRESS)> RTS = [this](ADDRESS source)
        {
            uint8_t low,high;

            low = pop();
            high = pop();

            programCounter = ((high << 8) | low) + 1;
        };

        const function<OPEXEC(ADDRESS)> SBC = [this](ADDRESS source)
        {
            uint8_t data = memory.readFromMemory(source);
            uint32_t temp = A - data - (CARRY ? 1 : 0);
            NEGATIVE = temp & 0x80;
            ZERO = !(temp & 0xFF);
            OVERFLOWBIT = ((A ^ temp) & 0x80) && ((A ^ data) & 0x80);

            if(DECIMAL)
            {
                if( ((A & 0x0F)  - CARRY)  < (data & 0x0F)) temp -= 6;
                if(temp > 0x99)
                    temp -= 0x60; 
            };  
            CARRY = temp < 0x100;
            A = (temp & 0xFF);
        };

        const function<OPEXEC(ADDRESS)> SEC = [this](ADDRESS source)
        {
            CARRY = 1;
        };

        const function<OPEXEC(ADDRESS)> SED = [this](ADDRESS source)
        {
            DECIMAL = 1;
        };

        const function<OPEXEC(ADDRESS)> SEI = [this](ADDRESS source)
        {
            INTERRUPT_DISABLE = 1;
        };

        const function<OPEXEC(ADDRESS)> STA = [this](ADDRESS source)
        {
            memory.writeToMemory(source,A);
        };

        const function<OPEXEC(ADDRESS)> STX = [this](ADDRESS source)
        {
            memory.writeToMemory(source,X);
        };

        const function<OPEXEC(ADDRESS)> STY = [this](ADDRESS source)
        {
            memory.writeToMemory(source,Y);
        };

        const function<OPEXEC(ADDRESS)> TAX = [this](ADDRESS source)
        {
            X = A;
            ZERO = !X;
            NEGATIVE = X & 0x80;
        };

        const function<OPEXEC(ADDRESS)> TAY = [this](ADDRESS source)
        {
            Y = A;
            ZERO = !Y;
            NEGATIVE = Y & 0x80;
        };

        const function<OPEXEC(ADDRESS)> TSX = [this](ADDRESS source)
        {
            X = SP;
            ZERO = !X;
            NEGATIVE = X & 0x80;
        };

        const function<OPEXEC(ADDRESS)> TXA = [this](ADDRESS source)
        {
            A = X;
            ZERO = !X;
            NEGATIVE = X & 0x80;
        };

        const function<OPEXEC(ADDRESS)> TYA = [this](ADDRESS source)
        {
            A = Y;
            ZERO = !Y;
            NEGATIVE = Y & 0x80;
        };

        const function<OPEXEC(ADDRESS)> TXS = [this](ADDRESS source)
        {
            SP = X;	
        };

        const function<OPEXEC(ADDRESS)> ILLEGAL = [this](ADDRESS source)
        {
            exit(1);
        };
      
        /*------------ADDRESSING MODES--------*/
        const function<ADDRESS()> ACC = [this]() { return A; }; // ACCUMULATOR
        const function<ADDRESS()> IMM = [this]() { return programCounter++; }; // IMMEDIATE
        const function<ADDRESS()> ABS = [this]() { uint16_t addrLower = memory.readFromMemory(programCounter++),
                                                        addrHigher = memory.readFromMemory(programCounter++); 
                                                        return addrLower + (addrHigher << 8); };  // ABSOLUTE
        const function<ADDRESS()> ZER = [this]() { return memory.readFromMemory(programCounter++); }; // ZERO PAGE
        const function<ADDRESS()> ZEX = [this]() { return (memory.readFromMemory(programCounter++) + X) % 256; }; // INDEXED-X ZERO PAGE
        const function<ADDRESS()> ZEY = [this]() { return (memory.readFromMemory(programCounter++) + Y) % 256; }; // INDEXED-Y ZERO PAGE
        const function<ADDRESS()> ABX = [this]() { return ABS() + X; }; // INDEXED-X ABSOLUTE
        const function<ADDRESS()> ABY = [this]() { return ABS() + X; }; // INDEXED-Y ABSOLUTE
        const function<ADDRESS()> IMP = [this]() { return 0; }; // IMPLIED
        const function<ADDRESS()> REL = [this]() { uint16_t offset = (uint16_t) memory.readFromMemory(programCounter++); 
                                                        if(offset & 0x80) offset |= 0xFF00; 
                                                        return programCounter + (int16_t) offset; }; // RELATIVE
        const function<ADDRESS()> INX = [this]() { uint16_t zeroLower = ZEX(),zeroHigher = (zeroLower + 1) % 256; 
                                                        return memory.readFromMemory(zeroLower) + (memory.readFromMemory(zeroHigher) << 8); }; // INDEXED-X INDIRECT
        const function<ADDRESS()> INY = [this]() { uint16_t zeroLower = memory.readFromMemory(programCounter++),
                                                        zeroHigher = (zeroLower + 1) % 256; 
                                                        return memory.readFromMemory(zeroLower) + (memory.readFromMemory(zeroHigher) << 8) + Y; }; // INDEXED-Y INDIRECT
        const function<ADDRESS()> ABI = [this]() { uint16_t addressLower = memory.readFromMemory(programCounter++),
                                                        addressHigher = memory.readFromMemory(programCounter++),
                                                        abs = (addressHigher << 8) | addressLower,
                                                        effLower = memory.readFromMemory(abs),
                                                        effHigher = memory.readFromMemory((abs & 0xFF00) + ((abs + 1) & 0x00FF));
                                                        return effLower + 0x100 * effHigher; }; // ABSOLUTE INDIRECT


};


#endif
