Status: Active
Source Idea Path: ideas/open/107_prealloc_inline_asm_memory_effect_metadata_contract.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Focused Inline Asm Proof

# Current Packet

## Just Finished

Step 4 focused proof completed for structured inline-asm metadata driving
stack-layout placement.

Focused test coverage added in
`tests/backend/bir/backend_prepare_stack_layout_test.cpp`:
- `prepare_inline_asm_stack_layout_module` builds a BIR inline-asm fixture with
  three local roots and runs stack layout only.
- The positive `MemoryInput` case passes an unrooted pointer argument but
  supplies `InlineAsmOperandMetadata::memory_address` with
  `MemoryAddress::PointerValue` rooted in `lv.inline.memory.root`; the test
  asserts the object becomes `address_exposed`, keeps `requires_home_slot`, and
  receives a frame slot with the original layout.
- The positive `AddressInput` case does the same through
  `InlineAsmOperandMetadata::address` rooted in `lv.inline.address.root`.
- The fail-closed/bounded fallback case uses side-effecting inline asm with a
  `"memory"` clobber and a `MemoryInput` operand that lacks structured
  `memory_address`; because its call argument is unrooted, the test asserts
  `lv.inline.missing.root` does not become address-exposed, does not require a
  home slot, and does not receive frame-slot storage.
- The fixture proves placement through prepared stack-layout object state, not
  through assembly output or target lowering symptoms.

## Suggested Next

Execute `plan.md` Step 5: final validation and close-readiness notes for the
inline-asm memory/address metadata contract.

## Watchouts

- The new fail-closed case intentionally keeps the inline-asm call argument
  unrooted so the assertion isolates missing structured metadata from generic
  call-argument escape behavior.
- No source changes were needed in this proof packet; the Step 3 stack-layout
  implementation already satisfied the focused contract.

## Proof

Passed. Ran:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: 169 backend tests passed.

Proof log: `test_after.log`.
