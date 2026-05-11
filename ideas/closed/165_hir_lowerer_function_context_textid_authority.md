# HIR Lowerer Function Context TextId Authority

Status: Closed
Created: 2026-05-11
Closed: 2026-05-11

Parent Ideas:
- `ideas/closed/161_hir_template_binding_domain_key_authority.md`
- `ideas/closed/162_link_name_id_backend_symbol_authority.md`

## Goal

Audit and convert HIR lowerer function-context lookup tables that still use
rendered strings as ordinary authority for local semantic lookup.

`Lowerer::FunctionCtx` still has string-keyed maps for locals, params, static
locals, function-pointer signatures, local constants, labels, and pack params.
Some of these are route-local display handles, but local/param/global semantic
lookups should prefer TextId or structured local-domain keys when AST metadata
is available.

## Why This Idea Exists

Idea 161 moved HIR template bindings to owner-aware keys, and idea 162 moved
link-visible backend symbols to `LinkNameId`. The remaining HIR lowerer
function-local maps are closer to lexical scope authority:

- `locals`
- `params`
- `static_globals`
- `local_fn_ptr_sigs`
- `param_fn_ptr_sigs`
- `local_const_bindings`
- `pack_params`
- `label_blocks`

Labels and some generated local handles may remain route-local strings. Name
lookup for source locals, params, static locals, local constants, and function
pointer signatures should not depend on rendered strings when TextId metadata
exists.

## In Scope

- Inventory `Lowerer::FunctionCtx` string maps and classify each as semantic
  source lookup, generated route-local handle, label control-flow handle,
  diagnostic/display, or compatibility bridge.
- Add TextId or structured local-domain maps for source locals, params, static
  locals, local constants, and function-pointer signatures where metadata is
  present.
- Prefer structured/TextId lookup before rendered string lookup for covered
  source-level names.
- Keep route-local SSA/local-slot labels and generated backend handles as
  strings when they do not represent source semantic identity.
- Add tests where same-spelled source names in different local domains or
  shadowing scopes resolve through structured local identity instead of raw
  string coincidence.
- Document retained string maps with owner, limitation, and removal condition.

## Out Of Scope

- Template binding maps already covered by idea 161.
- Link-visible function/global symbols already covered by idea 162.
- Replacing block labels or generated local slot names with global symbol ids.
- Full local SSA/value-id redesign.

## Acceptance Criteria

- FunctionCtx string maps are classified.
- Covered local/param/static/local-const/function-pointer signature lookups use
  TextId or structured local-domain metadata when complete.
- Route-local generated names remain strings only where they are not semantic
  source identity.
- Retained rendered-name fallbacks are documented compatibility bridges.
- Focused HIR tests cover shadowing or same-spelled local lookup behavior.

## Closure Summary

Completed as a structured/TextId authority conversion for covered HIR lowerer
function-context source lookups.

Converted source lookup groups:

- `param_fn_ptr_sigs`: source parameter `TextId -> param index -> FnPtrSig`,
  with complete parameter signature misses failing closed.
- `local_fn_ptr_sigs`: source local `TextId -> LocalId -> FnPtrSig`, with
  local-over-param shadowing and complete local misses failing closed.
- `params`: ordinary parameter value/type lookup through
  `param_indices_by_text_id`.
- `locals`: ordinary local value/type lookup through `local_ids_by_text_id`.
- `static_globals`: local extern/static bridge lookup through
  `static_global_ids_by_text_id`.
- `local_const_bindings`: local consteval lookup through
  `local_const_bindings_by_text` and `local_const_bindings_by_key`.

Retained string boundaries are fenced as compatibility or non-semantic route
state:

- `pack_params` remains string-keyed because forwarding use sites lack a
  structured pack binding key. Converting it requires a future source
  initiative that introduces a structured pack binding key at those forwarding
  use sites.
- `label_blocks` remains a function-local control-flow label map, not source
  value authority.
- Rendered maps for generated/no-metadata locals, params, static-global
  bridges, and local constants remain explicit compatibility boundaries only.

Reviewer reports cleared the route-quality blockers. The local/parameter
function-pointer shadowing blocker was fixed by `a05df8141`, and the parameter
function-pointer hit-without-signature fallback blocker was fixed by
`99c959a90`. The final follow-up review found no testcase overfit, semantic
downgrade, or remaining route-quality blocker.

Final focused proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_template_struct_body_instantiation|llvm_gcc_c_torture_src_20000112_1_c|frontend_hir_tests|frontend_hir_lookup_tests|positive_sema_ok_local_fn_ptr_decl_c|backend_codegen_route_x86_64_function_pointer_param_direct_arg_observe_semantic_bir|cpp_positive_sema_c_style_cast_template_fn_ptr_param_type_parse_cpp|cpp_positive_sema_operator_this_out_of_class_runtime_cpp|cpp_positive_sema_template_deferred_method_cpp|cpp_positive_sema_template_struct_method_cpp|eastl_cpp_external_piecewise_construct_frontend_basic_cpp|eastl_cpp_external_utility_frontend_basic_cpp|cpp_hir_sema_consteval_type_utils_structured_metadata|cpp_hir_static_member_base_structured_metadata|cpp_hir_template_value_arg_static_member_trait)$' > test_after.log`

Result: 15/15 selected tests passed. Matching before/after regression guard
passed with 15/15 before and 15/15 after.

Accepted full-suite baseline: `test_baseline.log` records 3135/3135 tests
passed at commit `99c959a90`.

## Reviewer Reject Signals

- A source local/param lookup still uses rendered string first when TextId
  metadata is complete.
- Route-local labels/slots are incorrectly treated as module-level symbols.
- A structured miss reopens rendered lookup for covered metadata.
- Tests only check emitted spelling rather than lookup identity.
