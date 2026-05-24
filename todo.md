Status: Active
Source Idea Path: ideas/open/shared-mir-same-block-producer-select-queries.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Rewire AArch64 Users To The Shared Query

# Current Packet

## Just Finished

Step 3: Rewire AArch64 Users To The Shared Query completed as a narrow code
slice. `dispatch_branch_fusion.cpp` no longer owns its duplicated
same-block binary-producer scan or recursive same-block integer-constant
evaluator.

Branch-fusion now calls `mir::find_same_block_binary_producer` and
`mir::evaluate_same_block_integer_constant` directly for the constant-RHS and
support-instruction decisions that only need target-neutral producer/constant
facts. AArch64-specific policy stayed local: condition suffix selection,
compare-immediate encodability, register spelling, value publication, frame-slot
load addressing, and assembler instruction construction remain in AArch64
codegen.

## Suggested Next

Continue Step 3 with one more narrow consumer migration if the supervisor wants
more AArch64 rewiring before review. The safest next candidate is an existing
direct `evaluate_same_block_integer_constant` wrapper consumer in value
materialization, while preserving the compatibility wrappers until all AArch64
call sites are settled.

## Watchouts

- `dispatch_branch_fusion.cpp` still has local cast/load producer helpers
  because those paths immediately cross into AArch64 emission and prepared
  frame-slot addressing policy.
- `dispatch_producers.cpp` remains the AArch64 compatibility surface for other
  consumers. Avoid deleting wrappers before call, publication, store, and
  branch-fusion users are migrated coherently.
- The shared traversal is predicate-based; keep target-specific questions such
  as direct-global-load materialization decisions outside `src/backend/mir`.

## Proof

Delegated proof command:

`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'`

Result: passed. `ctest` reported `155/155` backend tests passed, `0` failed.
Proof log: `test_after.log`.
