Status: Active
Source Idea Path: ideas/open/107_prealloc_inline_asm_memory_effect_metadata_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Decide The Metadata Contract

# Current Packet

## Just Finished

Step 2 metadata contract decision completed for inline-asm stack-layout
placement.

Chosen contract:
- Structured inline-asm operand metadata is the authority for specific
  inline-asm memory/address effects in prealloc stack-layout placement.
  `InlineAsmOperandMetadata::memory_address` for `MemoryInput` and
  `InlineAsmOperandMetadata::address` for `AddressInput`, each carrying
  `MemoryAddress`, are the facts that may identify a concrete pointer-value
  memory/address use.
- Stack layout owns only the placement consequence. It may mark the referenced
  stack object/address root as `address_exposed`, `requires_home_slot`, and
  `permanent_home_slot`; it must not move frame placement, register homes,
  operand storage, or target memory operand selection into BIR.
- The initial placement consequence should be narrow: when a structured
  inline-asm memory/address operand uses `MemoryAddress::PointerValue` whose
  base value names a local stack object or a known pointer alias rooted in one,
  stack layout treats that as an address escape/use of that object and keeps a
  home slot. This aligns with existing pointer-root placement analysis rather
  than inventing a new target-specific storage rule.
- Prepared inline-asm carrier validation and AArch64 lowering remain separate
  structured consumers. Step 3 should not duplicate target operand selection in
  stack layout; it should only consume BIR memory/address facts enough to drive
  stack-object publication/home-slot retention.

Retained conservative paths:
- Keep a named `ConservativeSideEffectInlineAsmPlacement` summary for the
  existing behavior: any side-effecting inline asm may reinforce home slots for
  objects that are already address-exposed by other stack-layout facts.
- This retained summary is placement policy only. It is not semantic
  memory/address provenance and must not claim a specific object was referenced
  by inline asm.
- The existing instruction-count note remains diagnostic-only.

`"memory"` clobber treatment:
- A structured `"memory"` clobber is already target-facing machine-effect
  metadata through prepared inline-asm carriers and AArch64 lowering.
- For stack layout, `"memory"` clobber should not identify a specific stack
  object because it has no address payload. It may participate only in the
  conservative placement summary, bounded to already address-exposed objects.
- Do not add raw clobber-text matching as semantic stack-object authority.
  If clobber handling is touched, name it as conservative clobber policy.

Implementation targets for Step 3:
- Extend `FunctionInlineAsmSummary` or adjacent stack-layout facts to carry
  structured inline-asm memory/address pointer roots derived from
  `InlineAsmOperandMetadata::{memory_address,address}`.
- Reuse existing stack-layout pointer-root/local-slot machinery where possible
  so a structured inline-asm memory/address operand has the same placement
  consequence as an explicit pointer/address escape.
- Keep prepared carrier validation fail-closed behavior unchanged for missing
  or unsupported memory/address metadata.
- Isolate the existing side-effect summary behind a named conservative helper
  so it is visibly not the metadata authority path.

Proof targets:
- Positive structured proof: a BIR inline asm call with `MemoryInput` or
  `AddressInput` carrying `MemoryAddress::PointerValue` rooted in a local slot
  keeps/publishes the expected home-slot placement even when the route depends
  on operand metadata rather than only generic side effects.
- Fail-closed or bounded-fallback proof: an inline asm call without structured
  memory/address metadata must not create a new specific stack-object
  placement fact; at most the named conservative side-effect path may reinforce
  an object that was already address-exposed.
- Existing prepared-printer/AArch64 dispatch tests remain proof that structured
  carriers and target memory/address operand selection stay intact.

Rejected routes:
- Do not satisfy the plan by only renaming `summarize_inline_asm` while still
  treating raw inline-asm presence or raw clobber text as object-specific memory
  authority.
- Do not weaken inline-asm memory/address carrier diagnostics or mark supported
  structured cases unsupported.
- Do not broaden into inline-asm parser, target constraint, intrinsic memory,
  or MIR lowering redesign.

## Suggested Next

Execute `plan.md` Step 3: implement the narrow metadata contract in
stack-layout analysis, with structured memory/address operand roots as the
specific authority and a named conservative side-effect fallback.

## Watchouts

- Keep the first implementation narrow. Do not try to resolve arbitrary
  address forms or target constraints in stack layout.
- The likely safe path is `MemoryAddress::PointerValue` with a named local-slot
  root or already-known pointer alias; other address kinds should either reuse
  existing stack-layout address machinery or fail closed.
- Preserve the prepared inline-asm carrier and AArch64 lowering contracts; the
  stack-layout work should not change target-facing operand selection.

## Proof

Passed. Ran:
`git diff --quiet -- src/backend/bir src/backend/prealloc tests && printf 'analysis-only proof: no implementation or test diff for inline-asm metadata contract decision\n' > test_after.log`

Proof log: `test_after.log`.
