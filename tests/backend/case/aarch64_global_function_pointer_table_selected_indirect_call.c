typedef int (*dispatch_fn)(int);

int choose_one(int x) { return x + 1; }
int choose_two(int x) { return x + 20; }
int choose_three(int x) { return x + 300; }

dispatch_fn dispatch_table[3] = {choose_one, choose_two, choose_three};

int call_index(int index, int value) {
  dispatch_fn selected = dispatch_table[index];
  return selected(value);
}

int main(void) {
  return call_index(0, 4) + call_index(1, 4) + call_index(2, 4) - 333;
}
