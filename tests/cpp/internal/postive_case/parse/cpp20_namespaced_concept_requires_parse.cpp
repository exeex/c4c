namespace foo {
template<typename T>
concept trivial_requirement = requires(T value) {
  value;
};
}

struct after_concept {
  int value;
};

int main() {
  after_concept x{42};
  return x.value;
}
