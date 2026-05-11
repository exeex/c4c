Status: Active
Source Idea Path: ideas/open/167_whole_codebase_string_authority_final_audit.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Covered Structured Domains

# Current Packet

## Just Finished

Step 2 - Classify Covered Structured Domains completed the first frontend
structured-domain classification packet for parser support/state, sema
validate/consteval/type-utils/canonical-symbol, and HIR rendered-compatibility
paths. No implementation or test files were edited.

Frontend classification table:

| Owner/domain | File/local symbol or path | Class | Evidence | Bridge boundary / follow-up |
|---|---|---|---|---|
| Parser support record probes | `src/frontend/parser/parser_support.hpp`: `resolve_record_type_spec(... compatibility_tag_map)` | `CB` | Header states record lookup should prefer `TypeSpec::record_def` and structured metadata before rendered maps. | Compatibility-only parser-local record probes; remove when callers carry `record_def` or structured record keys. |
| Parser constant evaluation | `src/frontend/parser/parser_support.hpp`: `eval_const_int` TextId overload vs rendered overload | `CB` | TextId overload accepts `std::unordered_map<TextId, long long>`; rendered overload is documented for legacy/HIR proof paths only. | Rendered `compatibility_named_consts` is legacy/HIR proof compatibility; new parser-owned callers should pass constant `TextId`s. |
| Parser typedef resolution | `src/frontend/parser/parser_support.hpp`: `resolve_typedef_chain` / `types_compatible_p` TextId and string overloads | `CB` | TextId overload says structured miss is authoritative and must not recover through rendered fallback names; string overloads are compatibility bridges. | Rendered typedef maps are legacy/HIR proof compatibility; remove when proof paths pass typedef `TextId`s or typed HIR bindings. |
| Parser state lookup tables | `src/frontend/parser/impl/parser_state.hpp`: `ParserBindingState` TextId/`QualifiedNameKey` maps | `CB` | `const_int_bindings`, `var_types_by_text_id`, `struct_typedefs`, and `value_bindings` are keyed by `TextId` or `QualifiedNameKey`. | Rendered companions such as `struct_tag_def_map` are mirrors only; semantic parser state is already structured. |
| Parser state rendered record mirror | `src/frontend/parser/impl/parser_state.hpp`: `struct_tag_def_map`; `src/frontend/parser/impl/support.cpp`: `eval_const_int_with_parser_tables` | `CB` | `struct_tag_def_map` comment calls it a rendered-tag compatibility mirror and `support.cpp` passes it beside structured const bindings. | Boundary is parser-local legacy record probes; not ordinary record identity authority. |
| Parser displayed/current names | `src/frontend/parser/impl/support.cpp`: `set_current_struct_tag`, `set_last_resolved_typedef`, `parser_text` | `DO/CB` | Stored strings are display fallbacks paired with `current_struct_tag_text_id` or `last_resolved_typedef_text_id`. | Display fallback when text table/id is absent; no complete structured miss was found reopening through these fields. |
| Sema structured key construction | `src/frontend/sema/validate.cpp`: `sema_structured_name_key`, `sema_symbol_name_key`, `sema_function_lookup_key` | `SA` | AST-backed signature inspection confirmed these helpers construct `SemaStructuredNameKey` from namespace, qualifier `TextId`s, and base `TextId`. | This is the intended structured authority, not a follow-up bug. |
| Sema globals/functions/overloads | `src/frontend/sema/validate.cpp`: `lookup_function_by_name`, `lookup_ref_overloads_by_name`, `lookup_cpp_overloads_by_name` | `CB` | Each lookup checks target/function structured keys first and returns `nullptr` after structured metadata misses before falling back to rendered maps. | Boundary is no-metadata references only; complete structured misses fail closed. |
| Sema consteval function handoff | `src/frontend/sema/validate.cpp`: `lookup_consteval_function_by_name` | `CB` | Structured key and TextId maps are checked before `consteval_funcs_`; populated structured/text maps make misses authoritative. | Boundary is no-metadata recursive/chained consteval calls. |
| Sema validator rendered inventory | `src/frontend/sema/validate.cpp`: `globals_`, `funcs_`, scope maps, `structured_*_by_name_` mirrors | `CB` | Local inventory comment explicitly separates rendered legacy tables from structured mirrors and says complete metadata misses fail closed. | Boundary is legacy carriers, diagnostics, and no-metadata parser comparison probes; no remaining `SA` follow-up found in this packet. |
| Sema type-utils enum eval | `src/frontend/sema/type_utils.cpp`: `static_eval_lookup_enum`, `same_rendered_type_name_compatibility` | `CB` | Enum eval checks key map, then TextId map, and returns `Miss`; rendered map is labeled no-metadata compatibility. Rendered type-name equality refuses carriers with text metadata. | Boundary is no-metadata enum/type carriers only. |
| Sema consteval type bindings | `src/frontend/sema/consteval.cpp`: `resolve_type`, `lookup_type_binding_by_*`, `record_type_binding_mirrors` | `CB` | Typespec key and TextId lookups return `Miss` and stop; rendered name maps only select TextId/key mirrors or final fallback when metadata channels are absent. | Boundary is legacy rendered `TypeSpec` tags without binding metadata. |
| Sema consteval NTTP/value bindings | `src/frontend/sema/consteval.cpp`: `lookup_forwarded_nttp_arg_by_text`, `record_nttp_binding_mirrors`, `evaluate_constant_expr` environment maps | `CB` | Text/key maps are recorded with rendered mirrors; valid forwarded `TextId` miss blocks rendered NTTP fallback. | Boundary is no-metadata forwarded NTTP spelling. |
| Sema/HIR record layout consteval | `src/frontend/sema/consteval.cpp`: `lookup_record_layout` | `CB` | Owner-key lookup is attempted first; canonical rendered tag only recovers a `TextId` and still ends at owner-index lookup; bare rendered fallback is disabled when owner index exists. | Boundary is legacy HIR layout handoff with no owner index. |
| Sema canonical symbols | `src/frontend/sema/canonical_symbol.cpp`: `qualified_name_identity_*`, `type_identity_from_typespec`, `CanonicalSymbolTable::lookup` | `SA/DO` | Symbol identity is `CanonicalIdentity` with structured qualified-name and type identity; display spelling is used for formatting/source names and fallback name bindings. | No string-authority follow-up; fallback names are no-metadata template substitution compatibility, not symbol table authority. |
| HIR compile-time registry | `src/frontend/hir/compile_time_engine.hpp`: `find_template_def`, `find_template_struct_def`, `find_consteval_def`, `find_enum_const`, `find_const_int_binding` | `CB` | Structured registry/value keys are checked first; complete key misses return `nullptr`/`nullopt` before rendered lookup. | Boundary is no-metadata declarations or incomplete registry keys. |
| HIR compile-time registry key fallback | `src/frontend/hir/compile_time_engine.hpp`: `CompileTimeRegistryKey::declaration_fallback` | `CB` | Incomplete text identity stores declaration pointer fallback; complete keys require namespace/base/qualifier `TextId`s and no declaration fallback. | Boundary is invalid-id/incomplete metadata on declaration carriers. |
| HIR type binding aliases | `src/frontend/hir/hir_ir.hpp`: `TypeBindings`, `NttpBindings`, `NttpTextBindings` | `CB` | Comments state rendered maps are migration compatibility and owner-aware lookups must prefer `HirTemplateParameterBindingKey` and fail closed on complete misses. | Boundary is no-metadata template binding creation/forwarding. |
| HIR lowerer local/param/static maps | `src/frontend/hir/impl/lowerer.hpp`: `FunctionCtx` `locals`, `params`, `rendered_compat_*` sets | `RL/CB` | Local/param string maps are function-scope lowering state; `rendered_compat_*` sets fence fallback eligibility. | Route-local lowering identity plus explicit rendered-compatibility membership. |
| HIR local/param type inference | `src/frontend/hir/hir_types.cpp`: `infer_generic_ctrl_type` local/param branches | `CB` | TextId maps are checked first; rendered lookup is reached only for names/text ids recorded in `rendered_compat_local_*` or `rendered_compat_param_*`. | Boundary is explicit rendered-compat set membership; otherwise no rendered fallback after a TextId miss. |
| HIR static/global/function decl resolution | `src/frontend/hir/hir_types.cpp`: `make_global_lookup_decl_ref`, `make_function_lookup_decl_ref`; `src/frontend/hir/hir_ir.hpp`: `classify_*_decl_lookup` | `CB` | Decl refs carry structured fields; `hir_ir.hpp` blocks legacy rendered lookup after structured-text miss unless rendered compatibility is explicitly allowed or self-consistent. | Boundary is rendered qualified-decl compatibility or self-consistent rendered name only. |
| HIR record layout lookup | `src/frontend/hir/hir_types.cpp`: `find_struct_def_for_layout_type`, `find_struct_def_by_layout_compatibility_tag` | `CB` | Complete owner key lookup returns `nullptr` on miss; rendered compatibility tag lookup is only after incomplete owner metadata. | Boundary is legacy `TypeSpec` producers lacking complete structured owner metadata. |

Remaining `SA` follow-up recommendation:
- None for this frontend classification packet. The only `SA` rows above are
  intended structured-authority keys/tables. Retained string maps are classified
  as `CB`, `RL`, or `DO` with explicit no-metadata, invalid-id/incomplete-key,
  or rendered-compatibility membership boundaries.

## Suggested Next

Continue Step 2 with the LinkNameId/LIR/HIR-to-LIR covered-domain packet:
classify `src/codegen/lir/ir.hpp`, `src/codegen/lir/verify.cpp`, and
`src/codegen/lir/hir_to_lir/*.cpp` around extern declarations, functions,
discardable functions, aggregate type references, `StructNameId`, and
`LinkNameId` boundaries.

## Watchouts

- The frontend packet found no remaining ordinary rendered-name semantic
  authority, but several bridges remain intentionally live for no-metadata
  parser/HIR/consteval handoffs.
- Do not retire the bridges during this audit packet; bridge retirement belongs
  to a later follow-up lane.
- HIR local/param rendered maps are route-local lowering state plus explicit
  compatibility membership, not module/global source identity.

## Proof

Audit replay command run:
`rg -n --glob '!build*/**' --glob '!ideas/closed/**' --glob '!review/**' -e 'rendered_compat' -e 'complete.*miss' -e 'no-metadata' -e 'invalid-id' -e 'legacy.*fallback' src/frontend src/codegen src/backend`

Frontend structured-domain tests run:
`ctest --test-dir build -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|frontend_hir_lookup_tests|cpp_hir_parser_support_residual_structured_metadata|cpp_hir_parser_type_helper_residual_structured_metadata|cpp_hir_sema_canonical_symbol_structured_metadata|cpp_hir_sema_consteval_type_utils_structured_metadata)$' --output-on-failure > test_after.log 2>&1`

Result: passed, 7/7 tests. Proof log: `test_after.log`.
