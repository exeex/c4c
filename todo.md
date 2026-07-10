Status: Active
Source Idea Path: ideas/open/659_rv64_byval_prepared_call_boundary.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Repair The Selected Publication Or Consumption Boundary

# Current Packet

## Just Finished

Step 2 runtime-boundary probe did not land code. The focused runtime rows
remain red from the clean committed route slice:
`backend_rv64_runtime_riscv64_byval_aggregate_fixed_call`,
`backend_rv64_runtime_riscv64_byval_preserved_pointer_args`, and
`backend_rv64_runtime_riscv64_byval_formal_gpr_publication`.

A narrow preservation-republication guard removed the visible `mv t0, s2`
clobber after `mv t0, a0`, but the rows still failed because caller aggregate
field stores still publish wrong local offsets before the byval call. A
prepared-fact keyed local-store attempt did not change that offset shape, so
the code edits were reverted and no implementation files remain dirty.

## Suggested Next

Next packet should inspect the prepared text local-store ownership boundary for
split aggregate local slots before reattempting runtime repair. The exact
remaining shape is caller stores such as `bir.store_local %lv.value.8, i32 0`
emitting `sw t1, 0(sp)` instead of the prepared frame-slot offset, while
prepared dumps contain frame-slot access rows for the split local slots.

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
- A result preservation-republication guard is likely still needed, but guard
  alone is insufficient; the caller aggregate field store offset shape must be
  fixed without broad raw-slot fallback.
- A local probe dump at `build/riscv64_byval_formal_gpr_publication.prepared.txt`
  showed prepared main access rows for `%lv.value.0`, `.4`, `.8`, and `.12`.

## Proof

Runtime-boundary proof ran:

```sh
cmake --build --preset default && (ctest --test-dir build -j --output-on-failure -R 'backend_rv64_runtime_riscv64_byval_(aggregate_fixed_call|preserved_pointer_args|formal_gpr_publication)' > test_after.log; test -s test_after.log)
```

Result: build succeeded, focused CTest stayed red with 0/3 passing, and all
three runtime rows returned `exit=1`. `test_after.log` is the preserved proof
log. No broad backend guard was run because the focused proof did not pass and
the code edits were reverted.
