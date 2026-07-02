Status: Active
Source Idea Path: ideas/open/559_bir_runtime_intrinsic_memory_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Intrinsic Memory Admission Boundary

# Current Packet

## Just Finished

Completed Step 1 from `plan.md`: inspected the current runtime/intrinsic
memory admission boundary for the representative `memcpy` and `memset` rows.
The delegated RV64 proof reproduces the intended diagnostics:
`src/20000703-1.c` fails in function `foo` in `memcpy runtime family`, and
`src/20041218-1.c` fails in function `baz` in `memset runtime family`.

LLVM/BIR-entry shapes:

- `src/20000703-1.c`: `foo(struct baz *p, ...)` lowers
  `__builtin_memcpy(p->b, "abc", 3)` to
  `%t0 = getelementptr %struct.baz, ptr %p.p, i32 0, i32 1`,
  `%t1 = getelementptr [3 x i8], ptr %t0, i64 0, i64 0`,
  `%t2 = getelementptr [4 x i8], ptr @.str0, i64 0, i64 0`,
  `%t3 = call ptr @memcpy(ptr %t1, ptr %t2, i32 3)`.
- `src/20041218-1.c`: `baz(unsigned int x)` lowers
  `__builtin_memset(&v, 0x55, sizeof(v))` to
  `%t0 = call ptr @memset(ptr @__static_local_baz_0, i32 85, i64 72)`.

The first missing producer fact is destination memory-effect publication for
non-local intrinsic targets. `try_lower_direct_memory_intrinsic_call` in
`src/backend/bir/lir_to_bir/memory/intrinsics.cpp` delegates both runtime
calls to `try_lower_immediate_local_memcpy` / `try_lower_immediate_local_memset`.
Those helpers can publish `LoadLocalInst` / `StoreLocalInst` facts for local
aggregate, array, and scalar slots, and `memcpy` already handles non-local
sources through `PointerValue` / `GlobalSymbol` memory addresses. They do not
publish stores for a pointer-value destination such as the parameter-derived
`p->b`, or for a global-symbol destination such as `@__static_local_baz_0`.
The owning boundary is therefore the runtime intrinsic memory producer in
`src/backend/bir/lir_to_bir/memory/intrinsics.cpp`, using existing pointer/global
provenance facts produced by `memory/addressing.cpp` and tracked through
`pointer_value_addresses_` / `global_types_`.

`memcpy` and `memset` should stay in one repair lane: the operand shapes differ,
but the shared repair boundary is non-local destination memory-effect facts for
runtime intrinsic memory operations, not separate testcase-specific lowering.

## Suggested Next

Add focused BIR note coverage for runtime intrinsic stores to non-local
destinations, then repair `src/backend/bir/lir_to_bir/memory/intrinsics.cpp` so
`memcpy` can store copied bytes to pointer-value destinations and `memset` can
store repeated bytes to global-symbol destinations with explicit
`MemoryAddress` provenance. Keep the first code packet inside the intrinsic
producer boundary unless the focused tests prove an addressing helper must
change.

## Watchouts

- Do not replace intrinsic rows with runtime call substitutions or
  target-specific named-case lowering.
- Do not weaken expectations, unsupported markers, allowlists, or semantic
  admission checks.
- `src/20000703-1.c` first fails before reaching the later `bar` `memset` and
  additional `memcpy` calls; the next packet should not claim full row movement
  from a `foo`-only proof.
- `src/20041218-1.c` requires global destination memset support. `memcpy`
  already has global-source support, but that is not enough for this row.
- The delegated proof writes to `build/agent_state/559_step1_intrinsic_memory.log`;
  no root-level `test_after.log` was requested for this inspection-only packet.

## Proof

- `printf '%s\n' src/20000703-1.c src/20041218-1.c > build/agent_state/559_step1_intrinsic_memory.allowlist && ALLOWLIST=build/agent_state/559_step1_intrinsic_memory.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/559_step1_intrinsic_memory.log 2>&1`
  exited `1`, expected for inspection, with both representatives failing in the
  current runtime/intrinsic semantic admission families.
- Row logs:
  `build/rv64_gcc_c_torture_backend/src_20000703-1.c/case.log` and
  `build/rv64_gcc_c_torture_backend/src_20041218-1.c/case.log`.
- LLVM shape dumps were inspected through
  `build/c4cll --target riscv64-linux-gnu --codegen llvm` for both rows.
- `build/c4cll --target riscv64-linux-gnu --dump-bir` for both rows stops at
  the same semantic admission diagnostics, so no complete BIR dump is available
  before the repair.
