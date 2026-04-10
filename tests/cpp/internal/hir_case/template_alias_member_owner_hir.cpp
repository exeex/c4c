// HIR regression: dependent `typename` spellings should resolve alias-template
// owners through parser-local typedef-chain helpers before selecting a member
// typedef from the instantiated record.

namespace api {

template <typename T>
struct box {
  using value_type = T;
};

template <typename T>
using alias_box = box<T>;

}  // namespace api

template <typename T>
typename ::api::alias_box<T>::value_type project(T value) {
  typename ::api::alias_box<T>::value_type local = value;
  return local;
}

int main() {
  return project<int>(42) - 42;
}
