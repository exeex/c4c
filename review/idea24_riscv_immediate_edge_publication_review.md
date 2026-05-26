# Idea 24 RISC-V Immediate Edge Publication Review

## Scope

- Active source idea: `ideas/open/24_riscv_prepared_edge_publication_register_destination_consumer.md`
- Review base: `34990333a` (`Record RISC-V edge publication validation review`)
- Commits reviewed: 2 (`44a1f20aa..6daec8d54`)
- Focus commit: `6daec8d54` (`Support RISC-V immediate edge publications`)

`34990333a` is the right checkpoint because it is the previous same-idea
review/validation checkpoint after the first RISC-V `Register -> Register`
consumer landed. The reviewed range covers the Step 4 decision to fold in
`RematerializableImmediate -> Register` plus the immediate-source
implementation and current `todo.md` proof metadata.

## Findings

No blocking findings.

## Review Notes

- Source-idea alignment: matches source idea. The slice stays inside
  register-destination edge-publication consumption and extends a source-home
  family explicitly listed as in scope by the source idea.
- Shared authority: acceptable. `consume_edge_publication_move_intent(...)`
  still obtains the publication through
  `prepare::find_unique_indexed_prepared_edge_publication(...)` before source
  rendering, and the new immediate handling only renders a target-local source
  operand after shared lookup succeeds
  (`src/backend/mir/riscv/codegen/emit.cpp:92`,
  `src/backend/mir/riscv/codegen/emit.cpp:119`).
- Target-local emission: acceptable. RISC-V keeps instruction spelling local:
  register sources emit `mv`, immediate sources emit `li`
  (`src/backend/mir/riscv/codegen/emit.cpp:132`).
- Fail-closed behavior: preserved. Unsupported or malformed source homes still
  return `UnsupportedSourceHome`, and non-register destinations still return
  `UnsupportedDestinationHome`
  (`src/backend/mir/riscv/codegen/emit.cpp:115`,
  `src/backend/mir/riscv/codegen/emit.cpp:125`).
- Testcase-overfit risk: not observed. The tests mutate semantic prepared home
  kinds rather than matching fixture names, edge labels, or value ids as
  special cases. They also clear the shared lookup index to prove the helper
  does not rediscover edge moves locally
  (`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:195`).
- Coverage quality: sufficient for the immediate packet. Positive coverage
  checks emitted `li a1, 42`, shared publication identity, source-home kind,
  value ids, immediate storage, and final instruction text
  (`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:168`).
  Negative coverage preserves pointer-base, malformed immediate, stack source,
  stack destination, missing lookup, missing publication, and non-move
  fail-closed paths
  (`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:225`).

## Validation

- Focused proof recorded in `todo.md`: build plus selected RISC-V/shared
  backend subset passed 5/5.
- Matching focused regression guard recorded in `todo.md`: passed with 5/5
  before and 5/5 after, no new failures.
- Broader backend validation recorded in `todo.md` and visible in
  `test_after.log`: `ctest -R '^backend_'` passed 163/163. This is broader
  validation, not the matching before/after regression comparison.

Validation is sufficient to proceed toward Step 6 handoff or closure, provided
the handoff does not overclaim unsupported homes.

## Judgments

- Idea alignment: matches source idea.
- Runbook transcription: plan matches idea.
- Route alignment: on track.
- Technical debt: acceptable.
- Validation sufficiency: narrow proof sufficient, with broader backend proof
  already available.
- Recommendation: continue current route.

## Handoff Caveat

Step 6 should state the supported RISC-V register-destination homes precisely:
`Register -> Register` and `RematerializableImmediate -> Register`. It should
continue to name `StackSlot -> Register`, `PointerBasePlusOffset -> Register`,
and source-to-`StackSlot` destinations as unsupported/fail-closed unless a
separate lifecycle packet brings their RISC-V emission policy into scope.
