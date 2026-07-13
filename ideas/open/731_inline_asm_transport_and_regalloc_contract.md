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

- Parser/HIR/LIR and pre-allocation BIR preserve the original opaque asm text,
  original opaque constraint text, ordered clobbers, and side-effect flags.
  LLVM-compatible spelling is a non-authoritative rendering concern and must
  not overwrite those original semantic fields. Only the late assembler parses
  mnemonics, directives, placeholders, `.insn`, or encoding details.
- `InlineAsm` uses the ordinary instruction value model. Inputs are normal SSA
  operands/uses, outputs are normal SSA results/definitions, and a read/write
  operand is represented by an incoming operand plus a distinct produced
  result. There is no separate `InlineAsmOperand` value system or hidden
  variable-name binding table. SSA, phi, CFG, liveness, and value ownership use
  the same rules as every other instruction.
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
- `InlineAsm` remains one opaque abstract BIR node. Its payload owns only the
  original asm text, original constraint text, ordered clobbers, and
  side-effect flags; its inputs and outputs live exclusively in the containing
  instruction's ordinary operands/results. During BIR allocation, the
  constraint text is typed and interpreted against those ordered generic
  operands/results and reviewed target tables. A tie equates abstract
  assignments, not SSA identities.
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
- The target-independent LIR-to-Raw/Canonical-BIR carrier bootstrap is
  explicitly authorized before the broader boundary review: it may add typed
  abstract nodes, views, builders, verification, and lossless opaque
  inline-asm carriage, but Raw/Canonical publication must remain unallocated
  and target-independent. Register allocation, spill/reload, target budgets,
  MIR-ready publication, MIR, and late assembly remain unauthorized until the
  ordered architecture and repaired boundary are jointly reviewed.
- LIR-to-BIR does not parse `=r`, `r`, `VR`, `VRM2`, or any other constraint
  into target meaning, and it does not parse asm text or textual LLVM argument
  rendering. The assembler is the first stage to interpret the asm instruction
  or template text and substitute allocated registers.
- Concrete register names written directly into asm text, such as `a0` or
  `a1`, are not inspected, reserved, or protected by compiler passes; conflicts
  are the user's responsibility. Explicit constraints and clobbers remain
  compiler-enforced contracts.

## Progress Checkpoint

- Commit `ac2f344f2` completed the bounded LIR-to-BIR bootstrap: verified
  target-independent Raw/Canonical BIR has an opaque `InlineAsmNode`, and its
  builder, views, and verifier already support generic SSA operand/result
  edges.
- The current `LirInlineAsmOp` still exposes its result and arguments through
  LLVM-oriented `LirOperand`/`LirTypeRef` plus preformatted `args_str`, and the
  direct importer intentionally accepts only void/no-argument inline asm. It
  does not yet import structured LIR input identities or result identities into
  BIR `ValueId`s.
- HIR-to-LIR currently produces LLVM-compatible rewritten constraint and
  argument text as the active LIR fields. Preserving original asm/constraint
  authority separately from compatibility rendering, then wiring ordinary LIR
  uses/results into ordinary BIR operands/results, remains incomplete.
- The next bounded runbook completes only that structured LIR-to-BIR wiring.
  General instruction-family lowering and the later target preparation,
  allocation, spill/reload, MIR-ready publication, ABI mapping, and late
  assembly work remain separate deferred packets under this idea.

## In Scope

- Giving current `LirInlineAsmOp` structured ordinary LIR input/result
  identities, types, and roles sufficient for generic SSA transport, without a
  special inline-asm value family.
- Separating original asm/constraint semantic fields from non-authoritative
  LLVM-compatible rendering fields in HIR-to-LIR and the LLVM LIR printer.
- Transactionally mapping structured LIR inline-asm inputs/results to generic
  BIR operands/results, including incoming-use/distinct-produced-result
  read/write behavior, without parsing textual arguments.
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

- Allocation, spill/reload, target-budget, MIR-ready, MIR, or late-assembly
  implementation while the architecture/checkpoint is unaccepted. The bounded
  target-independent LIR-to-BIR carrier bootstrap above is the only exception.
- Compiling or restoring `src/backend/legacy/**`, old prealloc/MIR, removed
  `c4c-as`, or deleted `src/backend/bir/mir/**` documents.
- Target interpretation in parser, HIR, LIR, LIR-to-BIR, or Canonical BIR.
- General LIR-to-BIR lowering for `Add`, `Load`, `Store`, `Call`, `Phi`,
  globals, stack objects, or unrelated instruction families as part of the
  structured inline-asm packet.
- Concrete physical register identifiers or target opcodes in MIR-ready BIR.
- Using MIR/backend spilling as a second normal allocation path.
- Parsing inline-asm instructions before late assembly.
- General GCC/LLVM constraint compatibility beyond reviewed target tables.
- Resuming idea 730 globals work or broad ABI/object/linker/runtime bring-up.

## Acceptance Criteria

- `LirInlineAsmOp` exposes ordinary structured input/result identities and
  types, while original semantic asm/constraint text is distinct from any
  LLVM-compatible rendering. No semantic consumer must recover values from
  `args_str` or other preformatted text.
- LIR-to-BIR maps inline-asm inputs to ordinary BIR operands and outputs to
  ordinary BIR results. A read/write case proves an incoming old value and a
  distinct produced new value that later SSA/phi users can consume.
- BIR inline-asm payload contains only original opaque asm/constraint text,
  ordered clobbers, and side-effect flags; allocation interpretation remains
  deferred to BIR regalloc and asm-text interpretation remains deferred to the
  assembler.
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
- At the final implementation checkpoint,
  `src/backend/bir/core/README.md`,
  `src/backend/bir/lir_to_bir/README.md`, and
  `src/backend/bir/verify/README.md` are reconciled against the implementation.
  The runbook closure note enumerates every remaining implementation/README
  mismatch or explicitly says none were found, and distinguishes intentional
  deferred scope from accidental desynchronization.

## Reviewer Reject Signals

- A new `InlineAsmOperand` value system, binding table, or variable-name lookup
  duplicates ordinary LIR/BIR SSA operands and results.
- LIR-to-BIR parses `args_str`, result spelling, constraint text, or asm text to
  reconstruct semantic values or target allocation meaning.
- LLVM-compatible asm/constraint/argument rendering overwrites or becomes the
  authority for original semantic text.
- A read/write operand reuses one SSA identity for both old use and new
  definition, or output/result/destination identities are collapsed by a tie.
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
- README text claims unimplemented structured wiring, hides known drift, or a
  closure note labels accidental implementation/documentation mismatch as
  merely deferred work.
