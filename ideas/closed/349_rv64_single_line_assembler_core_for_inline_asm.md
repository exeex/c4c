# RV64 Single-Line Assembler Core For Inline Asm

## Goal

Extract and complete the c4c-owned RV64 single-line assembler core used by
inline asm object emission.

The immediate target is to stop treating `.insn.d` inline asm object emission
as a carrier-only special case. After operand substitution, inline asm should
produce ordinary canonical assembly text and pass each instruction line through
the same RV64 line parser/encoder that future `c4c-as` will use.

## Motivation

The current custom vector route can already compile source-level inline asm to
object bytes and can print `.s` text. However, the implementation still has
separate paths:

- inline asm object emission classifies prepared carrier metadata directly
- inline asm text emission substitutes operands and prints text
- future `c4c-as` would need another parser if no shared line core exists

That split risks divergence between inline asm, assembler input, and future
objdump roundtrip output.

## Scope

Build a target-local RV64 assembly line parser/encoder layer that can be called
from inline asm after substitution.

First supported instruction subset:

- `.insn.d <major>, <op>, <vd>, <vs1>, <vs2>, <vs3>, <dtype>`
- `li <gpr>, <signed-imm>` for the currently emitted simple immediate shape
- `ret`

The line parser should accept physical register names and immediates directly.
It should not know about C/C++ expressions, inline asm constraints, or VRM
allocation. Those responsibilities remain in the inline asm carrier/substitution
path.

## Desired Flow

```text
source inline asm
  -> C/C++ parser captures template, constraints, operands
  -> prepared inline asm carrier assigns homes/registers/immediates
  -> substitute_prepared_riscv_inline_asm_operands(...)
  -> parse_rv64_asm_line(...)
  -> encode_rv64_asm_line(...)
  -> object bytes
```

The `.s` printer may continue to print substituted canonical text, but the
object path must prove it consumes the same parseable line text.

## In Scope

- A reusable RV64 line AST or equivalent structured parsed instruction type.
- Register-name parsing shared by `.insn.d`, `li`, and `ret` where applicable.
- Immediate parsing with range diagnostics/fail-closed behavior.
- Encoding through the same instruction helpers currently used by object
  emission.
- Rewiring inline asm object emission to assemble substituted text through this
  line core.
- Tests proving the old `.insn.d` object bytes are unchanged.
- Tests proving inline asm `.s` output is accepted by the same line parser.

## Out Of Scope

- Full `.s` file parsing, labels, sections, `.globl`, comments, or relocation.
- Public `c4c-as` behavior beyond placeholder targets.
- Object disassembly.
- General RISC-V assembler compatibility.
- Named GNU inline asm operands or template modifiers unless already supported
  by the substitution layer.

## Acceptance Criteria

- Source-level `.insn.d` inline asm still emits the existing deterministic
  object bytes:
  `0a0320080b0300001305000067800000`.
- Inline asm object emission no longer encodes `.insn.d` by reading prepared
  carrier fields directly; it first substitutes to canonical assembly text and
  then calls the RV64 single-line assembler core.
- The same line core can parse and encode:
  - `.insn.d 10, 11, v6, v0, v2, v4, 3`
  - `li a0, 0`
  - `ret`
- Invalid register names, malformed field counts, and out-of-range immediates
  fail closed.
- Existing RV64 object emission tests remain green.

## Reviewer Reject Signals

- The implementation adds another `.insn.d` special case outside the shared
  line parser/encoder instead of reducing duplication.
- `c4c-as` starts parsing whole `.s` files before the reusable line core exists.
- Inline asm constraints leak into the line parser.
- The proof only checks a fixture-built prepared module and skips source-level
  inline asm.
- Tests are weakened around object bytes or `.s` text output.

## Completion Note

Closed after adding the RV64 single-line parser/encoder, routing `.insn.d`
inline asm object emission through substituted canonical text and the shared
line core, and validating the backend subset with `319/319` passing tests.
