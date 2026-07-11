Status: Active
Source Idea Path: ideas/open/683_prepared_mir_view_contract_research.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Define Core And Optional View Contracts

# Current Packet

## Just Finished

Completed `plan.md` Step 2 by drafting
`docs/prepared_mir_view_contract_research/02_prepared_mir_core_view_shape.md`
and
`docs/prepared_mir_view_contract_research/03_feature_views_and_optional_contracts.md`.
The core-view research proposes read-only C++ accessors for target identity,
narrow BIR traversal, prepared names, function views, control flow, value
locations, stack layout, addressing, and view-owned lookups; it excludes the
rest of `PreparedBirModule` from core and states traversal and invariant
requirements. The feature-view research tables optional contracts for calls,
variadic entry, i128/f128 carriers, atomics, intrinsics, inline asm, and object
data with presence checks, fail-closed behavior, and target-local candidates.

## Suggested Next

Start `plan.md` Step 3 by drafting the proof/diagnostic boundary and
old/new-BIR equivalence documents:
`docs/prepared_mir_view_contract_research/04_debug_proof_and_diagnostic_boundaries.md`
and
`docs/prepared_mir_view_contract_research/05_old_bir_new_bir_equivalence_strategy.md`.

## Watchouts

- Step 2 intentionally chooses both read-only BIR traversal and a prepared
  instruction cursor. Step 3 should preserve that split when drawing the
  diagnostic/proof boundary.
- Optional feature absence is documented as fail-closed. Do not let Step 3
  promote diagnostic text, route names, completed phases, or verifier reports
  into codegen authority.
- This active idea remains research and architecture documentation only. Do not
  start idea 684 implementation cleanup from this runbook.

## Proof

No build or test proof is required for this documentation-only research packet.
Proof passed:
`git diff --check -- docs/prepared_mir_view_contract_research/02_prepared_mir_core_view_shape.md docs/prepared_mir_view_contract_research/03_feature_views_and_optional_contracts.md todo.md`.
No `test_after.log` was produced because the delegated proof is a direct
documentation diff check rather than a build or CTest command.
