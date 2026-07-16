# LIR Remaining Authority Owner Triage

Status: Closed - intentionally concluded with ordered successors
Type: Umbrella triage and follow-up idea generator
Parent: `ideas/open/734_lir_to_new_bir_container_completeness.md`
Predecessor: `ideas/closed/865_lir_next_non_body_parameter_authority_handoff.md`
Handoff Directory: `docs/lir_remaining_authority_owner_triage/`
Related:
- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `ideas/closed/865_lir_next_non_body_parameter_authority_handoff.md`
- `tests/backend/bir/backend_lir_to_bir_interface_test.cpp`

## Goal

Use the accepted 734 receiver history through commit `750b6b3ba`, the rejected
865 `LirAbsOp` selection, and the current open authority inventory to classify
the remaining current-LIR semantic families by first owning layer and generate
ordered follow-up ideas.

## Why This Exists

Idea 865 was created to publish one next non-body-parameter producer handoff,
but its selected row was already received by `0c44e810ad` and no replacement
bounded row was found inside that one-row scope. Direct implementation now
risks reopening accepted receiver work or mixing producer, verifier, Raw-BIR,
and residual-family ownership. A triage umbrella is needed to identify the
next true first owner before another implementation route is activated.

## Current Evidence

- Idea 734 accepted Step 7.51 in commit `750b6b3ba`.
- Idea 865 rejected scalar integer `LirAbsOp` selected-global/i32 as already
  received by commit `0c44e810ad`.
- `todo.md` for the concluded 865 route records rejected accepted rows:
  fixed direct-call argument 0/1 parameter rows, body parameters, floating
  call-result rows, local-object rows, and VLA stack-save/restore rows.
- Remaining broad families named by the 865 blocker include CFG/PHI,
  memory/VA, aggregate/vector, module/type/global/metadata, residual
  instruction/terminator, inline-assembly, and generic residual sweeps.
- Open historical ideas such as 795 and 796 may contain useful evidence, but
  their older return points must not be treated as current 734 post-7.51
  execution authority without reconciliation.

## In Scope

- Refresh or create handoff docs under
  `docs/lir_remaining_authority_owner_triage/`.
- Classify remaining families by first owning layer: LIR producer/schema,
  verifier, Raw-BIR receiver, importer, documentation, or broader policy.
- Reconcile stale open ideas whose return records name other parents before
  considering them current 734 successors.
- Generate ordered follow-up ideas under `ideas/open/` for the next executable
  first-owner routes.
- Record dependency order and the exact 734 return condition for each generated
  follow-up.

## Out Of Scope

- Implementation changes, tests, expectations, unsupported markers, allowlists,
  or runtime behavior.
- Raw-BIR receiver work for 734.
- Publishing producer/schema/verifier authority directly inside this umbrella.
- Mixing producer repair and target consumer receipt in one follow-up idea.
- Reopening rows already accepted by 734, including the `LirAbsOp`
  selected-global/i32 receipt from `0c44e810ad`.

## Priority Model

Prefer the earliest first-owner blocker that can unlock a single future 734
receiver row without presentation recovery. Order by source completion impact,
first owning layer clarity, and availability of native structured facts rather
than named testcase pressure or stale open-idea order.

## Required Follow-Up Ideas

Fresh triage must either create or explicitly reject follow-up ideas for these
families:

- CFG/PHI residual authority not already accepted.
- Memory/VA and object/lifetime authority.
- Aggregate/vector value and type authority.
- Module/type/global/metadata semantic authority.
- Residual instruction/terminator authority.
- Inline-assembly authority that preserves templates and constraints as
  opaque unless a separate policy explicitly owns parsing.
- Any stale open idea whose scope overlaps a current post-7.51 734 blocker.

## Acceptance Criteria

- The handoff directory contains current evidence, classification, and
  follow-up ordering documents.
- The documents agree that 865 produced no handoff and that `LirAbsOp`
  selected-global/i32 is already received by `0c44e810ad`.
- Follow-up ideas are generated under `ideas/open/` or explicitly rejected
  with evidence.
- Each follow-up idea names its first owning layer, exact non-goals, 734 return
  condition, and reject signals.
- No implementation, test, expectation, unsupported-marker, allowlist, runtime,
  Raw-BIR receiver, or default harness contract changes are made.

## Closure Note Requirements

The closure note must state which evidence was used, which docs were written,
which follow-up ideas were generated or rejected, why their order is valid, and
what remains intentionally deferred.

## Reviewer Reject Signals

- Reject direct implementation inside this umbrella.
- Reject a result that only lists families without first-owner classification
  and ordered follow-up ideas.
- Reject stale open idea return records as current authority unless reconciled
  against post-Step-7.51 734 state.
- Reject follow-up ideas that mix LIR producer/schema/verifier ownership with
  Raw-BIR receiver work.
- Reject testcase-shaped shortcuts, expectation rewrites, unsupported
  downgrades, allowlist filtering, or weaker runtime checks as progress.
- Reject reopening `LirAbsOp` selected-global/i32, direct-call argument 0/1,
  body-parameter, local-object, VLA, or accepted call-result receiver rows.

## Closure Note

Close accepted as lifecycle-only/documentation-only triage. The umbrella did
not implement capability work and did not produce a direct 734 receiver
handoff.

Evidence used:

- `docs/lir_remaining_authority_owner_triage/current_evidence.md`
- `docs/lir_remaining_authority_owner_triage/classification.md`
- `docs/lir_remaining_authority_owner_triage/ordering_and_closure.md`

Generated or reused successors, in dependency order:

1. `ideas/open/841_lir_compact_scalar_abi_leaf_migration.md`
2. `ideas/open/842_lir_restricted_first_class_value_unions.md`
3. `ideas/open/843_lir_direct_hir_family_construction_array_composition.md`
4. `ideas/open/844_lir_global_extern_initializer_family_facts.md`
5. `ideas/open/867_lir_memory_va_object_lifetime_authority.md`
6. `ideas/open/848_lir_global_policy_identity_evidence.md`
7. `ideas/open/849_lir_intrinsic_binding_evidence.md`
8. `ideas/open/850_lir_cfg_phi_raw_bindings_evidence.md`
9. `ideas/open/845_lir_typed_reference_carriers_collector_migration.md`
10. `ideas/open/846_lir_family_overloaded_verifier_dispatch_printer.md`
11. `ideas/open/847_lir_universal_model_string_escape_hatch_deletion.md`
12. `ideas/open/797_lir_to_new_bir_final_coverage_convergence.md`

Rejected current successors: direct Raw-BIR receiver work for 734, generic
residual sweeps, stale open idea return records that do not reconcile to
post-Step-7.51 734, and `LirAbsOp` selected-global/i32 because commit
`0c44e810ad` already receives it.

Deferred work: 734 remains paused after `750b6b3ba` until one listed
first-owner successor accepts an exact typed handoff. Raw-BIR receiver work
must consume one handoff at a time and must not precede producer/schema,
verifier, or documentation evidence where those are the classified first
owners.
