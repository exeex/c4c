// Parse test: C++ pointer-to-member declarators in params and locals.
template<typename R, typename C>
void accept(R C::*member);

struct Widget {
  int value;
};

void test() {
  int Widget::*member = 0;
  accept(member);
}

int main() {
  return 0;
}
