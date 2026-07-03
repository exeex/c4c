Status: Active
Source Idea Path: ideas/open/563_rv64_f64_global_memory_consumption.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused F64 Global-Memory Coverage

# Current Packet

## Just Finished

Completed plan Step 2, `Add Focused F64 Global-Memory Coverage`, for
`ideas/open/563_rv64_f64_global_memory_consumption.md`.

Changed files:

- `src/backend/mir/riscv/codegen/prepared_global_memory_emit.hpp`
- `src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`

Added focused backend coverage for a prepared `double` / `F64` global load with
explicit prepared global-symbol access facts: `size=8`, `align=8`,
direct base-plus-offset global-symbol addressing, scalar-layout authority,
proven-in-bounds range, and FPR destination storage. The positive test asserts
RV64 emits `fld ft0, 0(t1)` through the prepared relocation pair instead of
using a filename-shaped path or diagnostic filter.

Added the fail-closed companion coverage: the same prepared F64 load without
prepared memory-access facts still rejects with the prepared global-symbol
base-plus-offset diagnostic.

The packet also added the minimal RV64 consumer surface needed for the focused
contract: prepared F64 global loads now use the existing prepared global-symbol
facts, select an FPR destination, emit an `fld`, and keep unsupported or missing
prepared facts rejected. This effectively consumes Step 3's F64 global-memory
repair work for loads.

Supervisor representative follow-up for `src/20001121-1.c`:

```sh
printf '%s\n' src/20001121-1.c > build/agent_state/563_step2_f64_global_after.allowlist && ALLOWLIST=build/agent_state/563_step2_f64_global_after.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/563_step2_f64_global_after.log 2>&1
```

Result: expected failure remains, `total=1 passed=0 failed=1`, but the old
F64 global-memory type-gate diagnostic is gone. The new downstream owner is:

```text
unsupported_terminator_fragment: BIR terminator requires unsupported RV64 object lowering
```

## Suggested Next

Executor packet for plan Step 4, `Reconcile Representative And Residual Owner`:

- Classify the recorded `unsupported_terminator_fragment` residual as
  downstream from this F64 global-memory source idea.
- Recommend lifecycle close or split based on whether the terminator residual
  is already covered by an existing open idea.

## Watchouts

- The focused repair covers prepared F64 `LoadGlobalInst` consumption only.
  It does not implement F64 stores, F32 globals, F128, long-double, stack-frame,
  or FPR callee-saved work.
- The focused test intentionally uses a void function so the coverage remains
  about global-memory consumption, not F64 return ABI lowering.
- The representative has advanced to a downstream terminator-lowering residual;
  keep that owner distinct from this prepared F64 global-memory consumer lane.
- Do not special-case `src/20001121-1.c`, diagnostic strings, allowlists,
  expected outputs, unsupported markers, or pass/fail accounting.

## Proof

Required proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result: passed, `345/345` backend tests. Log path: `test_after.log`.

Supervisor follow-up checks:

- Regression guard passed: before=`345/0`, after=`345/0`.
- Representative probe log:
  `build/agent_state/563_step2_f64_global_after.log`.
- Representative result: `total=1 passed=0 failed=1`; old F64 global-memory
  type-gate diagnostic gone; downstream terminator residual recorded above.
