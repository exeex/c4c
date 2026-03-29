// Parser-debug regression: when a wrapper-level top-level failure is followed
// only by speculative qualified-type probes, keep the summary leaf on the
// deeper qualified-type parser instead of the outer parse_top_level wrapper.

template <class T>
constexpr bool is_default_constructible_v = true;

template <class T>
class VecBase {
    class allocator_type {};
    class _Bit_alloc_type {
      public:
        _Bit_alloc_type(const allocator_type&);
    };

    class _Bvector_impl {
      public:
        requires is_default_constructible_v<_Bit_alloc_type>
        _Bvector_impl() : _Bit_alloc_type() {}
    };

  public:
    _Bvector_impl _M_impl;
    VecBase() = default;

    constexpr
    VecBase(VecBase&& __x, const allocator_type& __a) noexcept
        : _M_impl(_Bit_alloc_type(__a), __x._M_impl) {}
};
