// HIR regression: basic control-flow lowering should keep the emitted
// branch-and-loop block structure stable across helper extraction from
// impl/stmt/stmt.cpp.

int drive(int limit) {
  int total = 0;
  if (limit > 3) {
    total = limit;
  } else {
    total = 3;
  }

  while (total < 8) {
    total = total + 1;
  }

  for (int i = 0; i < 2; i = i + 1) {
    total = total + i;
  }

  do {
    total = total - 1;
  } while (total > 8);

  return total;
}

int main() {
  return drive(5) - 8;
}
