# MIR-Ready Abstract BIR And Inline-Assembly Runbook

Status: Active
Source Idea: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Supersedes: the paused idea-730 globals packet; its WIP remains in `stash@{0}`

## Purpose

Define and then build a BIR-to-MIR boundary where the IR body is still BIR:
only verified abstract semantic nodes, with abstract physical registers and
normal spill/reload already resolved before target instruction selection.

## Goal

Publish a `MirReadyBirView` over one immutable allocated BIR revision, while
preserving inline asm opaquely and giving its structured constraints to the
BIR allocator.

## Core Rule

Canonical BIR has source semantics only. Target preparation injects typed
capacity/ABI facts; BIR owns normal allocation and spilling; MIR selects target
instructions and maps abstract assignments to concrete ABI registers.

## Read First

- `ideas/open/731_inline_asm_transport_and_regalloc_contract.md`
- `src/backend/bir/README.md`
- `src/backend/bir/pipeline/README.md`
- `src/backend/bir/REVIEW_TEMPLATE.md`
- `src/backend/bir/core/README.md`
- `src/backend/bir/verify/README.md`
- `src/backend/bir/preparation/README.md`
- `src/backend/bir/preparation/inline_asm/README.md`
- `docs/inline_asm_transport/`

## Current Targets

- accepted MIR-ready BIR node and stage/view contract
- target pool/capacity preparation for RV64, AArch64, and x86
- BIR-owned abstract allocation and spill/reload
- opaque inline-asm transport with structured allocation constraints
- MIR instruction selection and concrete ABI register mapping
- late assembler substitution and first parse

## Non-Goals

- Do not implement before Step 1 architecture acceptance.
- Do not put target facts in Canonical BIR or concrete registers/opcodes in
  MIR-ready BIR.
- Do not create a second prepared instruction graph.
- Do not make target backends the normal spill/reload owner.
- Do not parse inline-asm instructions before late assembly.
- Do not restore legacy/prealloc/old-MIR routes, deleted BIR-owned MIR docs, or
  apply `stash@{0}`.

## Working Model

1. Parser/HIR/LIR and Raw/Canonical BIR preserve opaque inline-asm payload,
   structured operands, and original constraint/clobber syntax.
2. Target preparation reads immutable verified Canonical BIR plus the selected
   target and produces revision-bound pool, ABI, class/group, tie, clobber,
   and capacity facts.
3. BIR allocation consumes those facts and transactionally publishes a new
   immutable BIR revision with abstract `(category, class/group, slot)`
   assignments and required abstract `Spill`/`Reload` nodes.
4. Prepared-input verification publishes a `PreparedBir` capability whose
   public boundary is `MirReadyBirView` over that exact allocated revision and
   its typed facts. It is not another IR.
5. MIR construction selects target instructions and maps abstract assignments
   to concrete RV64/AArch64/x86 registers under the calling convention.
6. Backend spill/reload is permitted only as a bounded final legalization or
   encoding fallback, never for ordinary capacity exhaustion.
7. Late assembly substitutes assigned registers and first parses the opaque
   inline-asm payload.

## Execution Rules

- Step 1 is design and independent review only. No implementation step is
  authorized until its completion check passes.
- Freeze an explicit closed node table; the named nodes are examples, not an
  implied exhaustive list.
- Define immutable revision creation and verifier gates rather than mutating a
  published Canonical BIR graph.
- Bind every target/preparation/allocation fact to the exact BIR revision and
  target-context identity/version.
- Prove ordinary pressure, retries, eviction, groups, and spill/reload all use
  the same capacity and legality model.
- Keep RV64 inline-asm evidence at `r`, `VR`, `VRM2`, `VRM4`, and `VRM8`;
  reject `VRM1` and unreviewed syntax.
- Do not choose a replacement MIR filesystem owner or recreate deleted
  `src/backend/bir/mir/**` documents in this runbook.
- Keep legacy translation units absent from compile metadata.

## Step 1: Freeze the MIR-ready abstract-BIR boundary

Goal: jointly accept the BIR node, target-preparation, abstract-allocation,
stage-view, MIR adjacency, and inline-asm contracts before implementation.

Primary targets:

- `docs/inline_asm_transport/`
- surviving BIR pipeline, preparation, and verification contracts
- the bounded BIR-to-new-MIR adjacency contract

Concrete actions:

- Freeze the closed MIR-ready abstract-node table, including abstract
  `InlineAsm`, `Spill`, and `Reload`, plus rejection of target opcodes and
  concrete physical registers.
- Define target-supplied caller-saved/callee-saved/temp capacities, reserved
  slots, ABI eligibility, register classes, and group rules for RV64, AArch64,
  and x86.
- Define abstract assignment identity `(category, class/group, slot)` and the
  invariants for all allocatable values, ties, clobbers, atomic groups, and
  abstract spill-slot identities without concrete frame offsets.
- Define BIR-owned allocation and capacity-driven spill/reload insertion as a
  transactional transformation producing a new immutable BIR revision.
- Normatively define `PreparedBir` as a capability, not an IR copy, and
  `MirReadyBirView` as a read-only view of that exact allocated revision plus
  typed revision-bound facts.
- Define the MIR adjacency: verified input view, target instruction selection,
  concrete ABI register mapping, output token/verifier, diagnostics,
  staleness, and transactional publication without inventing its filesystem
  owner.
- Bound backend spill/reload to final legalization/encoding constraints and
  require diagnostics/proof that it cannot hide ordinary BIR allocation
  failures.
- Preserve `InlineAsm` as an opaque BIR node; put structured constraint
  normalization in target preparation and instruction parsing only at late
  assembly. Keep source/output/result/destination identities distinct and
  treat ties as assignment equality rather than SSA identity.
- Repair the checkpoint proof matrix for stale/mismatched facts, incomplete
  assignments, capacity exhaustion, illegal groups, missing spills, forbidden
  nodes, and backend fallback misuse.
- Review the complete adjacency with `src/backend/bir/REVIEW_TEMPLATE.md` and
  record architecture-wide acceptance as an external prerequisite.

Completion check:

- Independent review accepts one coherent contract from Canonical BIR through
  target preparation, allocated immutable BIR publication,
  `MirReadyBirView`, and new-MIR construction; no open owner/token/verifier or
  fallback ambiguity remains. Step 2 is unauthorized until then.

## Step 2: Establish target-independent transport through Canonical BIR

Goal: publish source-semantic inline asm without target normalization.

Concrete actions:

- Add the smallest structured parser/HIR/LIR carrier for operand order,
  values, destinations, results, original constraints, and clobbers; keep LLVM
  rendering separate and non-authoritative.
- Preserve payload bytes exactly and remove early instruction parsing from the
  new-backend route.
- Add Raw/Canonical BIR node, builders, views, verification, results, and
  writebacks containing target-independent facts only.
- Test multi-output/UseDef identities, original syntax, malformed payload byte
  equality, and absence of target-normalized fields.

Completion check:

- Verified Canonical BIR retains exact source facts without target
  classification or instruction parsing.

## Step 3: Implement target-aware preparation

Goal: publish all capacity and constraint facts required by BIR allocation.

Concrete actions:

- Define target-context identity/version and reviewed abstract pool descriptors
  for RV64, AArch64, and x86.
- Normalize inline-asm roles, classes/groups, ties, early-clobbers, effects,
  and clobber units into immutable revision-bound facts.
- Reject unsupported syntax, stale revisions, target mismatch, impossible
  pool/group definitions, and `VRM1` with owner-specific diagnostics.
- Prove Canonical BIR remains unchanged and no downstream stage reads source
  constraint strings.

Completion check:

- Verified preparation contains every target fact needed for allocation while
  Canonical BIR remains target-independent.

## Step 4: Allocate abstract registers and publish MIR-ready BIR

Goal: make BIR the normal allocation and spill/reload owner.

Concrete actions:

- Assign every allocatable value an abstract category/class/group/slot under
  the prepared capacities and ABI constraints.
- Apply the same legality model in normal, retry, eviction, group, spill, and
  fallback paths; allocate groups atomically.
- Insert abstract `Spill`/`Reload` nodes for capacity pressure and verify their
  dominance, liveness, ownership, and slot consistency.
- Transactionally publish a new immutable allocated BIR revision; on failure,
  publish no partial graph or capability.
- Verify the closed node set and complete assignments, then publish
  `PreparedBir`/`MirReadyBirView` bound to the same revision and target facts.

Completion check:

- Direct BIR tests prove complete legal assignments and correct spills under
  scalar/group pressure; MIR-ready publication rejects every incomplete,
  stale, or forbidden graph.

## Step 5: Construct MIR and map concrete registers

Goal: lower verified abstract BIR without becoming a second allocator.

Concrete actions:

- Consume only verified `MirReadyBirView` and select target instructions.
- Map abstract category/class/group/slot assignments to concrete registers
  according to the selected RV64/AArch64/x86 calling convention.
- Preserve inline-asm payload/operand order and translate its verified
  assignments without reading source strings.
- Define and test the narrow final legalization spill/reload fallback; reject
  attempts to use it for ordinary capacity exhaustion or missing BIR spills.
- Publish verified MIR transactionally with explicit revision trace and
  diagnostics.

Completion check:

- Concrete mappings are deterministic and ABI-legal, and pressure already
  resolved by BIR cannot trigger ordinary backend allocation.

## Step 6: Add the late assembler seam

Goal: make late assembly the first inline-instruction parser.

Concrete actions:

- Accept opaque payload plus completed concrete operand assignments.
- Substitute the reviewed placeholder grammar, then parse mnemonics,
  directives, `.insn`, and encoding details.
- Prove invalid payloads survive earlier boundaries unchanged and fail here.

Completion check:

- Instrumentation and diagnostics identify late assembly as the first parser.

## Step 7: Prove the bounded end-to-end route

Goal: accept the route only when every ownership boundary works together.

Concrete actions:

- Exercise scalar, pressure/spill, calling-convention category, and admitted
  RV64 vector-group cases through transport, preparation, BIR allocation, MIR
  mapping, and late assembly.
- Verify payload equality, revision binding, abstract and concrete assignments,
  spill/reload ownership, substitution, and owner-specific negative failures.
- Confirm build metadata excludes legacy/prealloc/old-MIR sources and run the
  supervisor-selected broader regression guard.

Completion check:

- The bounded route is green and review finds no early parsing, target leakage
  into Canonical BIR, missing BIR allocation, backend second allocation, or
  testcase overfit.
