# RV64 Single-Line Assembler Core For Inline Asm

Status: Active
Source Idea: ideas/open/349_rv64_single_line_assembler_core_for_inline_asm.md

## Purpose

Extract the c4c-owned RV64 single-line assembler core used by inline asm object
emission, then route source-level inline asm object bytes through substituted
canonical assembly text instead of carrier-only encoding.

Goal: after inline asm operand substitution, object emission parses each
supported instruction line with the shared RV64 line core and encodes the
resulting structured instruction.

## Core Rule

Do not add another `.insn.d` object special case outside the shared RV64 line
parser/encoder. Inline asm constraints, operand homes, and template
substitution remain outside the line core.

## Read First

- `ideas/open/349_rv64_single_line_assembler_core_for_inline_asm.md`
- current RV64 inline asm substitution and object emission code
- current RV64 object emission tests for `.insn.d`, `li`, and `ret`

## Current Targets

First supported instruction subset:

- `.insn.d <major>, <op>, <vd>, <vs1>, <vs2>, <vs3>, <dtype>`
- `li <gpr>, <signed-imm>` for the currently emitted simple immediate shape
- `ret`

The line core should accept physical register names and immediates directly,
fail closed on malformed input, and encode through the existing RV64 helper
logic used by object emission.

## Non-Goals

- full `.s` file parsing, labels, sections, directives, comments, or relocation
- public `c4c-as` behavior beyond placeholders
- object disassembly
- general RISC-V assembler compatibility
- named GNU inline asm operands or template modifiers unless already supported
  by substitution

## Working Model

Desired object path:

```text
source inline asm
  -> C/C++ parser captures template, constraints, operands
  -> prepared inline asm carrier assigns homes/registers/immediates
  -> substitute_prepared_riscv_inline_asm_operands(...)
  -> parse_rv64_asm_line(...)
  -> encode_rv64_asm_line(...)
  -> object bytes
```

`.s` printing may continue to print substituted canonical text, but object
emission must consume the same parseable line text.

## Execution Rules

- Keep the line parser target-local and independent of C/C++ expressions,
  inline asm constraints, and VRM allocation.
- Prefer a small structured AST or equivalent parsed-instruction type over
  rendered-text substring probes.
- Share register-name and immediate parsing across `.insn.d`, `li`, and `ret`
  where applicable.
- Preserve existing deterministic object bytes:
  `0a0320080b0300001305000067800000`.
- Add fail-closed tests for invalid register names, malformed field counts, and
  out-of-range immediates.
- For code slices, prove with `build -> narrow test`, then add a broader RV64
  object-emission check when shared emission behavior changes.

## Steps

### Step 1: Locate Existing RV64 Inline Asm Surfaces

Goal: identify the current substitution, text printing, object encoding, and
test surfaces before moving behavior.

Primary targets:

- RV64 inline asm substitution helpers
- RV64 object emission helpers
- tests covering source-level `.insn.d` object bytes and `.s` text

Actions:

- inspect the current source-level inline asm object path
- identify where prepared carrier metadata is currently encoded directly
- identify existing helpers for RV64 register names, immediates, and encoding
- record the narrow test command for the current `.insn.d` object route in
  `todo.md`

Completion check:

- `todo.md` names the implementation files and narrow proof command for the
  first code packet

### Step 2: Add The RV64 Line Instruction Type And Parser

Goal: parse the supported single-line instruction subset into structured RV64
line instructions.

Primary target:

- target-local RV64 assembler-line parsing code

Actions:

- define a reusable parsed-instruction type for `.insn.d`, `li`, and `ret`
- parse physical vector and general-purpose register names
- parse signed immediates with range diagnostics or fail-closed errors
- reject unknown mnemonics, malformed operand counts, and invalid registers
- add focused parser tests for accepted canonical forms and rejected malformed
  forms

Completion check:

- the parser accepts:
  - `.insn.d 10, 11, v6, v0, v2, v4, 3`
  - `li a0, 0`
  - `ret`
- malformed lines fail closed under narrow tests

### Step 3: Add The RV64 Line Encoder

Goal: encode parsed line instructions through the same RV64 helper logic used
by object emission.

Primary target:

- target-local RV64 line encoder

Actions:

- encode parsed `.insn.d` using the existing custom 64-bit instruction helper
- encode parsed `li` for the currently emitted simple immediate shape
- encode parsed `ret`
- keep unsupported immediates and instruction shapes fail-closed
- add focused encode tests for canonical bytes

Completion check:

- encoding the three canonical lines yields:
  `0a0320080b0300001305000067800000`

### Step 4: Rewire Inline Asm Object Emission Through The Line Core

Goal: make source-level inline asm object emission substitute to canonical text
and assemble that text through the RV64 line parser/encoder.

Primary targets:

- RV64 inline asm object emission route
- RV64 inline asm text substitution route

Actions:

- call `substitute_prepared_riscv_inline_asm_operands(...)` before object
  encoding
- split substituted inline asm into supported instruction lines
- parse and encode each line through the RV64 line core
- remove or bypass direct `.insn.d` carrier-field encoding in the object path
- keep diagnostics fail-closed for unsupported substituted lines

Completion check:

- source-level `.insn.d` inline asm still emits object text bytes
  `0a0320080b0300001305000067800000`
- object emission no longer encodes `.insn.d` by reading prepared carrier fields
  directly

### Step 5: Prove Shared Text/Object Compatibility

Goal: prove `.s` text output from inline asm is accepted by the same line core
used by object emission.

Primary targets:

- RV64 inline asm `.s` text tests
- RV64 line parser/encoder tests

Actions:

- add or update tests so emitted canonical `.s` instruction lines are parsed by
  the RV64 line core
- keep object-byte assertions at source level
- check nearby supported RV64 object emission tests for regressions

Completion check:

- narrow parser/encoder tests pass
- source-level inline asm object test passes
- RV64 object emission subset remains green

### Step 6: Broader Validation And Closure Review

Goal: establish that the shared line core is stable enough for dependent
`c4c-as` work.

Actions:

- run the supervisor-selected broader RV64/backend validation subset
- confirm fail-closed behavior for invalid registers, malformed field counts,
  and out-of-range immediates
- inspect the final diff for testcase-overfit or duplicated instruction parsing
- decide whether idea 349 is complete and ready to close

Completion check:

- validation logs are recorded in canonical proof files
- no reviewer reject signal from the source idea applies
- dependent idea 350 can start without duplicating instruction parsing
