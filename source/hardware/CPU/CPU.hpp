#ifndef CPU_HPP
#define CPU_HPP

#include "Instruction.hpp"
#include "pch.hpp"

namespace nes
{
   class CPU final
   {
      enum class Opcode : std::uint8_t
      {
         BRK_implied = 0x00,
         ORA_x_indirect = 0x01,
         ORA_zero_page = 0x05,
         ASL_zero_page = 0x06,
         PHP_implied = 0x08,
         ORA_immediate = 0x09,
         ASL_accumulator = 0x0A,
         ORA_absolute = 0x0D,
         ASL_absolute = 0x0E,
         BPL_relative = 0x10,
         ORA_indirect_y = 0x11,
         ORA_zero_page_x = 0x15,
         ASL_zero_page_x = 0x16,
         CLC_implied = 0x18,
         ORA_absolute_y = 0x19,
         ORA_absolute_x = 0x1D,
         ASL_absolute_x = 0x1E,
         JSR_absolute = 0x20,
         AND_x_indirect = 0x21,
         BIT_zero_page = 0x24,
         AND_zero_page = 0x25,
         ROL_zero_page = 0x26,
         PLP_implied = 0x28,
         AND_immediate = 0x29,
         ROL_accumulator = 0x2A,
         BIT_absolute = 0x2C,
         AND_absolute = 0x2D,
         ROL_absolute = 0x2E,
         BMI_relative = 0x30,
         AND_indirect_y = 0x31,
         AND_zero_page_x = 0x35,
         ROL_zero_page_x = 0x36,
         SEC_implied = 0x38,
         AND_absolute_y = 0x39,
         AND_absolute_x = 0x3D,
         ROL_absolute_x = 0x3E,
         RTI_implied = 0x40,
         EOR_x_indirect = 0x41,
         EOR_zero_page = 0x45,
         LSR_zero_page = 0x46,
         PHA_implied = 0x48,
         EOR_immediate = 0x49,
         LSR_accumulator = 0x4A,
         JMP_absolute = 0x4C,
         EOR_absolute = 0x4D,
         LSR_absolute = 0x4E,
         BVC_relative = 0x50,
         EOR_indirect_y = 0x51,
         EOR_zero_page_x = 0x55,
         LSR_zero_page_x = 0x56,
         CLI_implied = 0x58,
         EOR_absolute_y = 0x59,
         EOR_absolute_x = 0x5D,
         LSR_absolute_x = 0x5E,
         RTS_implied = 0x60,
         ADC_x_indirect = 0x61,
         ADC_zero_page = 0x65,
         ROR_zero_page = 0x66,
         PLA_implied = 0x68,
         ADC_immediate = 0x69,
         ROR_accumulator = 0x6A,
         JMP_indirect = 0x6C,
         ADC_absolute = 0x6D,
         ROR_absolute = 0x6E,
         BVS_relative = 0x70,
         ADC_indirect_y = 0x71,
         ADC_zero_page_x = 0x75,
         ROR_zero_page_x = 0x76,
         SEI_implied = 0x78,
         ADC_absolute_y = 0x79,
         ADC_absolute_x = 0x7D,
         ROR_absolute_x = 0x7E,
         STA_x_indirect = 0x81,
         STY_zero_page = 0x84,
         STA_zero_page = 0x85,
         STX_zero_page = 0x86,
         DEY_implied = 0x88,
         TXA_implied = 0x8A,
         STY_absolute = 0x8C,
         STA_absolute = 0x8D,
         STX_absolute = 0x8E,
         BCC_relative = 0x90,
         STA_indirect_y = 0x91,
         STY_zero_page_x = 0x94,
         STA_zero_page_x = 0x95,
         STX_zero_page_y = 0x96,
         TYA_implied = 0x98,
         STA_absolute_y = 0x99,
         TXS_implied = 0x9A,
         STA_absolute_x = 0x9D,
         STX_absolute_y = 0x9E,
         LDY_immediate = 0xA0,
         LDA_x_indirect = 0xA1,
         LDX_immediate = 0xA2,
         LDY_zero_page = 0xA4,
         LDA_zero_page = 0xA5,
         LDX_zero_page = 0xA6,
         TAY_implied = 0xA8,
         LDA_immediate = 0xA9,
         TAX_implied = 0xAA,
         LDY_absolute = 0xAC,
         LDA_absolute = 0xAD,
         LDX_absolute = 0xAE,
         BCS_relative = 0xB0,
         LDA_indirect_y = 0xB1,
         LDX_zero_page_y = 0xB4,
         LDA_zero_page_x = 0xB5,
         LDX_zero_page_y_ = 0xB6,
         CLV_implied = 0xB8,
         LDA_absolute_y = 0xB9,
         TSX_implied = 0xBA,
         LDA_absolute_x = 0xBD,
         LDX_absolute_y = 0xBE,
         CPY_immediate = 0xC0,
         CMP_x_indirect = 0xC1,
         CPY_zero_page = 0xC4,
         CMP_zero_page = 0xC5,
         DEC_zero_page = 0xC6,
         INY_implied = 0xC8,
         CMP_immediate = 0xC9,
         DEX_implied = 0xCA,
         CPY_absolute = 0xCC,
         CMP_absolute = 0xCD,
         DEC_absolute = 0xCE,
         BNE_relative = 0xD0,
         CMP_indirect_y = 0xD1,
         CMP_zero_page_x = 0xD5,
         DEC_zero_page_x = 0xD6,
         CLD_implied = 0xD8,
         CMP_absolute_y = 0xD9,
         CMP_absolute_x = 0xDD,
         DEC_absolute_x = 0xDE,
         CPX_immediate = 0xE0,
         SBC_x_indirect = 0xE1,
         CPX_zero_page = 0xE4,
         SBC_zero_page = 0xE5,
         INC_zero_page = 0xE6,
         INX_implied = 0xE8,
         SBC_immediate = 0xE9,
         NOP_implied = 0xEA,
         CPX_absolute = 0xEC,
         SBC_absolute = 0xED,
         INC_absolute = 0xEE,
         BEQ_relative = 0xF0,
         SBC_indirect_y = 0xF1,
         SBC_zero_page_x = 0xF5,
         INC_zero_page_x = 0xF6,
         SED_implied = 0xF8,
         SBC_absolute_y = 0xF9,
         SBC_absolute_x = 0xFD,
         INC_absolute_x = 0xFE
      };

      enum class ProcessorStatusFlag : std::uint8_t
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

      public:
         using Memory = std::array<std::uint8_t, 65'536>;

         explicit CPU(Memory& memory);
         CPU(CPU const&) = delete;
         CPU(CPU&&) = default;

         ~CPU() = default;

         CPU& operator=(CPU const&) = delete;
         CPU& operator=(CPU&&) = delete;

         [[nodiscard]] bool tick();
         [[nodiscard]] std::uint16_t cycle() const;

      private:
         void change_processor_status_flag(ProcessorStatusFlag flag, bool set);

         [[nodiscard]] Instruction BRK_implied();

         Memory& memory_;

         std::uint16_t cycle_ = 0;
         std::uint16_t program_counter_ = 0x0000;
         std::uint8_t accumulator_ = 0x00;
         std::uint8_t x_ = 0x00;
         std::uint8_t y_ = 0x00;
         std::uint8_t stack_pointer_ = 0xFF;
         std::uint8_t processor_status_ = 0b00000000;

         std::optional<Instruction> current_instruction_;
   };
}

#endif