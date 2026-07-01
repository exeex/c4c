Status: Active
Source Idea Path: ideas/open/423_rv64_runtime_mismatch_triage.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Group Recurring Owners

# Current Packet

## Just Finished

Completed `plan.md` Step 4 by grouping the Step 1-3 runtime mismatch evidence
by first wrong edge and owning layer in
`docs/rv64_gcc_torture_post_contract/runtime_mismatch_groups.md`. The grouped
representatives are `src/pr81503.c` for binary operand/value materialization
after prepared BIR, `src/20080506-2.c` for frame-slot call-argument
publication/materialization, and `src/pr38533.c` for inline asm
tied-output/result materialization. The note keeps singleton and unclear-row
limits explicit and does not claim whole-bucket ownership beyond the evidence.

## Suggested Next

Execute Step 5 by creating follow-up ideas for the two ordinary-C-ready owner
groups, or parking unresolved rows if the supervisor wants lifecycle state to
remain triage-only. Recommended first idea: RV64 binary operand/value
materialization from prepared BIR, starting with `src/pr81503.c`; recommended
second idea: frame-slot call-argument publication/materialization, starting
with `src/20080506-2.c`.

## Watchouts

- This is a triage runbook, not an expectation or allowlist route.
- Do not weaken runtime comparison, expected output, unsupported markers, or
  pass/fail accounting.
- Do not assign the remaining 31 abort rows or 6 segfault rows to these groups
  based only on runtime exit mode.
- `src/20000314-2.c` should remain stale/nondeterministic for this triage until
  a later scan reproduces the original abort again.
- Inline asm tied-output materialization is clear for `src/pr38533.c`, but it
  is lower ordinary-C priority than the operand/value and frame-slot groups.

## Proof

Passed:
`git diff --check -- docs/rv64_gcc_torture_post_contract/runtime_mismatch_groups.md todo.md`.
