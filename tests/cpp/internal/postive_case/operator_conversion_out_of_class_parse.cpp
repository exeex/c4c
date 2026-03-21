struct B {
  unsigned long long mPart0;
  unsigned long long mPart1;

  operator float() const;
  operator double() const;
  operator long double() const;

  static void OperatorPlus(const B& value1, const B& value2, B& result);
};

inline B::operator float() const {
  return (float)mPart0;
}

inline B::operator double() const {
  return (double)mPart0;
}

inline B::operator long double() const {
  return (long double)mPart0;
}

inline void B::OperatorPlus(const B& value1, const B& value2, B& result) {
  unsigned long long t = value1.mPart0 + value2.mPart0;
  unsigned long long nCarry = (t < value1.mPart0) && (t < value2.mPart0);
  result.mPart0 = t;
  result.mPart1 = value1.mPart1 + value2.mPart1 + nCarry;
}

int main() {
  return 0;
}
