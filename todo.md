# Current Packet

Status: Complete
Source Idea Path: ideas/open/205_aarch64_arm_reference_layout_contract.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add BIR/Prepared Carrier Checklist

## Just Finished

Completed Step 3 by adding the BIR/prepared carrier checklist to
`src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md`. The checklist names the
BIR carrier, prepared carrier, target record, owner, and present, missing,
ambiguous, or deferred status for every layout family in the ledger.

## Suggested Next

Proceed to Step 4 by deciding whether the memory volatility/address-space
carrier gap needs a new `ideas/open/` source idea before the next implementation
wave. If the supervisor chooses to proceed without that gap as the next wave,
record that explicitly and keep memory lowering blocked until the shared
preparation carrier exists.

## Watchouts

- Required Step 4 gap candidate: `PreparedMemoryAccess` / `PreparedAddress`
  carries base, offset, size, and alignment, but no visible volatility or
  address-space fields. The ledger blocks target-local recovery from printed
  BIR.
- AAPCS64 call, return, and variadic metadata is present enough to snapshot
  current prepared call facts, but completeness remains ambiguous until a
  target ABI/call-frame contract reviews all call shapes.
- Assembler/object/linker, inline asm, f128/i128, atomics, and intrinsics are
  explicit deferred contracts, not accepted Step 4 implementation shortcuts.

## Proof

Docs-only packet; delegated proof command:

`git diff --check -- src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md todo.md`

Result: pass, exit 0.
