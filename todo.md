Status: Active
Source Idea Path: ideas/open/108_lir_struct_name_id_for_globals_functions_and_externs.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Existing Signature And Type-Ref Surfaces

# Current Packet

## Just Finished

Completed `plan.md` Step 1 audit for the active LIR signature/type-ref
surfaces. Exact targets:

- Globals: `LirGlobal::llvm_type` is still printer text only. Add
  `LirGlobal::llvm_type_ref` as the structured mirror, populated in
  `lower_globals()` at `src/codegen/lir/hir_to_lir/hir_to_lir.cpp` for the
  normal global path and the flexible-array literal path. Reuse the existing
  struct-id helper shape from `lir_aggregate_type_ref()` / `lir_field_type_ref()`
  in the same file, but target top-level global `TypeSpec` rather than struct
  fields.
- Functions: `LirFunction::signature_text` remains printer authority.
  `LirFunction::return_type` and `LirFunction::params` are HIR `TypeSpec`
  metadata, not LIR type refs. Add a small signature mirror surface to
  `LirFunction`, with a return `LirTypeRef` and parameter `LirTypeRef` entries
  that shadow the rendered types from `build_fn_signature()` in
  `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`; the definition and declaration
  builders assign `signature_text` at the same construction sites.
- Externs: `LirExternDecl::return_type` already exists as `LirTypeRef`, but
  `LirModule::ExternDeclInfo` / `record_extern_decl()` only carry
  `return_type_str`, and `finalize_module()` rebuilds the ref as raw
  `LirTypeRef(decl_info.return_type_str)`. Extend the dedup record path so the
  structured return ref is preserved through `StmtEmitter::record_extern_call_decl()`,
  `LirModule::record_extern_decl()`, and `finalize_module()`.
- Calls: `LirCallOp::return_type` already exists as `LirTypeRef`, but
  `make_lir_call_op()` currently accepts return type text and stores a raw
  `LirTypeRef`. `OwnedLirTypedCallArg::type`,
  `LirCallOp::callee_type_suffix`, and `LirCallOp::args_str` are string-only;
  call args are built in `StmtEmitter::prepare_call_arg()` /
  `prepare_call_args()` and formatted by `format_lir_call_fields()`. The call
  packet should add structured mirrors only where the HIR call target or
  argument `TypeSpec` is still available, while keeping formatted call-site text
  unchanged.
- Verifier/printer: `require_module_type_ref()` already checks
  `StructNameId` parity for existing `LirTypeRef` fields, including
  `LirCallOp::return_type` and `LirExternDecl::return_type`. Add global and
  function-signature parity checks only after those mirrors exist. The printer
  still reads `g.llvm_type`, `f.signature_text`, and
  `ed.return_type_str`.

## Suggested Next

First bounded implementation packet: implement Step 2 for globals only.
Add `LirGlobal::llvm_type_ref`, populate it in both `lower_globals()` paths
from the already-known global `TypeSpec`, preserve `LirGlobal::llvm_type` as
printer text, and add verifier parity between `llvm_type_ref.str()` and
`llvm_type` when the mirror is present. Keep function signatures, externs,
and calls untouched in that packet.

## Watchouts

Keep `signature_text`, `llvm_type`, and `return_type_str` as printer text. Do
not switch printer authority or remove legacy strings in this plan. For
globals, arrays, pointers, references, and flexible-array literal structural
types should stay raw text unless a top-level struct/union identity is known
and shadow-renders exactly to the existing `llvm_type`.

## Proof

Audit-only packet; no build or test proof is applicable because no code,
tests, `plan.md`, or source idea files were changed. No `test_after.log` was
produced for this packet.
