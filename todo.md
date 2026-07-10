Status: Active
Source Idea Path: ideas/open/659_rv64_byval_prepared_call_boundary.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Repair The Selected Publication Or Consumption Boundary

# Current Packet

## Just Finished

Step 2 repaired the prepared text local-store and call-result preservation
boundaries for the remaining RV64 byval runtime rows:
`backend_rv64_runtime_riscv64_byval_aggregate_fixed_call`,
`backend_rv64_runtime_riscv64_byval_preserved_pointer_args`, and
`backend_rv64_runtime_riscv64_byval_formal_gpr_publication`.

The local-store repair now rejects mismatched scalar prepared memory-access
rows and falls back only to an exact prepared stack-layout object match by
store destination slot name, rather than the broad raw `StoreLocalInst` slot
fallback. This fixed split aggregate stores such as `%lv.value.8` and
`%lv.value.4`.

The call repair suppresses after-call preservation republication only when it
would write over the exact GPR where the prepared call result was just
published.

## Suggested Next

Supervisor should run a same-scope `^backend_` before/after regression guard
before committing this code slice, because the focused runtime proof is green
and the packet requires broad guard coverage before commit.

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
- The focused runtime subset now passes, but this slice is not commit-ready
  until the supervisor runs or accepts a same-scope `^backend_` before/after
  guard.
- Avoid broad scalar `StoreLocalInst` fallbacks. A previous attempt that
  allowed mismatched or missing prepared accesses to fall back to the raw store
  slot made many unrelated local-memory routes fail.
- The object-runtime row was not part of this packet and should remain a
  separate owner unless the supervisor explicitly assigns shared helper work.
- A probe of the aggregate dump showed mismatched prepared memory-access rows
  for `main` inst 0-3, so the text emitter must not trust those rows unless
  they match the destination store slot name.
- A non-delegated inspection run of
  `ctest --test-dir build -j --output-on-failure -R 'backend_dump_riscv64_byval_aggregate_fixed_call|backend_codegen_route_riscv64_byval_aggregate_fixed_call'`
  still leaves the dump row red because its expected snippet is stale relative
  to the current prepared call-argument move shape; no test expectations were
  changed.

## Proof

Runtime-boundary proof ran:

```sh
cmake --build --preset default && (ctest --test-dir build -j --output-on-failure -R 'backend_rv64_runtime_riscv64_byval_(aggregate_fixed_call|preserved_pointer_args|formal_gpr_publication)' > test_after.log; test -s test_after.log)
```

Result: build succeeded and focused CTest passed 3/3. `test_after.log` is the
preserved proof log.

Supervisor acceptance ran a stash-based same-scope backend guard:

```sh
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R '^backend_' > test_before.log
ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Result: PASS. Before `passed=343 failed=25 total=368`; after
`passed=349 failed=19 total=368`; resolved 6 backend rows; new failing tests:
0.
