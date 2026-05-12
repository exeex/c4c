Status: Active
Source Idea Path: ideas/open/192_hir_compile_time_rendered_registry_api_retirement_audit.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Rendered Registry Authority

# Current Packet

## Just Finished

Completed Step 1 - Inventory Rendered Registry Authority.

Rendered registry API ledger:
- `CompileTimeState::find_template_def(std::string)` /
  `has_template_def(std::string)` /
  `is_consteval_template(std::string)`: rendered template-function
  compatibility lookup over `template_fn_defs_`. Metadata-rich overloads
  `find_template_def(Node*, rendered_name)`,
  `has_template_def(Node*, rendered_name)`, and
  `is_consteval_template(Node*, rendered_name)` use
  `CompileTimeRegistryKey` and fail closed on complete structured misses.
- `CompileTimeState::find_template_struct_def(std::string)` /
  `has_template_struct_def(std::string)`: rendered template-struct primary
  compatibility lookup over `template_struct_defs_`. Metadata-rich overloads
  by `Node*` and `QualifiedNameKey` use declaration/qualified-name keys before
  rendered names.
- `CompileTimeState::find_template_struct_specializations_no_metadata_compat`:
  explicitly fenced rendered primary-name compatibility lookup. The
  metadata-rich `find_template_struct_specializations(primary_def, rendered)`
  uses owner-key lookup and fails closed on complete owner-key misses.
- `CompileTimeState::find_consteval_def(std::string)` /
  `has_consteval_def(std::string)`: rendered consteval compatibility lookup
  over `consteval_fn_defs_`. The metadata-rich
  `find_consteval_def(Node*, rendered_name)` uses
  `ConstEvalStructuredNameKey` or TextId before rendered fallback.
- `CompileTimeState::consteval_fn_defs()`: rendered compatibility map passed
  into `evaluate_consteval_call`; paired metadata maps
  `consteval_fn_defs_by_text()` and `consteval_fn_defs_by_key()` are also
  passed at the same call sites, so recursive consteval lookup has structured
  authority available.
- `CompileTimeState::enum_consts()` and `const_int_bindings()`: rendered
  compatibility inputs for `ConstEvalEnv`; structured mirrors
  `enum_consts_by_key()` and `const_int_bindings_by_key()` are copied into
  consteval environments for global value-binding authority.
- `CompileTimeState::find_enum_const(...)` and
  `find_const_int_binding(...)`: structured value-binding lookup APIs that only
  fall back to rendered names when no complete key exists.

Call-site classification:
- Template function call lowering in `src/frontend/hir/impl/expr/call.cpp`
  and `src/frontend/hir/hir_types.cpp` still probes
  `has_template_def(name)` / `find_template_def(name)` from AST call spellings.
  These are no-metadata compatibility or legacy rendered-entry points for
  template function discovery; later instantiation identity uses primary
  definitions, specialization keys, TextId NTTP maps, and `LinkNameId` function
  lookup. Do not select this as Step 2 without a larger template-call owner
  carrier route.
- Template build/engine paths in `src/frontend/hir/hir_build.cpp` and
  `src/frontend/hir/impl/compile_time/engine.cpp` use rendered
  `source_template` / `fn_name` names as preservation keys for
  `TemplateCallInfo`, deferred-instantiation bookkeeping, and HIR metadata
  lookup. Classified as rendered preservation/no-metadata compatibility, not
  the first Step 2 route.
- Template-struct primary/specialization calls in
  `src/frontend/hir/impl/templates/templates.cpp` are already split:
  owner-key maps and `CompileTimeState` structured APIs are the metadata-rich
  path; `find_template_struct_specializations_no_owner_compat` is an explicit
  compatibility boundary. Classified as structured-authority mirror plus
  fenced no-owner compatibility.
- Consteval engine evaluation in
  `src/frontend/hir/impl/compile_time/engine.cpp` uses structured lookup for
  direct AST calls via `find_consteval_def(expr->left, expr->left->name)` and
  passes rendered plus TextId/key maps to `evaluate_consteval_call`.
  Classified as structured-authority mirror with rendered compatibility map
  still visible for no-metadata recursive calls.
- HIR direct consteval call lowering in
  `src/frontend/hir/impl/expr/call.cpp` uses
  `ct_state_->find_consteval_def(n->left->name)` before binding template/NTTP
  metadata. The callee `Node*` is available, so this is the selected
  metadata-rich authority bug candidate.
- Address-of consteval rejection in
  `src/frontend/hir/impl/expr/scalar_control.cpp` uses
  `has_consteval_def(n->left->name)`. The callee `Node*` is available and this
  is a candidate follow-up after the call-lowering route; it is diagnostic
  fencing, not the first semantic conversion.
- Enum and const-int registration in `src/frontend/hir/hir_build.cpp`,
  `src/frontend/hir/hir_types.cpp`, and
  `src/frontend/hir/impl/compile_time/engine.cpp` records rendered mirrors plus
  structured value-binding maps when keys exist. Consteval environments pass
  both rendered and key maps. Classified as structured-authority mirror plus
  rendered no-metadata compatibility.

Selected Step 2 route:
- Convert/fence `Lowerer::try_lower_consteval_call_expr` in
  `src/frontend/hir/impl/expr/call.cpp` to call
  `ct_state_->find_consteval_def(n->left, n->left->name)` instead of the
  rendered-only overload. First owned implementation file:
  `src/frontend/hir/impl/expr/call.cpp`. First focused tests:
  `tests/frontend/frontend_hir_tests.cpp` or
  `tests/frontend/frontend_hir_lookup_tests.cpp`, extending the existing
  compile-time-state consteval stale-rendered fixtures to prove call lowering
  with complete callee metadata fails closed rather than recovering through a
  stale rendered name.

## Suggested Next

Execute Step 2 by converting `Lowerer::try_lower_consteval_call_expr` to the
metadata-rich consteval lookup overload and adding a focused stale-rendered
fallback rejection test for that call-lowering route.

## Watchouts

Do not remove rendered consteval maps; `evaluate_consteval_call` still receives
them as an explicit compatibility surface alongside TextId/key maps. Keep
`find_consteval_def(std::string)` available for no-metadata callers and avoid
folding the broader template-function name route into this packet.

## Proof

Inventory-only packet; no build or test command was run. Existing
`test_after.log` was left untouched.
