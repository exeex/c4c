Status: Active
Source Idea Path: ideas/open/105_hir_to_lir_text_id_bridge_inventory_and_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Classify Payload, Printer, Diagnostic, Label, And Field Strings

# Current Packet

## Just Finished

Completed `plan.md` Step 4 classification for payload, printer, diagnostic,
label, field, designator, string-pool, and inline-asm string seams without
changing code.

- Field/member lookup strings are mixed identity and fallback text. Runtime
  member access prefers `MemberExpr::member_symbol_id` through
  `resolve_field_access_by_member_symbol_id()` when present, then falls back to
  `MemberExpr::field`/`HirStructField::name`; const-init and designator paths
  still resolve `InitListItem::field_designator_text_id` or legacy
  `field_designator` against `HirStructField::name`. Replacement authority for
  field identity is `MemberSymbolId` for member expressions and a structured
  field/designator owner for initializer items; until initializer designators
  carry that owner, those lookups are `temporary-rendered-lowering`.
- Field indices, GEP indices, extract/insert-value indices, bitfield masks,
  offsets, and struct literal field values are payload/lowering operands, not
  semantic names. Existing rendered strings such as `LirGepOp::indices` and
  const-init field literal fragments are `temporary-rendered-lowering` because
  they are final LLVM operand text today; replacement authority would be
  typed index/value operands plus byte-for-byte LIR printer proof.
- Block labels created as `entry`, `block_N`, and `%ulbl_<user-label>` are
  control-flow label text. LIR terminators and PHI/switch incoming pairs use
  label strings as printer/control-flow operands, not source symbol identity.
  User label names in `FnLoweringCtx::user_labels` and `LabelAddrExpr::label_name`
  are still source-label lookup text; replacement authority would be a HIR
  label ID or block owner key. Defer as `temporary-rendered-lowering` with
  focused computed-goto/label-address proof before changing spelling.
- String literal bytes are literal payload. `bytes_from_string_literal()`,
  `decode_c_escaped_bytes()`, `LirModule::intern_str(raw_bytes)`,
  `str_pool_map`, and `LirStringConst::raw_bytes` are byte-content authority and
  should remain strings/bytes. `LirStringConst::pool_name` is generated printer
  spelling for private constants, not source identity; replacing it would need a
  string-constant ID owned by `LirModule` plus exact emitted LLVM proof.
- Literal payload strings in constant initializers, integer/float textual
  constants, `zeroinitializer`/`null`/`undef`/`poison`, and final LLVM constant
  fragments are payload or printer-only text. They must not be swept into
  symbol identity cleanup.
- Inline asm template and constraint strings are literal payload. Lowering
  rewrites/escapes `InlineAsmStmt::asm_template` and `constraints` into
  `LirInlineAsmOp::asm_text` and `constraints`; the printer emits them verbatim
  inside the LLVM asm operand. Only `args_str` is `temporary-rendered-lowering`
  because it is preformatted typed operand text; replacing it requires
  structured asm argument operands and inline-asm output parity proof.
- Diagnostic/debug text is diagnostic-debug only. `LirVerifyError`,
  `fail_verify()`, `std::runtime_error` messages, and codegen compare
  `std::cerr` strings are not lookup authority and should remain plain text.
- Final LIR/LLVM printer operands are printer-only or temporary rendered
  lowering, depending on whether they are already typed. `LirOperand` and
  `LirTypeRef` classify and verify text wrappers, but printer fields such as
  `LirRet::value_str`, `LirCallOp::args_str`, `LirGepOp::indices`,
  `LirPhiOp::incoming` values, `LirGlobal::init_text`, and
  `LirFunction::signature_text` are still rendered LLVM operand text. Replace
  only with explicit structured operands/types and focused printer parity proof.

## Suggested Next

Proceed to `plan.md` Step 5 with one low-risk identity cleanup, preferably a
handoff that already has structured authority (`LinkNameId`, `TextId`, or
`MemberSymbolId`) and a narrow LIR/codegen proof command selected by the
supervisor.

## Watchouts

- Do not replace string literal bytes, inline asm text, diagnostics, final
  printer text, or generated pool names as identity cleanup.
- Initializer field designators remain the clearest Step 4 deferred cleanup:
  they have `TextId` fallback but no confirmed structured field owner in the
  const-init seam. Treat any replacement as requiring explicit designator owner
  authority and proof for struct/union designated initializers, flexible-array
  initializers, and byte-encoded aggregate constants.
- User labels and label-address text need a HIR label/block owner before
  cleanup. Do not alter `%ulbl_` spelling or blockaddress/indirectbr output
  without focused computed-goto and label-address parity tests.
- `collect_inst_refs()` intentionally scans string-valued operands for global
  references during dead-internal elimination. Any Step 5 cleanup that changes
  rendered operand storage must also preserve this reference discovery path or
  replace it with structured reference collection.

## Proof

No tests run. This was a classification-only lifecycle/task-state update per
the delegated proof rule, and `test_after.log` was not created or modified.
