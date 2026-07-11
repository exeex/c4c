Status: Active
Source Idea Path: ideas/open/683_prepared_mir_view_contract_research.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Trace Current MIR Dependencies

# Current Packet

## Just Finished

Completed `plan.md` Step 1 by drafting
`docs/prepared_mir_view_contract_research/01_current_mir_dependencies_on_prepared_bir.md`.
The research traces the current x86 prepared-module entry path from
`backend.cpp` through `x86::api::emit_prepared_module()` into
`x86::module::emit()`, inventories live MIR `PreparedBirModule` field-family
dependencies, separates live compiled consumers from markdown-only mirrors,
classifies dependencies, and states the smallest first `PreparedMirView`
dependency set.

## Suggested Next

Start `plan.md` Step 2 by drafting the core and optional view contract
documents:
`docs/prepared_mir_view_contract_research/02_prepared_mir_core_view_shape.md`
and
`docs/prepared_mir_view_contract_research/03_feature_views_and_optional_contracts.md`.

## Watchouts

- The first-view set in the Step 1 doc is intentionally x86-first and read-only;
  RV64/AArch64 dependencies should become optional feature views rather than
  broadening the core back into `PreparedBirModule`.
- Keep diagnostic route summaries and prepared dump filtering observational;
  they should not become semantic MIR authority.
- This active idea remains research and architecture documentation only. Do not
  start idea 684 implementation cleanup from this runbook.

## Proof

No build or test proof is required for this documentation-only research packet.
Proof passed:
`git diff --check -- docs/prepared_mir_view_contract_research/01_current_mir_dependencies_on_prepared_bir.md todo.md`.
No `test_after.log` was produced because the delegated proof is a direct
documentation diff check rather than a build or CTest command.
