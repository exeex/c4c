# Idea: Investigate EASTL Initializer List Runtime Validation Blocker

## Status

Open

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

Isolate whether this is:

1. a pre-existing stale-build/flaky validation issue, or
2. a real frontend / LLVM IR regression in the C++ path

Then rerun the full regression guard so the active backend plan can finish validation cleanly.
