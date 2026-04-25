Status: Active
Source Idea Path: ideas/open/105_hir_to_lir_text_id_bridge_inventory_and_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Build The HIR-To-LIR String Seam Inventory

# Current Packet

## Just Finished

Completed `plan.md` Step 1 inventory of HIR-to-LIR string seams. Inspected:
`src/codegen/lir/ir.hpp`, `src/codegen/lir/types.hpp`,
`src/codegen/lir/operands.hpp`, `src/codegen/lir/lir_printer.cpp`,
`src/codegen/lir/call_args_ops.hpp`, `src/codegen/lir/hir_to_lir/`,
`src/codegen/shared/fn_lowering_ctx.hpp`,
`src/codegen/shared/llvm_helpers.hpp`, and `src/codegen/llvm/`.

Seam classifications:
- `semantic-identity`: function/global/call/extern identity still appears in
  `LirFunction::name`, `LirGlobal::name`, `LirExternDecl::name`,
  `LirCallOp::callee`, `LirSpecEntry::mangled_name`, HIR dedup maps keyed by
  `Function::name`/`GlobalVar::name`, `select_global_object(std::string)`,
  and dead-internal reference maps keyed by printed names. Replacement
  authority is `LinkNameId` where present, with raw rendered spelling retained
  as fallback only when the id is invalid.
- `abi-link-spelling`: `emitted_link_name`, `llvm_global_sym`,
  `render_signature_with_link_name`, direct-call callee rendering, extern decl
  rendering, global rendering, and function signature text are emitted spelling
  seams. `LinkNameId`/`LinkNameTable` should remain authoritative for
  link-visible spelling; rendered names stay as parity fallback.
- `type-identity`: `LirTypeRef`, `LirGlobal::llvm_type`,
  `LirExternDecl::return_type_str`, `LirFunction::signature_text`,
  `LirModule::type_decls`, `llvm_ty`, `llvm_alloca_ty`,
  `llvm_struct_type_str`, and aggregate op type fields hold rendered LLVM type
  strings. Replacement authority is typed LIR type refs plus structured record
  owner keys/`TypeSpec` for future cleanup; current rendered strings are not a
  low-risk first cleanup.
- `temporary-rendered-lowering`: preformatted call args, GEP indices, PHI
  incoming value/label pairs, const-init `init_text`, flexible-array literal
  type/init text, `signature_text`, `type_decls`, and global-reference scanning
  over rendered operands are lowering convenience strings. Replacement
  authority is typed operands, typed LIR type refs, and structured symbol refs
  after the symbol-id path is stabilized.
- `printer-only`: final LLVM text assembly in `lir_printer.cpp`, opcode and
  predicate rendering, operand/type verification messages, and final module
  output strings. These should remain text at the printer boundary.
- `diagnostic-debug`: runtime error strings, verify field names, HIR dump test
  text, and debug formatting are not identity authority.
- `literal-bytes`: `LirStringConst::raw_bytes`, string-pool maps,
  string-literal byte decoding/escaping, inline asm `asm_text`/`constraints`,
  and literal constant text are payload and should stay string-backed.
- Field/designator/label seams: member access already has a structured
  `MemberSymbolId` path with name fallback; const-init designators prefer
  `TextId` via `field_designator_text_id` with string fallback; labels and user
  labels are control-flow spelling, not current symbol identity.

## Suggested Next

First low-risk cleanup candidate: update HIR-to-LIR function/global dedup in
`src/codegen/lir/hir_to_lir/hir_to_lir.cpp` to prefer `LinkNameId` keys before
falling back to raw `name` keys, matching the already-present
`LirModule::record_extern_decl` policy. Keep raw-name fallback for invalid
`LinkNameId`, preserve emitted spelling, and avoid touching type refs,
printer-only output, const-init payload bytes, or fallback removal.

## Watchouts

- Do not remove rendered fallback or change emitted spelling during inventory.
- Do not absorb HIR-internal legacy lookup cleanup already covered by idea 104.
- Keep printer-only, diagnostic, ABI/link spelling, and literal-byte strings
  separate from semantic identity handoffs.
- Do not downgrade tests or expectations to claim bridge cleanup progress.
- `LirModule::record_extern_decl` already prefers `LinkNameId`; do not rework
  that path unless the next packet needs consistency assertions.
- Dead-internal elimination still scans rendered `@name` strings and indexes
  functions by `LirFunction::name`; treat it as a later cleanup because it
  crosses printed operand scanning and reachability behavior.
- Type string cleanup is higher risk than symbol dedup because `LirTypeRef`,
  `type_decls`, `signature_text`, and const-init aggregate text currently feed
  printer/backend compatibility.

## Proof

Inventory-only lifecycle/task-state update. No build or tests were run, and no
`test_after.log` was produced. Recommended supervisor proof for the dedup
cleanup candidate:
`cmake --build build --target c4cll && ctest --test-dir build -R '^(frontend_hir_tests|positive_sema_ok_call_target_resolution_runtime_c|positive_sema_inline_call_discovery_c|cpp_positive_sema_namespace_global_var_runtime_cpp|cpp_positive_sema_namespace_function_call_runtime_cpp|cpp_positive_sema_anon_namespace_fn_lookup_cpp|cpp_llvm_spec_key_metadata|cpp_llvm_forward_pick_specialization_metadata)$' --output-on-failure`.
