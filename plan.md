# Inline-Assembly Opaque Transport And Regalloc Runbook

Status: Active
Source Idea: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Supersedes: the paused idea-730 globals packet; its WIP remains in `stash@{0}`

## Purpose

Establish a new-backend inline-assembly route with opaque instruction payload
transport, structured pre-regalloc constraints, a constrained MIR pseudo,
allocation enforcement, and parsing only at a new late assembler seam.

## Goal

Carry inline asm unchanged through LIR -> BIR -> MIR while giving regalloc
enough verified structure to allocate `r`, `=r`, ties/clobbers, and the
target-confirmed `VR`/`VRM*` register groups correctly.

## Core Rule

Instruction text is opaque until late assembly; allocation constraints are
structured before regalloc.  Never make regalloc parse strings, and never
restore legacy BIR/prealloc/MIR/`c4c-as` sources to obtain this behavior.

## Read First

- `ideas/open/731_inline_asm_transport_and_regalloc_contract.md`
- `src/codegen/lir/ir.hpp`
- `src/codegen/lir/verify.cpp`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/lir_to_bir.cpp`
- `docs/rv64_explicit_register_inline_asm/`
- quarantined inline-asm/prealloc/MIR files only as historical behavior
  evidence, never as implementation authority

## Current Targets

- LIR inline-asm carrier and verifier
- new-BIR semantic schema, builder, view, verifier, and importer
- a new active MIR inline-asm pseudo and direct BIR-to-MIR boundary
- a bounded new allocator contract for scalar and vector register groups
- a new minimal late assembler seam
- retained boundary and allocator tests only

## Non-Goals

- Do not resume idea 730 globals from `stash@{0}` in this runbook.
- Do not revive prepared BIR, routes, prealloc, old MIR, or `c4c-as`.
- Do not implement general constraint syntax beyond the checkpoint table.
- Do not parse mnemonics, directives, or `.insn` payloads before late assembly.
- Do not broaden into unrelated ABI, object, linker, or runtime work.

## Working Model

1. LIR carries opaque asm payload, structured operand identity, raw constraint
   syntax at its designated normalization boundary, and separate clobbers.
2. LIR-to-BIR normalizes the admitted constraint subset into semantic operand
   roles and allocation requirements; BIR retains the opaque payload.
3. BIR-to-MIR creates an inline-asm pseudo with virtual operands and physical
   class/group constraints; it does not inspect instruction syntax.
4. Regalloc selects legal scalar registers or aligned contiguous groups while
   enforcing ties, interference, early-clobbers, and clobbers.
5. The late assembler substitutes assigned operands and only then parses and
   encodes the opaque payload.

## Execution Rules

- Finish Step 1 and review its table/schema before any implementation packet.
- Freeze exact supported `VR`/`VRM*` widths and alignment rules from current
  target semantics; stop on ambiguity instead of inventing a table.
- Add one layer boundary at a time and prove payload equality plus structured
  fact integrity at that boundary.
- Reject unsupported constraint alternatives with explicit diagnostics.
- Require allocator negative tests for illegal ties, overlaps, clobbers, and
  misaligned/noncontiguous groups; positive named cases alone are insufficient.
- Keep `src/backend/legacy/**` absent from compile metadata.
- Use the validation ladder `build -> direct boundary/allocator subset ->
  broader regression` at milestone steps.

## Step 1: Freeze inventory, constraint table, and schema checkpoint

Goal: produce the evidence-backed contract that implementation packets follow.

Primary targets:

- current source/HIR-to-LIR inline-asm production
- `LirInlineAsmOp`, its printer, and verifier
- new-BIR schema/import rejection boundaries
- current target definitions for GPR/vector classes and register-group rules
- historical inline-asm/prealloc/MIR behavior only where it provides evidence

Concrete actions:

- Trace payload, operands, constraint text, ties, and clobbers from source to
  current LIR and identify every place that currently parses or rewrites them.
- Inventory the active HIR-to-LIR LLVM-compatibility rewrites and decide how
  original source constraint spelling/roles remain distinct from rendered LLVM
  constraint text; do not silently make the compatibility spelling semantic
  authority for the new backend.
- Freeze the designated constraint-normalization owner and distinguish it from
  late instruction parsing.
- Publish a closed supported-constraint table covering at least `r`, `=r`,
  read/write and numeric/matching ties, early-clobber representation, clobber
  sets, `VR`, and every target-confirmed `VRM*` group.  Record role, class,
  width, alignment, contiguity, tie behavior, and rejection diagnostics.
- Determine the exact vector group set from current target semantics.  If
  source and target evidence disagree, record the ambiguity and stop rather
  than choosing a spelling-derived answer.
- Specify the minimal target-independent BIR inline-asm facts, new MIR pseudo,
  allocator constraint records, and late assembler input/output contract.
- Record byte-for-byte payload preservation points and prove no earlier pass
  needs mnemonic or `.insn` interpretation.
- Write the checkpoint under a new focused `docs/` directory and obtain
  reviewer acceptance before Step 2.

Completion check:

- A reviewer can derive every admitted class/group rule and diagnostic from
  cited current evidence, and can identify exactly which layer owns constraint
  normalization, allocation, substitution, and instruction parsing.

## Step 2: Establish structured LIR carrier and new-BIR import

Goal: publish verified inline-asm semantics without interpreting the asm
payload.

Concrete actions:

- Replace any preformatted operand string that acts as semantic authority with
  the smallest structured LIR operand carrier required by the checkpoint;
  preserve externally rendered text only as non-authoritative output.
- Keep asm payload bytes/text unchanged and keep clobbers structurally separate.
- Add the checkpoint-approved target-independent BIR inline-asm operation,
  operand roles, class/group requirements, ties, early-clobbers, and clobbers.
- Extend builders, read-only views, and verification before importer success.
- Normalize only the admitted constraint grammar at LIR-to-BIR.  Reject every
  unmodeled alternative/fact explicitly; do not silently fall back to GPR or
  width one.
- Add direct LIR-to-BIR tests for `r`, `=r`, read/write/ties, clobbers,
  accepted vector groups, unsupported alternatives, and byte-identical opaque
  payload transport.

Completion check:

- The direct LIR-to-BIR interface publishes verified structured requirements
  and unchanged payload for the admitted subset, while unsupported forms fail
  closed and no instruction syntax is parsed.

## Step 3: Add the new MIR inline-asm pseudo and BIR-to-MIR boundary

Goal: translate verified BIR semantics into allocatable machine requirements
without restoring old MIR or prealloc.

Concrete actions:

- Define the minimal new active MIR ownership/identity needed for an inline-asm
  pseudo with virtual inputs/outputs, class/group constraints, ties,
  early-clobbers, clobbers, and opaque payload.
- Lower the accepted BIR subset directly to that pseudo; preserve operand order
  and payload exactly.
- Reject BIR facts the new MIR/allocator contract cannot represent.
- Add direct BIR-to-MIR tests for scalar roles, ties/clobbers, every admitted
  vector group, unsupported facts, and byte-identical payload transport.

Completion check:

- Direct BIR-to-MIR proof shows complete structured requirements and unchanged
  payload in a new active MIR pseudo, with no legacy/prealloc compile entry.

## Step 4: Enforce inline-asm requirements in new regalloc

Goal: make structured constraints causally determine legal assignments.

Concrete actions:

- Implement candidate filtering for register class, reserved registers, group
  width, alignment, and contiguity from target definitions frozen in Step 1.
- Enforce input/output/read-write liveness, matching ties, interference,
  early-clobber non-overlap, and explicit clobber exclusion.
- Treat group allocation atomically; no partial or noncontiguous vector group
  may satisfy a `VRM*` requirement.
- Produce explicit allocation failure diagnostics when no legal assignment
  exists; never weaken the requirement.
- Add focused positive and negative allocator invariants for `r`, `=r`, ties,
  early-clobbers, clobber conflicts, pressure, and every admitted vector group
  width/alignment.

Completion check:

- Removing or changing a structured requirement changes allocation outcomes,
  and illegal overlap/misalignment/noncontiguity cases fail for the general
  invariant rather than a named testcase check.

## Step 5: Add the late assembler parse and substitution seam

Goal: make this the first and only instruction-syntax interpretation point.

Concrete actions:

- Define a minimal assembler API that accepts the opaque payload plus completed
  physical operand assignments and returns encoded output or a structured
  parse/encoding diagnostic.
- Substitute positional/named operands from assigned registers according to
  the checkpoint contract, then parse mnemonics/directives/`.insn` payload.
- Keep constraint normalization and allocation out of the assembler.
- Prove an invalid mnemonic or `.insn` payload crosses LIR, BIR, MIR, and
  allocation without interpretation and fails specifically at this seam.
- Add only the minimal positive encoding proof needed to establish timing.

Completion check:

- Instrumented or diagnostic evidence identifies late assembly as the first
  instruction parser, and completed allocation is required before substitution
  and encoding.

## Step 6: Prove the bounded end-to-end route

Goal: accept the route only when its interfaces and allocation semantics hold
together.

Concrete actions:

- Run source-backed scalar input/output/read-write cases and target-confirmed
  vector group cases through the bounded pipeline.
- Verify payload equality at LIR, BIR, and MIR observation points and correct
  allocated register substitutions at late assembly.
- Verify negative unsupported-constraint, tie/clobber conflict, illegal vector
  group, and late parse/encoding diagnostics.
- Confirm durable backend tests remain limited to LIR-to-BIR, BIR-to-MIR,
  allocator invariants, and the minimal late assembler/end-to-end seam.
- Confirm compile metadata has no legacy/prealloc/old-MIR source and run the
  supervisor-selected broader regression guard.

Completion check:

- The bounded route is green end to end, its negative contracts fail at the
  correct owners, legacy sources remain quarantined, and reviewer finds no
  string guessing or testcase overfit.
