# AArch64 Natural Operator Naming And Printer Spelling

Status: Open
Created: 2026-05-14

Depends On:
- `ideas/closed/218_aarch64_structured_asm_encoder_linker_contract.md`

## Goal

Define and apply a readable AArch64 operator naming contract where code-facing
operator names, diagnostics, and printed assembly spelling stay as close as
possible.

The MIR / structured asm stream should use natural AArch64 names that are easy
for humans and LLM agents to write correctly. Encoder-level canonicalization
must handle aliases and concrete encoding forms later.

## Why This Idea Exists

The structured asm/encoding layer from idea 218 is meant to preserve semantics
all the way toward object/linker output, not to make MIR authors think in raw
encoding tables. If the code uses names that are far away from the assembly
that developers expect to read, future work will drift into guesswork.

For first implementation and agent ergonomics, names such as these are better
at the MIR / asm-stream boundary:

```cpp
enum class AArch64OperatorKind {
  Mov,
  Add,
  Sub,
  Cmp,
  Tst,
  Ldr,
  Str,
  B,
  BCond,
  Cbnz,
  Ret,
};
```

The assembler/encoder layer may later lower aliases into precise encoding
forms:

```text
Mov      -> ORR/MOVZ/MOVN/MOVK/etc. as needed
Cmp      -> SUBS with zero-register destination
Tst      -> ANDS with zero-register destination
Ldr/Str  -> selected load/store encoding form
BCond    -> conditional branch encoding
```

The key rule is that user/developer-facing operator names should match printed
assembly where practical, while encoder internals remain free to canonicalize
into exact AArch64 encoding families.

## In Scope

- Audit current AArch64 machine-node, structured asm/encoding, printer, and
  diagnostic enum names for places where code-facing names differ needlessly
  from the printed assembly spelling.
- Define naming tiers:
  - stream item kinds such as section, label, instruction/operator, directive,
    data, alignment, and relocation need
  - natural AArch64 operator kinds used by MIR / structured asm records
  - final printer mnemonic spelling
  - later encoder canonical forms
- Establish that operator kind names may use AArch64 mnemonic or alias names
  when that is the most natural MIR authoring surface.
- Centralize `kind -> string` mappings so printer and diagnostics use the same
  spelling table rather than scattering spelling switches across lowering code.
- Document alias handling:
  - aliases are valid operator names at the structured asm/MIR-friendly layer
  - assembler/encoder canonicalization owns resolving aliases into concrete
    encoding forms
  - printer may print the natural alias when it is valid external assembly
- Update relevant markdown/contracts and focused tests or compile checks for
  representative operator-name-to-printer-spelling consistency.

## Out Of Scope

- Implementing the full assembler, encoder, object writer, linker, or binary
  emitter.
- Requiring MIR authors to name every concrete AArch64 encoding form.
- Replacing the terminal `.s` printer with an internal parser.
- Expanding instruction selection coverage just to add more names.
- Accepting external `.s` or `.ll` as backend input.

## Acceptance Criteria

- AArch64 structured asm/operator naming is documented around natural
  developer-facing names that stay close to printed assembly spelling.
- The contract distinguishes stream item kinds, natural operator kinds,
  printer mnemonics, and later encoder canonical forms.
- Alias-like names such as `Mov`, `Cmp`, or `Tst` are allowed at the
  MIR/structured-asm layer when they improve readability, with explicit
  assembler/encoder ownership for canonicalization.
- Printer and diagnostics use centralized enum-kind-to-string mappings for the
  supported operator kinds.
- Representative tests or compile checks prove that code-facing operator names
  and printed spelling do not drift for the supported subset.

## Reviewer Reject Signals

- The route forces MIR/structured-asm authors to use obscure encoding-form
  names when a natural AArch64 mnemonic would be clearer.
- Printer spelling, diagnostics, and enum names diverge without a documented
  reason.
- Alias resolution is pushed back into MIR lowering instead of being owned by
  the assembler/encoder contract.
- The route treats printed `.s` as semantic input to the internal backend.
- The route expands into broad instruction selection or encoder implementation
  instead of fixing the naming contract.
