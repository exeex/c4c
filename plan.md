# Structured Inline-Asm LIR-to-BIR Wiring Runbook

Status: Active
Source Idea: ideas/open/731_inline_asm_transport_and_regalloc_contract.md

## Purpose

Finish the narrow gap between the existing generic-SSA BIR `InlineAsm` carrier
and the current string-oriented `LirInlineAsmOp`.

## Goal

Preserve original inline-asm semantic text in LIR, expose its values through
ordinary structured LIR uses/results, and import those identities into ordinary
BIR operands/results without parsing asm, constraints, or textual LLVM args.

## Core Rule

Inline asm is a normal multi-input/multi-result SSA instruction with an opaque
payload. Do not create an `InlineAsmOperand` value system and do not infer
semantic values from rendering strings.

## Read First

- `ideas/open/731_inline_asm_transport_and_regalloc_contract.md`
- `src/codegen/lir/ir.hpp`
- `src/codegen/lir/hir_to_lir/stmt.cpp`
- `src/codegen/lir/lir_printer.cpp`
- `src/codegen/lir/verify.cpp`
- `src/backend/bir/core/ir.hpp`
- `src/backend/bir/core/builder.hpp`
- `src/backend/bir/lir_to_bir.cpp`
- `src/backend/bir/verify/verifier.cpp`
- `tests/backend/bir/backend_lir_to_bir_interface_test.cpp`

## Current Checkpoint

- Commit `ac2f344f2` provides the BIR `InlineAsmNode` payload and generic BIR
  operand/result builder, view, and verifier support.
- `LirInlineAsmOp` still relies on LLVM-oriented `result`, `ret_type`, and
  preformatted `args_str`; the direct importer therefore accepts only the
  void/no-argument form.
- HIR-to-LIR currently writes LLVM-compatible rewritten constraint/argument
  text into the active LIR carrier instead of retaining separate original
  semantic authority.

## Non-Goals

- Do not implement general LIR-to-BIR lowering for arithmetic, memory, calls,
  phi, globals, stack objects, or other ordinary instruction families.
- Do not parse or type `=r`, `r`, `VR`, `VRM2`, or other constraints in
  HIR-to-LIR or LIR-to-BIR.
- Do not parse asm mnemonics, placeholders, hard-coded registers, directives,
  `.insn`, or encodings before the assembler.
- Do not implement target preparation, regalloc, spill/reload, MIR-ready
  publication, MIR selection, or ABI register mapping in this runbook.
- Do not restore legacy/prealloc/old-MIR sources, deleted BIR-owned MIR docs,
  or the paused idea-730 globals work.
- Do not broaden documentation edits beyond the three README files named in
  Step 5.

## Working Model

1. `LirInlineAsmOp` owns original opaque asm/constraint text, ordered
   clobbers, side-effect flags, and ordinary structured LIR uses/results with
   their types.
2. A read/write operand is one incoming ordinary use plus one distinct
   produced ordinary result. Allocation ties never merge SSA identities.
3. LLVM-compatible asm/constraint/argument spelling is a separate,
   non-authoritative rendering surface.
4. LIR-to-BIR resolves structured LIR value identities through the same value
   map used by ordinary SSA transport and appends one BIR `InlineAsm` with
   generic operands/results.
5. BIR regalloc later interprets constraint text against ordered
   operands/results and target tables. The assembler later parses asm text and
   substitutes assigned registers.
6. A concrete register written directly in asm text is not compiler-reserved;
   explicit constraints and clobbers remain compiler contracts.

## Execution Rules

- Prefer the existing ordinary LIR value and type carriers. Any inline-asm
  record may describe ordering/role, but it must reference ordinary values and
  must not become a parallel value-identity system.
- Preserve original asm and constraint bytes independently of LLVM rendering.
  Compatibility fields or helpers must be visibly non-authoritative.
- Keep import transactional: missing, duplicate, foreign, mistyped, or
  inconsistent values publish no partial Raw/Canonical BIR.
- Prove structured transport without enabling unrelated opcodes. An asm result
  feeding a later asm input is sufficient to establish an ordinary SSA chain;
  a read/write case must use an old input and produce a distinct new result.
- Retain explicit rejection for unsupported LIR shapes; do not parse
  `args_str` or manufacture identities from names to make a test pass.
- Use the supervisor-delegated proof commands. Record exact commands and
  results in `todo.md`.

## Step 1: Define the structured LIR inline-asm value contract

Goal: make current LIR represent inline-asm values and original text without
depending on LLVM argument rendering.

Primary targets:

- `src/codegen/lir/ir.hpp`
- `src/codegen/lir/verify.cpp`
- focused LIR verifier/model tests selected by the supervisor

Concrete actions:

- Add ordinary structured input identities and types, and ordinary result
  identities and types, to `LirInlineAsmOp` using the existing LIR value model
  wherever possible.
- Represent read/write as an incoming use plus a distinct produced result.
- Keep original opaque asm text, original opaque constraint text, ordered
  clobbers, and side-effect flags as semantic fields.
- Separate LLVM-compatible rendering fields from semantic authority; mark or
  name compatibility-only state so semantic consumers cannot mistake it for
  structured input.
- Extend LIR verification for value/type/role/order consistency without typing
  target constraints or parsing asm text.

Completion check:

- Focused tests prove ordinary input, output, and read/write shapes, reject
  malformed identity/type combinations, and show that no special
  `InlineAsmOperand` identity is required.

## Step 2: Populate semantics and compatibility rendering in HIR-to-LIR

Goal: make the producer preserve original authority while keeping the LLVM LIR
printer behavior available.

Primary targets:

- `src/codegen/lir/hir_to_lir/stmt.cpp`
- `src/codegen/lir/lir_printer.cpp`
- focused HIR-to-LIR and LIR-printer tests selected by the supervisor

Concrete actions:

- Populate structured ordinary LIR uses/results and their types from the HIR
  inline-asm declaration and expressions.
- Preserve original asm/constraint text separately from any LLVM-specific
  constraint, placeholder, mnemonic, escaping, or argument rendering.
- Keep LLVM rendering functional from explicit compatibility state or a
  renderer; do not let the printer's needs redefine semantic LIR authority.
- Preserve ordered clobbers and side-effect flags.

Completion check:

- Tests distinguish original text from LLVM output where rewriting is needed,
  and prove that structured values survive independently of `args_str`.

## Step 3: Import structured values into generic BIR SSA edges

Goal: replace the current void/no-argument adapter restriction with
transactional structured value mapping for inline asm.

Primary targets:

- `src/backend/bir/lir_to_bir.cpp`
- BIR builder/view/verifier files only if the existing generic contract exposes
  a real implementation gap
- `tests/backend/bir/backend_lir_to_bir_interface_test.cpp`

Concrete actions:

- Resolve structured LIR inline-asm input identities to ordinary BIR
  `ValueId`s and pass them through `InlineAsmSpec::inputs`.
- Translate structured result types and publish ordinary BIR instruction
  results; retain a value map so a later inline asm can consume an earlier asm
  result.
- Prove read/write as an incoming old BIR operand plus a distinct new BIR
  result that a later SSA use can consume.
- Copy only original opaque asm/constraint text, ordered clobbers, and
  side-effect flags into `InlineAsmNode`.
- Reject missing definitions, duplicate definitions, unsupported types, and
  textual-only value shapes transactionally. Never parse `args_str` or result
  spelling to recover a value.

Completion check:

- The direct interface test proves output-to-input chaining, distinct
  read/write identities, payload equality, stable views, and no partial
  publication on malformed structured input.

## Step 4: Prove the completed structured transport

Goal: establish that the bounded producer/printer/importer route is stable
without claiming general LIR-to-BIR support.

Concrete actions:

- Run the supervisor-selected backend-enabled configure/build command.
- Run the narrow direct LIR-to-BIR interface tests plus focused LIR verifier,
  HIR-to-LIR, and LLVM rendering tests changed by Steps 1-3.
- Escalate to the supervisor-selected broader regression guard if the touched
  LIR schema or printer has wider blast radius.
- Inspect failures for semantic route drift; do not weaken expectations or add
  named-case parsing shortcuts.

Completion check:

- The build and selected tests are green, proof commands/results are recorded
  in `todo.md`, and remaining failures are neither hidden nor reclassified as
  unsupported merely to finish the packet.

## Step 5: Reconcile BIR READMEs and prepare the closure audit

Goal: document only the final proven implementation and make any remaining
implementation/documentation drift explicit.

Primary targets (exact documentation scope):

- `src/backend/bir/core/README.md`
- `src/backend/bir/lir_to_bir/README.md`
- `src/backend/bir/verify/README.md`

Concrete actions:

- After Steps 1-4 are implementation- and proof-stable, reread the final code
  and update these three README files from implementation as source of truth.
- Document the actual generic operand/result contract, opaque payload fields,
  importer support/rejection boundary, transactionality, and verifier checks.
- Do not document planned regalloc, MIR, general opcode, or producer behavior
  as implemented.
- Add a `Closure Note Audit` to `todo.md` that enumerates every discovered
  remaining implementation/README mismatch, or explicitly says none were
  found. Classify each item as intentional deferred scope or accidental
  desynchronization; do not relabel accidental drift as deferred work.
- When this runbook is retired or the idea is eventually closed, the plan
  owner must preserve that audit in the source idea's closure/deactivation
  note.

Completion check:

- Exactly the three named README surfaces match the final implementation.
- `git diff --check` passes.
- The `Closure Note Audit` is explicit, distinguishes deferred work from
  accidental desynchronization, and is ready for lifecycle closure review.

## Deferred After This Runbook

General LIR opcode migration, target constraint typing, abstract register
allocation, spill/reload, MIR-ready BIR, concrete ABI mapping, and late
assembly remain under the source idea but are not authorized by this runbook.
