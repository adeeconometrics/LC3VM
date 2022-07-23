#ifndef __SPECIFICS_H__
#define __SPECIFICS_H__

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

#endif // __SPECIFICS_H__