# Source-to-BIR Inline-Assembly Constraint Syntax Flow

## Scope and finding

The source and LIR layers preserve constraint text broadly, but semantic BIR
admits only a closed token set. An explicit physical operand constraint such as
`{x10}` or `={x10}` is not lost by source parsing or LIR storage and is not
rejected by the LIR verifier; it is first semantically rejected in
`src/backend/bir/lir_to_bir/calling.cpp::make_inline_asm_metadata` as
`unsupported_constraint<index>:<token>`.

Clobbers follow a separate source field and semantic contract. A source clobber
name becomes both an entry in `LirInlineAsmOp::clobbers` and a rendered
`~{name}` constraint token. BIR admits that token as `Clobber` only when the
name is also present in the structured clobber vector. It never associates the
clobber with an input/output named value.

This document records current behavior only. Whether to add a narrow physical
operand syntax is reserved for Step 2.

## End-to-end flow

### 1. Source parser and HIR retain raw operand strings and separate clobbers

`src/frontend/hir/impl/stmt/stmt.cpp` handles `NK_ASM`:

- output and input expressions are lowered into `InlineAsmStmt::outputs` and
  `InlineAsmStmt::inputs`;
- each quoted operand constraint from `n->asm_constraints` is unquoted and
  concatenated into `InlineAsmStmt::constraints` without token classification;
- each source clobber is decoded independently into
  `InlineAsmStmt::clobbers`.

`src/frontend/hir/hir_ir.hpp::InlineAsmStmt` reflects that separation: one raw
comma-separated `constraints` string plus a distinct `clobbers` vector. It has
no structured physical-register operand identity.

Source-backed observations exist in `tests/backend/case/`:

- `inline_asm_input_i32.c` uses input constraint `"r"`;
- `inline_asm_output_readwrite_i32.c` uses read/write output `"+r"`;
- the corresponding semantic-BIR route tests are registered in
  `tests/backend/bir/CMakeLists.txt`.

These establish the real source route for class constraints, not a fixture-only
LIR constructor.

### 2. HIR-to-LIR preserves token bodies and renders operand roles

`src/codegen/lir/hir_to_lir/stmt.cpp::StmtEmitter::emit_non_control_flow_stmt`
for `InlineAsmStmt` performs these transformations:

- `rewrite_asm_constraints` preserves brace groups (and rewrites only `g` to
  `imr` outside them);
- `split_asm_constraints` splits commas only at brace depth zero;
- `llvm_output_constraint` changes leading `+` to `=` or inserts `=` for an
  output, while otherwise retaining the token body;
- scalar read/write output is represented as an output plus numeric tied input;
- input constraint text is appended unchanged;
- every structured clobber is rendered separately as `~{name}`.

The result is stored in `src/codegen/lir/ir.hpp::LirInlineAsmOp`, whose
`constraints` field remains a string and whose `clobbers` field remains a
separate vector. `src/codegen/lir/lir_printer.cpp` prints the constraint string
verbatim in the inline-assembly call.

Consequently, a hypothetical source input `{x10}` reaches LIR as `{x10}`, and
a scalar output reaches LIR as `={x10}`. This is text preservation, not semantic
support or target validation.

### 3. LIR verification checks shape, not constraint grammar

`src/codegen/lir/verify.cpp` handles `LirInlineAsmOp` by validating result/type
consistency. For optional RV64 `.insn r` metadata it calls
`count_inline_asm_constraints` and checks that referenced operand indices are
within the comma-counted list.

The verifier does not classify `r`, `VRM2`, `{x10}`, or `~{x10}` and does not
cross-check the clobber vector. Therefore explicit-register text survives this
phase without gaining authority. A malformed or unsupported operand token is
not diagnosed until LIR-to-BIR classification.

### 4. LIR-to-BIR is the first closed semantic grammar

`src/backend/bir/lir_to_bir/calling.cpp::make_inline_asm_metadata` copies the
top-level raw constraint string, splits it into trimmed comma tokens, and builds
one `bir::InlineAsmOperandMetadata` per token. The initializer starts every row
as `InlineAsmOperandKind::Unsupported`; only explicit branches admit it.

`src/backend/bir/bir.hpp::InlineAsmOperandMetadata` publishes role, constraint
index/text, argument/output/tie indices, register class/group width, optional
clobber name, and memory/address facts. It currently has no physical target
identity field.

The exact explicit-register discontinuity is the final fallback in
`make_inline_asm_metadata`: `{x10}`, `={x10}`, `+{x10}`, and other unrecognized
tokens retain `Unsupported` kind and append
`unsupported_constraint<index>:<token>` to `InlineAsmMetadata::unsupported_facts`.

## Accepted and rejected token classification

| Token family | Current BIR result | Structured facts | Evidence |
|---|---|---|---|
| `r` | `RegisterInput` | next argument index; general class; width 1 | `make_inline_asm_metadata`; source case `inline_asm_input_i32.c` |
| `=r` | `RegisterOutput` | next output index; general class; width 1 | structured metadata test in `backend_lir_to_bir_notes_test.cpp` |
| `+r` | `RegisterOutput` with input and output indices | general class; HIR-to-LIR normally renders output plus numeric tie | source case `inline_asm_output_readwrite_i32.c`; read/write metadata test |
| `VR`, `VRM1`, `VRM2`, `VRM4`, `VRM8` with supported `=`/`+` forms | vector register input/output | vector class and group width 1/2/4/8 | `classify_inline_asm_vector_constraint`; RV64 vector metadata test |
| `i`, `I` | `IntegerImmediateInput` | next argument index | structured metadata test |
| `m` | `MemoryInput` | next argument index; memory facts attached later | classifier branch |
| `p` | `AddressInput` | next argument index; address facts attached later | classifier branch |
| decimal digits such as `0` | `TiedInput` | next argument index and parsed output index | structured metadata test |
| `~{name}` plus matching `LirInlineAsmOp::clobbers` entry | `Clobber` | optional name, no argument/output/value identity | clobber branch and structured `memory`/`cc` test |
| `~{name}` without matching structured clobber | `Clobber` row plus `unsupported_clobber_constraint<index>` | no authoritative name | `make_rendered_only_clobber_inline_asm_metadata_module` negative test |
| unsupported vector-looking forms such as `=VRM3` | `Unsupported` | `unsupported_vector_constraint<index>:<token>` | RV64 vector metadata test |
| `{x10}`, `={x10}`, `+{x10}` | `Unsupported` | `unsupported_constraint<index>:<token>`; no target identity | final classifier fallback |
| empty or any other token | `Unsupported` | `empty_constraint<index>` or generic unsupported fact | classifier fallback |

## Clobber semantics are separate machine-state declarations

The source parser never adds clobbers to operand expression lists. HIR and LIR
store clobber names separately, and HIR-to-LIR only renders `~{name}` after all
input/output constraints. BIR requires agreement between that rendered token
and the structured clobber vector before publishing the name.

`tests/backend/bir/backend_lir_to_bir_notes_test.cpp` proves both directions:

- the structured fixture with `~{memory},~{cc}` plus matching names produces two
  `Clobber` operands with no argument/output indices;
- the rendered-only `~{memory}` fixture with an empty clobber vector receives
  `unsupported_clobber_constraint1` and does not become structured authority.

Thus `~{x10}` means that inline assembly may alter machine register x10. It does
not mean an input or output value must be allocated to x10. Reusing this route
for named-value allocation would invert its semantics.

## Existing test evidence and gaps

`tests/backend/bir/backend_lir_to_bir_notes_test.cpp` directly proves:

- `=r`, numeric tie, immediate, and structured clobber preservation;
- `+r` read/write classification;
- RV64 `.insn r` general-register operands and index validation;
- supported and unsupported RV64 vector group tokens;
- rendered-only clobbers fail closed.

The source-backed semantic-BIR observation tests cover class-only x86 input,
output, read/write, pointer, and nop cases. No repository test was found for a
source physical-register operand constraint reaching LIR or BIR. There is also
no focused test that constructs `{x10}` at LIR and asserts the generic
unsupported fact; the rejection follows directly from the exhaustive branch
structure and final fallback.

## Earliest owner and discontinuity

The source parser, HIR, HIR-to-LIR lowering, `LirInlineAsmOp`, LIR printer, and
LIR verifier all preserve or tolerate physical operand text without assigning
semantic meaning. The earliest owner of structured operand grammar is
`make_inline_asm_metadata`, and it is the first rejection boundary.

Therefore the exact missing capability is not raw-string transport. It is a
semantic BIR classification and structured-identity contract for physical
input/output operands, together with any earlier source-validity decision that
Step 2 may require. The later
`src/backend/prealloc/inline_asm.cpp::rv64_inline_asm_register_identity` parses
register names only from already-assigned homes during carrier validation and
does not repair this ingress discontinuity.

## Evidence method

- AST definition lookup located HIR-to-LIR `emit_non_control_flow_stmt` and
  inventories localized the LIR-to-BIR classifier helpers.
- Repository searches covered `InlineAsmStmt`, `LirInlineAsmOp`, verifier use,
  `InlineAsmOperandMetadata`, every accepted classifier branch, unsupported
  facts, clobber agreement, and focused tests.
- Direct inspection was limited to those localized definitions and tests.
