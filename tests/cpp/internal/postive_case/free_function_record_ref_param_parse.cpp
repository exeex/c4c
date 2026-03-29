// Reduced from libstdc++ headers: prior C++ record tags must stay usable as
// free-function parameter types, including reference declarators.

struct Widget;

void take_lref(Widget&);
void take_rref(Widget&&);
void take_const_lref(const Widget&);
void take_const_rref(const Widget&&);

