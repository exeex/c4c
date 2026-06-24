Status: Active
Source Idea Path: ideas/open/343_rv64_consteval_inline_asm_template_strings.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Existing String And Inline Asm Surfaces

# Current Packet

## Just Finished

Step 1 mapped the existing inline asm and compile-time string surfaces.

Inline asm parse/build path:

- `src/frontend/parser/impl/statements.cpp` parses statement-level
  `asm`/`__asm__` in the `TokenKind::KwAsm` case. The template operand is
  accepted directly only when it is a `TokenKind::StrLit`, using
  `consume_adjacent_string_literal(parser)` and `parser.make_str_lit(...)`.
- The same parser case can syntactically parse a non-literal template
  expression with `parse_expr(parser)`, but it currently only creates
  `NK_ASM` when `asm_template->kind == NK_STR_LIT`; otherwise it returns
  `NK_EMPTY`, silently dropping the unsupported asm statement.
- Operand constraints and clobbers are still literal-only in that parser path:
  `parse_asm_operand` consumes literal constraints with
  `consume_adjacent_string_literal(parser)`, and clobbers are collected as
  `NK_STR_LIT` nodes.
- `src/frontend/hir/impl/stmt/stmt.cpp` lowers `NK_ASM` to
  `hir::InlineAsmStmt` by decoding `n->left` with
  `rewrite_gcc_asm_template(decode_string_node(n->left))`, then preserving
  outputs, inputs, constraints, clobbers, and `has_side_effects`.
- `src/frontend/hir/hir_ir.hpp` carries the final HIR template text in
  `InlineAsmStmt::asm_template`.
- `src/codegen/lir/hir_to_lir/stmt.cpp` consumes that final text in
  `StmtEmitter::emit_non_control_flow_stmt`, rewrites target mnemonics,
  escapes LLVM bytes, rewrites constraints, and emits `lir::LirInlineAsmOp`.
- `src/codegen/lir/ir.hpp` carries inline asm text in
  `LirInlineAsmOp::asm_text`; `src/backend/bir/lir_to_bir/calling.cpp`
  copies it into `bir::InlineAsmMetadata::asm_text` via
  `make_inline_asm_metadata`.

Compile-time string surface:

- `src/frontend/parser/impl/core.cpp::consume_adjacent_string_literal` already
  concatenates adjacent string literals into one raw literal spelling.
- `src/frontend/parser/impl/expressions.cpp` also uses
  `consume_adjacent_string_literal` for expression-level string literals, so
  adjacent literal concatenation already lands as one `NK_STR_LIT`.
- `src/frontend/hir/hir_lowering_core.cpp::decode_string_node` decodes an
  `NK_STR_LIT` into final text. This is the exact helper the current inline asm
  HIR path already uses.
- `src/frontend/sema/consteval.hpp`/`.cpp` currently model constant values as
  integer, boolean, null pointer, or unknown only; there is no existing
  string-valued `ConstValue`, fixed-string object representation, or
  compile-time string concatenation evaluator.
- Existing consteval call reduction in
  `src/frontend/hir/impl/expr/call.cpp::Lowerer::try_lower_consteval_call_expr`
  and `src/frontend/hir/impl/compile_time/engine.cpp` evaluates integer-like
  arguments/results through `evaluate_constant_expr`; it does not return a
  string template value.

Narrowest route selected for Step 2:

- First accepted surface: adjacent string literal template fragments in inline
  asm, because they already fold through
  `consume_adjacent_string_literal(parser)` to a single `NK_STR_LIT` before
  HIR lowering and preserve the existing literal asm path unchanged.
- Precise fold point for the next implementation slice:
  `src/frontend/parser/impl/statements.cpp`, inside the `TokenKind::KwAsm`
  case immediately after parsing `asm_template`. Add a small
  asm-template-only folding helper that either returns an `NK_STR_LIT` carrying
  final raw template spelling or reports a clear unsupported/non-constant
  template diagnostic before the `NK_ASM` construction gate.
- Downstream HIR/LIR/BIR paths should not need representation changes for this
  first slice, because they already consume final template text.

## Suggested Next

Delegate Step 2 to `c4c-executor`: in
`src/frontend/parser/impl/statements.cpp`, preserve literal asm behavior and
replace the silent non-literal `NK_EMPTY` fallback with an asm-template folding
gate that accepts only `NK_STR_LIT`/already-adjacent literal fragments for the
first slice, emits a specific diagnostic for runtime or unsupported template
expressions, and leaves operands, constraints, clobbers, volatility, and
metadata unchanged.

## Watchouts

- Do not accept runtime strings.
- Do not add EV intrinsics or compiler-known EV mnemonics as a substitute for
  consteval inline asm template support.
- Keep literal asm templates on the existing path.
- Preserve `%0` positional placeholders, operand order, constraints, clobbers,
  volatility, and memory metadata after folding.
- Unsupported tempting forms for now: `const char*` variables, local/global
  `constexpr` character arrays, pointer arithmetic on string literals,
  `operator+` on user fixed-string objects, `std::array<char, N>`,
  `std::string_view`, named GNU operands, `%c[...]` modifiers, and consteval
  functions that would require a string-valued `ConstValue`.
- The current parser silently drops unsupported asm-template expressions; Step
  2 should turn that into an explicit diagnostic rather than accepting empty
  asm or runtime values.
- A true consteval/helper string route will require a later extension to the
  constant-evaluation value model or a separate asm-template-only fold result;
  do not imply that the existing `ConstValue` machinery can already carry
  strings.

## Proof

Mapping-only packet. No code or tests were edited, so no validation was
required and `test_after.log` was not refreshed by this packet.
