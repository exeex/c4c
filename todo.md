Status: Active
Source Idea Path: ideas/open/252_aarch64_calls_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Prove Behavior Preservation And Ownership Boundaries

# Current Packet

## Just Finished

Completed Step 6 behavior-preservation and ownership-boundary proof for the
AArch64 calls shard redistribution. The prior focused backend proof preserved
the calls-facing backend behavior, and the full-suite proof passed after the
`calls.md` deletion and compiled-owner reconciliation.

## Suggested Next

Supervisor should decide lifecycle closure or any final review packet for the
active source idea.

## Watchouts

- Ownership boundary proven for this redistribution: `calls.cpp` remains the
  compiled owner for prepared call-plan consumption, direct and indirect
  `CallInstructionRecord` lowering and printing, call-boundary moves,
  ABI-binding records, memory-return snapshots, preserved-value and clobber
  effects, and variadic entry helper records; `calls.md` has no remaining
  durable ownership role.
- Preserved non-calls ownership: historical ABI configuration, stack-area
  computation, stack-argument copy sequencing, local reclassification, final
  cleanup arithmetic, and runtime-helper `bl` emission were not absorbed into
  calls ownership by this closure step.
- Remaining closure consideration: deferred carrier/runtime-helper boundaries
  still need separate initiatives when required, including complete outgoing
  call-area and call-time stack-alignment carriers, indirect-callee spill-area
  ownership, target ABI classification carriers for HFA/F128/i128/aggregate
  splitting edge cases where prepared plans are insufficient, explicit special
  register role records for `x8`/`x16`/`x17`, and runtime-helper call resource
  carriers for F128 or soft-float helper paths.

## Proof

Proof passed with log at `test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure`.
The build was up to date and the full suite passed 3167/3167 tests.

Earlier focused backend proof also passed before this closure step:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
passed 139/139 `backend_` tests.
