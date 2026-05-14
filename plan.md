# AArch64 Natural Operator Naming And Printer Spelling Plan

Status: Active
Source Idea: ideas/open/219_aarch64_natural_operator_naming_and_printer_spelling.md

## Purpose

Make the AArch64 MIR / structured asm operator surface use natural names that
stay close to printed assembly, while leaving exact encoding-form
canonicalization to the assembler/encoder layer.

## Goal

Define and apply a narrow naming contract where operator enum names, diagnostic
strings, and printer mnemonics share one supported spelling table.

## Core Rule

Developer-facing AArch64 operator names should use readable mnemonic or alias
names such as `Mov`, `Cmp`, and `Tst` when practical; encoder internals may
canonicalize those names later into concrete encoding families.

## Read First

- `ideas/open/219_aarch64_natural_operator_naming_and_printer_spelling.md`
- `src/backend/mir/aarch64/MACHINE_INSTRUCTION_NODE_CONTRACT.md`
- `src/backend/mir/aarch64/codegen/records.md`
- AArch64 printer, diagnostics, assembler, and encoder headers touched by idea
  218.

## Current Targets

- AArch64 stream item kind naming.
- Natural AArch64 operator kind naming at the MIR / structured asm boundary.
- Printer mnemonic spelling for the supported subset.
- Diagnostic strings for supported enum/operator names.
- Centralized `kind -> string` helpers used by printer and diagnostics.
- Focused proof that supported operator names and printed spellings do not
  drift.

## Non-Goals

- Do not implement the full assembler, encoder, object writer, linker, or
  binary emitter.
- Do not require MIR authors to name every concrete AArch64 encoding form.
- Do not replace the terminal `.s` printer with an internal parser.
- Do not expand instruction selection coverage just to add more operator names.
- Do not accept external `.s` or `.ll` as backend input.
- Do not touch `ideas/open/220_aarch64_markdown_driven_backend_case_bringup.md`.

## Working Model

- Stream item kinds describe assembly-stream structure: section, label,
  instruction/operator, directive, data, alignment, and relocation need.
- Natural operator kinds are the MIR / structured asm authoring surface.
- Printer mnemonics are the external textual assembly spelling for supported
  operators.
- Encoder canonical forms are later internal encoding-family choices.
- Aliases may be natural operator names when they are valid and clearer at the
  MIR / structured asm layer.

## Execution Rules

- Keep changes behavior-preserving unless a step explicitly calls for a
  supported spelling correction.
- Centralize spelling through helper tables instead of adding scattered switch
  fragments in lowering code.
- Preserve idea 218's boundary: printed `.s` is terminal renderer output, not
  semantic backend input.
- Prefer small packets with build or focused compile proof after code changes.
- Treat expectation downgrades, testcase-shaped shortcuts, or broad instruction
  selection expansion as route drift.

## Steps

### Step 1: Audit Current AArch64 Naming And Spelling Surfaces

Goal: identify where code-facing operator names, printer mnemonics, and
diagnostic strings currently live and where they diverge.

Primary targets:
- `src/backend/mir/aarch64/`
- AArch64 machine-node, structured asm/encoding, printer, and diagnostic enum
  declarations or helpers.

Actions:
- Inspect current enum and record names for stream items and operator-like
  concepts.
- Locate printer mnemonic switches or tables.
- Locate diagnostics that spell enum/operator names.
- Record whether supported names already have a shared spelling source.
- Do not edit implementation code in this audit step unless needed to add a
  narrowly scoped note to `todo.md`.

Completion check:
- `todo.md` names the audited files, the supported naming surfaces, and the
  concrete drift or duplication targets for later steps.

### Step 2: Document The Naming Tiers And Alias Ownership

Goal: make the contract explicit before changing live names or spelling
helpers.

Primary targets:
- `src/backend/mir/aarch64/MACHINE_INSTRUCTION_NODE_CONTRACT.md`
- `src/backend/mir/aarch64/codegen/records.md`
- AArch64 assembler/encoder contract headers if they already carry relevant
  boundary wording.

Actions:
- Document stream item kinds, natural operator kinds, printer mnemonics, and
  later encoder canonical forms as separate tiers.
- State that natural aliases such as `Mov`, `Cmp`, and `Tst` are valid at the
  MIR / structured asm layer when they improve readability.
- State that assembler/encoder canonicalization owns resolving aliases into
  concrete encoding forms.
- Reaffirm that printed `.s` is not semantic backend input.

Completion check:
- Focused text search proves the tier names, alias examples, and ownership
  wording are present in the relevant contracts.

### Step 3: Introduce Or Normalize Central Spelling Helpers

Goal: provide one supported source for operator-kind diagnostic and printer
spelling.

Primary targets:
- AArch64 structured asm/operator enum declarations.
- Nearby implementation files for printer and diagnostics.

Actions:
- Add or normalize `kind -> string` helpers for supported stream item and
  operator kinds.
- Use natural AArch64 names for MIR / structured asm operators where the source
  idea calls for readability.
- Keep helper names and locations consistent with existing AArch64 code style.
- Avoid adding unsupported instruction-selection coverage.

Completion check:
- The project builds with the new helper surface.
- The helper table covers the supported operator subset without scattered
  duplicate spelling switches.

### Step 4: Route Printer And Diagnostics Through Shared Spelling

Goal: make printed mnemonics and diagnostic strings consume the centralized
spelling source.

Primary targets:
- AArch64 machine printer.
- AArch64 diagnostics or dump helpers that render operator kinds.

Actions:
- Replace duplicate spelling logic with calls to the shared helper where
  practical.
- Preserve existing valid printed assembly spelling.
- Keep any necessary exceptions documented near the helper or in the contract.

Completion check:
- Build proof succeeds.
- Focused search or tests show printer and diagnostics no longer maintain
  divergent spelling tables for the supported subset.

### Step 5: Add Representative Drift Guards

Goal: prove that supported code-facing names and printed spelling stay aligned.

Primary targets:
- Existing AArch64 backend tests or compile checks that can exercise the
  supported printer subset.
- Minimal helper-level tests if the project already has a suitable pattern.

Actions:
- Add focused tests or compile checks for representative supported operators,
  including at least one alias-like natural name if live code supports it.
- Keep tests semantic to the spelling contract; do not widen backend
  instruction selection only to satisfy this step.
- Do not downgrade existing expectations.

Completion check:
- The focused test or compile-check subset passes.
- The proof would fail if a supported operator's code-facing name and printer
  spelling drifted unexpectedly.

### Step 6: Final Contract And Validation Pass

Goal: verify the activated idea's acceptance criteria are met without expanding
scope.

Actions:
- Re-read idea 219 acceptance criteria and reviewer reject signals.
- Confirm the contract distinguishes all naming tiers.
- Confirm alias ownership remains in assembler/encoder canonicalization.
- Confirm printer and diagnostics use centralized mappings for the supported
  subset.
- Run the supervisor-delegated broader validation appropriate for the final
  code/doc blast radius.

Completion check:
- `todo.md` records final proof commands and results.
- No active acceptance criterion from idea 219 remains unaddressed.
