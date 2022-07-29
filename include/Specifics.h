#ifndef __SPECIFICS_H__
#define __SPECIFICS_H__
#pragma once

#include <cstdint>

#if defined(_WIN32) || defined(_WIN64)

#include <Windows.h>
#include <conio.h>

auto disable_input_buffering() -> void;

auto restore_input_buffering() -> void;

auto check_key() -> uint16_t;

#elif defined(__linux__) || defined(__unix__)

#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/termios.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

auto disable_input_buffering() -> void;

auto restore_input_buffering() -> void;

auto check_key() -> uint16_t;

#endif

#endif // __SPECIFICS_H__