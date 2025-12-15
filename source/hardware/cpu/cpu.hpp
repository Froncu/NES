#ifndef CPU_HPP
#define CPU_HPP

#include "hardware/memory/memory.hpp"
#include "instruction.hpp"
#include "pch.hpp"

namespace nes
{
   class CPU final
   {
      using ReadOperation = std::function<void(Data)>;
      using ModifyOperation = std::function<Data(Data)>;
      using WriteOperation = std::function<Data()>;

      public:
         static auto constexpr FREQUENCY = 1'789'773;

         enum class Opcode : Data
         {
            BRK_IMPLIED = 0x00,
            ORA_X_INDIRECT = 0x01,
            ORA_INDIRECT_Y = 0x11,
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

         [[nodiscard]] Cycle cycle() const;
         [[nodiscard]] ProgramCounter program_counter() const;
         [[nodiscard]] Accumulator accumulator() const;
         [[nodiscard]] Index x() const;
         [[nodiscard]] Index y() const;
         [[nodiscard]] StackPointer stack_pointer() const;
         [[nodiscard]] ProcessorStatus processor_status() const;

      private:
         [[nodiscard]] Instruction reset();
         [[nodiscard]] Instruction brk_implied();

         [[nodiscard]] Instruction immediate(ReadOperation operation);
         [[nodiscard]] Instruction absolute(ReadOperation operation);
         [[nodiscard]] Instruction zero_page(ReadOperation operation);
         [[nodiscard]] Instruction zero_page_indexed(ReadOperation operation, Index const& index);
         [[nodiscard]] Instruction absolute_indexed(ReadOperation operation, Index const& index);
         [[nodiscard]] Instruction x_indirect(ReadOperation operation);
         [[nodiscard]] Instruction indirect_y(ReadOperation operation);

         // [[nodiscard]] Instruction absolute(ModifyOperation operation);
         // [[nodiscard]] Instruction zeropage(ModifyOperation operation);
         // [[nodiscard]] Instruction zeropage_indexed(ModifyOperation operation, Index const& index);
         // [[nodiscard]] Instruction absolute_indexed(ModifyOperation operation, Index const& index);
         // [[nodiscard]] Instruction indexed_indirect(ModifyOperation operation);
         // [[nodiscard]] Instruction indirect_indexed(ModifyOperation operation);

         // [[nodiscard]] Instruction absolute(WriteOperation operation);
         // [[nodiscard]] Instruction zeropage(WriteOperation operation);
         // [[nodiscard]] Instruction zeropage_indexed(WriteOperation operation, Index const& index);
         // [[nodiscard]] Instruction absolute_indexed(WriteOperation operation, Index const& index);
         // [[nodiscard]] Instruction indexed_indirect(WriteOperation operation);
         // [[nodiscard]] Instruction indirect_indexed(WriteOperation operation);

         void lda(Data value);
         void ora(Data value);

         void change_processor_status_flag(ProcessorStatusFlag flag, bool set);
         void push(Data value);
         [[nodiscard]] Data pop();

         [[nodiscard]] static Address assemble_address(Data low_byte, Data high_byte);

         Memory& memory_;

         Cycle cycle_{};
         ProgramCounter program_counter_{};
         Accumulator accumulator_{};
         Index x_{};
         Index y_{};
         StackPointer stack_pointer_{ 0xFF };
         ProcessorStatus processor_status_{};

         std::optional<Instruction> current_instruction_{ reset() };
   };
}

#endif