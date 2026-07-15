# Current Packet

Status: Active
Source Idea Path: ideas/open/801_bir_node_kind_tag_algebra_and_phase_vocabulary_lowering_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define the closed tag algebra and SSA boundary

## Just Finished

- Completed `plan.md` Step 2 in
  `docs/backend/bir_node_kind_tag_algebra_and_phase_vocabulary.md`.
- Defined one validated `KindSchema` composed of six finite axes: value/SSA,
  semantic family, effects/control, stage vocabulary, operand/result/type/
  payload policy, and MIR realizability. Each axis now names its exclusive
  groups, allowed composable refinements, required combinations, invalid
  combinations, and extension-review rule; derived facts replace duplicate
  free booleans.
- Defined `SsaEligible` only as static participation when admitted to an
  SSA-governed vocabulary. B4 separately establishes and verifies definitions,
  uses, dominance, phi edges, and Canonical graph publication. A shared
  Raw/Canonical kind whose static meaning actually changes must use a
  stage-qualified query or split kinds; no timeless helper may make a false
  claim.
- Added bounded algebra illustrations for existing `Binary`, `Store`, and
  `Phi`, plus contract-only prepared, pseudo, and machine categories. They do
  not enumerate future production kinds. Step 3 query mechanics and Step 4
  transition rows remain explicit pending sections.

## Suggested Next

- Execute only Step 3: specify the hidden single schema authority and its
  stable compile-time/runtime query surface over the accepted algebra.

## Watchouts

- Do not revise or activate idea 732, reopen idea 746, or treat tag
  classification as proof of graph-stage SSA validity.
- Do not turn the closed refinements into independent booleans. Step 3 must
  derive views from a validated axis record, and Step 4 must decide actual
  cross-stage retention rather than infer it from shared storage.

## Proof

- Passed: `git diff --check > test_after.log 2>&1`.
- Proof log: `test_after.log`.
