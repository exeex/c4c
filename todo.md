Status: Active
Source Idea Path: ideas/open/307_aarch64_large_scalar_immediate_materialization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement Legal Large-Immediate Materialization

# Current Packet

## Just Finished

Step 2: Implement Legal Large-Immediate Materialization is complete for the
call-boundary scalar integer immediate printer path. Changed files:
`src/backend/mir/aarch64/codegen/calls.cpp`,
`src/backend/mir/aarch64/codegen/machine_printer.cpp`,
`src/backend/mir/aarch64/codegen/machine_printer.hpp`, and
`tests/backend/mir/backend_aarch64_machine_printer_test.cpp`.

Semantic repair point: `print_call_boundary_move` now detects immediate-to-GPR
call-boundary moves whose scalar integer value needs more than one move-wide
halfword and emits the shared AArch64 integer constant materialization sequence
instead of printing the primary `mov` with an arbitrary immediate. Single
move-wide values still use the existing prepared `mov` spelling.

The shared `materialize_integer_constant_lines` helper was exported from the
machine-printer module and reused by call-boundary printing. A machine-printer
regression test covers a generic multi-halfword I32 call-argument immediate
without matching `00182.c`, the `00182.c` value, or the exact former illegal
line.

Generated-code check after the focused proof: `00182.c.s` now contains
`movz x0, #54919` followed by `movk x0, #18, lsl #16` before `bl print_led`;
`mov x0, #1234567` is gone. The remaining `00182.c` failure has advanced to
`RUNTIME_MISMATCH`, so it is not the Step 2 illegal-immediate assembler-form
failure.

## Suggested Next

Supervisor should decide whether the new `00182.c` runtime mismatch belongs to
the current source idea as a follow-up runbook step or should remain parked
under the broader runtime-mismatch bucket. The next coherent packet is to
localize the first bad runtime-output fact only if that classification is inside
this active idea.

## Watchouts

- `clang-format` is not installed in this environment; formatting was checked
  manually against surrounding style.
- The Step 2 patch deliberately does not change expectations, allowlists,
  unsupported classifications, runner behavior, timeout policy, CTest
  registration, or `test_before.log`.
- Runtime mismatch, crash, timeout, and output-storm buckets were previously
  parked under umbrella idea 295; do not silently absorb the new runtime
  mismatch into this idea without supervisor classification.
- Do not special-case `00182.c`, `1234567`, or the exact emitted instruction
  string.

## Proof

Build proof passed: `cmake --build --preset default`.

Focused delegated proof command was run exactly and preserved in
`test_after.log`:
`ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_machine_printer|c_testsuite_aarch64_backend_src_00182_c)$' > test_after.log`.

Result: 1/2 passed. `backend_aarch64_machine_printer` passed.
`c_testsuite_aarch64_backend_src_00182_c` failed with `RUNTIME_MISMATCH` after
the illegal immediate was materialized legally.

Supervisor acceptance checks:

- Strict regression guard reported no new failing tests and no pass-count
  regression, but did not pass strict-improvement mode because the focused
  subset stayed 1/2.
- Non-decreasing regression guard passed:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`.
- Broader AArch64 backend unit sanity passed:
  `ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_'`
  selected 27 tests and passed 27/27.
