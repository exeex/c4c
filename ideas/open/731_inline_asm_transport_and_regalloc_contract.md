# Inline-Assembly Transport And Abstract-BIR Allocation Contract

Status: Open
Type: backend semantic transport, target preparation, abstract-register allocation, and late assembly

## Intent

Carry inline-assembly payloads unchanged through the frontend, LIR, and BIR,
while redesigning the BIR-to-MIR boundary as a verified read-only view over an
immutable BIR revision. That MIR-ready BIR contains only admitted abstract
semantic instruction nodes and has already completed normal register
allocation: every allocatable value has an abstract physical assignment, and
capacity-driven spill/reload nodes are already present. MIR performs target
instruction selection and maps those assignments to concrete ABI registers.

## Why This Idea Exists

Inline asm requires early structured constraint handling but late instruction
parsing. The broader backend boundary also needs a single clear allocation
owner. BIR must have enough target-supplied capacity and ABI-class information
to make correct allocation and spill decisions before MIR; otherwise each
target backend would silently become a second allocator and BIR correctness
could not be verified independently.

## Core Contract

- Parser/HIR/LIR and pre-allocation BIR preserve opaque asm payload bytes,
  structured source operands, and original constraint/clobber syntax. Only the
  late assembler parses mnemonics, directives, placeholders, `.insn`, or
  encoding details.
- Canonical BIR remains target-independent. Target preparation reads it with a
  selected RV64, AArch64, or x86 context and supplies immutable typed facts:
  abstract register categories/classes, caller-saved/callee-saved/temp pool
  capacities, reserved slots, group width/alignment/contiguity, ties,
  early-clobbers, and resolved clobber units.
- The BIR allocator consumes those verified facts. It transactionally
  publishes a new immutable BIR revision in which each allocatable value has
  an abstract physical assignment `(category, class/group, slot)` and normal
  capacity pressure has already produced abstract `Spill`/`Reload` nodes with
  abstract spill-slot identities, never concrete frame offsets.
- `PreparedBir` is a capability/stage token, not a second instruction graph.
  Its public BIR-to-MIR interface is a read-only `MirReadyBirView` over that
  exact allocated BIR revision plus revision-bound target, preparation, and
  allocation facts.
- The MIR-ready graph admits only a closed, reviewed set of abstract semantic
  nodes. Examples include `Load`, `Store`, `Add`, `Mul`, `Jump`, `InlineAsm`,
  `Spill`, and `Reload`; these examples are not the final exhaustive table.
  Target opcodes, concrete register names/numbers, frame offsets, and encoded
  instructions are forbidden.
- `InlineAsm` remains one opaque abstract BIR node. Its structured constraints
  participate in BIR allocation, including target-required classes and atomic
  register groups; its payload is still uninterpreted. Source operand ordinal,
  output ordinal, result index, incoming `UseDef` value, produced value, and
  output destination remain distinct; a tie equates abstract assignments, not
  SSA identities.
- MIR construction consumes only verified `MirReadyBirView`, performs target
  instruction selection, and maps abstract category/class/slot assignments to
  concrete physical registers under the selected calling convention.
- A target backend may add spill/reload only as a bounded final
  legalization/encoding fallback for constraints that could not be expressed
  at the abstract boundary. It may not handle ordinary pool exhaustion, hide
  missing BIR spills, weaken assignments, or act as the normal allocator.
- RV64 preparation must support the evidence-backed `r`, `=r`, `VR`, `VRM2`,
  `VRM4`, and `VRM8` forms, including read/write forms, matching ties,
  early-clobbers, and clobbers. `VRM1` remains unsupported.
- No implementation is authorized until the ordered architecture and this
  repaired boundary are jointly reviewed and accepted.

## In Scope

- Freezing the closed MIR-ready abstract-node admission table and verifier.
- Defining target register-pool/capacity descriptors for RV64, AArch64, and
  x86 without placing concrete register identities in BIR nodes.
- Defining abstract assignments, BIR-owned spill/reload insertion, revision
  binding, diagnostics, and transactional publication.
- Faithful inline-asm transport, target-aware constraint planning, and
  allocation of scalar and register-group operands.
- Defining `PreparedBir`/`MirReadyBirView` as capabilities over the same
  immutable allocated BIR revision, not a rewritten IR copy.
- MIR instruction selection, concrete ABI register mapping, bounded final
  legalization, and late assembly.

## Out Of Scope

- Implementation while the architecture/checkpoint is unaccepted.
- Compiling or restoring `src/backend/legacy/**`, old prealloc/MIR, removed
  `c4c-as`, or deleted `src/backend/bir/mir/**` documents.
- Target interpretation in parser, HIR, LIR, LIR-to-BIR, or Canonical BIR.
- Concrete physical register identifiers or target opcodes in MIR-ready BIR.
- Using MIR/backend spilling as a second normal allocation path.
- Parsing inline-asm instructions before late assembly.
- General GCC/LLVM constraint compatibility beyond reviewed target tables.
- Resuming idea 730 globals work or broad ABI/object/linker/runtime bring-up.

## Acceptance Criteria

- One authoritative contract defines Canonical BIR input, target preparation,
  BIR allocation, allocated-revision publication, `PreparedBir` capability,
  and `MirReadyBirView`, with exact revision/target binding and transactional
  failure behavior.
- The closed abstract-node table and verifier reject target opcodes, concrete
  registers, unresolved allocatable values, illegal abstract slots/groups,
  and missing or inconsistent spill/reload state.
- Target contexts provide reviewed capacities and eligibility for
  caller-saved, callee-saved, temp, and inline-asm-required classes/groups.
- Every allocatable value reaching MIR has a verified abstract category/class/
  slot assignment. Capacity exhaustion is handled by BIR-owned abstract
  spill/reload insertion across normal, retry, eviction, and fallback routes.
- `PreparedBir` owns no duplicate instruction graph. `MirReadyBirView` reads
  the exact allocated immutable BIR revision to which all typed facts bind.
- MIR maps abstract assignments to concrete RV64/AArch64/x86 ABI registers and
  performs instruction selection without rerunning ordinary allocation.
- Any backend legalization spill/reload is explicitly classified, bounded,
  tested, and unable to mask an allocation that BIR capacity facts could have
  resolved.
- Multi-output and `UseDef` inline asm preserve distinct source/result/
  destination identities; allocation ties constrain abstract slots without
  collapsing those identities.
- Original inline-asm payload and source identities survive unchanged;
  structured requirements alone drive allocation; invalid payloads fail only
  at late assembly.
- Build metadata excludes legacy/prealloc/old-MIR sources and the
  supervisor-selected regression proof is green.

## Reviewer Reject Signals

- `PreparedBir` or MIR-ready state copies/rebuilds a second instruction graph
  instead of viewing one immutable allocated BIR revision.
- MIR-ready BIR contains target opcodes, concrete register names/numbers,
  encoding details, or nodes outside the closed admission table.
- Any allocatable value reaches MIR without an abstract assignment, or normal
  pool exhaustion is deferred to a target backend.
- Backend spill/reload acts as a general allocator, conceals incorrect BIR
  capacity accounting, or has no narrow legalization-only invariant.
- Pool counts, reserved slots, group rules, ties, clobbers, or allocation facts
  are stored in Canonical BIR or guessed from inline-asm strings downstream.
- LIR-to-BIR or Canonical BIR normalizes `r`, `VR`, `VRM*`, aliases, clobbers,
  or target-specific register meaning.
- Any pre-assembler stage parses inline-asm mnemonics, directives,
  placeholders, `.insn`, or encoding fields.
- `VRM1` is accepted, or reviewed class/group tables are widened by guesswork.
- A named testcase shortcut, expectation downgrade, helper rename, or
  diagnostic reclassification is claimed as capability progress.
- Deleted BIR-owned MIR documents or legacy/prealloc sources are restored, or
  a replacement MIR filesystem owner is invented without a separate explicit
  architecture decision.
- Implementation begins before architecture acceptance, or unrelated backend
  work is mixed into this route.
