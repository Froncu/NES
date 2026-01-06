#include "cpu.hpp"
#include "exceptions/unsupported_opcode.hpp"

namespace nes
{
   Processor::Processor(Memory& memory)
      : memory_{ memory }
   {
   }

   bool Processor::tick()
   {
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

      ++cycle_;
      return instruction_completed;
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

      while (not tick());
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
      co_return std::nullopt;
   }

   Instruction Processor::brk_implied()
   {
      // read next instruction byte (and throw it away), increment PC
      std::ignore = memory_.read(program_counter++);
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
      program_counter = memory_.read(0xFFFE);
      co_await std::suspend_always{};

      // fetch PCH
      program_counter |= memory_.read(0xFFFF) << 8;
      co_return std::nullopt;
   }

   Instruction Processor::jam_implied()
   {
      while (true)
         co_await std::suspend_always{};

      co_return std::nullopt;
   }

   Instruction Processor::php_implied()
   {
      // read next instruction byte (and throw it away)
      std::ignore = memory_.read(program_counter);
      co_await std::suspend_always{};

      // push P on stack (with B and _ flag set)
      change_processor_status_flags({ ProcessorStatusFlag::B, ProcessorStatusFlag::_ }, true);
      push(processor_status_);
      co_return std::nullopt;
   }

   Instruction Processor::relative(BranchOperation const operation)
   {
      // fetch operand, increment PC
      auto const operand{ static_cast<SignedData>(memory_.read(program_counter++)) };
      co_await std::suspend_always{};

      // fetch opcode of next instruction, if branch is taken, add operand to PCL, otherwise increment PC
      Instruction next_instruction{ instruction_from_opcode(static_cast<Opcode>(memory_.read(program_counter))) };
      if (std::invoke(operation, this))
      {
         auto const [modified_program_counter, overflow] = add_low_byte(program_counter, operand);
         program_counter = modified_program_counter;
         co_await std::suspend_always{};

         // fetch opcode of next instruction, fix PCH, if it did not change, increment PC (+)
         next_instruction = instruction_from_opcode(static_cast<Opcode>(memory_.read(program_counter)));
         if (overflow)
         {
            program_counter = std::signbit(operand)
               ? subtract_high_byte(program_counter, 1).first
               : add_high_byte(program_counter, 1).first;
            co_await std::suspend_always{};

            // fetch opcode of next instruction, increment PC (!)
            next_instruction = instruction_from_opcode(static_cast<Opcode>(memory_.read(program_counter)));
         }
      }

      ++program_counter;
      co_return std::move(next_instruction);
   }

   Instruction Processor::immediate(ReadOperation const operation)
   {
      // fetch value, increment PC
      Data const value{ memory_.read(program_counter++) };

      std::invoke(operation, this, value);
      co_return std::nullopt;
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
      co_return std::nullopt;
   }

   Instruction Processor::zero_page(ReadOperation const operation)
   {
      // fetch address, increment PC
      Address const address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // read from effective address
      Data const value{ memory_.read(address) };

      std::invoke(operation, this, value);
      co_return std::nullopt;
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
      co_return std::nullopt;
   }

   Instruction Processor::absolute_indexed(ReadOperation const operation, Index const index)
   {
      // fetch low byte of address, increment PC
      Data const low_byte_of_address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // fetch high byte of address, add index register to low address byte, increment PC
      Data const high_byte_of_address{ memory_.read(program_counter++) };
      auto [effective_address, overflow]{ add_low_byte(assemble_address(low_byte_of_address, high_byte_of_address), index) };
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
      co_return std::nullopt;
   }

   Instruction Processor::indirect_y(ReadOperation const operation)
   {
      // fetch pointer address, increment PC
      Address const pointer_address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // fetch effective address low
      Data const effective_address_low{ memory_.read(pointer_address) };
      co_await std::suspend_always{};

      // fetch effective address high, add Y to low byte of effective address
      Data const effective_address_high{ memory_.read(pointer_address + 1) };
      auto [effective_address, overflow]{ add_low_byte(assemble_address(effective_address_low, effective_address_high), y_) };
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

   Instruction Processor::accumulator(ModifyOperation operation)
   {
      // do the operation on the accumulator
      accumulator_ = std::invoke(operation, this, accumulator_);
      co_return std::nullopt;
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
      co_return std::nullopt;
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
      co_return std::nullopt;
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
      co_return std::nullopt;
   }

   Instruction Processor::absolute_indexed(ModifyOperation const operation, Index const index)
   {
      // fetch low byte of address, increment PC
      Data const low_byte_of_address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // fetch high byte of address, add index register to low address byte, increment PC
      Data const high_byte_of_address{ memory_.read(program_counter++) };
      auto [effective_address, overflow]{ add_low_byte(assemble_address(low_byte_of_address, high_byte_of_address), index) };
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
      co_return std::nullopt;
   }

   Instruction Processor::indirect_y(ModifyOperation const operation)
   {
      // fetch pointer address, increment PC
      Address const pointer_address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // fetch effective address low
      Data const effective_address_low{ memory_.read(pointer_address) };
      co_await std::suspend_always{};

      // fetch effective address high, add Y to low byte of effective address
      Data const effective_address_high{ memory_.read(pointer_address + 1) };
      auto [effective_address, overflow]{ add_low_byte(assemble_address(effective_address_low, effective_address_high), y_) };
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
      co_return std::nullopt;
   }

   Instruction Processor::zero_page(WriteOperation const operation)
   {
      // fetch address, increment PC
      Address const address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // write register to effective address
      std::invoke(operation, this, address);
      co_return std::nullopt;
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
      co_return std::nullopt;
   }

   Instruction Processor::absolute_indexed(WriteOperation const operation, Index const index)
   {
      // fetch low byte of address, increment PC
      Data const low_byte_of_address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // fetch high byte of address, add index register to low address byte, increment PC
      Data const high_byte_of_address{ memory_.read(program_counter++) };
      auto [effective_address, overflow]{ add_low_byte(assemble_address(low_byte_of_address, high_byte_of_address), index) };
      co_await std::suspend_always{};

      // read from effective address, fix the high byte of effective address
      std::ignore = memory_.read(effective_address);
      if (overflow)
         effective_address = add_high_byte(effective_address, 1).first;
      co_await std::suspend_always{};

      // write to effective address
      std::invoke(operation, this, effective_address);
      co_return std::nullopt;
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
      co_return std::nullopt;
   }

   Instruction Processor::indirect_y(WriteOperation const operation)
   {
      // fetch pointer address, increment PC
      Address const pointer_address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // fetch effective address low
      Data const effective_address_low{ memory_.read(pointer_address) };
      co_await std::suspend_always{};

      // fetch effective address high, add Y to low byte of effective address
      Data const effective_address_high{ memory_.read(pointer_address + 1) };
      auto [effective_address, overflow]{ add_low_byte(assemble_address(effective_address_low, effective_address_high), y_) };
      co_await std::suspend_always{};

      // read from effective address, fix high byte of effective address
      std::ignore = memory_.read(effective_address);
      if (overflow)
         effective_address = add_high_byte(effective_address, 1).first;
      co_await std::suspend_always{};

      // write to effective address
      std::invoke(operation, this, effective_address);
      co_return std::nullopt;
   }

   bool Processor::bpl() const
   {
      return not processor_status_flag(ProcessorStatusFlag::N);
   }

   void Processor::ora(Data const value)
   {
      // ORA value with accumulator
      accumulator_ |= value;

      // Z set if the accumulator is zero after ORA
      change_processor_status_flag(ProcessorStatusFlag::Z, not accumulator_);

      // N set if bit 7 of the accumulator is set after ORA
      change_processor_status_flag(ProcessorStatusFlag::N, accumulator_ & 0b1000'0000);
   }

   void Processor::lda(Data const value)
   {
      // load value into accumulator
      accumulator_ = value;

      // Z set if the accumulator is zero after LDA
      change_processor_status_flag(ProcessorStatusFlag::Z, not accumulator_);

      // N set if bit 7 of the accumulator is set after LDA
      change_processor_status_flag(ProcessorStatusFlag::N, accumulator_ & 0b1000'0000);
   }

   Data Processor::asl(Data value)
   {
      // C set to bit 7 of value before ASL
      change_processor_status_flag(ProcessorStatusFlag::C, value & 0b1000'0000);

      // shift value left by 1
      value <<= 1;

      // Z set if the value is zero after ASL
      change_processor_status_flag(ProcessorStatusFlag::Z, not value);

      // N set if bit 7 of the value is set after ASL
      change_processor_status_flag(ProcessorStatusFlag::N, value & 0b1000'0000);

      return value;
   }

   Instruction Processor::instruction_from_opcode(Opcode const opcode)
   {
      switch (opcode)
      {
         case Opcode::BRK_IMPLIED:
            return brk_implied();

         case Opcode::ORA_X_INDIRECT:
            return x_indirect(&Processor::ora);

         case Opcode::JAM_IMPLIED_02:
            return jam_implied();

         // case Opcode::SLO_X_INDIRECT:
         //    return {};

         // case Opcode::NOP_ZERO_PAGE_04:
         //    return {};

         case Opcode::ORA_ZERO_PAGE:
            return zero_page(&Processor::ora);

         case Opcode::ASL_ZERO_PAGE:
            return zero_page(&Processor::asl);

         case Opcode::ORA_INDIRECT_Y:
            return indirect_y(&Processor::ora);

         // case Opcode::SLO_ZERO_PAGE:
         //    return {};

         case Opcode::PHP_IMPLIED:
            return php_implied();

         case Opcode::ORA_IMMEDIATE:
            return immediate(&Processor::ora);

         case Opcode::ASL_ACCUMULATOR:
            return accumulator(&Processor::asl);

         // case Opcode::ANC_IMMEDIATE_0B:
         //    return {};

         // case Opcode::NOP_ABSOLUTE:
         //    return {};

         case Opcode::ORA_ABSOLUTE:
            return absolute(&Processor::ora);

         case Opcode::ASL_ABSOLUTE:
            return absolute(&Processor::asl);

         // case Opcode::SLO_ABSOLUTE:
         //    return {};

         case Opcode::BPL_RELATIVE:
            return relative(&Processor::bpl);

         default:
            throw UnsupportedOpcode{
               static_cast<decltype(program_counter)>(program_counter - 1),
               static_cast<std::underlying_type_t<Opcode>>(opcode)
            };
      }
   }

   void Processor::change_processor_status_flag(ProcessorStatusFlag const flag, bool const set)
   {
      auto const underlying_flag{ static_cast<std::underlying_type_t<ProcessorStatusFlag>>(flag) };
      set
         ? processor_status_ |= underlying_flag
         : processor_status_ &= ~underlying_flag;
   }

   void Processor::change_processor_status_flags(std::initializer_list<ProcessorStatusFlag> const flags, bool const set)
   {
      ProcessorStatus underlying_flags{ 0b0000'0000 };
      for (ProcessorStatusFlag const flag : flags)
         underlying_flags |= static_cast<std::underlying_type_t<ProcessorStatusFlag>>(flag);

      set
         ? processor_status_ |= underlying_flags
         : processor_status_ &= ~underlying_flags;
   }

   bool Processor::processor_status_flag(ProcessorStatusFlag flag) const
   {
      auto const underlying_flag{ static_cast<std::underlying_type_t<ProcessorStatusFlag>>(flag) };
      return processor_status_ & underlying_flag;
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

   std::pair<Address, bool> Processor::add_low_byte(Address const address, Data const value)
   {
      auto const before{ address & 0x00FF };
      auto const after{ before + value & 0x00FF };
      return { address & 0xFF00 | after, after < before };
   }

   std::pair<Address, bool> Processor::add_low_byte(Address const address, SignedData const value)
   {
      auto const before{ address & 0x00FF };
      auto const after{ before + value & 0x00FF };
      return { address & 0xFF00 | after, std::signbit(after - before) not_eq std::signbit(value) };
   }

   std::pair<Address, bool> Processor::add_high_byte(Address const address, Data const value)
   {
      auto const before{ (address & 0xFF00) >> 8 };
      auto const after{ before + value & 0x00FF };
      return { after << 8 | address & 0x00FF, after < before };
   }

   std::pair<Address, bool> Processor::subtract_high_byte(Address const address, Data const value)
   {
      auto const before{ (address & 0xFF00) >> 8 };
      auto const after{ before - value & 0x00FF };
      return { after << 8 | address & 0x00FF, after > before };
   }
}