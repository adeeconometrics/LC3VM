#ifndef __LC3_H__
#define __LC3_H__

#include <array>

using std::array;

constexpr uint16_t max = 1 << 16;
array<uint16_t, max> memory; // 128KB memory stored in this array

// registers
enum class Registers : uint16_t {
  R_R0,
  R_R1,
  R_R2,
  R_R3,
  R_R4,
  R_R5,
  R_R6,
  R_R7,
  R_PC,
  R_COND,
  R_COUNT
};

// storing registers
array<uint16_t, 10> registers;

// memory mapped registers
enum class MappedReg : uint16_t { MR_KBSR = 0xFE00, MR_KBDR = 0xFE02 };

// instruction set definition
enum class Opcodes : uint16_t {
  OP_BR,  /* branch */
  OP_ADD, /* add */
  OP_LD,  /* load */
  OP_ST,  /* store */
  OP_JSR, /* jump register */
  OP_AND, /* bitwise and */
  OP_LDR, /* load register */
  OP_STR, /* store register */
  OP_RTI, /* unused */
  OP_NOT, /* bitwise not */
  OP_LDI, /* load indirect */
  OP_STI, /* store indirect */
  OP_JMT, /* jump */
  OP_RES, /* reserved (unused) */
  OP_LEA, /* load effective address */
  OP_TRAP /* execute trap */
};

// conditional  flags
enum class Flags : uint16_t {
  FL_POS = 1 << 0, /* P */
  FL_ZRO = 1 << 1, /* Z */
  FL_NEG = 1 << 2  /* N */
};

// trap codes
enum class TrapCodes : uint16_t {
  TRAP_GETC = 0x20,
  TRAP_OUT = 0x21,
  TRAP_PUTS = 0x22,
  TRAP_IN = 0x23,
  TRAP_PUTSP = 0x24,
  TRAP_HALT = 0x25
};

auto read_mem(uint16_t address) -> uint16_t;

auto write_mem(uint16_t address, uint16_t value) -> void;

auto extend_sign(uint16_t x, int bit_count) -> uint16_t;

auto update_flags(uint16_t idx) -> void;

auto read_image(const char *image_path) -> int;

auto trap_routines(uint16_t instruction) -> void;

auto run_vm() -> void;

#endif // __LC3_H__