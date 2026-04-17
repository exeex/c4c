// Parse test: template-template parameters should parse as dependent types.
template <typename Default, template <typename...> class Op, typename... Args>
struct detector {
  using type = Op<Args...>;
};

int main() {
  return 0;
}
