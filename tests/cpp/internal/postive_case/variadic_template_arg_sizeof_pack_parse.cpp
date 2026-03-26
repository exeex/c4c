// Reduced repro for EASTL tuple/function bring-up:
// sizeof...(pack) should be accepted as a non-type template argument in
// type/template-id context, e.g. TupleEqual<sizeof...(Ts)>.

template<int N>
struct X {};

template<typename... Ts>
X<sizeof...(Ts)> make();

int main() {
    return 0;
}
