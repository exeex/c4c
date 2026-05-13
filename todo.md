# Current Packet

Status: Complete
Source Idea Path: ideas/open/205_aarch64_arm_reference_layout_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Draft The Layout Ownership Ledger

## Just Finished

Completed Step 2 by extending
`src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md` into the layout ownership
ledger. The ledger now maps each required family from Step 1 to an owner or
deferred contract, carrier status, target-local record type, and first allowed
implementation route. `CLASSIFICATION_INDEX.md` now points reviewers back to
the ledger as the owning responsibility map.

## Suggested Next

Proceed to Step 3 by adding the BIR/prepared carrier checklist to
`src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md` or a companion markdown
file under `src/backend/mir/aarch64/`.

## Watchouts

- Step 3 should turn the Step 2 ledger rows into a reviewer checklist that
  names the BIR carrier, prepared carrier, target record, and gap/deferred
  status per family.
- Known Step 3 gap decisions to keep explicit: memory volatility/address-space
  is missing from visible prepared memory facts; AAPCS64 call/return/variadic
  completeness is ambiguous; assembler/object/linker, inline asm, f128/i128,
  atomics, and intrinsics remain deferred until separate contracts reopen them.
- Do not convert the existing `module/module.hpp` snapshot records into final
  ABI, instruction-selection, assembler, object, binary-utils, or linker
  ownership during carrier-checklist work.

## Proof

Docs-only packet; delegated proof command:

`git diff --check -- src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md src/backend/mir/aarch64/CLASSIFICATION_INDEX.md todo.md`

Result: pass, exit 0.
