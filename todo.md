Status: Active
Source Idea Path: ideas/open/110_aarch64_remaining_baseline_failure_recovery.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Instruction Dispatch Floating-Point Call Feed

# Current Packet

## Just Finished

Step 2 repaired the remaining AArch64 instruction-dispatch floating-point
call-feed failure without weakening the internal test contract. A selected
`f64` global readback that has already been published into its prepared FPR
home now feeds the call ABI through the selected call-boundary move instead of
being recomputed directly into `d0`. The same call-boundary materialization
gate now preserves plain full-width `f128` q-register argument moves as selected
move records unless their source actually requires global rematerialization.

Files changed:
- `src/backend/mir/aarch64/codegen/calls.cpp`

Before/after subset movement:
- PASS `backend_aarch64_instruction_dispatch`; moved from the selected `f64`
  global readback call-ABI publication failure to passing.
- PASS `c_testsuite_aarch64_backend_src_00172_c`; remained passing.
- PASS `c_testsuite_aarch64_backend_src_00180_c`; remained passing.
- PASS `c_testsuite_aarch64_backend_src_00216_c`; remained passing.
- PASS `c_testsuite_aarch64_backend_src_00220_c`; remained passing.
- PASS `c_testsuite_aarch64_backend_src_00204_c`; remained passing.

## Suggested Next

Supervisor should review and commit this completed Step 2 slice with
`todo.md`, or route lifecycle closure/review if the active runbook is now
exhausted.

## Watchouts

The call-boundary change is intentionally scoped to prepared FPR/Vreg register
arguments that already have selected source authority. It does not rewrite
expectations or mark unsupported cases, and it leaves global rematerialization
available when the source traces to a global expression that needs a fresh
readback.

## Proof

Ran:
`cmake --build --preset default; ctest --test-dir build -j1 --output-on-failure -R '^(backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00(172|180|216|220|204)_c)$' > test_after.log 2>&1`

Build rebuilt the touched AArch64 backend object and linked dependent tests.
Final CTest result after this packet: 6/6 passed. Passing targets:
`backend_aarch64_instruction_dispatch`, `00172`, `00180`, `00216`, `00220`,
and `00204`. Proof log: `test_after.log`.
