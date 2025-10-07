#ifndef CPU_HPP
#define CPU_HPP

#include "hardware/memory/memory.hpp"
#include "instruction.hpp"
#include "pch.hpp"

namespace nes
{
   class CPU final
   {
      enum class Opcode : Data
      {
         BRK_IMPLIED = 0x00,
         ORA_X_INDIRECT = 0x01,
         ORA_ZERO_PAGE = 0x05,
         ASL_ZERO_PAGE = 0x06,
         PHP_IMPLIED = 0x08,
         ORA_IMMEDIATE = 0x09,
         ASL_ACCUMULATOR = 0x0A,
         ORA_ABSOLUTE = 0x0D,
         ASL_ABSOLUTE = 0x0E,
         BPL_RELATIVE = 0x10,
         ORA_INDIRECT_Y = 0x11,
         ORA_ZERO_PAGE_x = 0x15,
         ASL_ZERO_PAGE_x = 0x16,
         CLC_IMPLIED = 0x18,
         ORA_ABSOLUTE_y = 0x19,
         ORA_ABSOLUTE_x = 0x1D,
         ASL_ABSOLUTE_x = 0x1E,
         JSR_ABSOLUTE = 0x20,
         AND_X_INDIRECT = 0x21,
         BIT_ZERO_PAGE = 0x24,
         AND_ZERO_PAGE = 0x25,
         ROL_ZERO_PAGE = 0x26,
         PLP_IMPLIED = 0x28,
         AND_IMMEDIATE = 0x29,
         ROL_ACCUMULATOR = 0x2A,
         BIT_ABSOLUTE = 0x2C,
         AND_ABSOLUTE = 0x2D,
         ROL_ABSOLUTE = 0x2E,
         BMI_RELATIVE = 0x30,
         AND_INDIRECT_Y = 0x31,
         AND_ZERO_PAGE_x = 0x35,
         ROL_ZERO_PAGE_x = 0x36,
         SEC_IMPLIED = 0x38,
         AND_ABSOLUTE_y = 0x39,
         AND_ABSOLUTE_x = 0x3D,
         ROL_ABSOLUTE_x = 0x3E,
         RTI_IMPLIED = 0x40,
         EOR_X_INDIRECT = 0x41,
         EOR_ZERO_PAGE = 0x45,
         LSR_ZERO_PAGE = 0x46,
         PHA_IMPLIED = 0x48,
         EOR_IMMEDIATE = 0x49,
         LSR_ACCUMULATOR = 0x4A,
         JMP_ABSOLUTE = 0x4C,
         EOR_ABSOLUTE = 0x4D,
         LSR_ABSOLUTE = 0x4E,
         BVC_RELATIVE = 0x50,
         EOR_INDIRECT_Y = 0x51,
         EOR_ZERO_PAGE_x = 0x55,
         LSR_ZERO_PAGE_x = 0x56,
         CLI_IMPLIED = 0x58,
         EOR_ABSOLUTE_y = 0x59,
         EOR_ABSOLUTE_x = 0x5D,
         LSR_ABSOLUTE_x = 0x5E,
         RTS_IMPLIED = 0x60,
         ADC_X_INDIRECT = 0x61,
         ADC_ZERO_PAGE = 0x65,
         ROR_ZERO_PAGE = 0x66,
         PLA_IMPLIED = 0x68,
         ADC_IMMEDIATE = 0x69,
         ROR_ACCUMULATOR = 0x6A,
         JMP_INDIRECT = 0x6C,
         ADC_ABSOLUTE = 0x6D,
         ROR_ABSOLUTE = 0x6E,
         BVS_RELATIVE = 0x70,
         ADC_INDIRECT_Y = 0x71,
         ADC_ZERO_PAGE_x = 0x75,
         ROR_ZERO_PAGE_x = 0x76,
         SEI_IMPLIED = 0x78,
         ADC_ABSOLUTE_y = 0x79,
         ADC_ABSOLUTE_x = 0x7D,
         ROR_ABSOLUTE_x = 0x7E,
         STA_X_INDIRECT = 0x81,
         STY_ZERO_PAGE = 0x84,
         STA_ZERO_PAGE = 0x85,
         STX_ZERO_PAGE = 0x86,
         DEY_IMPLIED = 0x88,
         TXA_IMPLIED = 0x8A,
         STY_ABSOLUTE = 0x8C,
         STA_ABSOLUTE = 0x8D,
         STX_ABSOLUTE = 0x8E,
         BCC_RELATIVE = 0x90,
         STA_INDIRECT_Y = 0x91,
         STY_ZERO_PAGE_x = 0x94,
         STA_ZERO_PAGE_x = 0x95,
         STX_ZERO_PAGE_y = 0x96,
         TYA_IMPLIED = 0x98,
         STA_ABSOLUTE_y = 0x99,
         TXS_IMPLIED = 0x9A,
         STA_ABSOLUTE_x = 0x9D,
         STX_ABSOLUTE_y = 0x9E,
         LDY_IMMEDIATE = 0xA0,
         LDA_X_INDIRECT = 0xA1,
         LDX_IMMEDIATE = 0xA2,
         LDY_ZERO_PAGE = 0xA4,
         LDA_ZERO_PAGE = 0xA5,
         LDX_ZERO_PAGE = 0xA6,
         TAY_IMPLIED = 0xA8,
         LDA_IMMEDIATE = 0xA9,
         TAX_IMPLIED = 0xAA,
         LDY_ABSOLUTE = 0xAC,
         LDA_ABSOLUTE = 0xAD,
         LDX_ABSOLUTE = 0xAE,
         BCS_RELATIVE = 0xB0,
         LDA_INDIRECT_Y = 0xB1,
         LDX_ZERO_PAGE_y = 0xB4,
         LDA_ZERO_PAGE_x = 0xB5,
         LDX_ZERO_PAGE_y_ = 0xB6,
         CLV_IMPLIED = 0xB8,
         LDA_ABSOLUTE_y = 0xB9,
         TSX_IMPLIED = 0xBA,
         LDA_ABSOLUTE_x = 0xBD,
         LDX_ABSOLUTE_y = 0xBE,
         CPY_IMMEDIATE = 0xC0,
         CMP_X_INDIRECT = 0xC1,
         CPY_ZERO_PAGE = 0xC4,
         CMP_ZERO_PAGE = 0xC5,
         DEC_ZERO_PAGE = 0xC6,
         INY_IMPLIED = 0xC8,
         CMP_IMMEDIATE = 0xC9,
         DEX_IMPLIED = 0xCA,
         CPY_ABSOLUTE = 0xCC,
         CMP_ABSOLUTE = 0xCD,
         DEC_ABSOLUTE = 0xCE,
         BNE_RELATIVE = 0xD0,
         CMP_INDIRECT_Y = 0xD1,
         CMP_ZERO_PAGE_x = 0xD5,
         DEC_ZERO_PAGE_x = 0xD6,
         CLD_IMPLIED = 0xD8,
         CMP_ABSOLUTE_y = 0xD9,
         CMP_ABSOLUTE_x = 0xDD,
         DEC_ABSOLUTE_x = 0xDE,
         CPX_IMMEDIATE = 0xE0,
         SBC_X_INDIRECT = 0xE1,
         CPX_ZERO_PAGE = 0xE4,
         SBC_ZERO_PAGE = 0xE5,
         INC_ZERO_PAGE = 0xE6,
         INX_IMPLIED = 0xE8,
         SBC_IMMEDIATE = 0xE9,
         NOP_IMPLIED = 0xEA,
         CPX_ABSOLUTE = 0xEC,
         SBC_ABSOLUTE = 0xED,
         INC_ABSOLUTE = 0xEE,
         BEQ_RELATIVE = 0xF0,
         SBC_INDIRECT_Y = 0xF1,
         SBC_ZERO_PAGE_x = 0xF5,
         INC_ZERO_PAGE_x = 0xF6,
         SED_IMPLIED = 0xF8,
         SBC_ABSOLUTE_y = 0xF9,
         SBC_ABSOLUTE_x = 0xFD,
         INC_ABSOLUTE_x = 0xFE
      };

      public:
         enum class ProcessorStatusFlag : ProcessorStatus
         {
            C = 0b00000001,
            Z = 0b00000010,
            I = 0b00000100,
            D = 0b00001000,
            B = 0b00010000,
            _ = 0b00100000,
            V = 0b01000000,
            N = 0b10000000
         };

         static auto constexpr FREQUENCY = 1'789'773;

         explicit CPU(Memory& memory);
         CPU(CPU const&) = delete;
         CPU(CPU&&) = delete;

         ~CPU() = default;

         CPU& operator=(CPU const&) = delete;
         CPU& operator=(CPU&&) = delete;

         void tick();

         [[nodiscard]] Cycle cycle() const;
         [[nodiscard]] ProgramCounter program_counter() const;
         [[nodiscard]] Accumulator accumulator() const;
         [[nodiscard]] X x() const;
         [[nodiscard]] Y y() const;
         [[nodiscard]] StackPointer stack_pointer() const;
         [[nodiscard]] ProcessorStatus processor_status() const;

      private:
         void change_processor_status_flag(ProcessorStatusFlag flag, bool set);
         void push(Data value);
         Data pop();

         [[nodiscard]] Instruction reset();
         [[nodiscard]] Instruction brk_implied();
         [[nodiscard]] Instruction ora_x_indirect();

         Memory& memory_;

         Cycle cycle_ = 0;
         ProgramCounter program_counter_ = 0x0000;
         Accumulator accumulator_ = 0x00;
         Index x_ = 0x00;
         Index y_ = 0x00;
         StackPointer stack_pointer_ = 0xFF;
         ProcessorStatus processor_status_ = 0b00000000;

         std::optional<Instruction> current_instruction_;
   };
}

#endif