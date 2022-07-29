// VM will simulate L3 architecture

/*
todo:
 - try using enum in cpp20
*/

#include <signal.h>
#include <stdio.h>

#include <cstdint>

#include <array>
#include <iostream>
#include <memory>

using std::array;
using std::cout;
using std::unique_ptr;

#if defined(_WIN32) || defined(_WIN64)

#include <Windows.h>
#include <conio.h>

HANDLE handle = INVALID_HANDLE_VALUE;
DWORD fdw_mode, fdw_old_mode;

auto disable_input_buffering() -> void {
  handle = GetStdHandle(STD_INPUT_HANDLE);
  GetConsoleMode(handle, &fdw_old_mode);      /* save old mode */
  fdw_mode = fdw_old_mode ^ ENABLE_ECHO_INPUT /* no input echo */
             ^ ENABLE_LINE_INPUT;             /* return when one or
                                               more characters are available */
  SetConsoleMode(handle, fdw_mode);           /* set new mode */
  FlushConsoleInputBuffer(handle);            /* clear buffer */
}

auto restore_input_buffering() -> void { SetConsoleMode(handle, fdw_old_mode); }

auto check_key() -> uint16_t {
  return WaitForSingleObject(handle, 1000) == WAIT_OBJECT_0 && _kbhit();
}

#elif defined(__linux__) || defined(__unix__)

#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/termios.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

static struct termios original_tio;

auto disable_input_buffering() -> void {
  tcgetattr(STDIN_FILENO, &original_tio);
  struct termios new_tio = original_tio;
  new_tio.c_lflag &= ~ICANON & ~ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
}

auto restore_input_buffering() -> void {
  tcsetattr(STDIN_FILENO, TCSANOW, &original_tio);
}

auto check_key() -> uint16_t {
  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(STDIN_FILENO, &readfds);

  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
  return select(1, &readfds, NULL, NULL, &timeout) != 0;
}
#endif

constexpr uint32_t max = 1 << 16;
array<uint16_t, max> memory; // 128KB memory stored in this array

// registers
namespace Registers {
constexpr uint16_t R_R0 = 0;
constexpr uint16_t R_R1 = 1;
constexpr uint16_t R_R2 = 2;
constexpr uint16_t R_R3 = 3;
constexpr uint16_t R_R4 = 4;
constexpr uint16_t R_R5 = 5;
constexpr uint16_t R_R6 = 6;
constexpr uint16_t R_R7 = 7;
constexpr uint16_t R_PC = 8;
constexpr uint16_t R_COND = 9;
constexpr uint16_t R_COUNT = 10;
} // namespace Registers

// storing registers
array<uint16_t, 10> registers;

// memory mapped registers
namespace MappedReg {
constexpr uint16_t MR_KBSR = 0xfe00;
constexpr uint16_t MR_KBDR = 0xfe02;
} // namespace MappedReg

// instruction set definition
namespace Opcodes {
constexpr uint16_t OP_BR = 0;    /* branch */
constexpr uint16_t OP_ADD = 1;   /* add */
constexpr uint16_t OP_LD = 2;    /* load */
constexpr uint16_t OP_ST = 3;    /* store */
constexpr uint16_t OP_JSR = 4;   /* jump register */
constexpr uint16_t OP_AND = 5;   /* bitwise and */
constexpr uint16_t OP_LDR = 6;   /* load register */
constexpr uint16_t OP_STR = 7;   /* store register */
constexpr uint16_t OP_RTI = 8;   /* unused */
constexpr uint16_t OP_NOT = 9;   /* bitwise not */
constexpr uint16_t OP_LDI = 10;  /* load indirect */
constexpr uint16_t OP_STI = 11;  /* store indirect */
constexpr uint16_t OP_JMP = 12;  /* jump */
constexpr uint16_t OP_RES = 13;  /* reserved (unused) */
constexpr uint16_t OP_LEA = 14;  /* load effective address */
constexpr uint16_t OP_TRAP = 15; /* execute trap */
}                               // namespace Opcodes

// conditional  flags
namespace Flags {
constexpr uint16_t FL_POS = 1 << 0; /* P */
constexpr uint16_t FL_ZRO = 1 << 1; /* Z */
constexpr uint16_t FL_NEG = 1 << 2; /* N */
}                                  // namespace Flags

// trap codes
namespace TrapCodes {
constexpr uint16_t TRAP_GETC = 0x20;
constexpr uint16_t TRAP_OUT = 0x21;
constexpr uint16_t TRAP_PUTS = 0x22;
constexpr uint16_t TRAP_IN = 0x23;
constexpr uint16_t TRAP_PUTSP = 0x24;
constexpr uint16_t TRAP_HALT = 0x25;
} // namespace TrapCodes

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

void handle_interrupt(int signal)
{
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
  // fclose(file);
  return 1;
}

// void read_image_file(FILE *file) {
//   /* the origin tells us where in memory to place the image */
//   uint16_t origin;
//   fread(&origin, sizeof(origin), 1, file);
//   origin = swap_16(origin);

//   /* we know the maximum file size so we only need one fread */
//   uint16_t max_read = max - origin;
//   uint16_t *p = &memory[origin];
//   size_t read = fread(p, sizeof(uint16_t), max_read, file);

//   /* swap to little endian */
//   while (read-- > 0) {
//     *p = swap_16(*p);
//     ++p;
//   }
// }

// int read_image(const char* image_path){
//     FILE* file = fopen(image_path, "rb");
//     if (!file) { return 0; };
//     read_image_file(file);
//     fclose(file);
//     return 1;
// }

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

auto main(int argc, const char *argv[]) -> int {
  if (argc < 2) {
    cout << "lc3 [image-file] ... \n";
    exit(2);
  } 
  for (int i = 1; i < argc; ++i) {
    if (!read_image(argv[i])) {
      cout << "failed to load image: " << argv[i] << '\n';
      exit(1);
    }
  }
  signal(SIGINT, handle_interrupt);
  disable_input_buffering();
  
  // std::cout << "VM is running!";
  run_vm();

  restore_input_buffering();
}
