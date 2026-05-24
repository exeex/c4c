Status: Active
Source Idea Path: ideas/open/prealloc-store-source-publication-planning.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Cross-Target Reuse

# Current Packet

## Just Finished

Completed Step 4 cross-target reuse proof for the prealloc store-source
publication plan.

Added `backend_x86_store_source_publication_plan_reuse`, an x86-labeled backend
MIR C++ test that consumes `prepare::PreparedStoreSourcePublicationPlan` through
the public prealloc API only. The test models an x86-side consumer decision over
neutral plan facts and does not include AArch64 codegen headers, AArch64 memory
operands, or machine instruction records.

Covered neutral facts:
- source-home storage facts for stack-slot and register-backed store sources
- destination frame slot/object and destination stack offset facts
- recovered source value and recovered instruction index facts
- pointer-value destination and pointer-base stack-home writeback facts

## Suggested Next

Step 5 handoff: closure review. Confirm the source idea's acceptance criteria
are satisfied by the Step 2 prealloc record/helper, Step 3 AArch64 adapter
consumption, and Step 4 x86-labeled cross-target reuse proof; decide whether the
active plan can close or needs a follow-up packet.

## Watchouts

- Keep AArch64 memory operand spelling, store opcode selection, scratch choice,
  global address materialization, stack-pointer sequences, and final instruction
  construction out of prealloc.
- Do not weaken store expectations or create testcase-shaped shortcuts.
- The x86 reuse proof is intentionally a target-labeled consumer test over
  neutral plan fields, not an x86 lowering implementation.
- Any closure review should verify no AArch64 codegen dependency leaked into
  the prealloc API or cross-target test surface.

## Proof

Ran:
`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'`

Result: `162/162` backend tests passed, `0` failed. `git diff --check` passed.
Proof log: `test_after.log`.
