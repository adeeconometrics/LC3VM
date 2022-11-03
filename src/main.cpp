#include "Specifics.h"
#include "LC3.h"

#include <iostream>
#include <signal.h>

using std::cout;

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
