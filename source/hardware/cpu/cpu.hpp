#ifndef CPU_HPP
#define CPU_HPP

#include "hardware/memory/memory.hpp"
#include "instruction.hpp"
#include "pch.hpp"

namespace nes
{
   class Processor final
   {
      using BranchOperation = bool(Processor::*)() const noexcept;
      using ReadOperation = void(Processor::*)(Data) noexcept;
      using ModifyOperation = Data(Processor::*)(Data) noexcept;
      using WriteOperation = void(Processor::*)(Address) noexcept;

      public:
         enum class Opcode : Data
         {
            BRK_IMPLIED      = 0x00,
            ORA_X_INDIRECT   = 0x01,
            JAM_IMPLIED_02   = 0x02,
            SLO_X_INDIRECT   = 0x03,
            NOP_ZERO_PAGE_04 = 0x04,
            ORA_ZERO_PAGE    = 0x05,
            ASL_ZERO_PAGE    = 0x06,
            SLO_ZERO_PAGE    = 0x07,
            PHP_IMPLIED      = 0x08,
            ORA_IMMEDIATE    = 0x09,
            ASL_ACCUMULATOR  = 0x0A,
            ANC_IMMEDIATE_0B = 0x0B,
            NOP_ABSOLUTE     = 0x0C,
            ORA_ABSOLUTE     = 0x0D,
            ASL_ABSOLUTE     = 0x0E,
            SLO_ABSOLUTE     = 0x0F,

            BPL_RELATIVE       = 0x10,
            ORA_INDIRECT_Y     = 0x11,
            JAM_IMPLIED_12     = 0x12,
            SLO_INDIRECT_Y     = 0x13,
            NOP_ZERO_PAGE_X_14 = 0x14,
            ORA_ZERO_PAGE_X    = 0x15,
            ASL_ZERO_PAGE_X    = 0x16,
            SLO_ZERO_PAGE_X    = 0x17,
            CLC_IMPLIED        = 0x18,
            ORA_ABSOLUTE_Y     = 0x19,
            NOP_IMPLIED_1A     = 0x1A,
            SLO_ABSOLUTE_Y     = 0x1B,
            NOP_ABSOLUTE_X_1C  = 0x1C,
            ORA_ABSOLUTE_X     = 0x1D,
            ASL_ABSOLUTE_X     = 0x1E,
            SLO_ABSOLUTE_X     = 0x1F,

            JSR_ABSOLUTE     = 0x20,
            AND_X_INDIRECT   = 0x21,
            JAM_IMPLIED_22   = 0x22,
            RLA_X_INDIRECT   = 0x23,
            BIT_ZERO_PAGE    = 0x24,
            AND_ZERO_PAGE    = 0x25,
            ROL_ZERO_PAGE    = 0x26,
            RLA_ZERO_PAGE    = 0x27,
            PLP_IMPLIED      = 0x28,
            AND_IMMEDIATE    = 0x29,
            ROL_ACCUMULATOR  = 0x2A,
            ANC_IMMEDIATE_2B = 0x2B,
            BIT_ABSOLUTE     = 0x2C,
            AND_ABSOLUTE     = 0x2D,
            ROL_ABSOLUTE     = 0x2E,
            RLA_ABSOLUTE     = 0x2F,

            BMI_RELATIVE       = 0x30,
            AND_INDIRECT_Y     = 0x31,
            JAM_IMPLIED_32     = 0x32,
            RLA_INDIRECT_Y     = 0x33,
            NOP_ZERO_PAGE_X_34 = 0x34,
            AND_ZERO_PAGE_X    = 0x35,
            ROL_ZERO_PAGE_X    = 0x36,
            RLA_ZERO_PAGE_X    = 0x37,
            SEC_IMPLIED        = 0x38,
            AND_ABSOLUTE_Y     = 0x39,
            NOP_IMPLIED_3A     = 0x3A,
            RLA_ABSOLUTE_Y     = 0x3B,
            NOP_ABSOLUTE_X_3C  = 0x3C,
            AND_ABSOLUTE_X     = 0x3D,
            ROL_ABSOLUTE_X     = 0x3E,
            RLA_ABSOLUTE_X     = 0x3F,

            RTI_IMPLIED      = 0x40,
            EOR_X_INDIRECT   = 0x41,
            JAM_IMPLIED_42   = 0x42,
            SRE_X_INDIRECT   = 0x43,
            NOP_ZERO_PAGE_44 = 0x44,
            EOR_ZERO_PAGE    = 0x45,
            LSR_ZERO_PAGE    = 0x46,
            SRE_ZERO_PAGE    = 0x47,
            PHA_IMPLIED      = 0x48,
            EOR_IMMEDIATE    = 0x49,
            LSR_ACCUMULATOR  = 0x4A,
            ALR_IMMEDIATE_4B = 0x4B,
            JMP_ABSOLUTE     = 0x4C,
            EOR_ABSOLUTE     = 0x4D,
            LSR_ABSOLUTE     = 0x4E,
            SRE_ABSOLUTE     = 0x4F,

            BVC_RELATIVE       = 0x50,
            EOR_INDIRECT_Y     = 0x51,
            JAM_IMPLIED_52     = 0x52,
            SRE_INDIRECT_Y     = 0x53,
            NOP_ZERO_PAGE_X_54 = 0x54,
            EOR_ZERO_PAGE_X    = 0x55,
            LSR_ZERO_PAGE_X    = 0x56,
            SRE_ZERO_PAGE_X    = 0x57,
            CLI_IMPLIED        = 0x58,
            EOR_ABSOLUTE_Y     = 0x59,
            NOP_IMPLIED_5A     = 0x5A,
            SRE_ABSOLUTE_Y     = 0x5B,
            NOP_ABSOLUTE_X_5C  = 0x5C,
            EOR_ABSOLUTE_X     = 0x5D,
            LSR_ABSOLUTE_X     = 0x5E,
            SRE_ABSOLUTE_X     = 0x5F,

            RTS_IMPLIED      = 0x60,
            ADC_X_INDIRECT   = 0x61,
            JAM_IMPLIED_62   = 0x62,
            RRA_X_INDIRECT   = 0x63,
            NOP_ZERO_PAGE_64 = 0x64,
            ADC_ZERO_PAGE    = 0x65,
            ROR_ZERO_PAGE    = 0x66,
            RRA_ZERO_PAGE    = 0x67,
            PLA_IMPLIED      = 0x68,
            ADC_IMMEDIATE    = 0x69,
            ROR_ACCUMULATOR  = 0x6A,
            ARR_IMMEDIATE_6B = 0x6B,
            JMP_INDIRECT     = 0x6C,
            ADC_ABSOLUTE     = 0x6D,
            ROR_ABSOLUTE     = 0x6E,
            RRA_ABSOLUTE     = 0x6F,

            BVS_RELATIVE       = 0x70,
            ADC_INDIRECT_Y     = 0x71,
            JAM_IMPLIED_72     = 0x72,
            RRA_INDIRECT_Y     = 0x73,
            NOP_ZERO_PAGE_X_74 = 0x74,
            ADC_ZERO_PAGE_X    = 0x75,
            ROR_ZERO_PAGE_X    = 0x76,
            RRA_ZERO_PAGE_X    = 0x77,
            SEI_IMPLIED        = 0x78,
            ADC_ABSOLUTE_Y     = 0x79,
            NOP_IMPLIED_7A     = 0x7A,
            RRA_ABSOLUTE_Y     = 0x7B,
            NOP_ABSOLUTE_X_7C  = 0x7C,
            ADC_ABSOLUTE_X     = 0x7D,
            ROR_ABSOLUTE_X     = 0x7E,
            RRA_ABSOLUTE_X     = 0x7F,

            NOP_IMMEDIATE_80 = 0x80,
            STA_X_INDIRECT   = 0x81,
            NOP_IMMEDIATE_82 = 0x82,
            SAX_X_INDIRECT   = 0x83,
            STY_ZERO_PAGE    = 0x84,
            STA_ZERO_PAGE    = 0x85,
            STX_ZERO_PAGE    = 0x86,
            SAX_ZERO_PAGE    = 0x87,
            DEY_IMPLIED      = 0x88,
            NOP_IMMEDIATE_89 = 0x89,
            TXA_IMPLIED      = 0x8A,
            ANE_IMMEDIATE    = 0x8B,
            STY_ABSOLUTE     = 0x8C,
            STA_ABSOLUTE     = 0x8D,
            STX_ABSOLUTE     = 0x8E,
            SAX_ABSOLUTE     = 0x8F,

            BCC_RELATIVE    = 0x90,
            STA_INDIRECT_Y  = 0x91,
            JAM_IMPLIED_92  = 0x92,
            SHA_INDIRECT_Y  = 0x93,
            STY_ZERO_PAGE_X = 0x94,
            STA_ZERO_PAGE_X = 0x95,
            STX_ZERO_PAGE_Y = 0x96,
            SAX_ZERO_PAGE_Y = 0x97,
            TYA_IMPLIED     = 0x98,
            STA_ABSOLUTE_Y  = 0x99,
            TXS_IMPLIED     = 0x9A,
            TAS_ABSOLUTE_Y  = 0x9B,
            SHY_ABSOLUTE_X  = 0x9C,
            STA_ABSOLUTE_X  = 0x9D,
            SHX_ABSOLUTE_Y  = 0x9E,
            SHA_ABSOLUTE_Y  = 0x9F,

            LDY_IMMEDIATE  = 0xA0,
            LDA_X_INDIRECT = 0xA1,
            LDX_IMMEDIATE  = 0xA2,
            LAX_X_INDIRECT = 0xA3,
            LDY_ZERO_PAGE  = 0xA4,
            LDA_ZERO_PAGE  = 0xA5,
            LDX_ZERO_PAGE  = 0xA6,
            LAX_ZERO_PAGE  = 0xA7,
            TAY_IMPLIED    = 0xA8,
            LDA_IMMEDIATE  = 0xA9,
            TAX_IMPLIED    = 0xAA,
            LXA_IMMEDIATE  = 0xAB,
            LDY_ABSOLUTE   = 0xAC,
            LDA_ABSOLUTE   = 0xAD,
            LDX_ABSOLUTE   = 0xAE,
            LAX_ABSOLUTE   = 0xAF,

            BCS_RELATIVE    = 0xB0,
            LDA_INDIRECT_Y  = 0xB1,
            JAM_IMPLIED_B2  = 0xB2,
            LAX_INDIRECT_Y  = 0xB3,
            LDY_ZERO_PAGE_X = 0xB4,
            LDA_ZERO_PAGE_X = 0xB5,
            LDX_ZERO_PAGE_Y = 0xB6,
            LAX_ZERO_PAGE_Y = 0xB7,
            CLV_IMPLIED     = 0xB8,
            LDA_ABSOLUTE_Y  = 0xB9,
            TSX_IMPLIED     = 0xBA,
            LAS_ABSOLUTE_Y  = 0xBB,
            LDY_ABSOLUTE_X  = 0xBC,
            LDA_ABSOLUTE_X  = 0xBD,
            LDX_ABSOLUTE_Y  = 0xBE,
            LAX_ABSOLUTE_Y  = 0xBF,

            CPY_IMMEDIATE    = 0xC0,
            CMP_X_INDIRECT   = 0xC1,
            NOP_IMMEDIATE_C2 = 0xC2,
            DCP_X_INDIRECT   = 0xC3,
            CPY_ZERO_PAGE    = 0xC4,
            CMP_ZERO_PAGE    = 0xC5,
            DEC_ZERO_PAGE    = 0xC6,
            DCP_ZERO_PAGE    = 0xC7,
            INY_IMPLIED      = 0xC8,
            CMP_IMMEDIATE    = 0xC9,
            DEX_IMPLIED      = 0xCA,
            SBX_IMMEDIATE    = 0xCB,
            CPY_ABSOLUTE     = 0xCC,
            CMP_ABSOLUTE     = 0xCD,
            DEC_ABSOLUTE     = 0xCE,
            DCP_ABSOLUTE     = 0xCF,

            BNE_RELATIVE       = 0xD0,
            CMP_INDIRECT_Y     = 0xD1,
            JAM_IMPLIED_D2     = 0xD2,
            DCP_INDIRECT_Y     = 0xD3,
            NOP_ZERO_PAGE_X_D4 = 0xD4,
            CMP_ZERO_PAGE_X    = 0xD5,
            DEC_ZERO_PAGE_X    = 0xD6,
            DCP_ZERO_PAGE_X    = 0xD7,
            CLD_IMPLIED        = 0xD8,
            CMP_ABSOLUTE_Y     = 0xD9,
            NOP_IMPLIED_DA     = 0xDA,
            DCP_ABSOLUTE_Y     = 0xDB,
            NOP_ABSOLUTE_X_DB  = 0xDC,
            CMP_ABSOLUTE_X     = 0xDD,
            DEC_ABSOLUTE_X     = 0xDE,
            DCP_ABSOLUTE_X     = 0xDF,

            CPX_IMMEDIATE    = 0xE0,
            SBC_X_INDIRECT   = 0xE1,
            NOP_IMMEDIATE_E2 = 0xE2,
            ISC_X_INDIRECT   = 0xE3,
            CPX_ZERO_PAGE    = 0xE4,
            SBC_ZERO_PAGE    = 0xE5,
            INC_ZERO_PAGE    = 0xE6,
            ISC_ZERO_PAGE    = 0xE7,
            INX_IMPLIED      = 0xE8,
            SBC_IMMEDIATE_E9 = 0xE9,
            NOP_IMPLIED_EA   = 0xEA,
            SBC_IMMEDIATE_EB = 0xEB,
            CPX_ABSOLUTE     = 0xEC,
            SBC_ABSOLUTE     = 0xED,
            INC_ABSOLUTE     = 0xEE,
            ISC_ABSOLUTE     = 0xEF,

            BEQ_RELATIVE       = 0xF0,
            SBC_INDIRECT_Y     = 0xF1,
            JAM_IMPLIED_F2     = 0xF2,
            ISC_INDIRECT_Y     = 0xF3,
            NOP_ZERO_PAGE_X_F4 = 0xF4,
            SBC_ZERO_PAGE_X    = 0xF5,
            INC_ZERO_PAGE_X    = 0xF6,
            ISC_ZERO_PAGE_X    = 0xF7,
            SED_IMPLIED        = 0xF8,
            SBC_ABSOLUTE_Y     = 0xF9,
            NOP_IMPLIED_FA     = 0xFA,
            ISC_ABSOLUTE_Y     = 0xFB,
            NOP_ABSOLUTE_X_FC  = 0xFC,
            SBC_ABSOLUTE_X     = 0xFD,
            INC_ABSOLUTE_X     = 0xFE,
            ISC_ABSOLUTE_X     = 0xFF
         };

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

         explicit Processor(Memory& memory) noexcept;
         Processor(Processor const&) = delete;
         Processor(Processor&&) = delete;

         ~Processor() noexcept = default;

         Processor& operator=(Processor const&) = delete;
         Processor& operator=(Processor&&) = delete;

         bool tick();
         void step();
         void reset() noexcept;

         [[nodiscard]] Cycle cycle() const noexcept;
         [[nodiscard]] Accumulator accumulator() const noexcept;
         [[nodiscard]] Index x() const noexcept;
         [[nodiscard]] Index y() const noexcept;
         [[nodiscard]] StackPointer stack_pointer() const noexcept;
         [[nodiscard]] ProcessorStatus processor_status() const noexcept;

         ProgramCounter program_counter{};

      private:
         // Addressing modes
         [[nodiscard]] Instruction relative(BranchOperation operation);

         [[nodiscard]] Instruction immediate(ReadOperation operation) noexcept;
         [[nodiscard]] Instruction absolute(ReadOperation operation) noexcept;
         [[nodiscard]] Instruction zero_page(ReadOperation operation) noexcept;
         [[nodiscard]] Instruction zero_page_indexed(ReadOperation operation, Index index) noexcept;
         [[nodiscard]] Instruction absolute_indexed(ReadOperation operation, Index index) noexcept;
         [[nodiscard]] Instruction x_indirect(ReadOperation operation) noexcept;
         [[nodiscard]] Instruction indirect_y(ReadOperation operation) noexcept;

         [[nodiscard]] Instruction accumulator(ModifyOperation operation) noexcept;
         [[nodiscard]] Instruction absolute(ModifyOperation operation) noexcept;
         [[nodiscard]] Instruction zero_page(ModifyOperation operation) noexcept;
         [[nodiscard]] Instruction zero_page_indexed(ModifyOperation operation, Index index) noexcept;
         [[nodiscard]] Instruction absolute_indexed(ModifyOperation operation, Index index) noexcept;
         [[nodiscard]] Instruction x_indirect(ModifyOperation operation) noexcept;
         [[nodiscard]] Instruction indirect_y(ModifyOperation operation) noexcept;

         [[nodiscard]] Instruction absolute(WriteOperation operation) noexcept;
         [[nodiscard]] Instruction zero_page(WriteOperation operation) noexcept;
         [[nodiscard]] Instruction zero_page_indexed(WriteOperation operation, Index index) noexcept;
         [[nodiscard]] Instruction absolute_indexed(WriteOperation operation, Index index) noexcept;
         [[nodiscard]] Instruction x_indirect(WriteOperation operation) noexcept;
         [[nodiscard]] Instruction indirect_y(WriteOperation operation) noexcept;
         // ---

         // Implied instructions
         [[nodiscard]] Instruction rst_implied() noexcept;
         [[nodiscard]] Instruction brk_implied() noexcept;
         [[nodiscard]] Instruction php_implied() noexcept;
         [[nodiscard]] Instruction clc_implied() noexcept;
         // ---

         // Branch operations
         bool bpl() const noexcept;
         // ---

         // Read operations
         void ora(Data value) noexcept;
         void lda(Data value) noexcept;
         // ---

         // Modify operations
         [[nodiscard]] Data asl(Data value) noexcept;
         // ---

         // Write operations
         // ---

         // Helper functions
         [[nodiscard]] Instruction instruction_from_opcode(Opcode opcode);
         void change_processor_status_flag(ProcessorStatusFlag flag, bool set) noexcept;
         void change_processor_status_flags(std::initializer_list<ProcessorStatusFlag> flags, bool set) noexcept;
         [[nodiscard]] bool processor_status_flag(ProcessorStatusFlag flag) const noexcept;
         void push(Data value) noexcept;
         [[nodiscard]] Data pop() noexcept;

         [[nodiscard]] static Address assemble_address(Data low_byte, Data high_byte) noexcept;
         [[nodiscard]] static std::pair<Address, bool> add_low_byte(Address address, Data value) noexcept;
         [[nodiscard]] static std::pair<Address, bool> add_low_byte(Address address, SignedData value) noexcept;
         [[nodiscard]] static std::pair<Address, bool> add_high_byte(Address address, Data value) noexcept;
         [[nodiscard]] static std::pair<Address, bool> subtract_high_byte(Address address, Data value) noexcept;
         // ---

         Memory& memory_;

         Cycle cycle_{};
         Accumulator accumulator_{};
         Index x_{};
         Index y_{};
         StackPointer stack_pointer_{ 0xFF };
         ProcessorStatus processor_status_{};

         Opcode current_opcode_{};
         std::optional<Instruction> current_instruction_{ rst_implied() };
   };
}

#endif