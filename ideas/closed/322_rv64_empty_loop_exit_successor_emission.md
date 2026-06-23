# RV64 Empty Loop-Exit Successor Emission

## Goal

Repair RV64 function/control-flow emission when a loop false successor is an
empty exit block at the end of emitted text and must still own a valid
return/epilogue path instead of falling through into the next section.

## Why This Exists

Idea 321 Step 3/4 repaired the i16 halfword local-array select/store body
exposed by `src/00143.c`. Fresh reprobe evidence under
`build/rv64_c_testsuite_probe_latest/triage_321_step3/` shows BIR dump,
prepared-BIR dump, RV64 emit, and clang link all return 0, and the generated
body contains semantic `lh`/`sh` halfword select/store sequences.

The remaining `src/00143.c` failure is qemu exit 132 (`SIGILL`). The loop false
successor `.Lmain_block_2` is defined at the end of emitted text with no
return/epilogue body. The loop-condition branch can jump to that label, which
then falls through into the next linked section (`_IO_stdin_used` in the
observed binary). This is separate from the halfword local-array publication
owned by idea 321.

## In Scope

- RV64 emission for empty loop-exit successor blocks.
- Return/epilogue ownership when a reachable successor block has no body
  instructions of its own.
- Focused backend coverage for branches to empty end-of-function successor
  blocks that must not fall through into unrelated sections.
- Candidate reprobe for `src/00143.c` after the i16 local-array body has
  already emitted.

## Out Of Scope

- I16 local-array select/store publication from idea 321.
- Stack-homed fused compare branch or loop-condition repair from idea 319.
- Nested select-chain/store-source publication for `src/00144.c` from idea
  320.
- Aggregate/byval repair, function-pointer repair, external-call policy, or
  broad switch/control-flow rewrites beyond empty exit successor emission.

## Acceptance Criteria

- Focused tests cover a reachable empty loop-exit successor at the end of an
  RV64 function and require a valid return/epilogue path.
- `src/00143.c` either emits, links, and runs under qemu, or any remaining
  failure is reclassified with concrete emitted-code evidence as a different
  mechanism after the empty-exit successor advances.
- Generated assembly no longer defines an empty reachable end label that falls
  through into the next linked section.
- Repairs use prepared/function control-flow facts and block reachability
  rather than matching `src/00143.c`, `.Lmain_block_2`, linked section names, or
  observed instruction addresses.

## Completion Note

Closed on 2026-06-23 after Step 4 reprobe showed the old empty successor
fallthrough/SIGILL was repaired. Focused empty loop-exit successor
codegen/runtime coverage is positive.

`src/00143.c` was reprobed under
`build/rv64_c_testsuite_probe_latest/triage_322_step4/`. BIR dump,
prepared-BIR dump, RV64 emit, and clang link all returned 0. The emitted RV64
now defines `.Lmain_block_2`, the Duff's-device switch/fallthrough body blocks,
the verification loop blocks, and valid return blocks. The linked binary no
longer falls through into `_IO_stdin_used`; qemu exits normally with status 1
instead of trapping with `SIGILL`.

The remaining wrong-result is classified as
`loop_carried_pointer_postincrement_residual`: repeated Duff's-device loop
iterations rematerialize `%lv.from` and `%lv.to` from fixed array-base offsets
instead of advancing from current loop-carried pointer values. That residual is
outside this idea and is tracked by
`ideas/open/323_rv64_loop_carried_pointer_postincrement_publication.md`.

Close gate: existing accepted backend guard state passed with
`test_before.log` compared to itself using `--allow-non-decreasing-passed`:
281 passed, 1 failed before and after, with the existing
`backend_riscv_prepared_edge_publication` failure unchanged.

## Reviewer Reject Signals

- The implementation special-cases `src/00143.c`, `.Lmain_block_2`,
  `_IO_stdin_used`, or a fixed block order instead of repairing empty successor
  emission.
- Progress is claimed by changing or weakening the i16 halfword focused tests
  from idea 321 rather than emitting a valid exit path.
- The route reopens stack-homed fused compare or halfword local-array
  publication even though current evidence shows those boundaries already
  advanced.
- The patch only hides qemu `SIGILL` by marking the candidate unsupported,
  skipping runtime proof, or adding an unreachable trap without a valid
  return/epilogue path for the reachable successor.
- Broad control-flow rewrites are mixed in without focused empty-exit
  successor proof.
