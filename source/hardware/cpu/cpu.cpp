#include "cpu.hpp"
#include "exceptions/unsupported_opcode.hpp"

namespace nes
{
   Processor::Processor(Memory& memory)
      : memory_{ memory }
   {
   }

   void Processor::tick()
   {
      if (not current_instruction_)
         switch (current_opcode_ = static_cast<Opcode>(memory_.read(program_counter++)))
         {
            case Opcode::BRK_IMPLIED:
               current_instruction_ = brk_implied();
               break;

            case Opcode::ORA_X_INDIRECT:
               current_instruction_ = x_indirect(&Processor::ora);
               break;

            case Opcode::JAM_IMPLIED_02:
               current_instruction_ = jam_implied();
               break;

            case Opcode::ORA_INDIRECT_Y:
               current_instruction_ = indirect_y(&Processor::ora);
               break;

            default:
               throw UnsupportedOpcode{
                  static_cast<decltype(program_counter)>(program_counter - 1),
                  static_cast<std::underlying_type_t<Opcode>>(current_opcode_)
               };
         }
      else if (current_instruction_->tick())
         current_instruction_.reset();

      ++cycle_;
   }

   void Processor::step()
   {
      tick();

      if (current_opcode_ == Opcode::JAM_IMPLIED_02 or
         current_opcode_ == Opcode::JAM_IMPLIED_12 or
         current_opcode_ == Opcode::JAM_IMPLIED_22 or
         current_opcode_ == Opcode::JAM_IMPLIED_32 or
         current_opcode_ == Opcode::JAM_IMPLIED_42 or
         current_opcode_ == Opcode::JAM_IMPLIED_52 or
         current_opcode_ == Opcode::JAM_IMPLIED_62 or
         current_opcode_ == Opcode::JAM_IMPLIED_72 or
         current_opcode_ == Opcode::JAM_IMPLIED_92 or
         current_opcode_ == Opcode::JAM_IMPLIED_B2 or
         current_opcode_ == Opcode::JAM_IMPLIED_D2 or
         current_opcode_ == Opcode::JAM_IMPLIED_F2)
         return;

      while (current_instruction_)
         tick();
   }

   void Processor::reset()
   {
      cycle_ = 0;
      current_opcode_ = {};
      current_instruction_ = rst_implied();
   }

   Cycle Processor::cycle() const
   {
      return cycle_;
   }

   Accumulator Processor::accumulator() const
   {
      return accumulator_;
   }

   Index Processor::x() const
   {
      return x_;
   }

   Index Processor::y() const
   {
      return y_;
   }

   StackPointer Processor::stack_pointer() const
   {
      return stack_pointer_;
   }

   ProcessorStatus Processor::processor_status() const
   {
      return processor_status_;
   }

   // TODO: find what exactly happens here
   Instruction Processor::rst_implied()
   {
      co_await std::suspend_always{};

      co_await std::suspend_always{};

      --stack_pointer_;
      co_await std::suspend_always{};

      --stack_pointer_;
      co_await std::suspend_always{};

      --stack_pointer_;
      co_await std::suspend_always{};

      program_counter = memory_.read(0xFFFC);
      co_await std::suspend_always{};

      program_counter |= memory_.read(0xFFFD) << 8;
      co_return;
   }

   Instruction Processor::brk_implied()
   {
      // read next instruction byte (and throw it away), increment PC
      std::ignore = memory_.read(program_counter++);
      co_await std::suspend_always{};

      // push PCH on stack (with B flag set), decrement S
      change_processor_status_flag(ProcessorStatusFlag::B, true);
      push(program_counter >> 8 & 0x00FF);
      co_await std::suspend_always{};

      // push PCL on stack, decrement S
      push(program_counter & 0x00FF);
      co_await std::suspend_always{};

      // push P on stack, decrement S
      push(processor_status_);
      co_await std::suspend_always{};

      // fetch PCL
      program_counter = memory_.read(0xFFFE);
      co_await std::suspend_always{};

      // fetch PCH
      program_counter |= memory_.read(0xFFFF) << 8;
      co_return;
   }

   Instruction Processor::jam_implied()
   {
      while (true)
         co_await std::suspend_always{};
   }

   Instruction Processor::immediate(ReadOperation const operation)
   {
      // fetch value, increment PC
      Data const value{ memory_.read(program_counter++) };

      std::invoke(operation, this, value);
      co_return;
   }

   Instruction Processor::absolute(ReadOperation const operation)
   {
      // fetch low byte of address, increment PC
      Data const low_byte_of_address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // fetch high byte of address, increment PC
      Data const high_byte_of_address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // read from effective address
      Address const effective_address{ assemble_address(low_byte_of_address, high_byte_of_address) };
      Data const value{ memory_.read(effective_address) };

      std::invoke(operation, this, value);
      co_return;
   }

   Instruction Processor::zero_page(ReadOperation const operation)
   {
      // fetch address, increment PC
      Address const address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // read from effective address
      Data const value{ memory_.read(address) };

      std::invoke(operation, this, value);
      co_return;
   }

   Instruction Processor::zero_page_indexed(ReadOperation const operation, Index const index)
   {
      // fetch address, increment PC
      Address const address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // read from address, add index register to it
      auto const effective_address{ static_cast<Address>(memory_.read(address) + index) };
      co_await std::suspend_always{};

      // read from effective address
      Data const value{ memory_.read(effective_address) };

      std::invoke(operation, this, value);
      co_return;
   }

   Instruction Processor::absolute_indexed(ReadOperation const operation, Index const index)
   {
      // fetch low byte of address, increment PC
      Data low_byte_of_address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // fetch high byte of address, add index register to low address byte, increment PC
      Data const high_byte_of_address{ memory_.read(program_counter++) };
      bool const overflow{ low_byte_of_address + index < low_byte_of_address };
      low_byte_of_address += index;
      co_await std::suspend_always{};

      // read from effective address, fix the high byte of effective address
      Address effective_address{ assemble_address(low_byte_of_address, high_byte_of_address) };
      Data value{ memory_.read(effective_address) };
      if (overflow)
      {
         ++effective_address;
         co_await std::suspend_always{};

         // re-read from effective address (+)
         value = memory_.read(effective_address);
      }

      std::invoke(operation, this, value);
      co_return;
   }

   Instruction Processor::x_indirect(ReadOperation const operation)
   {
      // fetch pointer address, increment PC
      Address const pointer_address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // read from the address, add X to it
      auto const pointer{ static_cast<Address>(memory_.read(pointer_address) + x_) };
      co_await std::suspend_always{};

      // fetch effective address low
      Data const effective_address_low{ memory_.read(pointer) };
      co_await std::suspend_always{};

      // fetch effective address high
      Data const effective_address_high{ memory_.read(pointer + 1) };
      co_await std::suspend_always{};

      // read from effective address
      Address const effective_address{ assemble_address(effective_address_low, effective_address_high) };
      auto const value{ memory_.read(effective_address) };

      std::invoke(operation, this, value);
      co_return;
   }

   Instruction Processor::indirect_y(ReadOperation const operation)
   {
      // fetch pointer address, increment PC
      Address const pointer_address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // fetch effective address low
      Data effective_address_low{ memory_.read(pointer_address) };
      co_await std::suspend_always{};

      // fetch effective address high, add Y to low byte of effective address
      Data const effective_address_high{ memory_.read(pointer_address + 1) };
      bool const overflow{ effective_address_low + y_ < effective_address_low };
      effective_address_low += y_;
      co_await std::suspend_always{};

      // read from effective address, fix high byte of effective address
      Address effective_address{ assemble_address(effective_address_low, effective_address_high) };
      Data value = memory_.read(effective_address);
      if (overflow)
      {
         ++effective_address;
         co_await std::suspend_always{};

         // re-read from effective address (+)
         value = memory_.read(effective_address);
      }

      std::invoke(operation, this, value);
      co_return;
   }

   Instruction Processor::absolute(ModifyOperation const operation)
   {
      // fetch low byte of address, increment PC
      Data const low_byte_of_address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // fetch high byte of address, increment PC
      Data const high_byte_of_address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // read from effective address
      Address const effective_address{ assemble_address(low_byte_of_address, high_byte_of_address) };
      Data value{ memory_.read(effective_address) };
      co_await std::suspend_always{};

      // write the value back to effective address, and do the operation on it
      memory_.write(effective_address, value);
      value = std::invoke(operation, this, value);
      co_await std::suspend_always{};

      // write the new value to effective address
      memory_.write(effective_address, value);
      co_return;
   }

   Instruction Processor::zero_page(ModifyOperation const operation)
   {
      // fetch address, increment PC
      Address const address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // read from effective address
      Data value{ memory_.read(address) };
      co_await std::suspend_always{};

      // write the value back to effective address, and do the operation on it
      memory_.write(address, value);
      value = std::invoke(operation, this, value);
      co_await std::suspend_always{};

      // write the new value to effective address
      memory_.write(address, value);
      co_return;
   }

   Instruction Processor::zero_page_indexed(ModifyOperation const operation, Index const index)
   {
      // fetch address, increment PC
      Address const address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // read from address, add index register to it
      auto const effective_address{ static_cast<Address>(memory_.read(address) + index) };
      co_await std::suspend_always{};

      // read from effective address
      Data value{ memory_.read(effective_address) };
      co_await std::suspend_always{};

      // write the value back to effective address, and do the operation on it
      memory_.write(address, value);
      value = std::invoke(operation, this, value);
      co_await std::suspend_always{};

      // write the new value to effective address
      memory_.write(address, value);
      co_return;
   }

   Instruction Processor::absolute_indexed(ModifyOperation const operation, Index const index)
   {
      // fetch low byte of address, increment PC
      Data low_byte_of_address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // fetch high byte of address, add index register to low address byte, increment PC
      Data const high_byte_of_address{ memory_.read(program_counter++) };
      bool const overflow{ low_byte_of_address + index < low_byte_of_address };
      low_byte_of_address += index;
      co_await std::suspend_always{};

      // read from effective address, fix the high byte of effective address
      Address effective_address{ assemble_address(low_byte_of_address, high_byte_of_address) };
      std::ignore = memory_.read(effective_address);
      if (overflow)
         effective_address += 0x0100;
      co_await std::suspend_always{};

      // re-read from effective address
      Data value{ memory_.read(effective_address) };
      co_await std::suspend_always{};

      // write the value back to effective address, and do the operation on it
      memory_.write(effective_address, value);
      value = std::invoke(operation, this, value);
      co_await std::suspend_always{};

      // write the new value to effective address
      memory_.write(effective_address, value);
      co_return;
   }

   Instruction Processor::x_indirect(ModifyOperation const operation)
   {
      // fetch pointer address, increment PC
      Address const pointer_address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // read from the address, add X to it
      auto const pointer{ static_cast<Address>(memory_.read(pointer_address) + x_) };
      co_await std::suspend_always{};

      // fetch effective address low
      Data const effective_address_low{ memory_.read(pointer) };
      co_await std::suspend_always{};

      // fetch effective address high
      Data const effective_address_high{ memory_.read(pointer + 1) };
      co_await std::suspend_always{};

      // read from effective address
      Address const effective_address{ assemble_address(effective_address_low, effective_address_high) };
      auto value{ memory_.read(effective_address) };
      co_await std::suspend_always{};

      // write the value back to effective address, and do the operation on it
      memory_.write(effective_address, value);
      value = std::invoke(operation, this, value);
      co_await std::suspend_always{};

      // write the new value to effective address
      memory_.write(effective_address, value);
      co_return;
   }

   Instruction Processor::indirect_y(ModifyOperation const operation)
   {
      // fetch pointer address, increment PC
      Address const pointer_address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // fetch effective address low
      Data effective_address_low{ memory_.read(pointer_address) };
      co_await std::suspend_always{};

      // fetch effective address high, add Y to low byte of effective address
      Data const effective_address_high{ memory_.read(pointer_address + 1) };
      bool const overflow{ effective_address_low + y_ < effective_address_low };
      effective_address_low += y_;
      co_await std::suspend_always{};

      // read from effective address, fix high byte of effective address
      Address effective_address{ assemble_address(effective_address_low, effective_address_high) };
      std::ignore = memory_.read(effective_address);
      if (overflow)
         effective_address += 0x0100;

      co_await std::suspend_always{};

      // read from effective address
      Data value = memory_.read(effective_address);
      co_await std::suspend_always{};

      // write the value back to effective address, and do the operation on it
      memory_.write(effective_address, value);
      value = std::invoke(operation, this, value);
      co_await std::suspend_always{};

      // write the new value to effective address
      memory_.write(effective_address, value);
      co_return;
   }

   Instruction Processor::absolute(WriteOperation const operation)
   {
      // fetch low byte of address, increment PC
      Data const low_byte_of_address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // fetch high byte of address, increment PC
      Data const high_byte_of_address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // write register to effective address
      Address const effective_address{ assemble_address(low_byte_of_address, high_byte_of_address) };
      std::invoke(operation, this, effective_address);
      co_return;
   }

   Instruction Processor::zero_page(WriteOperation const operation)
   {
      // fetch address, increment PC
      Address const address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // write register to effective address
      std::invoke(operation, this, address);
      co_return;
   }

   Instruction Processor::zero_page_indexed(WriteOperation const operation, Index const index)
   {
      // fetch address, increment PC
      Address const address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // read from address, add index register to it
      auto const effective_address{ static_cast<Address>(memory_.read(address) + index) };
      co_await std::suspend_always{};

      // write to effective address
      std::invoke(operation, this, effective_address);
      co_return;
   }

   Instruction Processor::absolute_indexed(WriteOperation const operation, Index const index)
   {
      // fetch low byte of address, increment PC
      Data low_byte_of_address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // fetch high byte of address, add index register to low address byte, increment PC
      Data const high_byte_of_address{ memory_.read(program_counter++) };
      bool const overflow{ low_byte_of_address + index < low_byte_of_address };
      low_byte_of_address += index;
      co_await std::suspend_always{};

      // read from effective address, fix the high byte of effective address
      Address effective_address{ assemble_address(low_byte_of_address, high_byte_of_address) };
      std::ignore = memory_.read(effective_address);
      if (overflow)
         effective_address += 0x0100;
      co_await std::suspend_always{};

      // write to effective address
      std::invoke(operation, this, effective_address);
      co_return;
   }

   Instruction Processor::x_indirect(WriteOperation const operation)
   {
      // fetch pointer address, increment PC
      Address const pointer_address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // read from the address, add X to it
      auto const pointer{ static_cast<Address>(memory_.read(pointer_address) + x_) };
      co_await std::suspend_always{};

      // fetch effective address low
      Data const effective_address_low{ memory_.read(pointer) };
      co_await std::suspend_always{};

      // fetch effective address high
      Data const effective_address_high{ memory_.read(pointer + 1) };
      co_await std::suspend_always{};

      // write to effective address
      Address const effective_address{ assemble_address(effective_address_low, effective_address_high) };
      std::invoke(operation, this, effective_address);
      co_return;
   }

   Instruction Processor::indirect_y(WriteOperation const operation)
   {
      // fetch pointer address, increment PC
      Address const pointer_address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // fetch effective address low
      Data effective_address_low{ memory_.read(pointer_address) };
      co_await std::suspend_always{};

      // fetch effective address high, add Y to low byte of effective address
      Data const effective_address_high{ memory_.read(pointer_address + 1) };
      bool const overflow{ effective_address_low + y_ < effective_address_low };
      effective_address_low += y_;
      co_await std::suspend_always{};

      // read from effective address, fix high byte of effective address
      Address effective_address{ assemble_address(effective_address_low, effective_address_high) };
      std::ignore = memory_.read(effective_address);
      if (overflow)
         effective_address += 0x0100;
      co_await std::suspend_always{};

      // write to effective address
      std::invoke(operation, this, effective_address);
      co_return;
   }

   void Processor::ora(Data const value)
   {
      // ORA value with accumulator
      accumulator_ |= value;

      // Z set if the accumulator is zero after ORA
      change_processor_status_flag(ProcessorStatusFlag::Z, not accumulator_);

      // N set if bit 7 of the accumulator is set after ORA
      change_processor_status_flag(ProcessorStatusFlag::N, accumulator_ & 0b10000000);
   }

   void Processor::lda(Data const value)
   {
      // load value into accumulator
      accumulator_ = value;

      // Z set if the accumulator is zero after LDA
      change_processor_status_flag(ProcessorStatusFlag::Z, not accumulator_);

      // N set if bit 7 of the accumulator is set after LDA
      change_processor_status_flag(ProcessorStatusFlag::N, accumulator_ & 0b10000000);
   }

   void Processor::change_processor_status_flag(ProcessorStatusFlag const flag, bool const set)
   {
      auto const underlying_flag{ static_cast<std::underlying_type_t<ProcessorStatusFlag>>(flag) };
      set
         ? processor_status_ |= underlying_flag
         : processor_status_ &= ~underlying_flag;
   }

   void Processor::push(Data const value)
   {
      memory_.write(0x0100 + stack_pointer_--, value);
   }

   Data Processor::pop()
   {
      return memory_.read(0x0100 + ++stack_pointer_);
   }

   Address Processor::assemble_address(Data const low_byte, Data const high_byte)
   {
      return high_byte << 8 | low_byte;
   }
}