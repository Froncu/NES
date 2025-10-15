#include "CPU.hpp"
#include "exceptions/unsupported_opcode.hpp"

namespace nes
{
   CPU::CPU(Memory& memory)
      : memory_{ memory }
      , current_instruction_{ reset() }
   {
   }

   void CPU::tick()
   {
      if (current_instruction_)
      {
         current_instruction_->resume();

         if (current_instruction_->done())
            current_instruction_.reset();
      }
      else
         switch (auto const opcode = static_cast<Opcode>(memory_[program_counter_++]))
         {
            case Opcode::BRK_IMPLIED:
               current_instruction_ = brk_implied();
               break;

            case Opcode::ORA_X_INDIRECT:
               current_instruction_ = x_indirect(std::bind(&CPU::ora, this, std::placeholders::_1));
               break;

            case Opcode::ORA_INDIRECT_Y:
               current_instruction_ = indirect_y(std::bind(&CPU::ora, this, std::placeholders::_1));
               break;

            case Opcode::ORA_ZERO_PAGE:
               current_instruction_ = zero_page(std::bind(&CPU::ora, this, std::placeholders::_1));
               break;

            default:
               throw UnsupportedOpcode{
                  static_cast<decltype(program_counter_)>(program_counter_ - 1),
                  static_cast<std::underlying_type_t<Opcode>>(opcode)
               };
         }

      ++cycle_;
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

   Index CPU::x() const
   {
      return x_;
   }

   Index CPU::y() const
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

   // TODO: find what exactly happens here
   Instruction CPU::reset()
   {
      co_await std::suspend_always{};

      co_await std::suspend_always{};

      --stack_pointer_;
      co_await std::suspend_always{};

      --stack_pointer_;
      co_await std::suspend_always{};

      --stack_pointer_;
      co_await std::suspend_always{};

      program_counter_ = memory_[0xFFFC];
      co_await std::suspend_always{};

      program_counter_ |= memory_[0xFFFD] << 8;
      co_return;
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
      co_return;
   }

   Instruction CPU::immediate(ReadOperation operation)
   {
      // fetch value, increment PC
      Data const value = memory_[program_counter_++];

      operation(value);
      co_return;
   }

   Instruction CPU::absolute(ReadOperation const operation)
   {
      // fetch low byte of address, increment PC
      Data const low_byte_of_address = memory_[program_counter_++];
      co_await std::suspend_always{};

      // fetch high byte of address, increment PC
      Data const high_byte_of_address = memory_[program_counter_++];
      co_await std::suspend_always{};

      // read from effective address
      auto const effective_address = high_byte_of_address << 8 | low_byte_of_address;
      Data const value = memory_[effective_address];

      operation(value);
      co_return;
   }

   Instruction CPU::zero_page(ReadOperation const operation)
   {
      // fetch address, increment PC
      Data const address = memory_[program_counter_++];
      co_await std::suspend_always{};

      // read from effective address
      Data const value = memory_[address];

      operation(value);
      co_return;
   }

   Instruction CPU::zero_page_indexed(ReadOperation const operation, Index const& index)
   {
      // fetch address, increment PC
      Data const address = memory_[program_counter_++];
      co_await std::suspend_always{};

      // read from address, add index register to it
      Data const effective_address = memory_[address] + index;
      co_await std::suspend_always{};

      // read from effective address
      Data const value = memory_[effective_address];

      operation(value);
      co_return;
   }

   Instruction CPU::absolute_indexed(ReadOperation const operation, Index const& index)
   {
      // fetch low byte of address, increment PC
      Data low_byte_of_address = memory_[program_counter_++];
      co_await std::suspend_always{};

      // fetch high byte of address, add index register to low address byte, increment PC
      Data const high_byte_of_address = memory_[program_counter_++];
      bool const overflow = low_byte_of_address + index < low_byte_of_address;
      low_byte_of_address += index;
      co_await std::suspend_always{};

      // read from effective address, fix the high byte of effective address
      auto effective_address = high_byte_of_address << 8 | low_byte_of_address;
      Data value = memory_[effective_address];
      if (overflow)
      {
         ++effective_address;
         co_await std::suspend_always{};

         // re-read from effective address (+)
         value = memory_[effective_address];
      }

      operation(value);
      co_return;
   }

   Instruction CPU::x_indirect(ReadOperation const operation)
   {
      // fetch pointer address, increment PC
      Data const pointer_address = memory_[program_counter_++];
      co_await std::suspend_always{};

      // read from the address, add X to it
      Data const pointer = memory_[pointer_address] + x_;
      co_await std::suspend_always{};

      // fetch effective address low
      Data const effective_address_low = memory_[pointer];
      co_await std::suspend_always{};

      // fetch effective address high
      Data const effective_address_high = memory_[pointer + 1];
      co_await std::suspend_always{};

      // read from effective address
      auto const effective_address = effective_address_high << 8 | effective_address_low;
      Data const value = memory_[effective_address];

      operation(value);
      co_return;
   }

   Instruction CPU::indirect_y(ReadOperation const operation)
   {
      // fetch pointer address, increment PC
      Data const pointer_address = memory_[program_counter_++];
      co_await std::suspend_always{};

      // fetch effective address low
      Data effective_address_low = memory_[pointer_address];
      co_await std::suspend_always{};

      // fetch effective address high, add Y to low byte of effective address
      Data const effective_address_high = memory_[pointer_address + 1];
      bool const overflow = effective_address_low + y_ < effective_address_low;
      effective_address_low += y_;
      co_await std::suspend_always{};

      // read from effective address, fix high byte of effective address
      auto effective_address = effective_address_high << 8 | effective_address_low;
      Data value = memory_[effective_address];
      if (overflow)
      {
         ++effective_address;
         co_await std::suspend_always{};

         // re-read from effective address (+)
         value = memory_[effective_address];
      }

      operation(value);
      co_return;
   }

   void CPU::lda(Data const value)
   {
      // load value into accumulator
      accumulator_ = value;

      // Z set if the accumulator is zero after LDA
      change_processor_status_flag(ProcessorStatusFlag::Z, not accumulator_);

      // N set if bit 7 of the accumulator is set after LDA
      change_processor_status_flag(ProcessorStatusFlag::N, accumulator_ & 0b10000000);
   }

   void CPU::ora(Data const value)
   {
      // ORA value with accumulator
      accumulator_ |= value;

      // Z set if the accumulator is zero after ORA
      change_processor_status_flag(ProcessorStatusFlag::Z, not accumulator_);

      // N set if bit 7 of the accumulator is set after ORA
      change_processor_status_flag(ProcessorStatusFlag::N, accumulator_ & 0b10000000);
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
}