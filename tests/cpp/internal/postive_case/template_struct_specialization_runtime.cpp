template <typename T>
struct SelectField {
  int value;
};

template <>
struct SelectField<void> {
  long value;
};

template <typename T>
struct RemoveConstField {
  int value;
};

template <typename T>
struct RemoveConstField<const T> {
  long value;
};

int main() {
  SelectField<void> a;
  RemoveConstField<const int> b;
  int want = (int)(sizeof(long) + sizeof(long));
  int got = (int)(sizeof(a.value) + sizeof(b.value));
  return got - want;
}
