#include "CPU.hpp"
#include "exceptions/unsupported_opcode.hpp"

namespace nes
{
   CPU::CPU(Memory& memory)
      : memory_{ memory }
   {
      program_counter_ = memory_[0xFFFD] << 8 | memory_[0xFFFC];
   }

   bool CPU::tick()
   {
      bool continue_execution;

      if (current_instruction_)
      {
         current_instruction_->resume();
         continue_execution = current_instruction_->continue_execution();

         if (current_instruction_->done())
            current_instruction_.reset();
      }
      else
      {
         switch (auto const opcode = static_cast<Opcode>(memory_[program_counter_++]))
         {
            case Opcode::BRK_IMPLIED:
               current_instruction_ = brk_implied();
               break;

            case Opcode::ORA_X_INDIRECT:
               current_instruction_ = ora_x_indirect();
               break;

            default:
               throw UnsupportedOpcode{
                  static_cast<decltype(program_counter_)>(program_counter_ - 1),
                  static_cast<std::underlying_type_t<Opcode>>(opcode)
               };
         }

         continue_execution = true;
      }

      ++cycle_;
      return continue_execution;
   }

   Cycle CPU::cycle() const
   {
      return cycle_;
   }

   ProgramCounter CPU::program_counter() const
   {
      return program_counter_;
   }

   Accumulator CPU::accumulator() const
   {
      return accumulator_;
   }

   X CPU::x() const
   {
      return x_;
   }

   Y CPU::y() const
   {
      return y_;
   }

   StackPointer CPU::stack_pointer() const
   {
      return stack_pointer_;
   }

   ProcessorStatus CPU::processor_status() const
   {
      return processor_status_;
   }

   void CPU::change_processor_status_flag(ProcessorStatusFlag const flag, bool const set)
   {
      if (auto const underlying_flag = static_cast<std::underlying_type_t<ProcessorStatusFlag>>(flag); set)
         processor_status_ |= underlying_flag;
      else
         processor_status_ &= ~underlying_flag;
   }

   void CPU::push(Data const value)
   {
      memory_[0x0100 + stack_pointer_--] = value;
   }

   Data CPU::pop()
   {
      return memory_[0x0100 + ++stack_pointer_];
   }

   Instruction CPU::brk_implied()
   {
      // read next instruction byte (and throw it away), increment PC
      std::ignore = memory_[program_counter_++];
      co_await std::suspend_always{};

      // push PCH on stack (with B flag set), decrement S
      change_processor_status_flag(ProcessorStatusFlag::B, true);
      push(program_counter_ >> 8 & 0x00FF);
      co_await std::suspend_always{};

      // push PCL on stack, decrement S
      push(program_counter_ & 0x00FF);
      co_await std::suspend_always{};

      // push P on stack, decrement S
      push(processor_status_);
      co_await std::suspend_always{};

      // fetch PCL
      program_counter_ = memory_[0xFFFE];
      co_await std::suspend_always{};

      // fetch PCH
      program_counter_ |= memory_[0xFFFF] << 8;
      co_return false;
   }

   Instruction CPU::ora_x_indirect()
   {
      // fetch pointer address, increment PC
      std::uint8_t const pointer_address = memory_[program_counter_++];
      co_await std::suspend_always{};

      // read from the address, add X to it
      std::uint8_t const pointer = memory_[pointer_address] + x_;
      co_await std::suspend_always{};

      // fetch effective address low
      std::uint8_t const effective_address_low = memory_[pointer];
      co_await std::suspend_always{};

      // fetch effective address high
      std::uint8_t const effective_address_high = memory_[pointer + 1];
      co_await std::suspend_always{};

      // read from effective address
      std::uint16_t const effective_address = effective_address_high << 8 | effective_address_low;
      std::uint8_t const value = memory_[effective_address];
      accumulator_ |= value;

      // Z set if the accumulator is zero after ORA
      change_processor_status_flag(ProcessorStatusFlag::Z, not accumulator_);

      // N set if bit 7 of the accumulator is set after ORA
      change_processor_status_flag(ProcessorStatusFlag::N, accumulator_ & 0b10000000);

      co_return true;
   }
}