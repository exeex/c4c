Status: Active
Source Idea Path: ideas/open/239_aarch64_intrinsic_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Post-Carrier Machine-Node Readiness

# Current Packet

## Just Finished

Lifecycle activation only. No executor packet has completed yet.

## Suggested Next

Start `plan.md` Step 1 by inventorying the completed CRC/vector carrier
authority and checking whether barrier, cache, pause/hint, and builtin-address
families already have enough structured semantic and prepared facts for
machine-node selection.

## Watchouts

- Treat scalar `fabs` selected-machine support as existing behavior to preserve.
- Do not select or print from intrinsic spelling, generic call plans, archived
  scratch registers, or final assembly text.
- If a family still lacks carrier authority, record the blocker instead of
  creating name-only AArch64 records.
- Keep unsupported x86-only and incomplete-fact paths fail-closed.

## Proof

Lifecycle proof required before handoff:

- lifecycle invariant checks
- `git diff --check`
