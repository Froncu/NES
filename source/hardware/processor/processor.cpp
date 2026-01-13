#include "processor.hpp"
#include "exceptions/unsupported_opcode.hpp"

namespace nes
{
   Processor::Processor(Memory& memory) noexcept
      : memory_{ memory }
   {
   }

   bool Processor::tick()
   {
      ++cycle_;

      bool instruction_completed;
      if (not current_instruction_)
      {
         current_opcode_ = static_cast<Opcode>(memory_.read(program_counter++));
         current_instruction_ = instruction_from_opcode(current_opcode_);
         instruction_completed = false;
      }
      else if (instruction_completed = current_instruction_->tick(); instruction_completed)
      {
         std::optional prefetched_instruction{ current_instruction_->prefetched_instruction() };
         current_instruction_ = std::move(prefetched_instruction);
      }

      return instruction_completed;
   }

   void Processor::step()
   {
      while (not tick());
   }

   void Processor::reset() noexcept
   {
      cycle_ = 0;
      current_opcode_ = {};
      current_instruction_ = RST();
   }

   Cycle Processor::cycle() const noexcept
   {
      return cycle_;
   }

   Accumulator Processor::accumulator() const noexcept
   {
      return accumulator_;
   }

   Index Processor::x() const noexcept
   {
      return x_;
   }

   Index Processor::y() const noexcept
   {
      return y_;
   }

   StackPointer Processor::stack_pointer() const noexcept
   {
      return stack_pointer_;
   }

   ProcessorStatus Processor::processor_status() const noexcept
   {
      return processor_status_;
   }

   Instruction Processor::relative(BranchOperation const operation)
   {
      // fetch operand, increment PC
      auto const operand{ static_cast<SignedData>(memory_.read(program_counter)) };
      ++program_counter;
      co_await std::suspend_always{};

      // fetch opcode of next instruction, if branch is taken, add operand to PCL, otherwise increment PC
      Opcode next_opcode{ memory_.read(program_counter) };
      if (std::invoke(operation, this))
      {
         auto const [modified_program_counter, overflow]{ add_low_byte(program_counter, operand) };
         program_counter = modified_program_counter;
         co_await std::suspend_always{};

         // fetch opcode of next instruction, fix PCH, if it did not change, increment PC (+)
         next_opcode = static_cast<Opcode>(memory_.read(program_counter));
         if (overflow)
         {
            program_counter =
               std::signbit(operand)
                  ? subtract_high_byte(program_counter, 1).first
                  : add_high_byte(program_counter, 1).first;
            co_await std::suspend_always{};

            // fetch opcode of next instruction, increment PC (!)
            next_opcode = static_cast<Opcode>(memory_.read(program_counter));
         }
      }

      ++program_counter;
      co_return instruction_from_opcode(next_opcode);
   }

   Instruction Processor::immediate(ReadOperation const operation) noexcept
   {
      // fetch value, increment PC
      Data const value{ memory_.read(program_counter) };
      ++program_counter;

      std::invoke(operation, this, value);
      co_return std::nullopt;
   }

   Instruction Processor::absolute(ReadOperation const operation) noexcept
   {
      // fetch low byte of address, increment PC
      Data const low_byte_of_address{ memory_.read(program_counter) };
      ++program_counter;
      co_await std::suspend_always{};

      // fetch high byte of address, increment PC
      Data const high_byte_of_address{ memory_.read(program_counter) };
      ++program_counter;
      co_await std::suspend_always{};

      // read from effective address
      Address const effective_address{ assemble_address(high_byte_of_address, low_byte_of_address) };
      Data const value{ memory_.read(effective_address) };

      std::invoke(operation, this, value);
      co_return std::nullopt;
   }

   Instruction Processor::zero_page(ReadOperation const operation) noexcept
   {
      // fetch address, increment PC
      Address const address{ memory_.read(program_counter) };
      ++program_counter;
      co_await std::suspend_always{};

      // read from effective address
      Data const value{ memory_.read(address) };

      std::invoke(operation, this, value);
      co_return std::nullopt;
   }

   Instruction Processor::zero_page_indexed(ReadOperation const operation, Index const index) noexcept
   {
      // fetch address, increment PC
      Data address{ memory_.read(program_counter) };
      ++program_counter;
      co_await std::suspend_always{};

      // read from address, add index register to it
      std::ignore = memory_.read(address); // ???
      address += index;
      co_await std::suspend_always{};

      // read from effective address
      Data const value{ memory_.read(address) };

      std::invoke(operation, this, value);
      co_return std::nullopt;
   }

   Instruction Processor::absolute_indexed(ReadOperation const operation, Index const index) noexcept
   {
      // fetch low byte of address, increment PC
      Data const low_byte_of_address{ memory_.read(program_counter) };
      ++program_counter;
      co_await std::suspend_always{};

      // fetch high byte of address, add index register to low address byte, increment PC
      Data const high_byte_of_address{ memory_.read(program_counter) };
      auto [effective_address, overflow]{ add_low_byte(assemble_address(high_byte_of_address, low_byte_of_address), index) };
      ++program_counter;
      co_await std::suspend_always{};

      // read from effective address, fix the high byte of effective address
      Data value{ memory_.read(effective_address) };
      if (overflow)
      {
         effective_address = add_high_byte(effective_address, 1).first;
         co_await std::suspend_always{};

         // re-read from effective address (+)
         value = memory_.read(effective_address);
      }

      std::invoke(operation, this, value);
      co_return std::nullopt;
   }

   Instruction Processor::x_indirect(ReadOperation const operation) noexcept
   {
      // fetch pointer address, increment PC
      Data pointer_address{ memory_.read(program_counter) };
      ++program_counter;
      co_await std::suspend_always{};

      // read from the address, add X to it
      std::ignore = memory_.read(pointer_address); // ???
      pointer_address += x_;
      co_await std::suspend_always{};

      // fetch effective address low
      Data const effective_address_low{ memory_.read(pointer_address) };
      co_await std::suspend_always{};

      // fetch effective address high
      Data const effective_address_high{ memory_.read(pointer_address + 1) };
      co_await std::suspend_always{};

      // read from effective address
      Address const effective_address{ assemble_address(effective_address_high, effective_address_low) };
      auto const value{ memory_.read(effective_address) };

      std::invoke(operation, this, value);
      co_return std::nullopt;
   }

   Instruction Processor::indirect_y(ReadOperation const operation) noexcept
   {
      // fetch pointer address, increment PC
      Address const pointer_address{ memory_.read(program_counter) };
      ++program_counter;
      co_await std::suspend_always{};

      // fetch effective address low
      Data const effective_address_low{ memory_.read(pointer_address) };
      co_await std::suspend_always{};

      // fetch effective address high, add Y to low byte of effective address
      Data const effective_address_high{ memory_.read(pointer_address + 1) };
      auto [effective_address, overflow]{ add_low_byte(assemble_address(effective_address_high, effective_address_low), y_) };
      co_await std::suspend_always{};

      // read from effective address, fix high byte of effective address
      Data value = memory_.read(effective_address);
      if (overflow)
      {
         effective_address = add_high_byte(effective_address, 1).first;
         co_await std::suspend_always{};

         // re-read from effective address (+)
         value = memory_.read(effective_address);
      }

      std::invoke(operation, this, value);
      co_return std::nullopt;
   }

   Instruction Processor::accumulator(ModifyOperation const operation) noexcept
   {
      // do the operation on the accumulator
      accumulator_ = std::invoke(operation, this, accumulator_);
      co_return std::nullopt;
   }

   Instruction Processor::absolute(ModifyOperation const operation) noexcept
   {
      // fetch low byte of address, increment PC
      Data const low_byte_of_address{ memory_.read(program_counter) };
      ++program_counter;
      co_await std::suspend_always{};

      // fetch high byte of address, increment PC
      Data const high_byte_of_address{ memory_.read(program_counter) };
      ++program_counter;
      co_await std::suspend_always{};

      // read from effective address
      Address const effective_address{ assemble_address(high_byte_of_address, low_byte_of_address) };
      Data value{ memory_.read(effective_address) };
      co_await std::suspend_always{};

      // write the value back to effective address, and do the operation on it
      memory_.write(effective_address, value);
      value = std::invoke(operation, this, value);
      co_await std::suspend_always{};

      // write the new value to effective address
      memory_.write(effective_address, value);
      co_return std::nullopt;
   }

   Instruction Processor::zero_page(ModifyOperation const operation) noexcept
   {
      // fetch address, increment PC
      Address const address{ memory_.read(program_counter) };
      ++program_counter;
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
      co_return std::nullopt;
   }

   Instruction Processor::zero_page_indexed(ModifyOperation const operation, Index const index) noexcept
   {
      // fetch address, increment PC
      Data address{ memory_.read(program_counter) };
      ++program_counter;
      co_await std::suspend_always{};

      // read from address, add index register to it
      std::ignore = memory_.read(address); // ???
      address += index;
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
      co_return std::nullopt;
   }

   Instruction Processor::absolute_indexed(ModifyOperation const operation, Index const index) noexcept
   {
      // fetch low byte of address, increment PC
      Data const low_byte_of_address{ memory_.read(program_counter) };
      ++program_counter;
      co_await std::suspend_always{};

      // fetch high byte of address, add index register to low address byte, increment PC
      Data const high_byte_of_address{ memory_.read(program_counter) };
      auto [effective_address, overflow]{ add_low_byte(assemble_address(high_byte_of_address, low_byte_of_address), index) };
      ++program_counter;
      co_await std::suspend_always{};

      // read from effective address, fix the high byte of effective address
      std::ignore = memory_.read(effective_address);
      if (overflow)
         effective_address = add_high_byte(effective_address, 1).first;
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
      co_return std::nullopt;
   }

   Instruction Processor::x_indirect(ModifyOperation const operation) noexcept
   {
      // fetch pointer address, increment PC
      Data pointer_address{ memory_.read(program_counter) };
      ++program_counter;
      co_await std::suspend_always{};

      // read from the address, add X to it
      std::ignore = memory_.read(pointer_address); // ???
      pointer_address += x_;
      co_await std::suspend_always{};

      // fetch effective address low
      Data const effective_address_low{ memory_.read(pointer_address) };
      co_await std::suspend_always{};

      // fetch effective address high
      Data const effective_address_high{ memory_.read(pointer_address + 1) };
      co_await std::suspend_always{};

      // read from effective address
      Address const effective_address{ assemble_address(effective_address_high, effective_address_low) };
      auto value{ memory_.read(effective_address) };
      co_await std::suspend_always{};

      // write the value back to effective address, and do the operation on it
      memory_.write(effective_address, value);
      value = std::invoke(operation, this, value);
      co_await std::suspend_always{};

      // write the new value to effective address
      memory_.write(effective_address, value);
      co_return std::nullopt;
   }

   Instruction Processor::indirect_y(ModifyOperation const operation) noexcept
   {
      // fetch pointer address, increment PC
      Address const pointer_address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // fetch effective address low
      Data const effective_address_low{ memory_.read(pointer_address) };
      co_await std::suspend_always{};

      // fetch effective address high, add Y to low byte of effective address
      Data const effective_address_high{ memory_.read(pointer_address + 1) };
      auto [effective_address, overflow]{ add_low_byte(assemble_address(effective_address_high, effective_address_low), y_) };
      co_await std::suspend_always{};

      // read from effective address, fix high byte of effective address
      std::ignore = memory_.read(effective_address);
      if (overflow)
         effective_address = add_high_byte(effective_address, 1).first;

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
      co_return std::nullopt;
   }

   Instruction Processor::absolute(WriteOperation const operation) noexcept
   {
      // fetch low byte of address, increment PC
      Data const low_byte_of_address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // fetch high byte of address, increment PC
      Data const high_byte_of_address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // write register to effective address
      Address const effective_address{ assemble_address(high_byte_of_address, low_byte_of_address) };
      memory_.write(effective_address, std::invoke(operation, this));
      co_return std::nullopt;
   }

   Instruction Processor::zero_page(WriteOperation const operation) noexcept
   {
      // fetch address, increment PC
      Address const address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // write register to effective address
      memory_.write(address, std::invoke(operation, this));
      co_return std::nullopt;
   }

   Instruction Processor::zero_page_indexed(WriteOperation const operation, Index const index) noexcept
   {
      // fetch address, increment PC
      Data address{ memory_.read(program_counter) };
      ++program_counter;
      co_await std::suspend_always{};

      // read from address, add index register to it
      std::ignore = memory_.read(address); // ???
      address += index;
      co_await std::suspend_always{};

      // write to effective address
      memory_.write(address, std::invoke(operation, this));
      co_return std::nullopt;
   }

   Instruction Processor::absolute_indexed(WriteOperation const operation, Index const index) noexcept
   {
      // fetch low byte of address, increment PC
      Data const low_byte_of_address{ memory_.read(program_counter) };
      ++program_counter;
      co_await std::suspend_always{};

      // fetch high byte of address, add index register to low address byte, increment PC
      Data const high_byte_of_address{ memory_.read(program_counter) };
      auto [effective_address, overflow]{ add_low_byte(assemble_address(high_byte_of_address, low_byte_of_address), index) };
      ++program_counter;
      co_await std::suspend_always{};

      // read from effective address, fix the high byte of effective address
      std::ignore = memory_.read(effective_address);
      if (overflow)
         effective_address = add_high_byte(effective_address, 1).first;
      co_await std::suspend_always{};

      // write to effective address
      memory_.write(effective_address, std::invoke(operation, this));
      co_return std::nullopt;
   }

   Instruction Processor::x_indirect(WriteOperation const operation) noexcept
   {
      // fetch pointer address, increment PC
      Data pointer_address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // read from the address, add X to it
      std::ignore = memory_.read(pointer_address); // ???
      pointer_address += x_;
      co_await std::suspend_always{};

      // fetch effective address low
      Data const effective_address_low{ memory_.read(pointer_address) };
      co_await std::suspend_always{};

      // fetch effective address high
      Data const effective_address_high{ memory_.read(pointer_address + 1) };
      co_await std::suspend_always{};

      // write to effective address
      Address const effective_address{ assemble_address(effective_address_high, effective_address_low) };
      memory_.write(effective_address, std::invoke(operation, this));
      co_return std::nullopt;
   }

   Instruction Processor::indirect_y(WriteOperation const operation) noexcept
   {
      // fetch pointer address, increment PC
      Address const pointer_address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // fetch effective address low
      Data const effective_address_low{ memory_.read(pointer_address) };
      co_await std::suspend_always{};

      // fetch effective address high, add Y to low byte of effective address
      Data const effective_address_high{ memory_.read(pointer_address + 1) };
      auto [effective_address, overflow]{ add_low_byte(assemble_address(effective_address_high, effective_address_low), y_) };
      co_await std::suspend_always{};

      // read from effective address, fix high byte of effective address
      std::ignore = memory_.read(effective_address);
      if (overflow)
         effective_address = add_high_byte(effective_address, 1).first;
      co_await std::suspend_always{};

      // write to effective address
      memory_.write(effective_address, std::invoke(operation, this));
      co_return std::nullopt;
   }

   // TODO: find what exactly happens here
   Instruction Processor::RST() noexcept
   {
      co_await std::suspend_always{};

      co_await std::suspend_always{};

      --stack_pointer_;
      co_await std::suspend_always{};

      --stack_pointer_;
      co_await std::suspend_always{};

      --stack_pointer_;
      co_await std::suspend_always{};

      program_counter = program_counter & 0xFF00 | memory_.read(0xFFFC);
      co_await std::suspend_always{};

      program_counter = memory_.read(0xFFFD) << 8 | program_counter & 0x00FF;
      co_return std::nullopt;
   }

   Instruction Processor::BRK() noexcept
   {
      // read next instruction byte (and throw it away), increment PC
      std::ignore = memory_.read(program_counter);
      ++program_counter;
      co_await std::suspend_always{};

      // push PCH on stack (with B flag set)
      change_processor_status_flag(ProcessorStatusFlag::B, true);
      push(program_counter >> 8 & 0x00FF);
      co_await std::suspend_always{};

      // push PCL on stack
      push(program_counter & 0x00FF);
      co_await std::suspend_always{};

      // push P on stack
      push(processor_status_);
      co_await std::suspend_always{};

      // fetch PCL
      program_counter = program_counter & 0xFF00 | memory_.read(0xFFFE);
      co_await std::suspend_always{};

      // fetch PCH
      program_counter = memory_.read(0xFFFF) << 8 | program_counter & 0x00FF;

      // ???
      change_processor_status_flag(ProcessorStatusFlag::I, true);
      co_return std::nullopt;
   }

   Instruction Processor::PHP() noexcept
   {
      // read next instruction byte (and throw it away)
      std::ignore = memory_.read(program_counter);
      co_await std::suspend_always{};

      // push P on stack (with B and _ flag set)
      change_processor_status_flags({ ProcessorStatusFlag::B, ProcessorStatusFlag::_ }, true);
      push(processor_status_);
      co_return std::nullopt;
   }

   Instruction Processor::CLC() noexcept
   {
      change_processor_status_flag(ProcessorStatusFlag::C, false);
      co_return std::nullopt;
   }

   Instruction Processor::JSR() noexcept
   {
      // fetch low address byte, increment PC
      Data const low_address_byte{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // internal operation (pre-decrement S?)
      // --stack_pointer_;
      co_await std::suspend_always{};

      // push PCH on stack
      push(program_counter >> 8 & 0x00FF);
      co_await std::suspend_always{};

      // push PCL on stack
      push(program_counter & 0x00FF);
      co_await std::suspend_always{};

      // copy low address byte to PCL, fetch high address byte to PCH
      program_counter = assemble_address(memory_.read(program_counter), low_address_byte);
      co_return std::nullopt;
   }

   Instruction Processor::PLP() noexcept
   {
      // read next instruction byte (and throw it away)
      std::ignore = memory_.read(program_counter);
      co_await std::suspend_always{};

      // increment S
      ++stack_pointer_;
      co_await std::suspend_always{};

      // pull register from stack (with B and _ flag ignored)
      processor_status_ = processor_status_ & 0b00'11'00'00 | memory_.read(0x0100 + stack_pointer_);
      co_return std::nullopt;
   }

   Instruction Processor::SEC() noexcept
   {
      // set C
      change_processor_status_flag(ProcessorStatusFlag::C, true);
      co_return std::nullopt;
   }

   Instruction Processor::RTI() noexcept
   {
      // read next instruction byte (and throw it away)
      std::ignore = memory_.read(program_counter);
      co_await std::suspend_always{};

      // increment S
      ++stack_pointer_;
      co_await std::suspend_always{};

      // pull P from stack, increment S
      processor_status_ = processor_status_ & 0b00'11'00'00 | memory_.read(0x0100 + stack_pointer_);
      ++stack_pointer_;
      co_await std::suspend_always{};

      // pull PCL from stack, increment S
      program_counter = program_counter & 0xFF00 | memory_.read(0x0100 + stack_pointer_);
      ++stack_pointer_;
      co_await std::suspend_always{};

      // pull PCH from stack
      program_counter = memory_.read(0x0100 + stack_pointer_) << 8 | program_counter & 0x00FF;
      co_return std::nullopt;
   }

   Instruction Processor::PHA() noexcept
   {
      // read next instruction byte (and throw it away)
      std::ignore = memory_.read(program_counter);
      co_await std::suspend_always{};

      // push register on stack
      push(accumulator_);
      co_return std::nullopt;
   }

   Instruction Processor::JMP_absolute() noexcept
   {
      // fetch low address byte, increment PC
      Data const low_address_byte{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // copy low address byte to PCL, fetch high address byte to PCH
      program_counter = assemble_address(memory_.read(program_counter), low_address_byte);
      co_return std::nullopt;
   }

   Instruction Processor::CLI() noexcept
   {
      // clear I
      change_processor_status_flag(ProcessorStatusFlag::I, false);
      co_return std::nullopt;
   }

   Instruction Processor::RTS() noexcept
   {
      // read next instruction byte (and throw it away)
      std::ignore = memory_.read(program_counter);
      co_await std::suspend_always{};

      // increment S
      ++stack_pointer_;
      co_await std::suspend_always{};

      // pull PCL from stack, increment S
      program_counter = program_counter & 0xFF00 | memory_.read(0x0100 + stack_pointer_);
      ++stack_pointer_;
      co_await std::suspend_always{};

      // pull PCH from stack
      program_counter = memory_.read(0x0100 + stack_pointer_) << 8 | program_counter & 0x00FF;
      co_await std::suspend_always{};

      // increment PC
      ++program_counter;
      co_return std::nullopt;
   }

   Instruction Processor::PLA() noexcept
   {
      // read next instruction byte (and throw it away)
      std::ignore = memory_.read(program_counter);
      co_await std::suspend_always{};

      // increment S
      ++stack_pointer_;
      co_await std::suspend_always{};

      // pull register from stack
      update_zero_and_negative_flag(accumulator_ = memory_.read(0x0100 + stack_pointer_));
      co_return std::nullopt;
   }

   Instruction Processor::JMP_indirect() noexcept
   {
      // fetch pointer address low, increment PC
      Data const pointer_address_low{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // fetch pointer address high, increment PC
      Data const pointer_address_high{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // fetch low address to latch
      Address const pointer_address{ assemble_address(pointer_address_high, pointer_address_low) };
      Data const low_address{ memory_.read(pointer_address) };
      co_await std::suspend_always{};

      // fetch PCH, copy latch to PCL
      program_counter = assemble_address(memory_.read(pointer_address + 1), low_address);
      co_return std::nullopt;
   }

   Instruction Processor::SEI() noexcept
   {
      // set I
      change_processor_status_flag(ProcessorStatusFlag::I, true);
      co_return std::nullopt;
   }

   Instruction Processor::DEY() noexcept
   {
      // decrement Y
      update_zero_and_negative_flag(--y_);
      co_return std::nullopt;
   }

   Instruction Processor::TXA() noexcept
   {
      // transfer X to A
      update_zero_and_negative_flag(accumulator_ = x_);
      co_return std::nullopt;
   }

   Instruction Processor::TYA() noexcept
   {
      // transfer Y to A
      update_zero_and_negative_flag(accumulator_ = y_);
      co_return std::nullopt;
   }

   Instruction Processor::TXS() noexcept
   {
      // transfer X to S
      stack_pointer_ = x_;
      co_return std::nullopt;
   }

   Instruction Processor::TAY() noexcept
   {
      // transfer A to Y
      update_zero_and_negative_flag(y_ = accumulator_);
      co_return std::nullopt;
   }

   Instruction Processor::TAX() noexcept
   {
      // transfer A to X
      update_zero_and_negative_flag(x_ = accumulator_);
      co_return std::nullopt;
   }

   Instruction Processor::CLV() noexcept
   {
      // clear V
      change_processor_status_flag(ProcessorStatusFlag::V, false);
      co_return std::nullopt;
   }

   Instruction Processor::TSX() noexcept
   {
      // transfer S to X
      update_zero_and_negative_flag(x_ = stack_pointer_);
      co_return std::nullopt;
   }

   Instruction Processor::INY() noexcept
   {
      // increment Y
      update_zero_and_negative_flag(++y_);
      co_return std::nullopt;
   }

   Instruction Processor::DEX() noexcept
   {
      // decrement X
      update_zero_and_negative_flag(--x_);
      co_return std::nullopt;
   }

   Instruction Processor::CLD() noexcept
   {
      // clear D
      change_processor_status_flag(ProcessorStatusFlag::D, false);
      co_return std::nullopt;
   }

   Instruction Processor::INX() noexcept
   {
      // increment X
      update_zero_and_negative_flag(++x_);
      co_return std::nullopt;
   }

   Instruction Processor::NOP() noexcept
   {
      co_return std::nullopt;
   }

   Instruction Processor::SED() noexcept
   {
      // set D
      change_processor_status_flag(ProcessorStatusFlag::D, true);
      co_return std::nullopt;
   }

   bool Processor::BPL() const noexcept
   {
      return not processor_status_flag(ProcessorStatusFlag::N);
   }

   bool Processor::BMI() const noexcept
   {
      return processor_status_flag(ProcessorStatusFlag::N);
   }

   bool Processor::BVC() const noexcept
   {
      return not processor_status_flag(ProcessorStatusFlag::V);
   }

   bool Processor::BVS() const noexcept
   {
      return processor_status_flag(ProcessorStatusFlag::V);
   }

   bool Processor::BCC() const noexcept
   {
      return not processor_status_flag(ProcessorStatusFlag::C);
   }

   bool Processor::BCS() const noexcept
   {
      return processor_status_flag(ProcessorStatusFlag::C);
   }

   bool Processor::BNE() const noexcept
   {
      return not processor_status_flag(ProcessorStatusFlag::Z);
   }

   bool Processor::BEQ() const noexcept
   {
      return processor_status_flag(ProcessorStatusFlag::Z);
   }

   void Processor::ORA(Data const value) noexcept
   {
      // ORA value with accumulator
      update_zero_and_negative_flag(accumulator_ |= value);
   }

   void Processor::LDA(Data const value) noexcept
   {
      // load value into accumulator
      update_zero_and_negative_flag(accumulator_ = value);
   }

   void Processor::AND(Data const value) noexcept
   {
      // AND value with accumulator
      update_zero_and_negative_flag(accumulator_ &= value);
   }

   void Processor::BIT(Data const value) noexcept
   {
      // N set to bit 7 of value
      change_processor_status_flag(ProcessorStatusFlag::N, value & 0b1000'0000);

      // V set to bit 6 of value
      change_processor_status_flag(ProcessorStatusFlag::V, value & 0b0100'0000);

      // Z set if value AND accumulator is zero
      change_processor_status_flag(ProcessorStatusFlag::Z, not(value & accumulator_));
   }

   void Processor::EOR(Data const value) noexcept
   {
      // EOR value with accumulator
      update_zero_and_negative_flag(accumulator_ ^= value);
   }

   void Processor::ADC(Data const value) noexcept
   {
      // perform addition
      auto result{ accumulator_ + value };
      if (processor_status_flag(ProcessorStatusFlag::C))
         ++result;

      // set C if there was a carry-out
      change_processor_status_flag(ProcessorStatusFlag::C, result > 0xFF);

      // set V if there was a signed overflow
      change_processor_status_flag(ProcessorStatusFlag::V,
         ((accumulator_ ^ result) & (value ^ result) & 0b1000'0000) not_eq 0);

      // store result in accumulator
      update_zero_and_negative_flag(accumulator_ = static_cast<Data>(result));
   }

   void Processor::LDY(Data const value) noexcept
   {
      // load value into Y register
      update_zero_and_negative_flag(y_ = value);
   }

   void Processor::LDX(Data const value) noexcept
   {
      // load value into X register
      update_zero_and_negative_flag(x_ = value);
   }

   void Processor::CPY(Data const value) noexcept
   {
      change_processor_status_flag(ProcessorStatusFlag::C, y_ >= value);
      update_zero_and_negative_flag(y_ - value);
   }

   void Processor::CMP(Data const value) noexcept
   {
      change_processor_status_flag(ProcessorStatusFlag::C, accumulator_ >= value);
      update_zero_and_negative_flag(accumulator_ - value);
   }

   void Processor::CPX(Data const value) noexcept
   {
      change_processor_status_flag(ProcessorStatusFlag::C, x_ >= value);
      update_zero_and_negative_flag(x_ - value);
   }

   void Processor::SBC(Data const value) noexcept
   {
      // perform subtraction
      auto result{ accumulator_ - value };
      if (not processor_status_flag(ProcessorStatusFlag::C))
         --result;

      // set C if there was no borrow
      change_processor_status_flag(ProcessorStatusFlag::C, result >= 0);

      // set V if there was a signed overflow
      change_processor_status_flag(ProcessorStatusFlag::V,
         ((accumulator_ ^ value) & (accumulator_ ^ result) & 0b1000'0000) != 0);

      // store result in accumulator
      update_zero_and_negative_flag(accumulator_ = static_cast<Data>(result));
   }

   Data Processor::ASL(Data value) noexcept
   {
      // C set to bit 7 of value before ASL
      change_processor_status_flag(ProcessorStatusFlag::C, value & 0b1000'0000);

      // shift value left by 1
      update_zero_and_negative_flag(value <<= 1);
      return value;
   }

   Data Processor::ROL(Data value) noexcept
   {
      // save old C
      bool const old_carry{ processor_status_flag(ProcessorStatusFlag::C) };

      // C set to bit 7 of value before ROL
      change_processor_status_flag(ProcessorStatusFlag::C, value & 0b1000'0000);

      // shift value left by 1, set bit 0 to old C
      update_zero_and_negative_flag(value = value << 1 | static_cast<Data>(old_carry));
      return value;
   }

   Data Processor::LSR(Data value) noexcept
   {
      // C set to bit 0 of value before LSR
      change_processor_status_flag(ProcessorStatusFlag::C, value & 0b0000'0001);

      // shift value right by 1
      update_zero_and_negative_flag(value >>= 1);
      return value;
   }

   Data Processor::ROR(Data value) noexcept
   {
      // save old C
      bool const old_carry{ processor_status_flag(ProcessorStatusFlag::C) };

      // C set to bit 0 of value before ROR
      change_processor_status_flag(ProcessorStatusFlag::C, value & 0b0000'0001);

      // shift value right by 1, set bit 7 to old C
      update_zero_and_negative_flag(value = value >> 1 | old_carry << 7);
      return value;
   }

   Data Processor::DEC(Data value) noexcept
   {
      // decrement value
      update_zero_and_negative_flag(--value);
      return value;
   }

   Data Processor::INC(Data value) noexcept
   {
      // increment value
      update_zero_and_negative_flag(++value);
      return value;
   }

   Data Processor::STA() noexcept
   {
      return accumulator_;
   }

   Data Processor::STY() noexcept
   {
      return y_;
   }

   Data Processor::STX() noexcept
   {
      return x_;
   }

   Instruction Processor::instruction_from_opcode(Opcode const opcode)
   {
      switch (opcode)
      {
         case Opcode::BRK_IMPLIED:
            return BRK();

         case Opcode::ORA_X_INDIRECT:
            return x_indirect(&Processor::ORA);

         case Opcode::JAM_IMPLIED_02:
            return Instruction{ {} };

         case Opcode::SLO_X_INDIRECT:
            return Instruction{ {} };

         case Opcode::NOP_ZERO_PAGE_04:
            return Instruction{ {} };

         case Opcode::ORA_ZERO_PAGE:
            return zero_page(&Processor::ORA);

         case Opcode::ASL_ZERO_PAGE:
            return zero_page(&Processor::ASL);

         case Opcode::SLO_ZERO_PAGE:
            return Instruction{ {} };

         case Opcode::PHP_IMPLIED:
            return PHP();

         case Opcode::ORA_IMMEDIATE:
            return immediate(&Processor::ORA);

         case Opcode::ASL_ACCUMULATOR:
            return accumulator(&Processor::ASL);

         case Opcode::ANC_IMMEDIATE_0B:
            return Instruction{ {} };

         case Opcode::NOP_ABSOLUTE:
            return Instruction{ {} };

         case Opcode::ORA_ABSOLUTE:
            return absolute(&Processor::ORA);

         case Opcode::ASL_ABSOLUTE:
            return absolute(&Processor::ASL);

         case Opcode::SLO_ABSOLUTE:
            return Instruction{ {} };

         case Opcode::BPL_RELATIVE:
            return relative(&Processor::BPL);

         case Opcode::ORA_INDIRECT_Y:
            return indirect_y(&Processor::ORA);

         case Opcode::JAM_IMPLIED_12:
            return Instruction{ {} };

         case Opcode::SLO_INDIRECT_Y:
            return Instruction{ {} };

         case Opcode::NOP_ZERO_PAGE_X_14:
            return Instruction{ {} };

         case Opcode::ORA_ZERO_PAGE_X:
            return zero_page_indexed(&Processor::ORA, x_);

         case Opcode::ASL_ZERO_PAGE_X:
            return zero_page_indexed(&Processor::ASL, x_);

         case Opcode::SLO_ZERO_PAGE_X:
            return Instruction{ {} };

         case Opcode::CLC_IMPLIED:
            return CLC();

         case Opcode::ORA_ABSOLUTE_Y:
            return absolute_indexed(&Processor::ORA, y_);

         case Opcode::NOP_IMPLIED_1A:
            return Instruction{ {} };

         case Opcode::SLO_ABSOLUTE_Y:
            return Instruction{ {} };

         case Opcode::NOP_ABSOLUTE_X_1C:
            return Instruction{ {} };

         case Opcode::ORA_ABSOLUTE_X:
            return absolute_indexed(&Processor::ORA, x_);

         case Opcode::ASL_ABSOLUTE_X:
            return absolute_indexed(&Processor::ASL, x_);

         case Opcode::SLO_ABSOLUTE_X:
            return Instruction{ {} };

         case Opcode::JSR_ABSOLUTE:
            return JSR();

         case Opcode::AND_X_INDIRECT:
            return x_indirect(&Processor::AND);

         case Opcode::JAM_IMPLIED_22:
            return Instruction{ {} };

         case Opcode::RLA_X_INDIRECT:
            return Instruction{ {} };

         case Opcode::BIT_ZERO_PAGE:
            return zero_page(&Processor::BIT);

         case Opcode::AND_ZERO_PAGE:
            return zero_page(&Processor::AND);

         case Opcode::ROL_ZERO_PAGE:
            return zero_page(&Processor::ROL);

         case Opcode::RLA_ZERO_PAGE:
            return Instruction{ {} };

         case Opcode::PLP_IMPLIED:
            return PLP();

         case Opcode::AND_IMMEDIATE:
            return immediate(&Processor::AND);

         case Opcode::ROL_ACCUMULATOR:
            return accumulator(&Processor::ROL);

         case Opcode::ANC_IMMEDIATE_2B:
            return Instruction{ {} };

         case Opcode::BIT_ABSOLUTE:
            return absolute(&Processor::BIT);

         case Opcode::AND_ABSOLUTE:
            return absolute(&Processor::AND);

         case Opcode::ROL_ABSOLUTE:
            return absolute(&Processor::ROL);

         case Opcode::RLA_ABSOLUTE:
            return Instruction{ {} };

         case Opcode::BMI_RELATIVE:
            return relative(&Processor::BMI);

         case Opcode::AND_INDIRECT_Y:
            return indirect_y(&Processor::AND);

         case Opcode::JAM_IMPLIED_32:
            return Instruction{ {} };

         case Opcode::RLA_INDIRECT_Y:
            return Instruction{ {} };

         case Opcode::NOP_ZERO_PAGE_X_34:
            return Instruction{ {} };

         case Opcode::AND_ZERO_PAGE_X:
            return zero_page_indexed(&Processor::AND, x_);

         case Opcode::ROL_ZERO_PAGE_X:
            return zero_page_indexed(&Processor::ROL, x_);

         case Opcode::RLA_ZERO_PAGE_X:
            return Instruction{ {} };

         case Opcode::SEC_IMPLIED:
            return SEC();

         case Opcode::AND_ABSOLUTE_Y:
            return absolute_indexed(&Processor::AND, y_);

         case Opcode::NOP_IMPLIED_3A:
            return Instruction{ {} };

         case Opcode::RLA_ABSOLUTE_Y:
            return Instruction{ {} };

         case Opcode::NOP_ABSOLUTE_X_3C:
            return Instruction{ {} };

         case Opcode::AND_ABSOLUTE_X:
            return absolute_indexed(&Processor::AND, x_);

         case Opcode::ROL_ABSOLUTE_X:
            return absolute_indexed(&Processor::ROL, x_);

         case Opcode::RLA_ABSOLUTE_X:
            return Instruction{ {} };

         case Opcode::RTI_IMPLIED:
            return RTI();

         case Opcode::EOR_X_INDIRECT:
            return x_indirect(&Processor::EOR);

         case Opcode::JAM_IMPLIED_42:
            return Instruction{ {} };

         case Opcode::SRE_X_INDIRECT:
            return Instruction{ {} };

         case Opcode::NOP_ZERO_PAGE_44:
            return Instruction{ {} };

         case Opcode::EOR_ZERO_PAGE:
            return zero_page(&Processor::EOR);

         case Opcode::LSR_ZERO_PAGE:
            return zero_page(&Processor::LSR);

         case Opcode::SRE_ZERO_PAGE:
            return Instruction{ {} };

         case Opcode::PHA_IMPLIED:
            return PHA();

         case Opcode::EOR_IMMEDIATE:
            return immediate(&Processor::EOR);

         case Opcode::LSR_ACCUMULATOR:
            return accumulator(&Processor::LSR);

         case Opcode::ALR_IMMEDIATE_4B:
            return Instruction{ {} };

         case Opcode::JMP_ABSOLUTE:
            return JMP_absolute();

         case Opcode::EOR_ABSOLUTE:
            return absolute(&Processor::EOR);

         case Opcode::LSR_ABSOLUTE:
            return absolute(&Processor::LSR);

         case Opcode::SRE_ABSOLUTE:
            return Instruction{ {} };

         case Opcode::BVC_RELATIVE:
            return relative(&Processor::BVC);

         case Opcode::EOR_INDIRECT_Y:
            return indirect_y(&Processor::EOR);

         case Opcode::JAM_IMPLIED_52:
            return Instruction{ {} };

         case Opcode::SRE_INDIRECT_Y:
            return Instruction{ {} };

         case Opcode::NOP_ZERO_PAGE_X_54:
            return Instruction{ {} };

         case Opcode::EOR_ZERO_PAGE_X:
            return zero_page_indexed(&Processor::EOR, x_);

         case Opcode::LSR_ZERO_PAGE_X:
            return zero_page_indexed(&Processor::LSR, x_);

         case Opcode::SRE_ZERO_PAGE_X:
            return Instruction{ {} };

         case Opcode::CLI_IMPLIED:
            return CLI();

         case Opcode::EOR_ABSOLUTE_Y:
            return absolute_indexed(&Processor::EOR, y_);

         case Opcode::NOP_IMPLIED_5A:
            return Instruction{ {} };

         case Opcode::SRE_ABSOLUTE_Y:
            return Instruction{ {} };

         case Opcode::NOP_ABSOLUTE_X_5C:
            return Instruction{ {} };

         case Opcode::EOR_ABSOLUTE_X:
            return absolute_indexed(&Processor::EOR, x_);

         case Opcode::LSR_ABSOLUTE_X:
            return absolute_indexed(&Processor::LSR, x_);

         case Opcode::SRE_ABSOLUTE_X:
            return Instruction{ {} };

         case Opcode::RTS_IMPLIED:
            return RTS();

         case Opcode::ADC_X_INDIRECT:
            return x_indirect(&Processor::ADC);

         case Opcode::JAM_IMPLIED_62:
            return Instruction{ {} };

         case Opcode::RRA_X_INDIRECT:
            return Instruction{ {} };

         case Opcode::NOP_ZERO_PAGE_64:
            return Instruction{ {} };

         case Opcode::ADC_ZERO_PAGE:
            return zero_page(&Processor::ADC);

         case Opcode::ROR_ZERO_PAGE:
            return zero_page(&Processor::ROR);

         case Opcode::RRA_ZERO_PAGE:
            return Instruction{ {} };

         case Opcode::PLA_IMPLIED:
            return PLA();

         case Opcode::ADC_IMMEDIATE:
            return immediate(&Processor::ADC);

         case Opcode::ROR_ACCUMULATOR:
            return accumulator(&Processor::ROR);

         case Opcode::ARR_IMMEDIATE:
            return Instruction{ {} };

         case Opcode::JMP_INDIRECT:
            return JMP_indirect();

         case Opcode::ADC_ABSOLUTE:
            return absolute(&Processor::ADC);

         case Opcode::ROR_ABSOLUTE:
            return absolute(&Processor::ROR);

         case Opcode::RRA_ABSOLUTE:
            return Instruction{ {} };

         case Opcode::BVS_RELATIVE:
            return relative(&Processor::BVS);

         case Opcode::ADC_INDIRECT_Y:
            return indirect_y(&Processor::ADC);

         case Opcode::JAM_IMPLIED_72:
            return Instruction{ {} };

         case Opcode::RRA_INDIRECT_Y:
            return Instruction{ {} };

         case Opcode::NOP_ZERO_PAGE_X_74:
            return Instruction{ {} };

         case Opcode::ADC_ZERO_PAGE_X:
            return zero_page_indexed(&Processor::ADC, x_);

         case Opcode::ROR_ZERO_PAGE_X:
            return zero_page_indexed(&Processor::ROR, x_);

         case Opcode::RRA_ZERO_PAGE_X:
            return Instruction{ {} };

         case Opcode::SEI_IMPLIED:
            return SEI();

         case Opcode::ADC_ABSOLUTE_Y:
            return absolute_indexed(&Processor::ADC, y_);

         case Opcode::NOP_IMPLIED_7A:
            return Instruction{ {} };

         case Opcode::RRA_ABSOLUTE_Y:
            return Instruction{ {} };

         case Opcode::NOP_ABSOLUTE_X_7C:
            return Instruction{ {} };

         case Opcode::ADC_ABSOLUTE_X:
            return absolute_indexed(&Processor::ADC, x_);

         case Opcode::ROR_ABSOLUTE_X:
            return absolute_indexed(&Processor::ROR, x_);

         case Opcode::RRA_ABSOLUTE_X:
            return Instruction{ {} };

         case Opcode::NOP_IMMEDIATE_80:
            return Instruction{ {} };

         case Opcode::STA_X_INDIRECT:
            return x_indirect(&Processor::STA);

         case Opcode::NOP_IMMEDIATE_82:
            return Instruction{ {} };

         case Opcode::SAX_X_INDIRECT:
            return Instruction{ {} };

         case Opcode::STY_ZERO_PAGE:
            return zero_page(&Processor::STY);

         case Opcode::STA_ZERO_PAGE:
            return zero_page(&Processor::STA);

         case Opcode::STX_ZERO_PAGE:
            return zero_page(&Processor::STX);

         case Opcode::SAX_ZERO_PAGE:
            return Instruction{ {} };

         case Opcode::DEY_IMPLIED:
            return DEY();

         case Opcode::NOP_IMMEDIATE_89:
            return Instruction{ {} };

         case Opcode::TXA_IMPLIED:
            return TXA();

         case Opcode::ANE_IMMEDIATE:
            return Instruction{ {} };

         case Opcode::STY_ABSOLUTE:
            return absolute(&Processor::STY);

         case Opcode::STA_ABSOLUTE:
            return absolute(&Processor::STA);

         case Opcode::STX_ABSOLUTE:
            return absolute(&Processor::STX);

         case Opcode::SAX_ABSOLUTE:
            return Instruction{ {} };

         case Opcode::BCC_RELATIVE:
            return relative(&Processor::BCC);

         case Opcode::STA_INDIRECT_Y:
            return indirect_y(&Processor::STA);

         case Opcode::JAM_IMPLIED_92:
            return Instruction{ {} };

         case Opcode::SHA_INDIRECT_Y:
            return Instruction{ {} };

         case Opcode::STY_ZERO_PAGE_X:
            return zero_page_indexed(&Processor::STY, x_);

         case Opcode::STA_ZERO_PAGE_X:
            return zero_page_indexed(&Processor::STA, x_);

         case Opcode::STX_ZERO_PAGE_Y:
            return zero_page_indexed(&Processor::STX, y_);

         case Opcode::SAX_ZERO_PAGE_Y:
            return Instruction{ {} };

         case Opcode::TYA_IMPLIED:
            return TYA();

         case Opcode::STA_ABSOLUTE_Y:
            return absolute_indexed(&Processor::STA, y_);

         case Opcode::TXS_IMPLIED:
            return TXS();

         case Opcode::TAS_ABSOLUTE_Y:
            return Instruction{ {} };

         case Opcode::SHY_ABSOLUTE_X:
            return Instruction{ {} };

         case Opcode::STA_ABSOLUTE_X:
            return absolute_indexed(&Processor::STA, x_);

         case Opcode::SHX_ABSOLUTE_Y:
            return Instruction{ {} };

         case Opcode::SHA_ABSOLUTE_Y:
            return Instruction{ {} };

         case Opcode::LDY_IMMEDIATE:
            return immediate(&Processor::LDY);

         case Opcode::LDA_X_INDIRECT:
            return x_indirect(&Processor::LDA);

         case Opcode::LDX_IMMEDIATE:
            return immediate(&Processor::LDX);

         case Opcode::LAX_X_INDIRECT:
            return Instruction{ {} };

         case Opcode::LDY_ZERO_PAGE:
            return zero_page(&Processor::LDY);

         case Opcode::LDA_ZERO_PAGE:
            return zero_page(&Processor::LDA);

         case Opcode::LDX_ZERO_PAGE:
            return zero_page(&Processor::LDX);

         case Opcode::LAX_ZERO_PAGE:
            return Instruction{ {} };

         case Opcode::TAY_IMPLIED:
            return TAY();

         case Opcode::LDA_IMMEDIATE:
            return immediate(&Processor::LDA);

         case Opcode::TAX_IMPLIED:
            return TAX();

         case Opcode::LXA_IMMEDIATE:
            return Instruction{ {} };

         case Opcode::LDY_ABSOLUTE:
            return absolute(&Processor::LDY);

         case Opcode::LDA_ABSOLUTE:
            return absolute(&Processor::LDA);

         case Opcode::LDX_ABSOLUTE:
            return absolute(&Processor::LDX);

         case Opcode::LAX_ABSOLUTE:
            return Instruction{ {} };

         case Opcode::BCS_RELATIVE:
            return relative(&Processor::BCS);

         case Opcode::LDA_INDIRECT_Y:
            return indirect_y(&Processor::LDA);

         case Opcode::JAM_IMPLIED_B2:
            return Instruction{ {} };

         case Opcode::LAX_INDIRECT_Y:
            return Instruction{ {} };

         case Opcode::LDY_ZERO_PAGE_X:
            return zero_page_indexed(&Processor::LDY, x_);

         case Opcode::LDA_ZERO_PAGE_X:
            return zero_page_indexed(&Processor::LDA, x_);

         case Opcode::LDX_ZERO_PAGE_Y:
            return zero_page_indexed(&Processor::LDX, y_);

         case Opcode::LAX_ZERO_PAGE_Y:
            return Instruction{ {} };

         case Opcode::CLV_IMPLIED:
            return CLV();

         case Opcode::LDA_ABSOLUTE_Y:
            return absolute_indexed(&Processor::LDA, y_);

         case Opcode::TSX_IMPLIED:
            return TSX();

         case Opcode::LAS_ABSOLUTE_Y:
            return Instruction{ {} };

         case Opcode::LDY_ABSOLUTE_X:
            return absolute_indexed(&Processor::LDY, x_);

         case Opcode::LDA_ABSOLUTE_X:
            return absolute_indexed(&Processor::LDA, x_);

         case Opcode::LDX_ABSOLUTE_Y:
            return absolute_indexed(&Processor::LDX, y_);

         case Opcode::LAX_ABSOLUTE_Y:
            return Instruction{ {} };

         case Opcode::CPY_IMMEDIATE:
            return immediate(&Processor::CPY);

         case Opcode::CMP_X_INDIRECT:
            return x_indirect(&Processor::CMP);

         case Opcode::NOP_IMMEDIATE_C2:
            return Instruction{ {} };

         case Opcode::DCP_X_INDIRECT:
            return Instruction{ {} };

         case Opcode::CPY_ZERO_PAGE:
            return zero_page(&Processor::CPY);

         case Opcode::CMP_ZERO_PAGE:
            return zero_page(&Processor::CMP);

         case Opcode::DEC_ZERO_PAGE:
            return zero_page(&Processor::DEC);

         case Opcode::DCP_ZERO_PAGE:
            return Instruction{ {} };

         case Opcode::INY_IMPLIED:
            return INY();

         case Opcode::CMP_IMMEDIATE:
            return immediate(&Processor::CMP);

         case Opcode::DEX_IMPLIED:
            return DEX();

         case Opcode::SBX_IMMEDIATE:
            return Instruction{ {} };

         case Opcode::CPY_ABSOLUTE:
            return absolute(&Processor::CPY);

         case Opcode::CMP_ABSOLUTE:
            return absolute(&Processor::CMP);

         case Opcode::DEC_ABSOLUTE:
            return absolute(&Processor::DEC);

         case Opcode::DCP_ABSOLUTE:
            return Instruction{ {} };

         case Opcode::BNE_RELATIVE:
            return relative(&Processor::BNE);

         case Opcode::CMP_INDIRECT_Y:
            return indirect_y(&Processor::CMP);

         case Opcode::JAM_IMPLIED_D2:
            return Instruction{ {} };

         case Opcode::DCP_INDIRECT_Y:
            return Instruction{ {} };

         case Opcode::NOP_ZERO_PAGE_X_D4:
            return Instruction{ {} };

         case Opcode::CMP_ZERO_PAGE_X:
            return zero_page_indexed(&Processor::CMP, x_);

         case Opcode::DEC_ZERO_PAGE_X:
            return zero_page_indexed(&Processor::DEC, x_);

         case Opcode::DCP_ZERO_PAGE_X:
            return Instruction{ {} };

         case Opcode::CLD_IMPLIED:
            return CLD();

         case Opcode::CMP_ABSOLUTE_Y:
            return absolute_indexed(&Processor::CMP, y_);

         case Opcode::NOP_IMPLIED_DA:
            return Instruction{ {} };

         case Opcode::DCP_ABSOLUTE_Y:
            return Instruction{ {} };

         case Opcode::NOP_ABSOLUTE_X_DC:
            return Instruction{ {} };

         case Opcode::CMP_ABSOLUTE_X:
            return absolute_indexed(&Processor::CMP, x_);

         case Opcode::DEC_ABSOLUTE_X:
            return absolute_indexed(&Processor::DEC, x_);

         case Opcode::DCP_ABSOLUTE_X:
            return Instruction{ {} };

         case Opcode::CPX_IMMEDIATE:
            return immediate(&Processor::CPX);

         case Opcode::SBC_X_INDIRECT:
            return x_indirect(&Processor::SBC);

         case Opcode::NOP_IMMEDIATE_E2:
            return Instruction{ {} };

         case Opcode::ISC_X_INDIRECT:
            return Instruction{ {} };

         case Opcode::CPX_ZERO_PAGE:
            return zero_page(&Processor::CPX);

         case Opcode::SBC_ZERO_PAGE:
            return zero_page(&Processor::SBC);

         case Opcode::INC_ZERO_PAGE:
            return zero_page(&Processor::INC);

         case Opcode::ISC_ZERO_PAGE:
            return Instruction{ {} };

         case Opcode::INX_IMPLIED:
            return INX();

         case Opcode::SBC_IMMEDIATE_E9:
            return immediate(&Processor::SBC);

         case Opcode::NOP_IMPLIED_EA:
            return NOP();

         case Opcode::SBC_IMMEDIATE_EB:
            return Instruction{ {} };

         case Opcode::CPX_ABSOLUTE:
            return absolute(&Processor::CPX);

         case Opcode::SBC_ABSOLUTE:
            return absolute(&Processor::SBC);

         case Opcode::INC_ABSOLUTE:
            return absolute(&Processor::INC);

         case Opcode::ISC_ABSOLUTE:
            return Instruction{ {} };

         case Opcode::BEQ_RELATIVE:
            return relative(&Processor::BEQ);

         case Opcode::SBC_INDIRECT_Y:
            return indirect_y(&Processor::SBC);

         case Opcode::JAM_IMPLIED_F2:
            return Instruction{ {} };

         case Opcode::ISC_INDIRECT_Y:
            return Instruction{ {} };

         case Opcode::NOP_ZERO_PAGE_X_F4:
            return Instruction{ {} };

         case Opcode::SBC_ZERO_PAGE_X:
            return zero_page_indexed(&Processor::SBC, x_);

         case Opcode::INC_ZERO_PAGE_X:
            return zero_page_indexed(&Processor::INC, x_);

         case Opcode::ISC_ZERO_PAGE_X:
            return Instruction{ {} };

         case Opcode::SED_IMPLIED:
            return SED();

         case Opcode::SBC_ABSOLUTE_Y:
            return absolute_indexed(&Processor::SBC, y_);

         case Opcode::NOP_IMPLIED_FA:
            return Instruction{ {} };

         case Opcode::ISC_ABSOLUTE_Y:
            return Instruction{ {} };

         case Opcode::NOP_ABSOLUTE_X_FC:
            return Instruction{ {} };

         case Opcode::SBC_ABSOLUTE_X:
            return absolute_indexed(&Processor::SBC, x_);

         case Opcode::INC_ABSOLUTE_X:
            return absolute_indexed(&Processor::INC, x_);

         case Opcode::ISC_ABSOLUTE_X:
            return Instruction{ {} };

         default:
            throw UnsupportedOpcode{
               static_cast<decltype(program_counter)>(program_counter - 1),
               static_cast<std::underlying_type_t<Opcode>>(opcode)
            };
      }
   }

   void Processor::change_processor_status_flag(ProcessorStatusFlag const flag, bool const set) noexcept
   {
      auto const underlying_flag{ static_cast<std::underlying_type_t<ProcessorStatusFlag>>(flag) };
      set
         ? processor_status_ |= underlying_flag
         : processor_status_ &= ~underlying_flag;
   }

   void Processor::change_processor_status_flags(std::initializer_list<ProcessorStatusFlag> const flags,
      bool const set) noexcept
   {
      ProcessorStatus underlying_flags{ 0b0000'0000 };
      for (ProcessorStatusFlag const flag : flags)
         underlying_flags |= static_cast<std::underlying_type_t<ProcessorStatusFlag>>(flag);

      set
         ? processor_status_ |= underlying_flags
         : processor_status_ &= ~underlying_flags;
   }

   bool Processor::processor_status_flag(ProcessorStatusFlag flag) const noexcept
   {
      auto const underlying_flag{ static_cast<std::underlying_type_t<ProcessorStatusFlag>>(flag) };
      return processor_status_ & underlying_flag;
   }

   void Processor::update_zero_and_negative_flag(Data const value) noexcept
   {
      change_processor_status_flag(ProcessorStatusFlag::Z, not value);
      change_processor_status_flag(ProcessorStatusFlag::N, value & 0b1000'0000);
   }

   void Processor::push(Data const value) noexcept
   {
      memory_.write(0x0100 + stack_pointer_--, value);
   }

   Data Processor::pop() noexcept
   {
      return memory_.read(0x0100 + ++stack_pointer_);
   }

   Address Processor::assemble_address(Data const high_byte, Data const low_byte) noexcept
   {
      return high_byte << 8 | low_byte;
   }

   std::pair<Address, bool> Processor::add_low_byte(Address const address, Data const value) noexcept
   {
      auto const before{ address & 0x00FF };
      auto const after{ before + value & 0x00FF };
      return { address & 0xFF00 | after, after < before };
   }

   std::pair<Address, bool> Processor::add_low_byte(Address const address, SignedData const value) noexcept
   {
      auto const before{ address & 0x00FF };
      auto const after{ before + value & 0x00FF };
      return { address & 0xFF00 | after, std::signbit(after - before) not_eq std::signbit(value) };
   }

   std::pair<Address, bool> Processor::add_high_byte(Address const address, Data const value) noexcept
   {
      auto const before{ (address & 0xFF00) >> 8 };
      auto const after{ before + value & 0x00FF };
      return { after << 8 | address & 0x00FF, after < before };
   }

   std::pair<Address, bool> Processor::subtract_high_byte(Address const address, Data const value) noexcept
   {
      auto const before{ (address & 0xFF00) >> 8 };
      auto const after{ before - value & 0x00FF };
      return { after << 8 | address & 0x00FF, after > before };
   }
}