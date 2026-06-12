Status: Active
Source Idea Path: ideas/open/218_phase_e1_semantic_duplicate_candidate_triage.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify the Six Candidate Families

# Current Packet

## Just Finished

Completed Step 2 of `plan.md`: classified all six E0 E1 candidate families in
`docs/bir_prealloc_fusion/phase_e1_semantic_duplicate_candidate_triage.md`.

The durable table now records, for each family, the exact helper/API surface,
semantic owner, retained prepared surface, public consumers, proof shape, and
readiness bucket.

Assigned readiness buckets:

- aggregate BIR semantic forwarding: blocked because the surface is
  target/prepared policy, fallback/oracle, or aggregate compatibility.
- control-flow identity helpers: ready to draft one implementation idea.
- route source/publication/join/call/comparison identity helpers: ready to
  draft one implementation idea.
- liveness identity helpers: needs more analysis before implementation.
- intrinsic metadata adapters: needs more analysis before implementation.
- row-scoped diagnostic/oracle helpers: defer to E3 for diagnostic/oracle
  replacement.

## Suggested Next

Execute Step 3: draft accepted follow-up ideas only for the families classified
as `ready to draft one implementation idea`, keeping each idea scoped to one
semantic duplicate helper family or one selected reader. Do not draft
aggregate, liveness, intrinsic, or row-scoped diagnostic/oracle follow-ups from
this Step 2 packet alone.

## Watchouts

- This active idea is analysis-only.
- Do not edit implementation files during Phase E1 triage.
- Do not treat E0 candidate categories as implementation permission.
- Do not open broad prepared retirement, aggregate `PreparedBirModule` /
  `PreparedFunctionLookups` retirement, or draft 155 / E5 work.
- Keep follow-up ideas scoped to one semantic duplicate helper family or one
  row-scoped semantic/oracle surface.
- Preserve target/prepared policy, fallback/oracle, diagnostics, helper-oracle
  names, supported-path status, baselines, and expected strings.
- Step 2 classified row-scoped diagnostic/oracle helpers as E3-owned, not as
  immediate E1 implementation ideas, unless a row is only the proof harness for
  a named E1 semantic reader.
- Step 3 should not draft aggregate `PreparedBirModule` work, liveness work, or
  intrinsic metadata work without another analysis packet narrowing one exact
  implementation-ready consumer.

## Proof

Docs-only proof selected by supervisor; no build was required and no
`test_after.log` was written for this packet because the proof commands are
document content checks and the packet forbids touching canonical log files.

Commands:

```sh
rg -n "ready to draft one implementation idea|needs more analysis before implementation|defer to E2|defer to E3|defer to E4|blocked because" docs/bir_prealloc_fusion/phase_e1_semantic_duplicate_candidate_triage.md
rg -n "aggregate BIR semantic forwarding|control-flow identity helpers|route source/publication/join/call/comparison identity helpers|liveness identity helpers|intrinsic metadata adapters|row-scoped diagnostic/oracle helpers" docs/bir_prealloc_fusion/phase_e1_semantic_duplicate_candidate_triage.md
```

Result: passed. The commands found all required readiness-bucket strings and
all six required candidate-family names in the durable E1 document.
