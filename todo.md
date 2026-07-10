Status: Active
Source Idea Path: ideas/open/659_rv64_byval_prepared_call_boundary.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Repair The Selected Publication Or Consumption Boundary

# Current Packet

## Just Finished

Continued `plan.md` Step 2 by repairing the remaining RV64 caller aggregate
local-store path and the adjacent prepared call-consumer register selection.

Implemented function-aware local text frame-slot access selection so a caller
`entry` store does not consume a same-index callee access. Added a scalar
`StoreLocalInst` frame-slot fallback for prepared local aggregate field stores
whose prepared access cannot be consumed directly. Adjusted RV64 prepared call
emission so a byval stack-copy argument consumes `a0` and the following local
frame-address GPR argument is emitted into its ABI register (`a1`) instead of
overwriting `a0`.

Result: complete for this packet. The two aggregate stack-copy route rows now
reach the prepared call consumer and include `addi sp, sp, -16`; all three
route rows in the delegated proof are green. The remaining failures are the
three runtime rows, all returning `exit=1`.

## Suggested Next

Next packet should repair the RV64 runtime result/preservation boundary after
the now-emitted byval calls. Start with the visible post-call restore pattern:
`backend_codegen_route_riscv64_byval_formal_gpr_publication` had already shown
`mv t0, s2` overwriting the call result, and the two aggregate runtime rows now
also reach the call path but return `exit=1`.

## Watchouts

- Do not edit expectations, unsupported markers, allowlists, runtime policy,
  timeout settings, or baseline acceptance files.
- Do not merge pointer-local, stack fan-in, AArch64, CLI, static object-data,
  callee-saved GPR, packed-member, or LLVM torture work into this route.
- Reject named-case or final-assembly-shape fixes.
- Do not treat the two dump failures as permission to rewrite expectations;
  their current output is useful positive evidence that prepared facts exist.
- Keep the object-runtime `BinaryInst` unsupported-fragment row as a separate
  split unless the supervisor explicitly assigns object-route coverage.
- The aggregate route rows now prove the caller local stores and byval
  stack-copy call consumer; do not re-open that path unless a regression points
  back to these selectors.
- Runtime remains red outside this packet: all three runtime rows now return
  `exit=1`, so the next owner is call result preservation/restore ordering, not
  missing route emission.

## Proof

Ran exactly:

```sh
cmake --build --preset default && (ctest --test-dir build -j --output-on-failure -R 'backend_(codegen_route_riscv64_byval_aggregate_fixed_call|codegen_route_riscv64_byval_preserved_pointer_args|codegen_route_riscv64_byval_formal_gpr_publication|rv64_runtime_riscv64_byval_aggregate_fixed_call|rv64_runtime_riscv64_byval_preserved_pointer_args|rv64_runtime_riscv64_byval_formal_gpr_publication)' > test_after.log; test -s test_after.log)
```

Result: build succeeded, focused CTest is red with 3/6 failures, and
`test_after.log` is populated. Passing rows:
`backend_codegen_route_riscv64_byval_aggregate_fixed_call`,
`backend_codegen_route_riscv64_byval_preserved_pointer_args`, and
`backend_codegen_route_riscv64_byval_formal_gpr_publication`. Failing rows:
the three `backend_rv64_runtime_*` rows, all with `exit=1`.

Supervisor acceptance also ran a stash-based backend before/after guard:

```sh
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R '^backend_' > test_before.log
ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Result: PASS. Before `passed=336 failed=32 total=368`; after
`passed=343 failed=25 total=368`; resolved 7 backend rows; new failing tests:
0.
