Status: Active
Source Idea Path: ideas/open/659_rv64_byval_prepared_call_boundary.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Repair The Selected Publication Or Consumption Boundary

# Current Packet

## Just Finished

Step 2 route-emission slice is complete and committed. RV64 prepared call text
emission now reaches the byval/prepared call consumer for the three route rows,
emits the 16-byte outgoing stack argument area for aggregate stack-copy calls,
uses the correct ABI register for the following GPR argument, and emits the
prepared immediate GPR argument path such as `li a2, 5`.

Rejected follow-up attempt: a runtime-focused patch made the three
`backend_rv64_runtime_riscv64_byval_*` rows pass locally, but its local-store
fallback caused broad backend regressions (`^backend_` dropped from
`passed=343 failed=25 total=368` to `passed=320 failed=48 total=368`, with 27
new failing tests). That code was removed before commit.

## Suggested Next

Next packet should repair the RV64 runtime result/preservation boundary for
the three remaining runtime rows without broad local-store fallback. Start from
the rejected attempt's useful observation: a preservation-republication guard
alone was not enough, and broad direct `StoreLocalInst` fallback regressed
unrelated local-memory routes.

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
- The runtime rows still fail when only the committed route-emission slice is
  present; do not claim them fixed until both the focused runtime subset and a
  same-scope backend guard pass.
- Avoid broad scalar `StoreLocalInst` fallbacks. A previous attempt that
  allowed mismatched or missing prepared accesses to fall back to the raw store
  slot made many unrelated local-memory routes fail.
- The object-runtime row was not part of this packet and should remain a
  separate owner unless the supervisor explicitly assigns shared helper work.

## Proof

Committed route-emission proof ran:

```sh
cmake --build --preset default && (ctest --test-dir build -j --output-on-failure -R 'backend_(codegen_route_riscv64_byval_aggregate_fixed_call|codegen_route_riscv64_byval_preserved_pointer_args|codegen_route_riscv64_byval_formal_gpr_publication|rv64_runtime_riscv64_byval_aggregate_fixed_call|rv64_runtime_riscv64_byval_preserved_pointer_args|rv64_runtime_riscv64_byval_formal_gpr_publication)' > test_after.log; test -s test_after.log)
```

Result for the committed route slice: build succeeded, focused CTest was red
with 3/6 failures, all three route rows passed, and all three runtime rows
returned `exit=1`. A stash-based backend before/after guard passed with before
`passed=336 failed=32 total=368`, after `passed=343 failed=25 total=368`, and
new failing tests: 0.
