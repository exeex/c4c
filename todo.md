Status: Active
Source Idea Path: ideas/open/231_aarch64_call_frame_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Validate And Summarize

# Current Packet

## Just Finished

Completed Step 6, Validate And Summarize, by rerunning the broader backend
validation proof and preserving the current AArch64 call/frame route status in
canonical execution state.

Current route status:

- Direct fixed-arity calls, register-indirect calls, simple fixed frames,
  selected call-boundary register moves, prepared clobber spellings, explicit
  frame-slot memory-return storage, and explicit callee-saved-register
  preservation have structured selected-record coverage and printer support for
  the currently complete prepared facts.
- Missing prepared call facts fail closed through `MissingPreparedCallPlan`;
  target lowering should not restore any BIR-only fallback ABI reconstruction.
- Simple fixed-frame output remains intentionally limited to complete
  no-save/no-dynamic records.

## Suggested Next

Supervisor decision needed: decide whether
`ideas/open/231_aarch64_call_frame_machine_nodes.md` is complete under the
currently implemented narrower prepared-fact boundary, blocked, or should
remain open pending reintegration after the split prepared-authority initiatives
for callee-save placement and stack-slot preserved-value extent.

## Watchouts

- Do not reconstruct call ABI classification, frame layout, outgoing stack
  areas, callee-save slots, sret storage, scratch policy, or relocation/object
  behavior inside AArch64 codegen.
- Callee-save save/restore lowering remains blocked on explicit saved-register
  to frame-slot and offset prepared authority; follow-up lives in
  `ideas/open/241_prepared_callee_save_slot_placement.md`.
- Stack-slot preserved-value structured memory effects remain blocked on
  prepared size/alignment authority; follow-up lives in
  `ideas/open/242_prepared_stack_slot_preserved_value_extent.md`.
- Dynamic-stack frames, explicit zero-frame records, full variadic function
  entry `va_list` behavior, stack/FPR/grouped/cycle-temp call-boundary moves,
  stack results, non-register indirect callees requiring scratch selection, and
  malformed prepared clobber diagnostics remain unsupported in this route.
- Memory-return structured effects are limited to prepared frame-slot storage
  with explicit slot id and prepared stack offset; other encodings remain raw
  provenance and fail closed.
- Reject expectation downgrades, named-case shortcuts, text-only printer
  patches, or local inference from `save_index`, `frame_slot_order`, retained
  BIR, or incomplete prepared carriers as progress.

## Proof

Proof log: `test_after.log`.

Command run exactly:

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: green. The build completed and CTest reported `100% tests passed, 0
tests failed out of 139` for the broader `^backend_` subset.
