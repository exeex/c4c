Status: Active
Source Idea Path: ideas/open/86_full_x86_backend_contract_first_replan.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Tighten Remaining Ownership And Markdown Contracts
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Completed plan Step 2 for the thinnest remaining x86 public-seam markdown
companions by rewriting `prepared/prepared.hpp.md`,
`lowering/lowering.hpp.md`, and `core/core.hpp.md` as durable design-contract
documents with legacy evidence, owned inputs and outputs, invariants, direct
dependencies, and deferred or upstream boundaries aligned with the x86 README
policy and the live thin headers.

## Suggested Next

Run the Step 3 conformance packet against the touched contract surfaces so the
live x86 tree is re-proved through these public seams after the companion
cleanup.

## Watchouts

- Keep this runbook focused on contract/layout ownership, not behavior recovery.
- Do not reopen already-closed upstream seam work inside this plan.
- The companion rewrites deliberately describe thin public seams; do not widen
  the live headers or pull implementation policy into these docs just to make
  them sound more complete.
- If the next real gap is behavior-only, split it into a separate idea instead
  of stretching idea 86.

## Proof

Ran the delegated docs-only proof command:
`bash -lc "printf 'docs-only packet: no build required\n' > test_after.log"`.
This is sufficient for the owned markdown-only packet. Proof log:
`/workspaces/c4c/test_after.log`.
