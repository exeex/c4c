Status: Active
Source Idea Path: ideas/open/155_bir_block_entry_publication_identity.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Semantic Publication Identity Boundary

# Current Packet

## Just Finished

Completed Step 4 for `plan.md`: tightened test-only proof for the semantic
publication identity boundary without switching consumers or touching
implementation files.

`backend_prealloc_block_entry_publications_test.cpp` now compares prepared/BIR
block-entry publication equivalence only for the shared available PHI
destination identity fields: successor identity, destination value id/name/type,
destination value pointer identity, PHI instruction pointer, and instruction
index. It also proves divergent readiness boundaries: prepared home/register
fallback readiness can be available while BIR has no PHI-entry identity, and
BIR PHI-entry identity can be available while prepared emission readiness is
missing or unavailable because prepared move/storage facts do not support
emission.

`backend_prepared_lookup_helper_test.cpp` now carries matching boundary cases
for prepared move/register readiness without a PHI, BIR PHI identity without a
prepared publication move, and BIR PHI identity for a prepared stack-destination
publication that remains prepared-unavailable.

`backend_prepare_frame_stack_call_contract_test.cpp` now checks the BIR
block-entry semantic fields more explicitly and proves that a BIR PHI-entry
destination identity does not imply prepared entry-publication emission
readiness in the call-contract context.

## Suggested Next

Step 5 candidate: inspect the remaining prepared/AArch64 consumers that still
own register parsing, destination storage decisions, and scalar emission policy.
If the plan still calls for a consumer switch, split it into a narrow packet
that names exactly one consumer boundary and keeps BIR identity data limited to
semantic source/destination identity.

## Watchouts

- BIR entry-publication availability is intentionally PHI-entry semantic
  availability. Exact prepared availability from move/value-home/register facts
  remains outside BIR and must stay prepared-owned.
- Step 4 proof intentionally does not compare prepared hook kind, destination
  home, storage encoding, stack-source policy, register view, immediate/move
  payload, emitted-storage availability, destination register spelling, or
  scalar emission policy as BIR facts.
- Treat prepared-only readiness positives as oracle/fallback behavior and BIR
  PHI-entry positives as semantic identity only, not emission readiness.
- Leave register parsing plus destination storage decisions in existing
  prepared/AArch64 consumers until a later explicit switch packet.
- Escalate validation if any future packet touches dispatch publication, calls,
  scalar publication planning, value homes, or emission behavior.

## Proof

Supervisor-selected proof passed and wrote `test_after.log`:

```sh
cmake --build build --target backend_prepared_lookup_helper_test backend_prepare_frame_stack_call_contract_test backend_prealloc_block_entry_publications_test && ctest --test-dir build --output-on-failure -R 'backend_(prepared_lookup_helper|prepare_frame_stack_call_contract|prealloc_block_entry_publications)' > test_after.log 2>&1
```

`test_after.log`: 3/3 targeted tests passed.
