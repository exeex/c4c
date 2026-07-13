# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Repair the A1 import order identifiers

## Just Finished

- Plan Step 2.1 repaired the memory import sub-boundary to use root `A1` draft
  construction, root `A2` Draft/Raw verification and publication, and root
  `B1` / `P01 legalize` adjacency while preserving its lossless transactional
  failure contract.

## Suggested Next

- Execute Plan Step 2.2, "Close core architecture choices and repair its BIR
  disposition."

## Watchouts

- Keep the exact `A1 -> A2 -> B1` root adjacency distinct from the canonical
  `P01` pass identity while repairing the core surface.
- Step 14 and implementation remain forbidden pending completion of the repair
  route and a new blocker-free independent Step 13 review.

## Proof

- Passed: `git diff --check && ! rg -n 'S00|S01|G01'
  src/backend/bir/lir_to_bir/memory/README.md && rg -n
  'A1|A2|Raw|ModuleDraft|verify_and_publish_raw|transaction|failure'
  src/backend/bir/lir_to_bir/memory/README.md`.
- The supervisor-selected documentation proof was sufficient; this packet did
  not create or modify `test_after.log` because regression logs were explicitly
  outside packet ownership.
