Status: Active
Source Idea Path: ideas/open/105_hir_to_lir_text_id_bridge_inventory_and_cleanup.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Final Validation And Follow-Up Boundaries

# Current Packet

## Just Finished

Lifecycle review after `plan.md` Steps 1-4 found that Step 5 is already
satisfied by the committed function/global dedup cleanup in
`src/codegen/lir/hir_to_lir/hir_to_lir.cpp`:
`dedup_globals` and `dedup_functions` now prefer valid `LinkNameId`, then valid
`name_text_id`, then raw rendered `name` only when both stable ids are invalid.
The cleanup keeps ABI/link spelling and rendered-name fallback behavior intact.

Step 5 proof was recorded in commit `dfb16fb6`:
`cmake --build build --target c4cll && ctest --test-dir build -R '^(frontend_hir_tests|positive_sema_ok_call_target_resolution_runtime_c|positive_sema_inline_call_discovery_c|cpp_positive_sema_namespace_global_var_runtime_cpp|cpp_positive_sema_namespace_function_call_runtime_cpp|cpp_positive_sema_anon_namespace_fn_lookup_cpp|cpp_llvm_spec_key_metadata|cpp_llvm_forward_pick_specialization_metadata)$' --output-on-failure`.

Result there was green: `c4cll` rebuilt successfully and all 7 selected tests
passed.

Current lifecycle decision: keep the runbook active at Step 6 for final
validation and follow-up boundary recording. Do not close in this packet because
close requires the plan-owner close gate and the available root logs do not
include a current `test_after.log`; this delegated packet also forbids touching
root-level logs.

Steps 1-4 classification summary remains:

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

Proceed with `plan.md` Step 6. Supervisor should decide the final validation
scope and either provide/refresh matching close-gate logs or delegate closure
after the regression guard can run under the root-log constraints.

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
- Remaining follow-up boundaries are not part of this cleanup slice:
  dead-internal structured reference collection, initializer designator owner
  authority, user-label/block owner identity, structured inline-asm argument
  operands, typed GEP/aggregate operand text, and a structured LIR type-decl /
  signature printer model.

## Proof

No tests run by this lifecycle packet. Step 5 focused proof is documented above
from committed state. Closure was not accepted because the close-time regression
guard was not run and root-level log files were intentionally left untouched.
