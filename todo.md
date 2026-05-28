Status: Active
Source Idea Path: ideas/open/65_aarch64_target_owner_relocation.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Target-Owner Relocation Candidates

# Current Packet

## Just Finished

Step 1 completed: inspected the AArch64 dispatch-family helper surfaces without
implementation edits and classified relocation candidates for idea 65.

Helper inventory:
- `dispatch_producers.*`: `find_load_global_target` and
  `load_global_symbol_label` are target-local global/symbol lookup-spelling
  adapters with direct consumers in `globals.cpp` and
  `fp_value_materialization.cpp`; destination should be `globals.*`.
  `producer_instruction_index`, same-block select/global-load chain discovery,
  and current-block join prepared-query routing are temporary route/query
  adapters or out-of-scope semantic authority and should not be moved as target
  emission helpers in this plan.
- `dispatch_value_materialization.*`: `emit_value_publication_to_register`
  owns target-local scalar value publication, immediate/register spelling,
  prepared home reloads, local/global load materialization, select-chain
  publication, recursive scalar emission, and scratch/write-hazard ordering.
  It is still a broad public adapter used by dispatch, select, FP, edge-copy,
  ALU, call, memory, and cast owners; destination should eventually be a
  non-dispatch scalar/value publication owner, but it is too wide for the first
  relocation packet. Its private `emit_prepared_global_load_to_register` is a
  global load spelling candidate for `globals.*` after the symbol helpers move.
- `dispatch_publication.*`: register/type/address spelling helpers
  (`registers_alias`, scalar views/register names, scalar load/store mnemonics,
  frame-slot/register-indirect/relocation address formatting) are target-local
  utility emission and should move toward existing precise owners
  (`operands`/`alu` for register/type facts, `memory` for load/store/frame
  address spelling, `globals` for relocation spelling). Prepared home and block
  entry publication lookups are temporary AArch64 adapters over shared prepared
  data. Local-slot address publication, fixed-formal store publication,
  retargeting, publication recording, branch-fusion operand publication, and
  scratch/read-hazard helpers are target-local emission/hazard behavior, but
  they should move in later memory/publication packets after globals are split.
- `dispatch.cpp`: address-materialization retry routing, prepared-memory index
  retry, block-entry/before-instruction/before-return sequencing, and
  branch-fusion hook wiring are route-level dispatch behavior. They are not
  first-packet relocation targets; later packets may only extract helper logic
  to precise owners while preserving this routing.

Public-surface shrink opportunities:
- After moving global/symbol helpers, `dispatch_producers.hpp` can drop
  `find_load_global_target` and `load_global_symbol_label`; direct consumers
  should include `globals.hpp`.
- `dispatch_publication.hpp` is overexposed as a utility header. Later packets
  can shrink it by moving scalar mnemonic/address/register helpers to precise
  owners and removing callers from the publication header.
- `dispatch_value_materialization.hpp` should remain a temporary adapter until
  the value-publication helper has a coherent non-dispatch owner; avoid a
  forwarding-only rename.

## Suggested Next

Step 2 should be the first coherent code-moving packet: relocate
`find_load_global_target` and `load_global_symbol_label` from
`dispatch_producers.*` to `globals.*`, update `globals.cpp` and
`fp_value_materialization.cpp` to include/use the globals owner directly, and
remove the declarations from `dispatch_producers.hpp` if no producer-owned
caller remains. Optionally move only the private global-load publication helper
from `dispatch_value_materialization.cpp` in the same packet if it stays
behavior-preserving and does not force value-publication ownership changes.

## Watchouts

Proof needs to preserve: global load spelling, GOT-required and direct symbol
labels, frame-slot address spelling, fixed-formal stores, scalar load/store
mnemonics, local-slot and global value materialization, scratch-register read
and write hazards, prepared-memory retry routing, and address-materialization
retry routing.

Do not move semantic producer discovery, current-block join prepared-query
routing, branch-fusion sequencing, before-return publication ordering,
prepared-memory index retry decisions, or target-local clobber/register
spelling into shared code. Avoid idea 67's local-slot null-offset probe.

## Proof

Read-only inventory plus `git diff --check`. Inspection commands included
targeted `sed` reads of `plan.md`, idea 65, `todo.md`,
`dispatch_publication.*`, `dispatch_value_materialization.*`,
`dispatch_producers.*`, `dispatch.cpp`, and precise owner headers, plus `rg`
scans for current declarations/call sites across AArch64 codegen.
