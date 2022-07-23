#ifndef __LC3UNIX_H__
#define __LC3UNIX_H__

// System-specific configurations

#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/termios.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

struct termios original_tio;

auto disable_input_buffering() -> void; 

auto restore_input_buffering() -> void; 

auto check_key() -> uint16_t;


#endif // __LC3UNIX_H__