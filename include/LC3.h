#ifndef __LC3_H__
#define __LC3_H__

#include "Specifics.h"

#include <cstdint>
#include <array>
#include <memory>

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
} // namespace Opcodes

// conditional  flags
namespace Flags {
constexpr uint16_t FL_POS = 1 << 0; /* P */
constexpr uint16_t FL_ZRO = 1 << 1; /* Z */
constexpr uint16_t FL_NEG = 1 << 2; /* N */
} // namespace Flags

// trap codes
namespace TrapCodes {
constexpr uint16_t TRAP_GETC = 0x20;
constexpr uint16_t TRAP_OUT = 0x21;
constexpr uint16_t TRAP_PUTS = 0x22;
constexpr uint16_t TRAP_IN = 0x23;
constexpr uint16_t TRAP_PUTSP = 0x24;
constexpr uint16_t TRAP_HALT = 0x25;
} // namespace TrapCodes

auto read_mem(uint16_t address) -> uint16_t;

auto write_mem(uint16_t address, uint16_t value) -> void;

auto extend_sign(uint16_t x, int bit_count) -> uint16_t;

auto update_flags(uint16_t idx) -> void;

auto swap_16(uint16_t x) -> uint16_t;

auto handle_interrupt(int signal) -> void;

static auto make_unique_file_ptr(const char* fname, const char* flags) -> std::unique_ptr<std::FILE, decltype(&fclose)>;

auto read_image_file(std::FILE* file) -> void;

auto read_image(const char *image_path) -> int;

inline auto trap_routines(uint16_t& instruction, bool& is_running) -> void;

auto run_vm() -> void;

#endif // __LC3_H__