#ifndef CPU_HPP
#define CPU_HPP

#include "hardware/memory/memory.hpp"
#include "instruction.hpp"
#include "pch.hpp"

namespace nes
{
   class CPU final
   {
      using ReadOperation = void(CPU::*)(Data);
      using ModifyOperation = Data(CPU::*)(Data);
      using WriteOperation = void(CPU::*)(Address);

      public:
         enum class Opcode : Data
         {
            BRK_IMPLIED       = 0x00,
            ORA_X_INDIRECT    = 0x01,
            JAM_02            = 0x02,
            SLO_X_INDIRECT    = 0x03,
            NOP_ZERO_PAGE     = 0x04,
            ORA_ZERO_PAGE     = 0x05,
            ASL_ZERO_PAGE     = 0x06,
            SLO_ZERO_PAGE     = 0x07,
            PHP_IMPLIED       = 0x08,
            ORA_IMMEDIATE     = 0x09,
            ASL_ACCUMULATOR   = 0x0A,
            ANC_IMMEDIATE     = 0x0B,
            NOP_ABSOLUTE      = 0x0C,
            ORA_ABSOLUTE      = 0x0D,
            ASL_ABSOLUTE      = 0x0E,
            SLO_ABSOLUTE      = 0x0F,

            BPL_RELATIVE      = 0x10,
            ORA_INDIRECT_Y    = 0x11,
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

         explicit CPU(Memory& memory);
         CPU(CPU const&) = delete;
         CPU(CPU&&) = delete;

         ~CPU() = default;

         CPU& operator=(CPU const&) = delete;
         CPU& operator=(CPU&&) = delete;

         void tick();
         void step();
         void reset();

         [[nodiscard]] Cycle cycle() const;
         [[nodiscard]] Accumulator accumulator() const;
         [[nodiscard]] Index x() const;
         [[nodiscard]] Index y() const;
         [[nodiscard]] StackPointer stack_pointer() const;
         [[nodiscard]] ProcessorStatus processor_status() const;

         ProgramCounter program_counter{};

      private:
         [[nodiscard]] Instruction rst_implied();
         [[nodiscard]] Instruction brk_implied();

         [[nodiscard]] Instruction immediate(ReadOperation operation);
         [[nodiscard]] Instruction absolute(ReadOperation operation);
         [[nodiscard]] Instruction zero_page(ReadOperation operation);
         [[nodiscard]] Instruction zero_page_indexed(ReadOperation operation, Index index);
         [[nodiscard]] Instruction absolute_indexed(ReadOperation operation, Index index);
         [[nodiscard]] Instruction x_indirect(ReadOperation operation);
         [[nodiscard]] Instruction indirect_y(ReadOperation operation);

         [[nodiscard]] Instruction absolute(ModifyOperation operation);
         [[nodiscard]] Instruction zero_page(ModifyOperation operation);
         [[nodiscard]] Instruction zero_page_indexed(ModifyOperation operation, Index index);
         [[nodiscard]] Instruction absolute_indexed(ModifyOperation operation, Index index);
         [[nodiscard]] Instruction x_indirect(ModifyOperation operation);
         [[nodiscard]] Instruction indirect_y(ModifyOperation operation);

         [[nodiscard]] Instruction absolute(WriteOperation operation);
         [[nodiscard]] Instruction zero_page(WriteOperation operation);
         [[nodiscard]] Instruction zero_page_indexed(WriteOperation operation, Index index);
         [[nodiscard]] Instruction absolute_indexed(WriteOperation operation, Index index);
         [[nodiscard]] Instruction x_indirect(WriteOperation operation);
         [[nodiscard]] Instruction indirect_y(WriteOperation operation);

         void lda(Data value);
         void ora(Data value);

         void change_processor_status_flag(ProcessorStatusFlag flag, bool set);
         void push(Data value);
         [[nodiscard]] Data pop();

         [[nodiscard]] static Address assemble_address(Data low_byte, Data high_byte);

         Memory& memory_;

         Cycle cycle_{};
         Accumulator accumulator_{};
         Index x_{};
         Index y_{};
         StackPointer stack_pointer_{ 0xFF };
         ProcessorStatus processor_status_{};

         std::optional<Instruction> current_instruction_{ rst_implied() };
   };
}

#endif