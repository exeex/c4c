# 245 Phase F1 riscv Route 5/Route 3 edge-publication adapter

## Goal

Introduce a riscv edge-publication adapter that compares Route 5 edge/source
identity and Route 3 memory-source identity facts against the current prepared
publication/source-memory authority while preserving target-local wrapper
policy, exact instruction text, statuses, fallback, and fail-closed behavior.

## Why This Exists

Phase F0 found that riscv currently consumes prepared edge-publication shape
directly. Route 5 is the candidate owner for edge/source identity and Route 3
is the candidate owner for memory/source identity, but riscv register names,
stack offsets, signed-12-bit decisions, scratch registers, load/address
spelling, and wrapper output remain target policy.

## In Scope

- One riscv adapter boundary for prepared edge publication consumption.
- Route 5 comparison for edge/source identity where the source is a scalar or
  stack/register publication.
- Route 3 comparison for memory-source identity where the prepared publication
  requires source-memory access.
- Prepared fallback for missing route facts, missing prepared publications,
  unsupported publications, unsupported source homes, unsupported destination
  homes, non-agreement, large offsets, dynamic stack, aggregate width, subword,
  floating-point, and pointer-base policy-sensitive cases.
- Preservation of `EdgePublicationMoveIntentStatus` names and exact riscv
  wrapper instruction text.

## Out Of Scope

- Whole `edge_publications`, `edge_publication_source_producers`,
  `memory_accesses`, `PreparedFunctionLookups`, or `PreparedBirModule`
  deletion, privatization, or replacement.
- Moving riscv ABI, register allocation, stack layout, scratch-register
  choice, signed-offset decisions, load/store/address instruction spelling,
  formatting, or emission policy into BIR.
- Renaming `EdgePublicationMoveIntentStatus` or prepared publication/source
  status strings.
- Expectation weakening, unsupported downgrades, wrapper-output relabeling,
  timeout masking, or baseline refreshes as proof.

## Acceptance Criteria

- The selected riscv adapter consumes Route 5 and Route 3 semantic facts only
  when they agree with current prepared publication/source-memory answers.
- Positive behavior preserves exact instruction text for existing accepted
  cases such as register moves, immediates, stack loads, large stack address
  materialization, pointer-base address formation, and same-register moves.
- Negative and policy-sensitive cases keep the current fail-closed behavior,
  including no accidental unsupported subword, unsigned, F32, dynamic stack,
  aggregate-width, non-move, pointer-base-as-address, or large-offset load
  emission.
- `EdgePublicationMoveIntentStatus` and prepared publication/source-memory
  status strings remain compatibility authority until route-native diagnostics
  prove the same cases independently.
- No deletion or draft 155 readiness claim is made.

## Proof Requirements

- Targeted riscv prepared edge-publication tests covering positive move,
  immediate, stack-source, memory-source, pointer-base, and large-offset cases.
- Negative coverage for missing shared lookups, missing publication,
  unsupported publication, unsupported source home, unsupported destination
  home, unsupported subword/unsigned/F32, dynamic stack, aggregate-width,
  non-move, pointer-base-as-address, and large-offset fail-closed cases.
- Route 5/Route 3 diagnostic or helper-oracle proof for positive, missing,
  mismatch, duplicate/conflict, incomplete memory-source, and fallback cases.
- Matching before/after regression guard over riscv edge publication and
  prepared lookup helper tests.

## Reviewer Reject Signals

- The patch treats prepared edge-publication shape as route-native truth
  without agreement-gating or explicit compatibility fallback labeling.
- The implementation moves riscv register, stack, scratch, offset,
  instruction-spelling, formatting, wrapper, or emission policy into BIR.
- Existing status names, wrapper instruction text, fallback behavior,
  supported-path contracts, or baselines are rewritten instead of preserved.
- The route handles only one named fixture, one string, or one testcase shape
  instead of a semantic Route 5/Route 3 fact family.
- The change claims whole prepared lookup or aggregate retirement readiness.
