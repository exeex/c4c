Status: Active
Source Idea Path: ideas/open/107_prealloc_inline_asm_memory_effect_metadata_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Inline Asm Memory Effect Consumers

# Current Packet

## Just Finished

Step 1 inventory completed for inline-asm memory/address effect consumers.

Routes inventoried:
- BIR structured source: `bir::InlineAsmMetadata` carries asm text,
  constraints, `side_effects`, operand metadata, clobbers, unsupported facts,
  named-reference/template-modifier flags. `bir::InlineAsmOperandMetadata`
  carries operand kind, indexes, name, and optional `MemoryAddress`
  facts for `memory_address` and `address`.
- Prepared inline-asm carrier construction:
  `src/backend/prealloc/inline_asm.cpp` copies structured
  `InlineAsmOperandMetadata` into `PreparedInlineAsmOperand`, preserving
  `memory_address`/`address`, value names, value homes, immediate values, and
  tied-home authority. This is a structured metadata consumer.
- Prepared inline-asm memory/address validation:
  `validate_inline_asm_carrier` requires `MemoryInput` operands to carry
  `operand.memory_address` and `AddressInput` operands to carry
  `operand.address`. It also requires both to be selectable through
  `MemoryAddress::PointerValue` matching the operand value with a prepared
  register home. Missing or unsupported metadata records fail-closed missing
  facts such as `*_memory_address_authority`, `*_address_authority`,
  `*_memory_address_selection`, and `*_address_selection`.
- Prepared inline-asm carrier storage/printer:
  `PreparedInlineAsmOperand` stores optional `memory_address` and `address`.
  `prepared_printer/inline_asm.cpp` exposes only presence as
  `memory_address=yes` / `address=yes`; it does not print the full address
  payload. This is display/diagnostic visibility, not placement authority.
- AArch64 inline-asm lowering:
  `src/backend/mir/aarch64/codegen/inline_asm.cpp` looks up a complete
  prepared carrier, consumes `MemoryInput.memory_address` and
  `AddressInput.address`, validates the same pointer-value/register-home
  relation, builds a prepared `MemoryOperand`, and fail-closes malformed or
  incomplete carriers before emitting machine records.
- AArch64 inline-asm clobber route:
  prepared carrier clobber strings are parsed by AArch64 lowering into machine
  clobber effects, including `"memory"` as a machine memory clobber. This is a
  target-facing effect route, not stack-placement authority.
- Stack-layout summary route:
  `src/backend/prealloc/stack_layout/inline_asm.cpp` currently scans
  `CallInst` with `inline_asm` and records only instruction count plus
  `inline_asm.side_effects`. It does not inspect operand kinds, structured
  `memory_address`/`address`, or `"memory"` clobbers.
- Stack-layout placement consumer:
  `apply_regalloc_hints` consumes `FunctionInlineAsmSummary`. If any inline asm
  has side effects and a stack object is already `address_exposed`, it forces
  `requires_home_slot` and `permanent_home_slot`. The source of the inline-asm
  fact here is the raw conservative side-effect summary, while the object
  address exposure itself comes from local-slot metadata or earlier
  stack-layout pointer-root/address scans.
- Stack-layout note route:
  `run_stack_layout` emits a note when inline asm instructions are observed.
  This is diagnostic bookkeeping only; it has no direct placement consequence.

Current classification:
- Structured metadata consumers already exist for prepared carrier validation,
  prepared carrier records, prepared dumps, AArch64 memory/address operand
  selection, and AArch64 machine clobber effects.
- Raw/conservative summary remains in stack layout: presence plus
  `side_effects` of any inline asm gates home-slot reinforcement for already
  address-exposed objects. This summary does not distinguish structured memory
  operands, address operands, memory clobbers, or purely register-only asm.
- Placement consequence today is conservative: side-effecting inline asm keeps
  already address-exposed objects in permanent home slots, but structured
  inline-asm memory/address metadata does not independently drive stack-object
  publication or home-slot retention.

Candidate contract gaps:
- Stack layout should consume structured inline-asm memory/address metadata
  where present instead of treating only `side_effects` as the memory/address
  effect source.
- Any retained side-effect-only route should be named as conservative
  placement policy, not BIR memory provenance authority.
- Need decide whether `"memory"` clobber should affect stack placement, and if
  so whether it is a structured clobber fact or part of the conservative
  summary.
- Need decide which placement consequence is intended for `MemoryInput` or
  `AddressInput`: mark referenced local slot/address root address-exposed,
  require home slot for already exposed roots, or only preserve current
  conservative behavior.

Proof gaps:
- Existing prepared-printer and AArch64 dispatch tests prove structured
  memory/address operands and fail-closed malformed carriers, but they do not
  prove stack-layout placement consumes inline-asm operand metadata.
- Existing stack-layout tests cover generic inline-asm summary effects only
  indirectly through `FunctionInlineAsmSummary` / `apply_regalloc_hints`; they
  do not prove memory/address operand metadata drives home-slot retention.
- Need a focused proof fixture with inline asm `MemoryInput` or `AddressInput`
  and a local/pointer address fact, showing the intended prealloc stack-layout
  placement consequence.

## Suggested Next

Execute `plan.md` Step 2: decide the narrow inline-asm memory/address effect
contract for stack-layout placement, including which structured facts should
drive placement and which conservative summary paths remain named exceptions.

## Watchouts

- The prepared carrier and AArch64 paths already consume structured
  `memory_address`/`address` metadata. The main gap is stack-layout placement.
- Avoid broad inline-asm parsing or target constraint rewrites; the source idea
  is about prealloc memory/address effect authority.
- Do not weaken existing fail-closed carrier diagnostics for malformed memory
  or address operands.

## Proof

Passed. Ran:
`git diff --quiet -- src/backend/bir src/backend/prealloc tests && printf 'analysis-only proof: no implementation or test diff for inline-asm memory effect inventory\n' > test_after.log`

Proof log: `test_after.log`.
