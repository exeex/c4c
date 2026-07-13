# Active Inline-Assembly Inventory

## Result

The active frontend can produce `LirInlineAsmOp`, but that record is currently
an LLVM-rendered carrier rather than a lossless new-backend carrier. It has no
structured operand list and does not retain the original source constraint
spelling separately from LLVM compatibility spelling. The active new-BIR
importer rejects every inline-asm instruction as an unsupported ordinary
instruction.

Therefore Step 2 must first add separate opaque/source and compatibility
fields to LIR. It must not treat the current `asm_text`, `constraints`, or
`args_str` strings as semantic authority.

## Active source to HIR production

`src/frontend/parser/impl/statements.cpp` parses GNU extended asm into output
expressions, input expressions, quoted constraint strings, and clobber string
literals. `AsmOperand` is only `{constraint, expr}`; source operand names such
as `[name]` are not retained. `src/frontend/parser/ast.hpp::Node` stores the
constraints in output-then-input order and keeps the counts, but has no
structured operand-name field.

`src/frontend/hir/impl/stmt/stmt.cpp` lowers `NK_ASM` to
`src/frontend/hir/hir_ir.hpp::InlineAsmStmt`:

- output and input expressions remain separate vectors;
- `output_readwrite` is inferred by searching the source token for `+`;
- all operand constraints are joined into one comma-separated string;
- clobbers remain a distinct vector;
- `asm_template` is not the decoded source bytes: it has already passed
  through `rewrite_gcc_asm_template`;
- `.insn r` is parsed eagerly into `InlineAsmInsnRMetadata` and malformed
  shapes throw from the HIR lowerer.

`src/frontend/hir/hir_lowering_core.cpp::rewrite_gcc_asm_template` changes
`%0`/`%w0` into LLVM `${0}`/`${0:w}` syntax. This is useful compatibility
rendering, but it is not opaque transport. The eager
`parse_inline_asm_insn_r_metadata` in `src/frontend/hir/impl/stmt/stmt.cpp` is
also directly contrary to the late-parse contract. Step 2 must preserve the
decoded `decode_string_node(n->left)` byte sequence before either operation;
the old derived forms may survive only as compatibility output.

## Active HIR to LIR production

AST-backed inventory with `c4c-clang-tool-ccdb function-signatures` confirms
that `src/codegen/lir/hir_to_lir/stmt.cpp` defines the relevant active helpers:

- `rewrite_asm_constraints(const std::string&)` rewrites `g` to `imr`;
- `split_asm_constraints` is brace-aware;
- `llvm_output_constraint` turns `+` into `=` and adds indirect-memory `*`;
- `llvm_memory_input_constraint` renders memory inputs;
- `rewrite_inline_asm_mnemonics` changes x86 `yield` to `pause`;
- `StmtEmitter::emit_non_control_flow_stmt(const InlineAsmStmt&)` renders the
  final LLVM call carrier.

That emitter also escapes bytes for LLVM string syntax, converts a scalar
read/write output into `=` plus a numeric matching input, concatenates
preformatted LLVM argument text, and appends clobbers as `~{name}` tokens.
These are compatibility transformations, not the original semantic carrier.

`src/codegen/lir/ir.hpp::LirInlineAsmOp` currently contains:

- `result` and `ret_type`;
- rendered `asm_text`;
- rendered `constraints`;
- `side_effects`;
- preformatted `args_str`;
- structured clobber strings;
- optional early-parsed `.insn r` metadata.

It has no structured input/output values and no original source constraint
tokens. `src/codegen/lir/lir_printer.cpp::render_inst` prints these rendered
strings directly as LLVM inline-asm syntax. The printer is an LLVM observation
point, not a new-backend semantic source.

## Active LIR verification

AST-backed inventory locates `count_inline_asm_constraints` and
`verify_inst` in `src/codegen/lir/verify.cpp`. For `LirInlineAsmOp`, verification
checks only result/return-type consistency and the range of optional early
`.insn r` operand indices. It does not verify operand roles, ties, register
classes, group widths, early clobbers, structured clobber agreement, or
original-versus-rendered spelling.

The current verifier therefore neither supplies nor protects the facts needed
by regalloc. Step 2 must verify the new structured source carrier while still
leaving instruction text uninterpreted.

## Active new-BIR rejection boundary

AST-backed inventory of `src/backend/bir/lir_to_bir.cpp` confirms that
`validate_function` rejects any nonempty `LirBlock::insts` with
`ImportErrorCode::UnsupportedOrdinaryInstruction`. Since inline asm is a
`LirInst`, it cannot reach the new `RawBir` today. The old
`src/backend/bir/lir_to_bir/calling.cpp` classifier is not called by this
importer and is not compiled into the active backend.

The Step 2 admission owner is the active `lower_lir_to_raw_bir` boundary. It
must normalize only source constraint records and publish verified BIR facts.
It must never inspect the opaque template.

## Payload preservation contract

The authoritative payload origin is the decoded byte string returned by
`decode_string_node(n->left)`, before GCC-placeholder rewriting, LLVM escaping,
target mnemonic aliases, or `.insn` recognition. Lexical quote and C escape
spelling are not retained; the decoded bytes are.

The required byte-equality observations are:

1. HIR `InlineAsmStmt::opaque_template`;
2. LIR `LirInlineAsmOp::opaque_template`;
3. BIR `InlineAsmInst::opaque_template` through `BlockView`;
4. MIR `MirInlineAsm::opaque_template` before allocation and after allocation.

All four byte spans must be equal in length and content. LLVM printer output
uses a separate escaped/rendered field and is excluded from this equality.
Only `LateAssembler::assemble_inline_asm` may substitute operands and parse the
result. An invalid mnemonic or malformed `.insn` must survive observations
1-4 and fail there.

## Historical/reference-only evidence

The following sources are uncompiled or quarantined. They explain prior
behavior but are not implementation authority:

- `src/backend/bir/lir_to_bir/calling.cpp` classified `r`, `=r`, `+r`, numeric
  ties, clobbers, and `VR`/`VRM*`; it also parsed template modifiers and `.insn`
  metadata too early.
- `src/backend/legacy/bir.hpp` carried old inline-asm metadata and VRM types.
- `src/backend/legacy/prealloc/**` derived register-group overrides and
  allocated candidate spans.
- `src/backend/mir/riscv/codegen/inline_asm.cpp`, `asm_emitter.cpp`, and object
  emission code performed string classification and substitution.

`docs/pre_regalloc_value_constraints/` proves an important negative lesson:
old `PreparedAllocationConstraint` rows were descriptive and assignment did
not consume them. The new allocator requirement must causally filter every
candidate path. `docs/prepared_mir_view_contract_research/` also classifies old
inline-asm carriers as feature-specific rather than core MIR authority.
`docs/rv64_explicit_register_inline_asm/` accurately traces the former
source-to-old-BIR route, but its old BIR classifier is no longer active.

## Evidence audit

Active symbols and paths used for this inventory:

- `src/frontend/parser/impl/statements.cpp::AsmOperand` and `NK_ASM` handling;
- `src/frontend/hir/hir_ir.hpp::InlineAsmStmt`;
- `src/frontend/hir/impl/stmt/stmt.cpp::parse_inline_asm_insn_r_metadata` and
  `Lowerer::lower_stmt`'s `NK_ASM` arm;
- `src/frontend/hir/hir_lowering_core.cpp::rewrite_gcc_asm_template`;
- `src/codegen/lir/hir_to_lir/stmt.cpp` helpers listed above;
- `src/codegen/lir/ir.hpp::LirInlineAsmOp`;
- `src/codegen/lir/lir_printer.cpp::render_inst`;
- `src/codegen/lir/verify.cpp::count_inline_asm_constraints` and `verify_inst`;
- `src/backend/bir/lir_to_bir.cpp::validate_function` and
  `lower_lir_to_raw_bir`;
- `src/backend/bir/lir_to_bir.hpp::ImportErrorCode`.
