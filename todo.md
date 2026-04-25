Status: Active
Source Idea Path: ideas/open/98_parser_sema_post_cleanup_structured_identity_leftovers.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Parser/Sema Leftover Callers

# Current Packet

## Just Finished

Step 1 - Inventory Parser/Sema Leftover Callers is complete.

Inspected `review/97_structured_identity_completion_audit.md`,
`src/frontend/parser/parser_support.hpp`,
`src/frontend/parser/impl/support.cpp`, parser helper call sites under
`src/frontend/parser/impl/`, and sema leftovers in
`src/frontend/sema/validate.cpp`, `src/frontend/sema/consteval.cpp`,
`src/frontend/sema/consteval.hpp`, and `src/frontend/sema/type_utils.cpp`.

Parser helper inventory:
- `eval_const_int` has a `TextId` named-constant overload and a legacy
  `std::string` named-constant overload in `parser_support.hpp` /
  `impl/support.cpp`.
- Parser-owned named-constant callers already use the `TextId` table:
  `Parser::eval_const_int_with_parser_tables`, global const/constexpr binding
  registration, and `aligned` / `vector_size` attribute evaluation pass
  `binding_state_.const_int_bindings`.
- Parser-owned callers without a named-constant table still use the helper for
  integer literals, enum/bit-field widths, `offsetof`, array dimensions, and
  static member initializers; these are not string named-constant lookups.
- No current parser-owned caller was found passing a
  `std::unordered_map<std::string, long long>` named-constant table.
- `resolve_typedef_chain` and `types_compatible_p` remain string-map helpers,
  but the current callers are HIR/template compatibility users with empty or
  HIR-owned string maps, not parser-owned structured-input opportunities.

Sema leftover inventory:
- Enum variant mirrors are declared and lookup-compared
  (`structured_enum_consts_`, `enum_const_vals_*_by_text`,
  `enum_const_vals_*_by_key`), but `bind_enum_constants_global` and
  `bind_enum_constants_local` only populate legacy name maps today.
- Template NTTP validation placeholders are string-only in
  `bind_template_nttps`; AST template parameter metadata is still
  `const char** template_param_names` without parallel parameter `TextId`
  fields.
- Template type-parameter validation is name-set based in
  `template_type_params_`, `record_template_type_params_recursive`, and cast
  validation checks against `TypeSpec::tag`.
- Consteval call NTTP bindings have lookup slots
  (`nttp_bindings_by_text`, `nttp_bindings_by_key`) but
  `bind_consteval_call_env` only accepts/populates the legacy
  `out_nttp_bindings` map for explicit and default NTTP values.
- Type-binding text mirror plumbing exists
  (`TypeBindingTextMap`, `TypeBindingNameTextMap`) and is threaded through
  `bind_consteval_call_env`, but `record_type_binding_mirrors` deliberately
  discards the text outputs and only writes structured-key mirrors.

Bridge-required / string-only callers:
- `eval_const_int` still requires rendered struct tags and field names for
  `struct_tag_def_map`, `sizeof`, `alignof`, and `offsetof`; that is a
  HIR/type bridge and not a Step 2 removal target.
- `resolve_typedef_chain` / `types_compatible_p` callers in
  `src/frontend/hir/hir_functions.cpp` and
  `src/frontend/hir/impl/templates/templates.cpp` remain string-only because
  they consume `TypeSpec::tag` / HIR template binding surfaces.
- `static_eval_int` in sema and HIR enum/const-int consumers remain
  rendered-name bridge paths and are outside the parser Step 2 helper packet.

## Suggested Next

First implementation packet for Step 2: make the parser helper boundary
explicitly structured-preferred without changing behavior.

Owned implementation files should be limited to
`src/frontend/parser/parser_support.hpp`, `src/frontend/parser/impl/support.cpp`,
and any parser-owned call site needed for compile cleanup. Recommended work:
keep the `TextId` `eval_const_int` overload as the primary parser-owned
surface, mark or reshape the string named-constant overload as a compatibility
bridge, add local parity/fallback checking only if both maps are available, and
leave `resolve_typedef_chain` / `types_compatible_p` as HIR/type bridge helpers
unless a real parser-owned caller is introduced.

## Watchouts

- Keep this plan limited to parser/sema cleanup; HIR module lookup migration
  belongs to idea 99.
- Preserve rendered-string bridges required by AST, HIR, consteval, diagnostics,
  codegen, and link-name output.
- Do not touch parser struct/tag maps, template rendered names, `TypeSpec::tag`
  outputs, or HIR/type/codegen identity surfaces.
- Do not downgrade expectations or add testcase-shaped exceptions.
- Do not treat the parser `struct_tag_def_map` argument to `eval_const_int` as a
  removable string leftover; it is still the rendered tag bridge used by
  `sizeof`, `alignof`, and `offsetof`.
- Enum variant mirrors look like the clean first sema implementation after the
  parser helper packet. Template NTTP/type-parameter and consteval NTTP mirror
  work needs a parameter-identity strategy first because current AST template
  parameter arrays do not carry `TextId`s.

## Proof

Source inventory only. No build or tests were run, and no `test_after.log` was
required for this packet.
