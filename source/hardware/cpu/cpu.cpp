#include "cpu.hpp"
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

      program_counter = memory_.read(0xFFFC);
      co_await std::suspend_always{};

      program_counter |= memory_.read(0xFFFD) << 8;
      co_return std::nullopt;
   }

   Instruction Processor::BRK() noexcept
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
      co_await std::suspend_always{};

      // push PCH on stack
      push(program_counter >> 8 & 0x00FF);
      co_await std::suspend_always{};

      // push PCL on stack
      push(program_counter & 0x00FF);
      co_await std::suspend_always{};

      // copy low address byte to PCL, fetch high address byte to PCH
      program_counter = memory_.read(program_counter) << 8 | low_address_byte;
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

   Instruction Processor::immediate(ReadOperation const operation) noexcept
   {
      // fetch value, increment PC
      Data const value{ memory_.read(program_counter++) };

      std::invoke(operation, this, value);
      co_return std::nullopt;
   }

   Instruction Processor::absolute(ReadOperation const operation) noexcept
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

   Instruction Processor::zero_page(ReadOperation const operation) noexcept
   {
      // fetch address, increment PC
      Address const address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // read from effective address
      Data const value{ memory_.read(address) };

      std::invoke(operation, this, value);
      co_return std::nullopt;
   }

   Instruction Processor::zero_page_indexed(ReadOperation const operation, Index const index) noexcept
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

   Instruction Processor::absolute_indexed(ReadOperation const operation, Index const index) noexcept
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

   Instruction Processor::x_indirect(ReadOperation const operation) noexcept
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

   Instruction Processor::indirect_y(ReadOperation const operation) noexcept
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

   Instruction Processor::accumulator(ModifyOperation const operation) noexcept
   {
      // do the operation on the accumulator
      accumulator_ = std::invoke(operation, this, accumulator_);
      co_return std::nullopt;
   }

   Instruction Processor::absolute(ModifyOperation const operation) noexcept
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

   Instruction Processor::zero_page(ModifyOperation const operation) noexcept
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

   Instruction Processor::zero_page_indexed(ModifyOperation const operation, Index const index) noexcept
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

   Instruction Processor::absolute_indexed(ModifyOperation const operation, Index const index) noexcept
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

   Instruction Processor::x_indirect(ModifyOperation const operation) noexcept
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

   Instruction Processor::absolute(WriteOperation const operation) noexcept
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

   Instruction Processor::zero_page(WriteOperation const operation) noexcept
   {
      // fetch address, increment PC
      Address const address{ memory_.read(program_counter++) };
      co_await std::suspend_always{};

      // write register to effective address
      std::invoke(operation, this, address);
      co_return std::nullopt;
   }

   Instruction Processor::zero_page_indexed(WriteOperation const operation, Index const index) noexcept
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

   Instruction Processor::absolute_indexed(WriteOperation const operation, Index const index) noexcept
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

   Instruction Processor::x_indirect(WriteOperation const operation) noexcept
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

   bool Processor::BPL() const noexcept
   {
      return not processor_status_flag(ProcessorStatusFlag::N);
   }

   void Processor::ORA(Data const value) noexcept
   {
      // ORA value with accumulator
      accumulator_ |= value;

      // Z set if the accumulator is zero after ORA
      change_processor_status_flag(ProcessorStatusFlag::Z, not accumulator_);

      // N set if bit 7 of the accumulator is set after ORA
      change_processor_status_flag(ProcessorStatusFlag::N, accumulator_ & 0b1000'0000);
   }

   void Processor::LDA(Data const value) noexcept
   {
      // load value into accumulator
      accumulator_ = value;

      // Z set if the accumulator is zero after LDA
      change_processor_status_flag(ProcessorStatusFlag::Z, not accumulator_);

      // N set if bit 7 of the accumulator is set after LDA
      change_processor_status_flag(ProcessorStatusFlag::N, accumulator_ & 0b1000'0000);
   }

   void Processor::AND(Data const value) noexcept
   {
      // AND value with accumulator
      accumulator_ &= value;

      // Z set if the accumulator is zero after AND
      change_processor_status_flag(ProcessorStatusFlag::Z, not accumulator_);

      // N set if bit 7 of the accumulator is set after AND
      change_processor_status_flag(ProcessorStatusFlag::N, accumulator_ & 0b1000'0000);
   }

   Data Processor::ASL(Data value) noexcept
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

   void Processor::push(Data const value) noexcept
   {
      memory_.write(0x0100 + stack_pointer_--, value);
   }

   Data Processor::pop() noexcept
   {
      return memory_.read(0x0100 + ++stack_pointer_);
   }

   Address Processor::assemble_address(Data const low_byte, Data const high_byte) noexcept
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