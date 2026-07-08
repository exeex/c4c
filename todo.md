Status: Active
Source Idea Path: ideas/open/592_typed_aggregate_branch_stack_source_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Typed And Aggregate Branch Stack-Source Producers

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from
`ideas/open/592_typed_aggregate_branch_stack_source_publication.md`. No
implementation packet has completed yet.

## Suggested Next

Start `plan.md` Step 1 by auditing typed and aggregate branch stack-source
producer paths around `PreparedBranchStackLoadRole::{Condition,Lhs,Rhs}` and
identifying the narrowest pointer `Lhs` or `Rhs` consumer candidate.

## Watchouts

- Do not accept a branch stack source because a stack home, frame slot,
  aggregate lane, or clobber-safety fact exists.
- Do not use target-local branch emission inference as freshness authority.
- Do not weaken expectations, unsupported markers, allowlists, or runtime
  contracts as proof of capability progress.
- Keep broader branch, select, edge-publication, RV64, AArch64, and x86
  migrations out of the first packet.

## Proof

No proof run for lifecycle-only activation. First code-changing executor packet
should use the supervisor-selected backend proof, normally:

```bash
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R '^backend_'
```
