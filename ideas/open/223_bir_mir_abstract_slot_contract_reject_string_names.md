# BIR MIR Abstract Slot Contract Reject String Names

Status: Open
Created: 2026-05-14

Depends On:
- `ideas/closed/212_bir_mir_allocation_contract.md`

Discovered During:
- `ideas/open/222_aarch64_scalar_return_alu_selected_nodes.md`

## Goal

Make the BIR/prepared-to-MIR boundary use structured abstract placement
contracts instead of string-derived identities.

BIR and prepared allocation facts must describe where a value belongs in terms
of BIR/MIR contract slots, not target register spellings or names derived from
BIR display text.

Examples of allowed contract facts:

```text
GPR argument slot #0
GPR return slot #0
long-lived GPR slot #1
caller-temp GPR slot #0
reserved MIR scratch GPR slot #0
spill slot #3
```

Examples of rejected facts at the BIR/prepared boundary:

```text
x0
x20
d0
rdi
rax
%t1
tmp.add.result
```

Target MIR owns the conversion from abstract slot identity to target physical
register records. Printers own the final conversion from target register
records to textual assembly spelling.

## Why This Idea Exists

`prealloc` currently publishes register identity through fields such as
`register_name`, `occupied_register_names`, `source_register_name`, and
`destination_register_name`. AArch64 then parses or forwards those strings into
MIR records, and some records preserve them as `std::string_view` provenance.

That keeps the old string-based path alive:

```text
BIR/prealloc decides target register spelling
  -> AArch64 MIR parses or carries the spelling
  -> printer later prints another spelling
```

This violates the intended boundary. BIR does not know the final instruction
set and should not know names like `x0`, `x20`, `rdi`, or `rax`. It should only
know the abstract slot chosen by the BIR/MIR calling and allocation contract.

The same rule applies to names derived from BIR values, blocks, temporaries, or
debug/display text. Those names may be carried as provenance for diagnostics,
but they must not become MIR allocation, register, branch, label, symbol, or
machine-instruction identity.

## In Scope

- Define a structured abstract placement model for the BIR/prepared-to-MIR
  boundary, including:
  - register bank such as GPR, FPR, future vector, or aggregate-address
  - slot pool such as argument, return, long-lived allocatable,
    caller-saved temp, reserved MIR scratch, special/forbidden, or spill
  - slot index
  - contiguous width or occupied abstract slots when a value spans more than
    one slot
- Replace or deprecate string-based register identity in `prealloc` records:
  - `register_name`
  - `occupied_register_names`
  - `source_register_name`
  - `source_occupied_register_names`
  - `destination_register_name`
  - `destination_occupied_register_names`
  - `fixed_register_name`
  - `preferred_register_names`
  - `forbidden_register_names`
- Keep any remaining spelling fields explicitly marked `deprecated`,
  `legacy`, `display`, or `compatibility`, and prevent new code from treating
  them as authoritative.
- Update AArch64 MIR conversion so it consumes abstract slots and maps them to
  `aarch64::abi::RegisterReference` only inside the AArch64 target layer.
- Remove AArch64 MIR record fields that preserve prepared register spellings as
  semantic data, such as `std::vector<std::string_view> occupied_registers`.
- Reject MIR identity derived from BIR spelling/display names. BIR names may
  remain optional provenance only when carried through explicit provenance
  structs and never used as allocation/register/label/symbol identity.
- Add tests that fail when a BIR/prepared-to-MIR path requires or asserts
  target register names before entering the target MIR layer.

## Out Of Scope

- Implementing a full optimized register allocator.
- Implementing internal assembler, encoder, object writer, linker, or binary
  output.
- Changing the final assembly printer spelling.
- Removing all debug/display names from dumps. Human-readable names are allowed
  when clearly marked as display/provenance and not used as semantic identity.
- Enabling additional AArch64 backend cases merely because the slot contract
  cleanup touches related files.

## Required Contract Shape

The exact names can follow local style, but the model should be close to:

```cpp
enum class PreparedRegisterSlotPool {
  None,
  Argument,
  Return,
  LongLived,
  CallerSavedTemp,
  ReservedMirScratch,
  Special,
  Forbidden,
};

struct PreparedRegisterSlot {
  PreparedRegisterBank bank = PreparedRegisterBank::None;
  PreparedRegisterSlotPool pool = PreparedRegisterSlotPool::None;
  std::size_t index = 0;
  std::size_t contiguous_width = 1;
};

struct PreparedSpillSlot {
  PreparedFrameSlotId slot_id = 0;
};
```

The important rule is not the exact spelling of these structs. The important
rule is that BIR/prepared owns abstract slot placement, while target MIR owns
physical register interpretation.

## Acceptance Criteria

- The BIR/prepared-to-MIR contract documents that BIR/prepared emits abstract
  slots, not target register spellings.
- `prealloc` has a structured representation for register placement that can
  identify bank, slot pool, slot index, and width without using target register
  names as identity.
- Existing string register fields are removed or explicitly marked
  `deprecated` / `legacy` / `display` / `compatibility` with a migration path.
- AArch64 MIR maps abstract slots to `aarch64::abi::RegisterReference` inside
  the target layer and does not require prepared register spellings for normal
  lowering.
- AArch64 MIR records do not preserve `std::string_view` register-name
  provenance as semantic allocation data.
- BIR value, block, temporary, or display names are never used as MIR semantic
  identity. They may only appear in explicit provenance/debug fields.
- Focused tests prove representative argument, return, long-lived, scratch,
  and spill placements without asserting target register spellings before the
  AArch64 target mapping step.

## Reviewer Reject Signals

- New code adds or relies on `register_name`, `occupied_register_names`, or
  similar string fields as authoritative placement identity.
- BIR/prepared chooses `x0`, `x20`, `rdi`, `rax`, or any other ISA spelling
  before target MIR mapping.
- AArch64 MIR treats a BIR value name, block label spelling, temporary name, or
  display text as register, label, symbol, or instruction identity.
- String fields are renamed without changing the contract.
- Tests continue to prove placement mainly by checking strings like `"x0"` in
  prepared/BIR records instead of checking abstract slot identity.
- The cleanup is used as an excuse to enable unrelated AArch64 feature cases or
  to add testcase-shaped backend shortcuts.
