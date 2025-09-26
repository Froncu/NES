#include "CPU.hpp"

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
         auto const opcode = static_cast<Opcode>(read(program_counter_));
         increment_program_counter();

         switch (opcode)
         {
            case Opcode::BRK_implied:
               current_instruction_ = BRK_implied();
               break;

            default:
               throw std::runtime_error{ std::format("opcode 0x{:02X} is not supported", static_cast<std::uint8_t>(opcode)) };
         }

         continue_execution = true;
      }

      ++cycle_;
      return continue_execution;
   }

   std::uint16_t CPU::cycle() const
   {
      return cycle_;
   }

   std::uint8_t CPU::read(std::uint16_t const address) const
   {
      return memory_[address];
   }

   void CPU::write(std::uint16_t const address, std::uint8_t const data) const
   {
      memory_[address] = data;
   }

   void CPU::increment_program_counter()
   {
      ++program_counter_;
   }

   Instruction CPU::BRK_implied()
   {
      // 2: read next instruction byte (and throw it away), increment PC
      std::ignore = memory_[program_counter_++];
      co_await std::suspend_always{};

      // 3: push PCH on stack (with B flag set), decrement S
      processor_status_ |= 0b00010000;
      memory_[0x0100 + stack_pointer_--] = program_counter_ >> 8 & 0x00FF;
      co_await std::suspend_always{};

      // 4: push PCL on stack, decrement S
      memory_[0x0100 + stack_pointer_--] = program_counter_ & 0x00FF;
      co_await std::suspend_always{};

      // 5: push P on stack, decrement S
      memory_[0x0100 + stack_pointer_--] = processor_status_;
      co_await std::suspend_always{};

      // 6: fetch PCL
      program_counter_ = memory_[0xFFFE];
      co_await std::suspend_always{};

      // 7: fetch PCH
      program_counter_ |= memory_[0xFFFF] << 8;
      co_return false;
   }
}