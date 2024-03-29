/**
 * @file main.cc
 * @author Mao Zhang (mao.zhang233@gmail.com)
 * @brief
 * @version 0.1
 * @date 2023-01-02
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <iostream>

#include "application.h"

int main(int argc, char* argv[]) {
  std::clog << "----- Starting -----" << std::endl;

  std::clog << "----- Arguments Count: " << argc << " -----" << std::endl;
  for (int i = 0; i < argc; ++i) {
    std::clog << "----- Arguments No." << i + 1 << ": " << argv[i] << std::endl;
  }

  swr::Application app{};

  try {
    app.Run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return EXIT_FAILURE;
  }

  std::clog << "----- Terminating -----" << std::endl;

  return EXIT_SUCCESS;
}