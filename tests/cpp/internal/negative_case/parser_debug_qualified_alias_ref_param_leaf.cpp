// Parser-debug regression: keep the qualified-type probe leaf when a
// namespace-scoped template alias parameter declaration degrades at the
// reference token.

namespace ns {
template <class T>
struct Box {};

using Alias = Box<int>;
}

void value(const Alias&);
