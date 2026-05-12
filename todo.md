# Current Packet

Status: Active
Source Idea Path: ideas/open/181_function_pointer_signature_type_identity.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Function-Pointer Signature Carriers

## Just Finished

Step 1 from `plan.md`: inventoried function-pointer signature carriers across
parser, sema, HIR, LIR, BIR, and backend surfaces, then selected a bounded
repair path without implementation edits.

Inventory:

- Parser structured carriers: `TypeSpec::is_fn_ptr`,
  `Node::fn_ptr_params` / `n_fn_ptr_params` / `fn_ptr_variadic`, and
  `Node::ret_fn_ptr_params` / `n_ret_fn_ptr_params` /
  `ret_fn_ptr_variadic` are produced by
  `src/frontend/parser/impl/types/declarator.cpp` and retained on declarations,
  params, globals, fields, and nested return function-pointer declarators.
  `ParserFnPtrTypedefInfo` / `typedef_fn_ptr_info` propagates typedef function
  pointer params by `TextId`. These are structured parser facts, but
  `TypeSpec::is_fn_ptr` alone is flag-only identity risk.
- Sema structured carriers: `sema::CanonicalFunctionSig` carries structured
  `return_type`, ordered `params`, `is_variadic`, and `unspecified_params`;
  `CanonicalType` wraps pointer-to-function as `Pointer -> Function`.
  `canonicalize_fn_ptr_type()`, `is_callable_type()`, `get_function_sig()`,
  `types_equal()`, and `prototypes_compatible()` are the semantic signature
  authority. `TypeSpec` comparison helpers remain partial-shape risk when used
  without canonical function signatures.
- HIR structured carriers: `hir::FnPtrSig` carries `return_type`, `params`,
  `variadic`, `unspecified_params`, and `canonical_sig`. It is attached to
  `Param`, `LocalDecl`, globals, casts, callable fields, call expressions, and
  return-function-pointer signatures. `Lowerer::fn_ptr_sig_from_decl_node()`
  prefers sema canonical data and falls back to parser params. `FunctionCtx`
  maps by local id / param index and also keeps rendered-name compatibility
  maps for generated or no-metadata names. Structured fact, with rendered-name
  compatibility boundary.
- HIR call inference risk: `same_call_storage_type()` and
  `generic_type_compatible()` check storage/base/flag shape and nominal
  metadata only partially; they are not full function-pointer signature
  identity. `infer_call_result_type_from_callee()` can recover return type from
  `FnPtrSig`, but bare `TypeSpec::is_fn_ptr` remains insufficient for deciding
  signature equality.
- LIR structured carriers: `LirFunction` already has
  `signature_return_type_ref`, `signature_params`,
  `signature_param_type_refs`, `signature_is_variadic`, and
  `signature_has_void_param_list`. `LirCallOp` has structured
  `return_type` and `arg_type_refs`, but its function-pointer call prototype is
  still rendered through `callee_type_suffix`; `make_lir_call_op_with_return_type_ref()`
  mirrors argument type refs but does not carry a structured callee signature.
- LIR call bridge risk: `resolve_callee_fn_ptr_sig()` and
  `llvm_fn_type_suffix_str()` use `FnPtrSig` to render an indirect call suffix.
  This is useful output spelling, but BIR/backend consumers later parse or scan
  that string, so semantic identity can collapse to rendered text once the call
  leaves HIR/LIR construction.
- BIR carriers: `bir::CallInst` carries lowered ABI facts
  `arg_types`, `arg_abi`, `return_type`, `return_type_name`,
  `result_abi`, `is_indirect`, and `is_variadic`, plus
  `callee_link_name_id` / `callee_value`. These are structured ABI facts after
  lowering, but they no longer retain nominal function-pointer parameter
  identities. `BirFunctionLowerer::parse_typed_call()` parses
  `LirCallOp::callee_type_suffix` and `args_str`, so this boundary is rendered
  text / partial lowered ABI shape risk.
- Backend carriers: prealloc and MIR consume `bir::CallInst` ABI facts for
  call wrapper classification, register placement, sret/byval handling, and
  indirect-call emission. Aarch64 still checks variadic call state by scanning
  `LirCallOp::callee_type_suffix` in one LIR fast path, which is rendered text
  risk. Backend surfaces generally use ABI shape, not source-level function
  pointer signature identity.

Selected bounded repair path:

- Carry structured indirect-call callee signature identity from
  `hir::FnPtrSig` into `LirCallOp`, then make BIR lowering prefer that
  structured call-signature carrier over reparsing `callee_type_suffix`.
- Exact first implementation target:
  `src/codegen/lir/ir.hpp` / `src/codegen/lir/call_args_ops.hpp` add a
  bounded optional structured call-signature mirror on `LirCallOp` for fixed
  params, variadic/unspecified state, return type ref, and param type refs;
  populate it first in
  `src/codegen/lir/hir_to_lir/call/target.cpp` when
  `CallTargetInfo::callee_fn_ptr_sig` is available.
- Compatibility boundary for missing metadata: if `FnPtrSig::canonical_sig` or
  structured param/return refs are missing, keep emitting and parsing
  `callee_type_suffix` as the legacy display/compatibility path, but do not
  treat that fallback as proof of semantic function-pointer identity for
  metadata-rich calls. Metadata-rich mismatches should fail closed in the LIR
  verifier or BIR lowering rather than silently accept rendered-compatible
  suffixes.
- Nearby same-feature guards: preserve direct-call behavior, `LirFunction`
  signature structured fields, typedef-propagated function-pointer params,
  nested return function-pointer params, variadic and unspecified parameter
  lists, aggregate/sret/byval call ABI, and existing rendered LIR syntax for
  diagnostics/output. Do not add name-based or testcase-shaped matching.

## Suggested Next

Implement the first bounded slice: add the `LirCallOp` structured indirect-call
signature carrier and populate it from `CallTargetInfo::callee_fn_ptr_sig`,
then make verifier checks ensure it mirrors the existing rendered suffix for
metadata-rich indirect calls.

## Watchouts

- Keep display text intact: `callee_type_suffix`, `signature_text`, and printed
  LIR/BIR output remain output/compatibility spelling.
- The high-risk boundary is LIR call construction into BIR lowering:
  `BirFunctionLowerer::parse_typed_call()` currently derives call params from
  rendered suffix text and typed arg strings.
- Guard same-feature cases, not just one fixture: fixed-arity indirect call,
  variadic function pointer, unspecified params, typedef-backed function
  pointer param, nested return function pointer, and nominal record parameter
  identity.
- Do not expand into full parser or canonical type replacement. The selected
  path should be one call-site metadata bridge plus verifier/BIR preference.
- `ideas/open/182_type_identity_migration_closure_gate.md` remains blocked
  until this source idea is complete.

## Proof

Lifecycle/mapping-only packet; no build or CTest required by supervisor, and
no `test_after.log` created.

Targeted proof command family for later implementation:

- Build/compile proof: `cmake --build build --target frontend_lir_call_type_ref_test frontend_lir_function_signature_type_ref_test backend_lir_to_bir_notes_test -j`
- Focused LIR call/type-ref proof:
  `ctest --test-dir build -R '^(frontend_lir_call_type_ref|frontend_lir_function_signature_type_ref)$' --output-on-failure`
- Focused BIR bridge proof:
  `ctest --test-dir build -R '^backend_lir_to_bir_notes$' --output-on-failure`
- Nearby same-feature proof for indirect call ABI after code changes:
  `ctest --test-dir build -R 'backend_codegen_route_x86_64_indirect_(aggregate_param_return_pair|variadic_sum2|variadic_pair_second)_observe_semantic_bir' --output-on-failure`
- Parser/sema function-pointer guard family when HIR signature extraction is
  touched:
  `ctest --test-dir build -R '(fn_returns_fn_ptr|fn_returns_variadic_fn_ptr|local_fn_ptr_decl|nested_fn_returning_fn_ptr|declarator_parenthesized_fn_ptr|declarator_member_fn_ptr|template_fn_ptr_param_type)' --output-on-failure`
