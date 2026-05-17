Status: Active
Source Idea Path: ideas/open/267_aarch64_inline_asm_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit The Inline-Asm Shard And Current Route

# Current Packet

## Just Finished

Completed Step 1 audit of `src/backend/mir/aarch64/codegen/inline_asm.md`
against the current compiled AArch64 route.

Durable current facts to preserve:

- BIR retains structured inline-asm metadata: template text, constraints,
  typed argument values, side effects, structured clobbers, operand kinds, and
  explicit unsupported facts.
- Prealloc builds `PreparedInlineAsmCarrier` records with complete/missing
  status, operand homes, AArch64 register identity, tied-output coallocation
  authority, integer immediate values, clobbers, and selectable memory/address
  facts.
- AArch64 dispatch lowers complete carriers into selected assembler machine
  nodes with inline-asm payload, selected operands, defs/uses, side effects,
  and clobber effects.
- The current printer supports literal `%%`, positional placeholders, prepared
  named-operand lookup when a complete carrier already supplies names, raw
  signed integer immediates, prepared base+offset memory/address operands, and
  GP register modifiers `%wN` and `%xN`.

Unsupported/deferred facts to keep fail-closed:

- Public AArch64 inline-asm cases remain blocked in the bring-up matrix; current
  coverage is internal prepared-route coverage, not broad public support.
- Frontend/BIR marks named operand references unsupported even though the
  printer can consume prepared names if a complete carrier supplies them.
- Template modifiers beyond the current `%wN`/`%xN` prepared subset are
  unsupported, including the legacy FP/SIMD modifiers and `%a` address
  modifier.
- Multiple outputs, missing or non-register homes, unproven tied-input/output
  coallocation, unsupported constraints, incomplete memory/address authority,
  rendered-only clobber constraints, malformed/unknown/target-invalid clobbers,
  and clobber operands without structured authority must remain explicit
  missing-fact/diagnostic failures.

Obsolete reference-only material to reject:

- Do not port the old static string substitution surface wholesale:
  `substitute_asm_operands_static`, legacy GCC-number fallback to `x<num>`,
  unknown placeholder preservation, r-prefixed alias normalization, FP/SIMD
  register spelling conversion, immediate-symbol rendering, and broad modifier
  semantics are not all backed by current prepared AArch64 facts.
- The atomic helper content in the shard is not inline-asm ownership. Atomic
  exclusive mnemonic selection and RMW ALU emission now belong to the compiled
  atomics owner, not the new inline-asm owner.
- The legacy Rust reference dependency list is context only; Step 2 should move
  current compiled route boundaries, not rebuild from the historical reference
  surface.

Concrete Step 2 owner issue:

- Current inline-asm AArch64 code is embedded in `dispatch.cpp` and
  `machine_printer.cpp`. Step 2 should create `inline_asm.cpp`/`.hpp` and move
  the accepted carrier-to-machine-record and template-printing boundary there
  while leaving atomic lowering, allocation policy, assembler parsing, and
  unrelated instruction spelling in their existing owners.

## Suggested Next

Execute Step 2 from `plan.md`: create the AArch64 inline-asm owner boundary and
move only the current accepted prepared-route helpers out of broad dispatch and
printer files.

## Watchouts

- Keep Step 2 behavior-preserving: no new public inline-asm capability should
  be claimed unless existing prepared facts and tests already prove it.
- Preserve fail-closed diagnostics for named operands, unsupported modifiers,
  clobbers, memory/address authority, and tied-home/coallocation gaps.
- Do not move or reimplement atomic helper behavior; it is already owned
  outside this shard.

## Proof

No build required. The delegated packet was audit-only and explicitly selected
focused text search; no `test_after.log` was produced.
