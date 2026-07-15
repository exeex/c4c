# Current Packet

Status: Active
Source Idea Path: ideas/open/801_bir_node_kind_tag_algebra_and_phase_vocabulary_lowering_contract.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Specify the single query authority

## Just Finished

- Completed `plan.md` Step 3 in the normative artifact's Section 11.
- Specified one hidden C++17 per-kind `KindSchema` inventory, preferably a
  constexpr registry with compile-time completeness proof; an internal
  X-macro may emit enum and registry only when required, but may not become a
  second inventory, TableGen-like DSL, or pass-visible mechanism.
- Required registry completeness and all six-axis relational validation to
  pass namespace-scope `static_assert` before traits are queryable. Runtime
  schema/descriptor views, payload admission, shape, effect, and stage facts
  are mechanical projections of the same entry, not hand-maintained switches.
- Defined generic and named compile-time/runtime helper surfaces, including
  tag, stage admission, stage-qualified SSA, value/effect/control, and MIR
  disposition queries. Unknown runtime enum/tag/stage values and illegal
  admission fail closed; unknown compile-time values are ill-formed.
- Selected only `Binary`, `Store`, `Phi`, and contract-only
  prepared/pseudo/machine categories for any later bounded feasibility proof.
  Step 4 remains pending and no transition rules were invented.

## Suggested Next

- Execute only Step 4: publish the exact B/C/D/E/F admitted vocabularies and
  complete B-to-C, C-to-D, D-to-E, and E-to-F transition matrix.

## Watchouts

- Do not revise or activate idea 732, reopen idea 746, or treat tag
  classification as proof of graph-stage SSA validity.
- Step 4 must enumerate transition outcomes explicitly; neither successful
  classification nor shared node storage authorizes catch-all retention.

## Proof

- Passed: `git diff --check > test_after.log 2>&1`.
- Proof log: `test_after.log`.
