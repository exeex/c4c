# EASTL C++17 Preprocessor Diff Audit

## Goal

Document the concrete differences between:

- c4c internal preprocessing:

```bash
build/c4cll --pp-only -I ref/EASTL/include -I ref/EABase/include/Common \
  tests/cpp/eastl/eastl_type_traits_simple.cpp > /tmp/eastl_pp_only.cpp
```

- system preprocessing under C++17:

```bash
c++ -std=c++17 -E -P -I ref/EASTL/include -I ref/EABase/include/Common \
  tests/cpp/eastl/eastl_type_traits_simple.cpp \
  -o /tmp/eastl_type_traits_cpp17.pp.cpp
```

This note is intended as handoff material for a follow-up agent.


## Environment Notes

- We changed internal `__cplusplus` from `201402L` to `201703L` in
  [src/frontend/preprocessor/preprocessor.cpp](/workspaces/c4c/src/frontend/preprocessor/preprocessor.cpp).
- Verified with a probe that internal `--pp-only` now takes the `__cplusplus == 201703L` branch.
- This audit compares internal C++17-mode preprocessing against system `c++ -std=c++17`.


## High-Level Findings

1. Internal preprocessing and system C++17 preprocessing still diverge materially even after aligning `__cplusplus` to C++17.
2. The divergence is not just formatting or line markers.
3. Internal PP output is much larger:

```text
/tmp/eastl_pp_only.cpp              44472 lines
/tmp/eastl_type_traits_cpp17.pp.cpp 15395 lines
```

4. Feeding internal PP output back into `c4cll` reproduces the same failure:

```bash
build/c4cll /tmp/eastl_pp_only.cpp
```

Result:

```text
error: object has incomplete type: eastl::is_arithmetic
```

This means the failure is preserved after internal PP, so the next-stage parser/sema issue is real. But PP divergence still matters because it changes which headers / codepaths are visible downstream.
5. During follow-up debugging, the earlier `EANonCopyable` parse failure turned out not to be a unique PP corruption.
6. The immediate parser desynchronization there was caused by a separate frontend bug:
   C++11 `[[...]]` attributes before templated declarations were not skipped early
   enough at top level / local-decl entry, so later declarations could be parsed
   from a bad token position.
7. After fixing that parser bug, both:
   - `build/c4cll --parse-only /tmp/eastl_pp_only.cpp`
   - `build/c4cll tests/cpp/eastl/eastl_type_traits_simple.cpp -I ref/EASTL/include -I ref/EABase/include/Common`
   
   now fail directly at the downstream semantic issue:
   
```text
error: object has incomplete type: eastl::is_arithmetic
```

So the current live blocker has been narrowed back down to `eastl::is_arithmetic`,
while the PP-diff concerns below remain relevant as likely contributors.


## Confirmed Content Diffs

### 1. System C++17 output contains `__decltype(0.0bf16)`; internal PP output does not

System output contains:

```cpp
typedef __decltype(0.0bf16) __bfloat16_t;
```

Observed at:

- [/tmp/eastl_type_traits_cpp17.pp.cpp](/tmp/eastl_type_traits_cpp17.pp.cpp#L57)

Search results:

- internal PP: no `__decltype`
- system C++17: hit present

This is one of the clearest concrete divergences.


### 2. System C++17 output contains `__detected_or`; internal PP output does not

System output contains:

```cpp
using __detected_or = __detector<_Default, void, _Op, _Args...>;
```

Observed at:

- [/tmp/eastl_type_traits_cpp17.pp.cpp](/tmp/eastl_type_traits_cpp17.pp.cpp#L5237)

Search results:

- internal PP: no `__detected_or`
- system C++17: multiple hits

This matters because it indicates the internal PP and the system preprocessor are not exposing the same libstdc++ path under C++17.


### 3. `requires requires` is absent in both internal PP and system C++17 output

This is expected and good:

- internal PP: no hits
- system C++17: no hits

So the earlier `requires` findings were a C++20-only branch, not a C++17 blocker.


### 4. Internal PP and system output disagree near the earliest libstdc++ bootstrap region

Representative diff from the first few hundred non-empty lines:

- system output includes `#pragma GCC visibility push/pop`
- system output includes `__glibcxx_assert_fail()` inline stub
- system output includes `__gnu_cxx::__bfloat16_t`
- system output includes `max_align_t`
- internal PP differs in `__is_constant_evaluated()` body and lacks the `__bfloat16_t` region

The earliest diff captured during inspection showed:

- internal PP had:
  `return false;`
- system output had:
  `return __builtin_is_constant_evaluated();`

This suggests macro / builtin-header / include-resolution drift early in the libstdc++ include chain.


### 5. `EANonCopyable` looks odd in both outputs, but is not a unique internal-PP corruption

Internal PP contains:

- [/tmp/eastl_pp_only.cpp](/tmp/eastl_pp_only.cpp#L15536)

System output contains:

- [/tmp/eastl_type_traits_cpp17.pp.cpp](/tmp/eastl_type_traits_cpp17.pp.cpp#L161)

Both have the same macro-expanded shape:

```cpp
EANonCopyable() = default;
~EANonCopyable() = default;
; EANonCopyable(const EANonCopyable&) = delete; void operator=(const EANonCopyable&) = delete; ;
```

So this odd `= default` / `= delete` formatting is probably not a PP bug by itself. It is macro noise present in both paths.

Follow-up result:

- The parser failure previously seen around this region was fixed by teaching
  top-level / local declaration parsing to skip leading C++11 `[[...]]`
  attributes before templated declarations.
- A regression test was added at
  [tests/cpp/internal/postive_case/cpp11_attr_template_decl_parse.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/cpp11_attr_template_decl_parse.cpp).
- After that fix, `EANonCopyable` no longer appears to be the active EASTL
  blocker.


### 6. `is_arithmetic_v` exists in both outputs, but at very different structural locations

Internal PP:

- [/tmp/eastl_pp_only.cpp](/tmp/eastl_pp_only.cpp#L38521)

System output:

- [/tmp/eastl_type_traits_cpp17.pp.cpp](/tmp/eastl_type_traits_cpp17.pp.cpp#L14505)

This alone is not a bug, but it reinforces that the overall preprocessed TU shape is very different.


## Likely Investigation Areas

These are hypotheses, not yet confirmed:

1. Internal PP may be taking a different include path or include ordering than system C++17.
2. Builtin header injection or predefined macro values may be altering early libstdc++ branches.
3. The internal preprocessor may be expanding or suppressing certain builtin/compiler intrinsics differently from system preprocessing.
4. The line-marker preserving output is expected, but the missing semantic content (`__decltype`, `__detected_or`) is not explained by line markers alone.
5. The remaining `eastl::is_arithmetic` incomplete-type failure may still be downstream of PP divergence, but the previously observed `EANonCopyable` parse noise should no longer be treated as evidence of that.


## Suggested Next Steps

1. Continue from the now-restored primary failure:
   - `error: object has incomplete type: eastl::is_arithmetic`
2. Compare early include-chain markers between internal PP and system C++17 for:
   - `/usr/include/c++/14/cstddef`
   - `/usr/include/.../bits/c++config.h`
   - builtin headers such as `<builtin-stddef.h>`
3. Diff predefined macro sets visible to headers:
   - `__cplusplus`
   - `__GNUG__`
   - `__GXX_ABI_VERSION`
   - any builtin-feature macros that affect libstdc++ branches
4. Audit builtin-header injection and fallback logic in:
   - [src/frontend/preprocessor/preprocessor.cpp](/workspaces/c4c/src/frontend/preprocessor/preprocessor.cpp)
   - [src/frontend/preprocessor/pp_include.*](/workspaces/c4c/src/frontend/preprocessor)
   - [src/frontend/preprocessor/pp_predefined.*](/workspaces/c4c/src/frontend/preprocessor)
5. Create a reduced PP-diff repro around the earliest missing construct:
   - `typedef __decltype(0.0bf16) __bfloat16_t;`
6. If PP parity work stalls, debug `eastl::is_arithmetic` directly in the
   internally preprocessed TU now that the attribute-related parser desync is out
   of the way.


## Follow-up Fixes Completed

1. Fixed parser entrypoints so leading C++11 attributes such as
   `[[nodiscard, __gnu__::__always_inline__]]` are skipped before parsing a
   declaration at top level and local scope.
2. Verified that the reduced repro:

```cpp
template<typename T>
[[nodiscard, __gnu__::__always_inline__]]
constexpr T f(T x) noexcept { return T(x); }

struct EANonCopyable {
  EANonCopyable() = default;
  ~EANonCopyable() = default;
  EANonCopyable(const EANonCopyable&) = delete;
  void operator=(const EANonCopyable&) = delete;
};
```

   now parses correctly.
3. Verified that both internal-PP and system-PP EASTL repros no longer stop at
   the `EANonCopyable` region before reaching later failures.


## Reproduction Commands

Internal PP:

```bash
build/c4cll --pp-only -I ref/EASTL/include -I ref/EABase/include/Common \
  tests/cpp/eastl/eastl_type_traits_simple.cpp > /tmp/eastl_pp_only.cpp
```

System PP:

```bash
c++ -std=c++17 -E -P -I ref/EASTL/include -I ref/EABase/include/Common \
  tests/cpp/eastl/eastl_type_traits_simple.cpp \
  -o /tmp/eastl_type_traits_cpp17.pp.cpp
```

Round-trip internal PP through parser/sema:

```bash
build/c4cll /tmp/eastl_pp_only.cpp
```
