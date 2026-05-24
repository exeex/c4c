Status: Active
Source Idea Path: ideas/open/entry-formal-publication-planning-prealloc.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Current Entry-Formal Publication Planning

# Current Packet

## Just Finished

Step 1 inventory completed for AArch64 entry-formal publication planning and
the existing prealloc value-location/variadic surfaces.

Target-neutral formal publication planning currently embedded in AArch64:
- `lower_entry_formal_publications()` filters entry block only, skips varargs
  and sret formals, resolves each formal name through prepared names, looks up
  the formal `PreparedValueHome`, and decides whether a formal has publication
  work based on incoming source class and home kind.
- The reusable planning facts are formal index/name/value id, BIR type,
  `PreparedValueHomeKind` (`Register`, `StackSlot`, `None` for current f128
  stack handling), stack offset presence, and whether the formal is sourced
  from incoming stack or incoming ABI register.
- `value_locations.hpp` already owns prepared formal homes and block-entry
  move bundles; these are the right authority records for target-neutral
  publication intent.
- `prepared_lookups.hpp/.cpp` already provides value-home and move-bundle
  lookup infrastructure; a formal-publication helper should reuse those lookup
  shapes instead of scanning target code.
- `variadic_entry_plans.cpp` already resolves helper operand homes and missing
  helper facts in prealloc. That is adjacent evidence that prealloc may publish
  home-based entry facts, but its AAPCS64 va-list layout/register-save policy
  should not be folded into the first formal-publication helper.

AArch64 ABI source selection that must stay target-local:
- register-bank slot counting and register start selection
  (`entry_formal_aarch64_register_slot_count`,
  `entry_formal_aarch64_register_slot_start`,
  `entry_formal_abi_register_index`)
- source register naming and view conversion via AAPCS64 register rules
  (`entry_formal_source_register`, `entry_formal_register_view`)
- incoming stack classification/offset math and frame-size interaction
  (`entry_formal_uses_incoming_stack`,
  `entry_formal_incoming_stack_offset`, `entry_formal_frame_size_bytes`)
- byval aggregate lane handling, f128 stack special handling, and fixed-home
  base register choice (`sp` vs `x29`)

Concrete AArch64 copy/prologue emission that must stay target-local:
- load/store opcode selection, scratch register selection, byte copy loops,
  register move spelling, and inline-asm payload construction in
  `dispatch_entry_formals.cpp`
- `record_entry_formal_register_home()` updates to the AArch64 scalar lowering
  state
- `prologue.cpp` frame boundary/prologue/epilogue instruction records and saved
  register printing
- `dispatch_publication.cpp` block-entry publication register scans and
  clobber checks, which are edge-copy/publication emission concerns rather than
  formal-entry planning

Diagnostics/status behavior:
- The current entry-formal path mostly skips unsupported or missing facts by
  returning empty lines rather than producing explicit diagnostics.
- A first prealloc helper should report structured status for no formal value
  id, no home, unsupported home kind, missing stack offset, and no publication
  needed, but AArch64 should continue choosing whether that becomes a skip or a
  diagnostic.

Smallest first extraction boundary:
- Add a prealloc formal-publication planning/query helper over a
  `PreparedBirModule` or per-function inputs (`PreparedNameTables`,
  `bir::Function`, `PreparedValueLocationFunction`, optional
  `PreparedValueHomeLookups`).
- Return records that preserve source Prepared facts: formal index, prepared
  value name/id, `bir::TypeKind`, original `bir::Param` pointer or stable
  formal metadata, `PreparedValueHome*`, home kind, and target-neutral action
  such as `NoPublication`, `IncomingRegisterToHome`,
  `IncomingStackToHome`, or `Unsupported/MissingAuthority`.
- Do not return AArch64 registers, stack offsets from ABI layout, MIR operands,
  inline asm lines, prologue facts, or diagnostics.

## Suggested Next

Step 2 should add the prealloc formal-publication plan/query API and focused
direct coverage for register-home, stack-home, missing-home/missing-field, and
no-publication forms without adapting AArch64 yet.

## Watchouts

Keep ABI register and stack source policy, AArch64 register spelling, prologue
shape, and entry-copy instruction emission target-local. Do not combine this
with call-boundary classification, edge-copy bookkeeping, or operand decoding
migrations.

`dispatch_publication.cpp` block-entry move scans may be reusable later, but
they are not the first formal-publication extraction. Keep this packet scoped
to entry formal-to-home publication intent.

## Proof

Inventory/documentation-only packet. No implementation files were modified, so
no build or ctest proof was required or run.
