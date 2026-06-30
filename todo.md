Status: Active
Source Idea Path: ideas/open/421_rv64_instruction_fragment_bucket_classification.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Instruction Fragments By First Owner

# Current Packet

## Just Finished

Step 2 completed the first-owner classification for
`unsupported_instruction_fragment`. The packet regenerated
`build/agent_state/unsupported_instruction_fragment_rows.tsv` from the single
fresh scan
`build/agent_state/rv64_gcc_torture_backend_current_20260630T032216Z.log`
(`total=1467 passed=404 failed=1063`), classified its 190 rows in
`docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`, and recorded
disjoint owner buckets with counts, representative rows, prepared-BIR evidence,
and owning-layer notes.

## Suggested Next

Execute Step 3 from `plan.md`: rank high-impact ordinary non-F128 follow-ups in
`docs/rv64_gcc_torture_post_contract/followup_idea_plan.md`, starting from the
largest ordinary buckets: select/join materialization (54), call-adjacent
scalar and inline-asm fragments (38), pointer/address materialization (21),
aggregate ABI call-storage (19), and integer div/rem (17). Keep F128
quarantined unless it blocks a broader non-F128 owner.

## Watchouts

- Do not implement RV64 lowering in this classification runbook.
- Do not make F128 the primary route.
- Do not weaken expectations, allowlists, unsupported markers, runtime
  comparison, or pass/fail accounting.
- Do not classify missing BIR/prepared producer facts as RV64 lowering work.
- The fresh scan no longer has older `udiv` representatives such as
  `src/20000402-1.c` in this bucket; classify and rank from the 190-row
  regenerated TSV, not older handoff fragments.
- Aggregate ABI rows with suspicious prepared size/alignment facts and pointer
  rows with incomplete provenance need producer-owner review before they become
  RV64 implementation ideas.

## Proof

Passed: `git diff --check > test_after.log 2>&1`.

Proof log: `test_after.log`.
