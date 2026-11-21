// UI simulator runner: reads one JSON message per line on stdin, writes one
// JSON message per line on stdout. Usage: ui_runner [--backend firmware]
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "protocol.h"
#include "ui_backend.h"


int main(int argc, char** argv) {
  const char* backend_name = "firmware";
  for (int i = 1; i < argc; i++) {
    if (std::strcmp(argv[i], "--backend") == 0 && i + 1 < argc) {
      backend_name = argv[++i];
    } else if (std::strcmp(argv[i], "--help") == 0) {
      std::printf("usage: ui_runner [--backend firmware]\n");
      return 0;
    }
  }
  sim::UiBackend* backend = sim::createBackend(backend_name);
  if (!backend) {
    std::fprintf(stderr, "unknown backend: %s\n", backend_name);
    return 2;
  }

  sim::Session session(backend);
  std::cout << session.hello() << std::endl;

  std::string line;
  std::vector<std::string> replies;
  while (std::getline(std::cin, line)) {
    replies.clear();
    bool keep = session.handleLine(line, replies);
    for (const auto& r : replies) std::cout << r << '\n';
    std::cout.flush();
    if (!keep) break;
  }
  delete backend;
  return 0;
}
