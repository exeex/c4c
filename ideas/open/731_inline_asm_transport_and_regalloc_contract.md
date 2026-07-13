# Inline-Assembly Transport And Register-Allocation Contract

Status: Open
Type: backend semantic transport, target preparation, machine allocation, and late assembly

## Intent

Carry inline-assembly payloads unchanged through the target-independent
frontend, LIR, and canonical BIR stages. Preserve structured source operands
plus original constraint and clobber syntax until target-aware preparation.
Preparation resolves that syntax into a revision-bound typed plan that MIR
construction and register allocation can consume without parsing strings.
Instruction/template parsing remains exclusively at the late assembler seam.

## Why This Idea Exists

Inline asm has two facts with different interpretation times. The instruction
payload must remain opaque until assembly, while operand constraints must be
resolved before register allocation. The replacement architecture now defines
an ordered boundary for that split: canonical BIR is target-independent;
`preparation/inline_asm` is the sole target-aware constraint owner; MIR and
regalloc consume its verified plan. Putting RV64 normalization, physical
register meaning, or constraint-contract provenance in LIR-to-BIR or canonical
BIR would violate that architecture even if a narrow testcase passed.

## Core Contract

- Parser/HIR/LIR and Raw/Canonical BIR carry the opaque asm payload, structured
  source operands, and original constraint/clobber syntax without target
  normalization.
- No stage before the late assembler interprets mnemonics, directives,
  placeholders, `.insn` fields, or encoding details.
- Canonical BIR owns only target-independent inline-asm semantics. It stores no
  RV64 contract ID, physical register interpretation, register pool, group
  classification, allocator requirement, or prepared-plan result.
- `preparation/inline_asm` consumes immutable verified `CanonicalBir` plus the
  selected target context. It is the sole normalizer of constraints and
  clobbers and produces an immutable, revision-bound typed plan.
- The typed inline-asm plan carries operand roles, register classes, group
  width/alignment/contiguity, ties, early-clobbers, memory/condition-code
  effects, and resolved clobber units with structured diagnostics.
- BIR-to-MIR/MIR construction requires the verified plan. MIR and regalloc
  consume structured requirements only and never inspect original strings.
- RV64 preparation must support the evidence-backed `r`, `=r`, `VR`, `VRM2`,
  `VRM4`, and `VRM8` contract, including read/write forms, matching ties,
  early-clobbers, and clobbers. `VRM1` remains unsupported.
- The late assembler receives the opaque payload plus completed physical
  assignments, performs substitution, and is the first instruction parser.
- No implementation is authorized until both the complete ordered BIR
  architecture scaffold and the repaired inline-asm checkpoint have been
  reviewed and accepted together.

## In Scope

- Repairing the inline-asm ownership/schema checkpoint to match
  `src/backend/bir/README.md`, `pipeline/README.md`, and the preparation/MIR
  contracts.
- A target-independent structured source carrier across parser/HIR/LIR and
  Raw/Canonical BIR, while retaining separate LLVM-compatibility rendering.
- A target-aware `preparation/inline_asm` plan tied to exact canonical BIR and
  target-context revisions.
- New MIR construction, allocator requirements, and late assembly consuming
  only the appropriate verified products.
- Direct transport, preparation, BIR-to-MIR, allocator, and bounded late
  assembler/end-to-end proof after architecture acceptance.

## Out Of Scope

- Implementing any part of this route while the ordered architecture remains
  `scaffold` or the repaired checkpoint is unaccepted.
- Compiling, copying, wrapping, or re-exporting `src/backend/legacy/**`, old
  prealloc/MIR code, or the removed `c4c-as` route.
- Target-specific constraint normalization in parser, HIR, LIR, LIR-to-BIR,
  canonical passes, or canonical BIR verification.
- Target-contract provenance, physical register indices, resolved clobber
  units, allocator requirements, or prepared plans stored in canonical BIR.
- Parsing asm instructions/templates before late assembly.
- General GCC/LLVM constraint compatibility beyond reviewed target tables.
- Resuming idea 730 globals work or broad ABI/object/linker/runtime bring-up.

## Acceptance Criteria

- The inline-asm checkpoint and its adjacent ordered-architecture contracts
  jointly identify exact input/output stage tokens, revision binding,
  diagnostics, failure behavior, verification gates, and legacy coverage.
- This idea does not take over exhaustive review of unrelated scaffold areas;
  the architecture-wide `accepted` state remains an external prerequisite for
  implementation.
- Original payload bytes, structured source operand identities, constraints,
  and clobbers survive through published `CanonicalBir`; LLVM rendering never
  becomes semantic authority.
- Canonical BIR contains no target-specific normalized class/group, physical
  register, RV64 provenance, or allocator-plan state.
- RV64 preparation alone maps `r`, `=r`, read/write and numeric ties,
  early-clobbers, clobbers, `VR`, `VRM2`, `VRM4`, and `VRM8` into a verified
  typed plan; `VRM1` and unreviewed forms fail closed.
- The plan is bound to the exact canonical module/function revision and target
  context, and stale or mismatched plans are rejected before MIR publication.
- MIR construction preserves distinct input/output SSA identities while
  translating the verified plan into allocatable requirements.
- Regalloc enforces class, group width/alignment/contiguity, ties,
  early-clobbers, interference, and clobber units without reading syntax.
- Invalid mnemonic or `.insn` payloads cross all earlier boundaries unchanged
  and fail specifically at late assembly.
- Build metadata remains free of legacy/prealloc/old-MIR translation units and
  the supervisor-selected regression proof is green.

## Reviewer Reject Signals

- LIR-to-BIR or canonical BIR classifies `r`, `VR`, `VRM*`, aliases, clobbers,
  physical registers, or any target-specific constraint meaning.
- Canonical BIR stores a target contract/provenance token whose purpose is to
  authorize physical interpretation, or stores a preparation/allocator result.
- Preparation mutates `CanonicalBir`, produces an untyped side table, or emits
  a plan without exact BIR-revision and target-context binding.
- MIR construction or regalloc reads original constraint/clobber strings,
  guesses from spelling, or bypasses preparation verification.
- Any pre-assembler pass tokenizes or interprets mnemonics, directives,
  placeholders, `.insn`, or encoding fields.
- `r`, `=r`, `VR`, or a `VRM*` case is implemented as a named-case shortcut
  rather than a general target preparation rule with nearby negative proof.
- `VRM1` is accepted as an alias, or the `VR`/`VRM2`/`VRM4`/`VRM8` table is
  widened or guessed without reviewed target evidence.
- Tests are downgraded, expectations rewritten, helpers renamed, or
  diagnostics merely reclassified and claimed as capability progress.
- Implementation begins before joint architecture/checkpoint acceptance, or
  legacy BIR/prealloc/MIR/assembler sources re-enter the active build.
- The old target-normalization-in-importer failure is retained behind a new
  abstraction name, or unrelated backend work is mixed into this route.
