Status: Active
Source Idea Path: ideas/open/592_typed_aggregate_branch_stack_source_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define The Producer Publication Contract

# Current Packet

## Just Finished

Completed `plan.md` Step 2 by defining the producer-publication contract for
branch stack-source freshness.

Added `PreparedBranchStackSourceFreshnessPublicationInputs` and
`publish_prepared_branch_stack_source_freshness_candidate` in shared prealloc.
The helper publishes only an explicit freshness candidate matching the branch
source value id/name, `BranchStackLoadSource`, `BranchStackSlot`,
`BranchTerminatorOrdering`, `BranchStackSlot` rank, the stack-slot home, and
the exact branch block plus terminator instruction point.

Wired `collect_prepared_branch_stack_load_authorities` through the helper so
the existing condition route still gets the same selected candidate, and the
pointer `Lhs` candidate now carries one explicit branch stack-slot freshness
candidate while remaining `policy=none`, `status=missing_policy`, and
`pointer_status=unknown`. This defines the producer contract without migrating
the consumer availability decision.

Focused coverage now checks that pointer `Lhs` can publish the exact freshness
tuple and that register structural facts, wrong-value stack facts, and missing
branch-point references cannot create branch stack-slot freshness authority.

## Suggested Next

Proceed to `plan.md` Step 3 by migrating the pointer
`PreparedBranchStackLoadRole::Lhs` consumer from inventory-only `policy=none`
to selected shared freshness authority, requiring the candidate published by
Step 2 plus pointer proof and clobber safety before it can become available.

## Watchouts

- Step 2 intentionally does not flip `Lhs` to `LoadFromStackSlot`; the row now
  exposes `source_freshness_candidates=1` but still fails closed at
  `MissingPolicy`.
- The current frame-slot source-fact collector treats pointer values and
  select-materialization results as unsupported boundaries; do not reuse those
  unsupported structural rows as Step 3 authority.
- Aggregate-adjacent authority currently exposes concrete source facts but
  reports missing aggregate copy authority; treat those facts as inputs to a
  future producer rule, not as direct branch freshness.
- Do not accept a branch stack source because a stack home, frame slot,
  aggregate lane, or clobber-safety fact exists.
- Do not use target-local branch emission inference as freshness authority.
- Do not weaken expectations, unsupported markers, allowlists, or runtime
  contracts as proof of capability progress.
- Keep broader branch, select, edge-publication, RV64, AArch64, and x86
  migrations out of the first packet.

## Proof

Ran the delegated proof command:

```bash
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1
```

Result: passed on rerun, with `test_after.log` reporting 346 passed, 0 failed.
The first attempt hit an external compile resource failure (`cc1plus` killed
while compiling an AArch64 test object); rerunning the exact command completed
successfully and overwrote `test_after.log` with the green proof.
