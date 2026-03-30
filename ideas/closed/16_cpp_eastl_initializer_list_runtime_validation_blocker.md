# Idea: Investigate EASTL Initializer List Runtime Validation Blocker

## Status

Complete

## Trigger

Discovered while validating the active `param_member_array` AArch64 asm slice after a clean rebuild on 2026-03-30.

## Problem

The clean-build full-suite validation introduced one new failing test outside the active backend slice:

- `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`

The regression guard reported:

- before: passed=715 failed=25 total=740
- after: passed=715 failed=26 total=741

The failing test currently reports:

- `C2LL_RUNTIME_UNEXPECTED_RETURN`
- source: `tests/cpp/internal/postive_case/eastl_probe_initializer_list_runtime.cpp`
- exit: `no such file or directory`

Manual reproduction showed `/usr/bin/clang -x ir` rejecting the compiler output for that case with:

- `<stdin>:43:16: error: '%p.__lhs' defined with type '%"struct.std::byte" ...' but expected 'i32'`

## Why Separate

This is not part of the bounded AArch64 `param_member_array` asm plan:

- the failing case is a C++ positive runtime test
- it uses the normal `c4cll | clang -x ir` pipeline rather than the AArch64 asm backend path
- the active plan already completed its bounded implementation and targeted runtime promotion work

## Next Step

Resolved on 2026-03-30.

## Resolution

- `enum class byte : unsigned char` was not being parsed as a scoped enum, which left `std::byte` materialized as a zero-sized struct and produced invalid LLVM IR for the bitwise helper overloads.
- libc++ `std::initializer_list` materialization also needed to recognize and populate the Darwin libc++ field names `__begin_` and `__size_` instead of only `_M_array` and `_M_len`.
- C++ assignment expressions needed to preserve lvalue identity in LIR emission so helper overloads returning references could lower correctly.

## Validation

- Targeted tests now pass:
  - `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`
  - `cpp_positive_sema_scoped_enum_underlying_type_parse_cpp`
- Clean-build regression guard now passes:
  - before: passed=715 failed=25 total=740
  - after: passed=718 failed=24 total=742
  - resolved failing tests: `cpp_llvm_initializer_list_runtime_materialization`
  - new failing tests: none

## Follow-On

- The superseded backend idea `ideas/open/15_backend_aarch64_param_member_array_plan.md` can resume closure work with the validation blocker removed.
