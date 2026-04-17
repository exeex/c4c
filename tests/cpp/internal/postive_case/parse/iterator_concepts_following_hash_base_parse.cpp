#include <bits/iterator_concepts.h>

template<typename R, typename A>
struct hash_base {
    typedef R result_type;
    typedef A argument_type;
};
