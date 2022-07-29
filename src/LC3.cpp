// VM will emulate LC3 CPU architecture

#include "Specifics.h"
#include "LC3.h"

#include <signal.h>
#include <stdio.h>
#include <cstdint>

#include <array>
#include <iostream>
#include <memory>

using std::array;
using std::cout;
using std::unique_ptr;


constexpr uint32_t max = 1 << 16;
array<uint16_t, max> memory; // 128KB memory store

// storing registers
array<uint16_t, 10> registers;

auto read_mem(uint16_t address) -> uint16_t {
  if (address == MappedReg::MR_KBSR) {
    if (check_key()) {
      memory[MappedReg::MR_KBSR] = (1 << 15);
      memory[MappedReg::MR_KBDR] = getchar();
    } else {
      memory[MappedReg::MR_KBSR] = 0;
    }
  }
  return memory[address];
}

auto write_mem(uint16_t address, uint16_t value) -> void {
  memory[address] = value;
}

auto extend_sign(uint16_t x, int bit_count) -> uint16_t {
  if ((x >> (bit_count - 1)) & 1) {
    x |= (0xFFFF << bit_count);
  }
  return x;
}

auto update_flags(uint16_t idx) -> void {
  if (registers[idx] == 0)
    registers[Registers::R_COND] = Flags::FL_ZRO;
  else if (registers[idx] >> 15)
    registers[Registers::R_COND] = Flags::FL_NEG;
  else
    registers[Registers::R_COND] = Flags::FL_POS;
}

auto swap_16(uint16_t x) -> uint16_t { return (x << 8) | (x >> 8); }

auto handle_interrupt(int signal) -> void {
    restore_input_buffering();
    cout << '\n';
    exit(-2);
}

static auto make_unique_file_ptr(const char *fname, const char *flags)
    -> unique_ptr<std::FILE, decltype(&fclose)> {
  return unique_ptr<std::FILE, decltype(&fclose)>(std::fopen(fname, flags), std::fclose);
}
// non-owning ptr
auto read_image_file(std::FILE* file) -> void {
  uint16_t origin;
  fread(&origin, sizeof(uint16_t), 1, file);
  origin = swap_16(origin);

  uint16_t max_read = max - origin;
  uint16_t* p = &memory[origin]; // check for soundness
  size_t read = fread(p, sizeof(uint16_t), max_read, file);

  while (read > 0) {
    *p = swap_16(*p);
    ++p;

    read--;
  }
}

auto read_image(const char *image_path) -> int {
  auto file = make_unique_file_ptr(image_path, "rb");
  if (!file.get())
    return 0;

  read_image_file(file.get());
  return 1;
}

inline auto trap_routines(uint16_t& instruction, bool &is_running) -> void {

  registers[Registers::R_R7] = registers[Registers::R_PC];

  switch (instruction & 0xff) {
  case TrapCodes::TRAP_GETC: {
    registers[Registers::R_R0] = static_cast<uint16_t>(getchar());
    update_flags(Registers::R_R0);
  } break;

  case TrapCodes::TRAP_OUT: {
    putc(static_cast<char>(registers[Registers::R_R0]), stdout);
    fflush(stdout);
  } break;

  case TrapCodes::TRAP_PUTS: {
    // should you use non-owning pointers? 
    uint16_t* ptr = &memory[registers[Registers::R_R0]];

    while (*ptr) {
      putc(static_cast<char>(*ptr), stdout);
      ++ptr;
    }

    fflush(stdout);
  } break;

  case TrapCodes::TRAP_IN: {
    cout << "Enter a caracter: ";

    char c = getchar();
    putc(c, stdout);
    fflush(stdout);

    registers[Registers::R_R0] = static_cast<uint16_t>(c);
    update_flags(Registers::R_R0);
  } break;

  case TrapCodes::TRAP_PUTSP: {
    uint16_t* ptr = &memory[registers[Registers::R_R0]];

    while (*ptr) {
      char c1 = (*ptr) & 0xff;
      putc(c1, stdout);
      char c2 = (*ptr) >> 8;
      if (c2) {
        putc(c2, stdout);
      }
      ++ptr;
    }
    fflush(stdout);
  } break;

  case TrapCodes::TRAP_HALT: {
    puts("HALT");
    fflush(stdout);
    is_running = false;
  } break;
  }
}

auto run_vm() -> void {
  
  registers[Registers::R_COND] = Flags::FL_ZRO;
  constexpr uint16_t PC_START = 0x3000;
  registers[Registers::R_PC] = PC_START;

  bool is_running = true;
  while (is_running) {

    auto instruction = read_mem(registers[Registers::R_PC]++);
    auto op = instruction >> 12;

    switch (op) {
    case Opcodes::OP_ADD: {
      uint16_t r0 = (instruction >> 9) & 0x7;
      uint16_t r1 = (instruction >> 6) & 0x7;
      uint16_t imm_flag = (instruction >> 5) & 0x1;

      if (imm_flag) {
        auto imm = extend_sign(instruction & 0x1F, 5);
        registers[r0] = registers[r1] + imm;
      } else {
        uint16_t r2 = (instruction & 0x7);
        registers[r0] = registers[r1] + registers[r2];
      }
      update_flags(r0);
    } break;
    case Opcodes::OP_AND: {
      uint16_t r0 = (instruction >> 9) & 0x7;
      uint16_t r1 = (instruction >> 6) & 0x7;
      uint16_t imm_flag = (instruction >> 5) & 0x1;

      if (imm_flag) {
        uint16_t imm5 = extend_sign(instruction & 0x1F, 5);
        registers[r0] = registers[r1] & imm5;
      } else {
        uint16_t r2 = instruction & 0x7;
        registers[r0] = registers[r1] & registers[r2];
      }
      update_flags(r0);
    } break;
    case Opcodes::OP_NOT: {
      uint16_t r0 = (instruction >> 9) & 0x7;
      uint16_t r1 = (instruction >> 6) & 0x7;

      registers[r0] = ~registers[r1];
      update_flags(r0);
    } break;
    case Opcodes::OP_BR: {
      uint16_t pc_offset = extend_sign(instruction & 0x1FF, 9);
      uint16_t cond_flag = (instruction >> 9) & 0x7;
      if (cond_flag & registers[Registers::R_COND])
        registers[Registers::R_PC] += pc_offset;
    } break;
    case Opcodes::OP_JMP: {
      uint16_t r1 = (instruction >> 6) & 0x7;
      registers[Registers::R_PC] = registers[r1];
    } break;
    case Opcodes::OP_JSR: {
      uint16_t long_flag = (instruction >> 11) & 1;
      registers[Registers::R_R7] = registers[Registers::R_PC];

      if (long_flag) {
        uint16_t long_pc_offset = extend_sign(instruction & 0x7FF, 11);
        registers[Registers::R_PC] += long_pc_offset;
      } else {
        uint16_t r1 = (instruction >> 6) & 0x7;
        registers[Registers::R_PC] = registers[r1];
      }
    } break;
    case Opcodes::OP_LD: {
      uint16_t r0 = (instruction >> 9) & 0x7;
      uint16_t pc_offset = extend_sign(instruction & 0x1FF, 9);

      registers[r0] = read_mem(registers[Registers::R_PC] + pc_offset);
      update_flags(r0);
    } break;
    case Opcodes::OP_LDI: {
      uint16_t r0 = (instruction >> 9) & 0x07;
      uint16_t pc_offset = extend_sign(instruction & 0x1FF, 9);

      registers[r0] = read_mem(read_mem(registers[Registers::R_PC] + pc_offset));
      update_flags(r0);
    } break;
    case Opcodes::OP_LDR: {
      uint16_t r0 = (instruction >> 9) & 0x7;
      uint16_t r1 = (instruction >> 6) & 0x7;
      uint16_t offset = extend_sign(instruction & 0x3f, 6);

      registers[r0] = read_mem(registers[r1] + offset);
      update_flags(r0);
    } break;
    case Opcodes::OP_LEA: {
      uint16_t r0 = (instruction >> 9) & 0x7;
      uint16_t pc_offset = extend_sign(instruction & 0x1FF, 9);

      registers[r0] = registers[Registers::R_PC] + pc_offset;
      update_flags(r0);
    } break;
    case Opcodes::OP_ST: {
      uint16_t r0 = (instruction >> 9) & 0x7;
      uint16_t pc_offset = extend_sign(instruction & 0x1FF, 9);

      write_mem(registers[Registers::R_PC] + pc_offset, registers[r0]); // redundant?
    } break;
    case Opcodes::OP_STI: {
      uint16_t r0 = (instruction >> 9) & 0x7;
      uint16_t pc_offset = extend_sign(instruction & 0x1FF, 9);

      write_mem(read_mem(registers[Registers::R_PC] + pc_offset), registers[r0]);
    } break;
    case Opcodes::OP_STR: {
      uint16_t r0 = (instruction >> 9) & 0x7;
      uint16_t r1 = (instruction >> 6) & 0x7;
      uint16_t offset = extend_sign(instruction & 0x3F, 6);

      write_mem(registers[r1] + offset, registers[r0]);
    } break;
    default:{
      trap_routines(instruction, is_running);
      break;
    }
    }
    // is_running = false;
  }
}
