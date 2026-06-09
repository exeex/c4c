# 145 Current-block join fact and routing split

## Goal

Split reusable current-block join parallel-copy facts from target-facing
instruction routing convenience.

## Why This Exists

Idea 141 found that current-block join source facts are reusable prepared facts,
while `PreparedCurrentBlockJoinParallelCopyInstructionRouting` and related
booleans are AArch64-oriented routing convenience. Keeping both under the same
residual prepared lookup surface obscures owner boundaries.

## In Scope

- Separate current-block join source status naming from AArch64 instruction
  routing naming where possible.
- Move reusable join parallel-copy source facts toward
  `src/backend/prealloc/publication_plans.hpp/.cpp` and, where needed,
  `src/backend/prealloc/control_flow.hpp`.
- Localize or separately name target-facing instruction routing convenience in
  AArch64 dispatch code.
- Split value-home/out-of-SSA helper predicates into reusable fact predicates
  and routing use.

## Out Of Scope

- Moving AArch64 register spelling, scratch policy, hazard policy, or final
  instruction emission into shared prealloc code.
- Deleting reusable join-copy source facts.
- Reopening edge-publication facade splitting from idea 140 except for the
  explicit current-block join-copy residual surface.
- Folding unrelated return-chain or stack-source publication work into this
  split.

## Acceptance Criteria

- Reusable join-copy facts and target routing convenience no longer appear as
  one undifferentiated prepared-lookup API.
- RISC-V/x86 reusable fact needs are not regressed by AArch64 routing cleanup.
- Proof includes `cmake --build --preset default` and
  `ctest --test-dir build -R '^backend_' --output-on-failure`.
- Escalate to full CTest if reusable publication/control-flow facts change.

## Reviewer Reject Signals

- Reusable facts and target routing remain mixed under a new name.
- AArch64 routing booleans become shared prealloc policy.
- The route deletes facts that RISC-V/x86 could reuse.
- The proof only shows one dispatch case while nearby join-copy cases are
  unexamined.
