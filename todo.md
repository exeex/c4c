Status: Active
Source Idea Path: ideas/open/105_hir_to_lir_text_id_bridge_inventory_and_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Separate Type Identity From Rendered LLVM Type Text

# Current Packet

## Just Finished

Inspected `plan.md` Step 3 rendered LLVM type-text seams in
`src/codegen/lir/types.hpp`, `src/codegen/lir/hir_to_lir/types.cpp`,
`src/codegen/lir/hir_to_lir/hir_to_lir.cpp`, and
`src/codegen/shared/llvm_helpers.hpp`. No behavior-preserving typed-reference
cleanup was narrow enough for the delegated ownership. `LirTypeRef` already
wraps many instruction operand type strings and verifies cached kind/width
against rendered text, but the remaining Step 3 identity authority is still
held by pre-rendered module/function/global text fields or by HIR `TypeSpec`
and struct tags before final rendering.

## Suggested Next

Defer Step 3 implementation to a packet that explicitly owns the LIR public
surface and printer boundary, or route to Step 4 classification if the
supervisor wants to avoid a type-surface rewrite in this runbook.

## Watchouts

- `build_type_decls()` renders struct/union declarations into
  `LirModule::type_decls` as final LLVM text. Replacement authority should be
  a structured LIR type-decl model keyed by HIR `struct_def_order`/record owner
  and field `TypeSpec`, with rendering delayed to the printer.
- `build_fn_signature()` and `llvm_fn_type_suffix_str()` render function
  signatures and callee suffixes from HIR `TypeSpec`. Replacement authority
  should be `LirFunction::return_type`, `LirFunction::params`, and a structured
  call-signature/type-ref object, not `signature_text`.
- `lower_global()` stores `LirGlobal::llvm_type` as precomputed text even
  though `LirGlobal::type` is available. Replacement authority should be
  `LirGlobal::type`, plus an explicit structured escape hatch for flexible
  array literal aggregate types.
- `finalize_module()` fills both `LirExternDecl::return_type_str` and
  `LirExternDecl::return_type`, but the printer still consumes
  `return_type_str`. Replacing that text authority requires changing the LIR
  public model/printer, not just `hir_to_lir.cpp`.
- Const-init aggregate, GEP, and initializer strings use rendered type text in
  payload/backend compatibility paths. Do not change them under Step 3 without
  a packet that explicitly owns const-init text behavior and byte-for-byte
  output proof.
- A proof-capable Step 3 implementation should own `src/codegen/lir/ir.hpp`,
  `src/codegen/lir/lir_printer.cpp`, the chosen HIR-to-LIR producer, and
  narrowly relevant backend tests, then run:
  `cmake --build build --target c4cll && ctest --test-dir build -R '^backend_' --output-on-failure`.

## Proof

No tests run. This was a blocker/candidate inventory only, per the delegated
proof rule, and `test_after.log` was not created or modified by this packet.
