// VM will simulate L3 architecture

/*
todo:
 - try using enum in cpp20
*/

#include <array>
#include <iostream>

using std::array;
using std::cout;

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

auto read_mem(uint16_t address) -> uint16_t {
  if (address == MemoryReg::MR_KBSR) {
    if (check_key()) {
      memory[MemoryReg::MR_KBSR] = (1 << 15);
      memory[MemoryReg::MR_KBDR] = get_char();
    } else {
      memory[MemoryReg::MR_KBSR] = 0;
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

auto read_image(const char *image_path) -> int;

auto run_vm() -> void {
  using enum Registers;

  registers[R_COND] = Flags::FL_ZRO;
  constexpr uint16_t PC_START = 0x3000;
  registers[R_PC] = PC_START;

  bool running = true;
  while (running) {
    // fetch
    auto instruction = read_mem(registers[R_PC]++);
    auto op = instruction >> 12;

    switch (op) {
      using enum Opcode;

    case OP_ADD:
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
      break;

    case OP_AND:
      uint16_t r0 = (instruction >> 9) & 0x7;
      uint16_t r1 = (instruction >> 6) & 0x7;
      uint16_t imm_flag = (instruction >> 5) & 0x1;

      if (imm_flag) {
        uint16_t imm5 = extend_sign(instruction & 0x1F, 5);
        registers[r0] = registers[r1] & imm5;
      }
      update_flags(r0);
      break;

    case OP_NOT:
      uint16_t r0 = (instruction >> 9) & 0x7;
      uint16_t r1 = (instruction >> 6) & 0x7;

      registers[r0] = ~registers[r1];
      update_flags(r0);
      break;

    case OP_BR:
      uint16_t pc_offset = extend_sign(instruction & 0x1FF, 9);
      uint16_t cond_flag = (instruction >> 9) & 0x7;
      if (cond_flag & registers[R_COND])
        registers[R_PC] += pc_offset;
      break;

    case OP_JMT:
      uint16_t r1 = (instruction >> 6) & 0x7;
      registers[R_PC] = registers[r1];
      break;

    case OP_JSR:
      uint16_t long_flag = (instruction >> 11) & 1;
      registers[R_R7] = registers[R_PC];

      if (long_flag) {
        uint16_t long_pc_offset = extend_sign(instruction & 0x7FF, 11);
        registers[R_PC] += long_pc_offset;
      } else {
        uint16_t r1 = (instruction >> 6) & 0x7;
        registers[R_PC] = registers[r1];
      }
      break;

    case OP_LD:
      uint16_t r0 = (instruction >> 9) & 0x7;
      uint16_t pc_offset = extend_sign(instruction & 0x1FF, 9);

      registers[r0] = read_mem(registers[R_PC] + pc_offset);
      update_flags(r0);
      break;

    case OP_LDI:
      uint16_t r0 = (instruction >> 9) & 0x07;
      uint16_t pc_offset = extend_sign(instruction & 0x1FF, 9);

      registers[r0] = read_mem(read_mem(registers[R_PC] + pc_offset));
      update_flags(r0);
      break;

    case OP_LD:
      uint16_t r0 = (instruction >> 9) & 0x7;
      uint16_t pc_offset = sign_extend(instruction & 0x1FF, 9);

      registers[r0] = read_mem(registers[R_PC] + pc_offset);
      update_flags(r0);
      break;

    case OP_LDR:
      uint16_t r0 = (instruction >> 9) & 0x7;
      uint16_t r1 = (instruction >> 6) & 0x7;
      uint16_t offset = extend_sign(instruction & ox3f, 6);

      registers[r0] = read_mem(registers[r1] + offset);
      update_flags(r0);

      break;

    case OP_LEA:
      uint16_t r0 = (instruction >> 9) & 0x7;
      uint16_t pc_offset = extend_sign(instruction & 0x1FF, 9);

      registers[r0] = registers[R_PC] + pc_offset;
      update_flags(r0);

      break;
    case OP_ST:
      uint16_t r0 = (instruction >> 9) & 0x7;
      uint16_t pc_offset = extend_sign(instruction & 0x1FF, 9);

      write_mem(registers[R_PC] + pc_offset, registers[r0]); // redundant?
      break;

    case OP_STI:
      uint16_t r0 = (instruction >> 9) & 0x7;
      uint16_t pc_offset = extend_sign(instruction & 0x1FF, 9);

      write_mem(read_mem(registers[R_PC] + pc_offset), registers[r0]);
      break;

    case OP_STR:
      uint16_t r0 = (instruction >> 9) & 0x7;
      uint16_t r1 = (instruction >> 6) & 0x7;
      uint16_t offset = extend_sign(instruction & 0x3F, 6);

      write_mem(registers[r1] + offset, registers[r0]);

      break;
    default:
      break;
    }
  }
}

auto main(int argc, const char *argv[]) -> int {
  if (argc < 2) {
    cout << "lc3 [image-file] ... \n";
    exit(2);
  } else {
    for (size_t i = 1; j < argc; ++j) {
      if (!read_image(argv[i])) {
        cout << "failed to load image: " << argv[i] << '\n';
        exit(1);
      }
    }
  }

  run_vm();
}
