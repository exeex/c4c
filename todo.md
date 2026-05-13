Status: Active
Source Idea Path: ideas/open/206_prepared_memory_volatility_address_space_carrier.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Update Prepared Carrier Documentation

# Current Packet

## Just Finished

Completed Step 4, `Update Prepared Carrier Documentation`.

Updated the AArch64 backend-facing markdown to name
`PreparedMemoryAccess::address_space` and
`PreparedMemoryAccess::is_volatile` as the shared typed carrier fields for
address-space and volatility facts.

Removed stale ledger claims about absent shared prepared memory fields, while
preserving the boundary that
target-local memory operands, assembler/object/linker behavior, atomics,
inline asm, and alias-analysis policy remain deferred or separate contracts.

## Suggested Next

Execute the next supervisor-selected packet from `plan.md`; likely decide
whether the active idea is complete or whether any remaining documentation
alignment is needed before lifecycle closure.

## Watchouts

- Current LIR/frontend memory producers still do not appear to publish volatile
  or non-default address-space facts for ordinary loads/stores.
- Target MIR lowering remains out of scope for this idea; consumers should read
  `PreparedMemoryAccess::address_space` and
  `PreparedMemoryAccess::is_volatile` rather than infer from names, dumps, or
  target-local patterns.
- No expectation downgrades or unsupported-test rewrites were made.

## Proof

Command:
`git diff --check -- src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md src/backend/mir/aarch64/CLASSIFICATION_INDEX.md todo.md`

Result: pass.

Proof log: none required for this docs-only packet.
