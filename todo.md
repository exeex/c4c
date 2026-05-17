Status: Active
Source Idea Path: ideas/open/271_aarch64_variadic_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add The Compiled Variadic Owner

# Current Packet

## Just Finished

Completed Plan Step 2, `Add The Compiled Variadic Owner`.

Added `src/backend/mir/aarch64/codegen/variadic.hpp` and `variadic.cpp`, wired
`variadic.cpp` into `src/backend/CMakeLists.txt`, and moved the current live
variadic boundary behavior behind that compiled owner:

- helper classification for `llvm.va_start.p0`, `llvm.va_copy.p0.p0`,
  `llvm.va_arg.*`, and `llvm.va_arg.aggregate`
- prepared variadic entry fact gating and missing-fact diagnostics
- helper operand-home completeness checks and helper-specific consumption
  diagnostics
- structured helper record builders for `va_start`, scalar `va_arg`, aggregate
  `va_arg`, and `va_copy`
- variadic helper machine-node selection diagnostics and printer branches

The narrow call sites in `dispatch.cpp` and `calls.cpp` now call the variadic
owner. Existing prepared-carrier authority, opcodes, side effects, direct extern
variadic wrapper/FPR-count metadata, unsupported contracts, and emitted printer
text were preserved; no new `va_start`, `va_arg`, `va_copy`, register-save-area,
overflow-area, or `va_list` semantics were implemented.

## Suggested Next

Execute Step 3 by reviewing the resulting compiled route boundary and deciding
whether any remaining variadic integration hooks should move to
`variadic.{hpp,cpp}`. Keep that packet behavior-preserving; if no further live
route movement is needed, record the deferral in `todo.md` rather than changing
ABI behavior.

## Watchouts

The direct extern variadic call metadata still belongs to `PreparedCallPlan` and
`CallInstructionRecord`; this packet did not change call ABI classification or
the ordinary direct-call printer output. `variadic.md` remains historical and
was not touched.

## Proof

Ran:

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`

Result: passed. `test_after.log` contains the fresh build plus backend subset
proof; the backend subset reports 139/139 tests passed.
