# LIR Remaining Ordinary Value Identity Publication

Status: Open (inactive; prerequisite 746 active)
Type: producer-side ordinary value-authority decomposition
Blocked Consumer: ideas/open/734_lir_to_new_bir_container_completeness.md
Builds On: ideas/closed/741_lir_structured_operand_and_terminator_identity_decomposition.md

## Goal

Publish stable typed result and use identity for the remaining production
ordinary `LirInst` rows required by idea 734, extending only the existing
`LirOperand` alternatives, `LirValueId`, typed-immediate, and `LinkNameId`
model, with reachable ownership/type verification and no reconstruction from
display text.

## Why This Exists

Idea 734 accepted direct zero-argument void `Call` receipt in commit
`49ed1b386`, with a full 3033/3033 monotonic regression proof. The next
smallest call rows expose a producer-side identity family rather than a missing
receiver node:

- `StmtEmitter::emit_call_with_result` allocates result spelling through
  `fresh_tmp(ctx)` instead of allocating an owning `LirValueId` through
  `fresh_value(ctx)`
- `lir_call_structured_args` constructs argument `LirOperand` values from
  formatted string operands, so production scalar SSA call uses lack stable
  current-function identity
- the current `fresh_value` producer search finds authoritative ordinary value
  allocation only in the bounded selected-global load and GEP paths completed
  by closed idea 741

Most remaining result/use-bearing ordinary variants therefore collide with the
same missing authority family. Idea 734 forbids recovering those facts from
`%t*` names, formatted operands, printer output, or testcase identity, so its
receiver plan must pause while this separate producer initiative decomposes
and publishes the missing native identities.

Closed idea 741 remains complete. Its four store/load/GEP/return contracts are
the stable model and must not be reopened. Its exhaustive matrix explicitly
classified the other ordinary rows as raw or unclaimed compatibility; those
rows are the baseline inventory source for this initiative.

## Active-Plan Routing Note

The active runbook was switched to
`ideas/open/746_lir_unresolved_external_direct_call_signature_result_authority.md`
because Plan Step 7.32 found the plain-`DeclRef` unresolved-external scalar
call producer lacks both retained native `FnPtrSig` and a
`fresh_value(ctx)`-allocated result `LirValueId`. This idea remains open and
must not retry Step 7.32 until idea 746 hands off that bounded native-authority
contract with accepted proof.

## Authority Contract

- Every ordinary produced result is allocated by the owning `LirFunction` as a
  unique `LirValueId` before presentation is rendered.
- Every ordinary SSA use refers to an existing result ID owned by the same
  function; native immediates and module link identities keep their existing
  typed alternatives.
- `LirOperand` remains the single result/use carrier. No parallel value table,
  name-binding map, or testcase-specific identity field is permitted.
- Display spelling may observe a native identity but may never create, repair,
  or select one.
- Closed idea-741 contracts remain unchanged and provide neighboring
  compatibility proof for the generic mechanism.

## In Scope

- establish a checked baseline for every remaining production ordinary
  `LirInst` result/use row named raw or unclaimed by the closed idea-741 matrix
- record for each row its producer, current carrier alternative, missing native
  fact, focused probe, verifier obligation, dependency, and disposition
- extract focused one-contract probes under `tests/backend/case/` before broad
  producer changes
- bind each probe to one minimal generic producer/carrier/verifier seam
- change ordinary HIR-to-LIR producers to allocate results with the owning
  function's `fresh_value(ctx)` path and propagate existing `LirValueId`
  operands through structured operations
- preserve native `LirIntegerImmediate` and `LinkNameId` uses where they are
  already the exact semantic authority
- extend reachable LIR verification for invalid, duplicate, missing,
  use-before-definition where prohibited, unknown, cross-function, and
  alternative/type-conflicting identities
- implement bounded generic producer packets after the baseline and probes
  prove which ordinary rows share one carrier contract
- classify every remaining ordinary result/use row as completed by a proven
  generic seam, dependent on another bounded ordinary seam, or outside this
  initiative with an exact reason
- publish a checked handoff naming the exact new idea-734 receiver rows
  unblocked and every row that remains fail-closed

## Required Seams And Focused Probes

1. Direct zero-argument scalar-result call result identity:
   `tests/backend/case/lir_direct_scalar_result_call_identity.c` proves a
   native call-result `LirValueId` independent of its display spelling.
2. Direct void scalar immediate argument authority:
   `tests/backend/case/lir_direct_void_immediate_arg_identity.c` proves the
   existing typed-immediate alternative reaches one structured call argument
   without formatted-text reconstruction.
3. Direct void scalar SSA argument authority:
   `tests/backend/case/lir_direct_void_ssa_arg_identity.c` sources an argument
   from the already-authoritative selected-global load contract and proves the
   same current-function `LirValueId` becomes the call use.
4. Representative scalar ordinary result/use chaining:
   `tests/backend/case/lir_scalar_ordinary_value_chain_identity.c` proves the
   generic mechanism beyond calls, binds one producer result to a later
   ordinary use, and supports truthful classification of the remaining rows.

Probe names may be mechanically normalized to repository naming conventions,
but their one-contract boundaries must not be combined into a monolithic case.

## Progress Contract

Classification or schema edits alone are not capability progress. Progress
requires a focused probe bound to one ordinary producer seam, native identity
allocated before rendering, reachable verifier rejection of malformed
ownership, and neighboring proof that the same mechanism is generic rather
than named-case behavior.

Implementation starts only after the checked baseline and probe binding name
the exact producer and verifier obligations. Each later packet owns one
coherent generic result/use seam and its exact proof.

## Explicitly Separate Blocked Families

- Stack slots, allocas, local objects, lifetime ownership, and local-address
  bindings remain a separate family because their production allocation,
  result, and object ownership cross raw stack/local seams. Do not absorb them
  unless checked evidence proves the same ordinary carrier contract is both
  sufficient and production-complete.
- CFG and terminator target identity remains separate: branch/conditional/
  switch targets are raw labels and the structured indirect-branch ID form is
  producerless. Ordinary value publication does not authorize target recovery.
- Body parameter value identity remains separate until parameters have native
  source IDs; signature ordinals, names, and ABI positions are not identities.

## Out Of Scope

- any new-BIR opcode, node, container, builder, verifier, importer, or receiver
  implementation; those remain owned by idea 734
- reopening or weakening the four completed idea-741 store/load/GEP/return
  contracts
- parsing or matching result names, formatted operands, `args_str`,
  `value_str`, type strings, printer output, LLVM text, or testcase paths
- a parallel LIR value model, string-to-ID repair table, name-binding map, or
  testcase-specific carrier field
- ABI policy, target lowering, call ABI placement, argument expansion,
  variadic lowering, canonicalization, allocation, MIR, emission, or assembler
  work
- CFG/terminator target identity, indirect-branch production, stack/local
  object ownership, alloca/lifetime identity, or body parameter binding unless
  a separate lifecycle decision explicitly moves those families
- expectation downgrades, supported-to-unsupported changes, allowlists, or
  named-case producer branches
- closing or superseding idea 734

## Acceptance Criteria

- A checked remaining-row matrix accounts for every production ordinary
  `LirInst` result/use row outside the four closed idea-741 contracts, with no
  catch-all or omitted row.
- Each required focused probe exists as one primary contract and is bound to an
  exact generic producer/carrier/verifier obligation before implementation.
- Direct scalar-result calls publish an owning result `LirValueId`; direct void
  immediate calls preserve native immediate authority; direct void SSA calls
  preserve the exact already-authoritative load result ID.
- A representative non-call scalar chain proves generic result allocation and
  use propagation beyond call-specific code.
- Every implemented ordinary seam rejects invalid, duplicate, missing,
  unknown, cross-function, or conflicting authority through reachable LIR
  verification.
- Closed idea-741 contracts remain green and unchanged; no parallel value or
  symbol model is introduced.
- Stack/local-object, CFG/terminator target, and body-parameter identity remain
  explicitly separate blocked families unless evidence and lifecycle scope
  deliberately move one.
- A checked handoff names exact idea-734 rows unblocked, their native facts and
  verifier guarantees, and all remaining fail-closed rows.
- Fresh build, focused producer/verifier proof, relevant backend boundary
  proof, and the supervisor-selected full regression checkpoint pass before
  closure and return to idea 734.

## Reviewer Reject Signals

- Reject parsing `%t*` spellings, formatted call arguments, `args_str`,
  `value_str`, type text, printer output, or testcase names to invent result or
  use identity.
- Reject replacing `fresh_tmp(ctx)` with a renamed string allocator while
  leaving the produced `LirOperand` without an owning `LirValueId`.
- Reject a call-only side table, result-name lookup, parallel value graph, or
  testcase-specific field instead of extending the existing generic carrier.
- Reject schema or helper changes made before the remaining-row matrix and a
  focused probe bind them to one generic producer/verifier contract.
- Reject a named-case fix that passes only one probe while neighboring calls or
  scalar chains remain text-only.
- Reject weakening, reopening, or claiming again the completed idea-741 four
  contracts; they are stable prerequisites and regression neighbors.
- Reject new-BIR receipt, target/ABI lowering, argument placement,
  canonicalization, allocation, MIR, emission, or assembler work in this idea.
- Reject silently absorbing raw CFG/terminator targets, stack/local-object
  ownership, alloca/lifetime identity, or body parameters into the ordinary
  value initiative without a separate evidence-backed lifecycle decision.
- Reject expectation downgrades, supported-to-unsupported changes, allowlists,
  malformed-input weakening, or classification-only documents claimed as
  producer capability.
- Reject closure based on the four named probes alone if the remaining ordinary
  row matrix still has unclassified production result/use authority gaps.
