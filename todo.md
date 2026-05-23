Status: Active
Source Idea Path: ideas/open/386_aarch64_calls_mechanical_split.md
Source Plan Path: plan.md
Current Step ID: Step 9
Current Step Title: Size And Integration Checkpoint

# Current Packet

## Just Finished

Lifecycle close rejected after Step 8 because Step 9 - Size And Integration
Checkpoint is not complete.

Size criteria are satisfied:

- `calls.cpp`: 200 lines.
- largest extracted calls implementation file: `calls_moves.cpp`, 3397 lines.

## Suggested Next

Keep the active plan open at Step 9. Investigate whether the three full-suite
failures are known baseline failures or regressions from the mechanical split,
then rerun the source idea's full close proof before another closure attempt.

## Watchouts

- Unrelated dirty implementation/test files were already present when this
  lifecycle state was activated; do not revert or sweep them into a lifecycle
  commit.
- Keep this idea behavior-preserving and reject testcase-shaped shortcuts,
  expectation downgrades, or backend capability changes.
- `append_call_diagnostic` already has an external implementation in
  `dispatch_diagnostics.cpp`; do not add a second same-signature body in
  `calls_common.cpp` unless that broader diagnostic ownership is explicitly
  changed.
- `calls_common.hpp` currently includes `calls.hpp` for shared call/codegen
  types. Later narrower headers can reduce that include after more clusters are
  split.
- `calls_printing.cpp` includes `calls_byval_aggregates.hpp`, `calls_effects.hpp`,
  `float_ops.hpp`, `machine_printer.hpp`, `memory.hpp`, and `variadic.hpp` for
  dependencies exposed by the moved print/status/record cluster.
- `calls_moves.cpp` already has its own immediate-cast publication helper copy;
  this packet did not touch that unowned file.
- `clang-format` was unavailable in the environment.

## Proof

Step 8 packet proof was green:

```bash
cmake --build build --target c4cll backend_aarch64_instruction_dispatch_test -j10 && ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_lir_to_bir_notes|c_testsuite_aarch64_backend_src_00204_c)$' --output-on-failure
```

Result: green; all 3 selected tests passed.

Step 9 close proof was run on 2026-05-23:

```bash
cmake --build build --target c4cll backend_aarch64_instruction_dispatch_test -j10 && ctest --test-dir build -j10 --output-on-failure
```

Result: failed; 3394 of 3397 tests passed. Failing tests:

- `c_testsuite_aarch64_backend_src_00176_c`
- `c_testsuite_aarch64_backend_src_00182_c`
- `c_testsuite_aarch64_backend_src_00205_c`
