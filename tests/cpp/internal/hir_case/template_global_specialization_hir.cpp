// HIR regression: template global instantiation should keep mangled global
// emission stable after the helper family moves out of hir_templates.cpp.

namespace ns {

template <typename T>
int value = sizeof(T);

}  // namespace ns

int main() {
  return ns::value<int>;
}
