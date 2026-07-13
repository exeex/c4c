# Inline-Assembly Transport And Register-Allocation Contract

Status: Open
Type: backend semantic transport, machine allocation, and late assembly

## Intent

Build the new backend's inline-assembly route so instruction/template payloads
survive unchanged through LIR, BIR, and MIR and are interpreted only at a new
late assembler seam.  Separately normalize operand constraints early enough
that register allocation can correctly assign inputs, outputs, read/write
operands, tied operands, register classes, and contiguous register groups.

## Why This Idea Exists

The active new BIR currently supports only a minimal import subset, while the
old inline-assembly, preallocation, MIR, and assembler implementations are
quarantined legacy references.  Reintroducing those sources would restore the
architecture that the BIR rebuild intentionally removed.  At the same time,
carrying only raw constraint strings until emission would leave regalloc unable
to enforce `r`, `=r`, vector `VR`/`VRM*` groups, ties, early-clobbers, and
clobbered registers.  The route therefore needs two deliberately different
contracts: opaque instruction payload transport and structured allocation
requirements.

## Core Contract

- The asm template/instruction payload is opaque bytes or text across
  LIR -> BIR -> MIR.  No pass before the late assembler may interpret
  mnemonics, directives, `.insn` fields, or instruction encoding.
- Constraint syntax is not asm instruction parsing.  It must be normalized
  before regalloc into structured operand roles (input, output, read/write),
  register class, group width/alignment/contiguity, ties or matching
  constraints, early-clobber behavior, and clobber sets.
- The minimum named constraint families include GPR input `r`, GPR output
  `=r`, and vector classes/groups beginning with `VR` and `VRM2`.  The exact
  admitted `VRM*` set and its width/alignment rules must be frozen from current
  target semantics during the inventory checkpoint; they must not be guessed
  from token spelling or legacy implementation accidents.
- BIR owns target-independent inline-asm semantics and structured constraint
  requirements.  It may name a target register-class requirement without
  selecting a physical register or embedding an allocator plan.
- MIR owns an allocatable inline-asm pseudo with physical-class/group
  requirements and the unchanged opaque payload.  Regalloc must honor class,
  group width/alignment/contiguity, matching ties, interference,
  early-clobbers, and explicit clobbers.
- Only the late assembler substitutes assigned operands and parses/encodes the
  asm payload.  The assembler seam must be new active code and must not revive
  legacy `c4c-as`, prealloc, or MIR translation units.
- Unsupported alternatives or constraint features must produce explicit,
  stable diagnostics.  Regalloc may not infer requirements by inspecting raw
  strings, and no testcase-shaped special case may stand in for a semantic
  rule.

## In Scope

- An evidence-backed inventory and schema checkpoint covering the current
  source/HIR/LIR carriers, current target constraint meanings, active new BIR,
  the required new MIR pseudo, allocator invariants, and the late assembler
  boundary.
- A structured LIR operand carrier where needed to replace preformatted operand
  text as semantic authority, while preserving the asm payload verbatim and
  distinguishing source constraint spelling from LLVM-compatibility rendering.
- New-BIR inline-asm semantic nodes, constraint requirements, builders, views,
  verification, and direct LIR-to-BIR lowering.
- A bounded new active MIR representation and BIR-to-MIR lowering for the
  admitted inline-asm subset.
- Constraint-aware register allocation for scalar GPR and the target-confirmed
  vector register groups, including ties, interference, early-clobbers, and
  clobber sets.
- A minimal late assembler seam sufficient to prove that instruction parsing,
  operand substitution, and encoding occur only after allocation.
- Direct LIR-to-BIR and BIR-to-MIR tests, allocator invariant tests, and a
  bounded end-to-end proof.

## Out Of Scope

- Compiling, copying, wrapping, or re-exporting `src/backend/legacy/**`, the
  removed `c4c-as` target, or the old prealloc/MIR pipeline.
- Restoring prepared BIR, route tables, value homes, or legacy publication
  records as an intermediate architecture.
- Parsing instruction mnemonics or `.insn` payloads in LIR, BIR, BIR-to-MIR,
  or regalloc.
- General GCC/LLVM constraint compatibility beyond the explicitly inventoried
  and modeled subset.
- Object/linker/runtime bring-up unrelated to proving the minimal late
  assembler boundary.
- Continuing the paused globals migration from idea 730 as part of this route.

## Acceptance Criteria

- A reviewed checkpoint freezes the supported constraint table from current
  target semantics, including the exact admitted vector group widths and their
  alignment/contiguity rules, before implementation begins.
- The checkpoint decides how original source constraints survive independently
  of the current HIR-to-LIR LLVM-compatibility rewrites; rewritten printer text
  must not silently replace source semantic authority.
- An inline-asm payload can be compared byte-for-byte at LIR, published BIR,
  and pre-assembly MIR boundaries; no earlier layer parses instruction syntax.
- BIR exposes verified target-independent operand roles, class/group
  requirements, ties, early-clobbers, and clobber semantics without physical
  allocation state.
- BIR-to-MIR produces a new allocatable inline-asm pseudo carrying the opaque
  payload and structured requirements; unsupported facts fail closed.
- Regalloc proves correct handling for `r`, `=r`, read/write and matching ties,
  clobber/interference conflicts, and every accepted `VR`/`VRM*` group width
  and alignment rule.
- The late assembler receives allocated operands, performs substitution, and
  only then parses/encodes the payload.  At least one negative proof shows an
  invalid mnemonic or `.insn` payload survives earlier boundaries and fails
  specifically at this seam.
- Durable tests are limited to direct LIR-to-BIR, direct BIR-to-MIR, allocator
  invariants, and the smallest late-assembler/end-to-end proof required to
  establish parse timing.
- Default build metadata remains free of legacy/prealloc/old-MIR translation
  units, and the supervisor-selected broader regression proof is green.

## Reviewer Reject Signals

- Any pre-assembler pass tokenizes or interprets asm mnemonics, directives,
  `.insn` fields, or encoding details rather than transporting the payload
  opaquely.
- Regalloc scans constraint or asm strings, recognizes testcase spellings, or
  obtains register needs from anything other than verified structured facts.
- `r`, `=r`, `VR`, or a `VRM*` spelling is accepted by a named-case branch
  without a general role/class/group rule and nearby negative coverage.
- The exact vector group table is guessed, copied without evidence, or widened
  beyond current target semantics during implementation.
- Ties, early-clobbers, clobber sets, group alignment, or group contiguity are
  carried as decorative metadata but not enforced by allocator interference
  and candidate selection.
- An unsupported constraint alternative is silently weakened to `r`, width
  one, an untied operand, or an empty/clobber-free record.
- Tests are downgraded to unsupported, expectations are rewritten, helpers are
  renamed, or diagnostics are merely reclassified and claimed as capability
  progress without interface and allocation proof.
- Legacy `c4c-as`, BIR, prealloc, prepared, or MIR sources re-enter the build,
  even behind new facade names.
- Broad object emission, ABI, globals, or unrelated backend rewrites are mixed
  into the inline-asm route.
