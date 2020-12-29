#include "signal_tl/ast.hpp"
#include "signal_tl/fmt.hpp"

#include <fmt/format.h>

namespace stl = signal_tl;

int main() {
  fmt::print("phi := {}\n", stl::ast::Predicate("a"));

  fmt::print("----------------\n");

  fmt::print("phi := {}\n", stl::Predicate("a"));

  return 0;
}
