// Parse-only regression: expression parsing should accept a template-id owner
// followed by a qualified operator call inside a pack expansion.
// RUN: %c4cll --parse-only %s

template <int I, typename T>
struct Leaf {
    int operator=(T value) {
        return value;
    }
};

template <typename... Ts>
void swallow(Ts...);

template <int... Indices>
struct IndexPack {};

template <typename Indices, typename... Ts>
struct TupleImpl;

template <int... Indices, typename... Ts>
struct TupleImpl<IndexPack<Indices...>, Ts...> : Leaf<Indices, Ts>... {
    template <typename OtherTuple>
    void assign(OtherTuple&& t) {
        swallow(Leaf<Indices, Ts>::operator=(t)...);
    }
};

int main() {
    return 0;
}
