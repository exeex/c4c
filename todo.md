Status: Active
Source Idea Path: ideas/open/107_prealloc_inline_asm_memory_effect_metadata_contract.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Document The Contract

# Current Packet

## Just Finished

Step 3 implementation completed for the narrow inline-asm stack-layout metadata
contract.

Changed routes:
- `stack_layout::record_call_pointer_uses` now consumes structured
  `InlineAsmOperandMetadata::memory_address` for `MemoryInput` and
  `InlineAsmOperandMetadata::address` for `AddressInput`.
- Those structured addresses feed the existing `record_memory_address_use`
  pointer-root path, so `MemoryAddress::LocalSlot` and
  `MemoryAddress::PointerValue` rooted in a local slot or known pointer alias
  drive the same address-exposure/home-slot placement consequence as other
  explicit stack-layout address uses.
- `FunctionInlineAsmSummary` now records
  `structured_memory_address_operand_count` for observable structured metadata
  presence while keeping instruction count diagnostic-only.
- The old side-effect placement behavior is isolated as
  `has_conservative_side_effect_placement` and the named
  `conservative_side_effect_inline_asm_placement_applies` helper. It still only
  reinforces home slots for objects already address-exposed by other facts.

Preserved boundaries:
- No parser, BIR lowering, prepared inline-asm carrier validation, MIR, or
  AArch64 target operand-selection behavior changed.
- Missing inline-asm memory/address metadata remains fail-closed in prepared
  carrier validation; stack layout only consumes metadata that is already
  present.
- `"memory"` clobber text is not used as specific stack-object authority.

## Suggested Next

Execute `plan.md` Step 4: add focused tests for structured inline-asm
memory/address metadata driving local-slot or pointer-root placement, plus a
bounded/fail-closed case for missing structured metadata.

## Watchouts

- The structured route currently relies on the existing stack-layout
  `MemoryAddress` handling. If Step 4 needs more observability, add tests
  around prepared stack-layout objects rather than assembly-only symptoms.
- The conservative side-effect path intentionally remains broad only for
  already address-exposed objects; do not turn raw side-effect or clobber
  spelling into object-specific authority.

## Proof

Passed. Ran:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: 169 backend tests passed.

Proof log: `test_after.log`.
