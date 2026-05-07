Status: Active
Source Idea Path: ideas/open/150_nttp_type_binding_domain_key_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory String Binding Authority

# Current Packet

## Just Finished

Completed Step 1 inventory for `plan.md` Step 1, "Inventory String Binding Authority", across the primary targets:

- `src/frontend/parser/parser.hpp`: compatibility adapter plus current semantic authority. `eval_deferred_nttp_default`, `eval_deferred_nttp_expr_tokens`, and `eval_captured_template_arg_expr_tokens` still expose `vector<pair<string, TypeSpec>>` and `vector<pair<string, long long>>` as binding inputs. The optional `ParserNttpBindingMetadata` argument is a partial structured carrier for NTTP values, but the API still makes string-pair bindings authoritative for type parameters and fallback NTTP lookup.
- `src/frontend/parser/impl/types/template.cpp`: semantic authority. `eval_deferred_nttp_expr_tokens` binds type parameters by `token_spelling(tok) == pn`; NTTP tokens first try partial metadata, then fall back to `token_spelling(tok) == pn`. Calls through `eval_captured_template_arg_expr_tokens` and `eval_deferred_nttp_default` are compatibility wrappers over the same evaluator. Uses of `token_spelling` for builtins, numeric lexemes, member names, template names, and display text are syntax/display behavior or unrelated.
- `src/frontend/hir/impl/templates/type_resolution.cpp`: semantic authority plus compatibility/display behavior. `find_template_typedef_binding`, nested template-arg rebinding, member typedef resolution, and array-size substitution all use `TypeBindings` or `NttpBindings` keyed by `std::string`; owner/index/TextId carriers are consulted but still resolve into string-key maps. `TemplateArgRef::debug_text` is semantic authority where it is used to find `type_bindings` or `nttp_bindings`; `encode_template_arg_ref_hir` and refreshed debug strings are display/compatibility behavior.
- `src/frontend/hir/impl/templates/materialization.cpp`: semantic authority plus compatibility adapter. `HirTemplateArgMaterializer` merges legacy string-pair environments for deferred NTTP evaluation, checks `has_*_binding` by parameter-name string, resolves explicit string/typed args through `nttp_bindings.find(ref)`, `tpl_bindings.find(ref)`, and `TemplateArgRef::debug_text`, and emits resolved bindings as name/value or name/type pairs. Pack binding names and mangled-name assembly are compatibility/display behavior, but currently depend on those string keys.
- `src/frontend/hir/hir_functions.cpp`: semantic authority with compatibility fallback. `populate_template_type_text_bindings` derives TextId bindings from string-key `TypeBindings`; signature substitution maps structured TextId/index carriers back through `tpl_bindings->find(template_param_names[index])`; `TemplateArgRef::debug_text` participates in pack binding lookup. Function parameter local names are unrelated.
- `src/frontend/hir/compile_time_engine.hpp`: mixed. `TypeBindings`, `NttpBindings`, and `SpecializationArgumentIdentity::parameter_name` are current string-keyed semantic carrier types. `make_pending_template_*_binding_identities`, specialization selection, seed recording, and `mangle_template_name` consume these keys semantically. `format_pending_type_ref_for_display`, `encode_pending_type_ref`, `format_pending_template_type_key_for_display`, and canonical/display strings are syntax/display or compatibility behavior; comments already label some display-only paths.

## Suggested Next

First concrete implementation packet: define structured parameter-domain binding carriers and parser-side compatibility conversion before changing evaluation behavior.

Owned files for that packet:
- `src/frontend/parser/parser_types.hpp`
- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/impl/types/template.cpp`
- `src/frontend/parser/impl/types/base.cpp`

Packet boundary:
- Extend the existing parser NTTP carrier into a typed parser binding-domain carrier with parameter spelling `TextId`, owner/template key, parameter index, parameter kind, and value/type payload as applicable.
- Add conversion helpers from the legacy parser `vector<pair<string, TypeSpec>>` and `vector<pair<string, long long>>` environments into the structured carrier at the deferred-evaluation boundary.
- Keep behavior-preserving wrappers for the current string-pair API shape, but make the wrappers build structured bindings before evaluator lookup.
- Do not touch HIR files in this first packet; HIR migration should follow after the parser carrier contract compiles.

Supervisor-style proof command for the first code slice:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$|^frontend_cxx_'
```

Choice rationale: the first code slice touches parser template argument/default evaluation, but it changes carriers consumed by both parser template tests and HIR template materialization handoff paths. The delegated broader frontend subset is more credible than a parser-only command because the inventory found parser/HIR binding authority coupled through legacy string-pair environments and deferred NTTP evaluation.

## Watchouts

- Do not treat `TextId` alone as semantic parameter identity.
- Do not weaken template tests or mark supported paths unsupported.
- Do not claim progress by renaming existing string-keyed authority.
- The existing `ParserNttpBindingMetadata` is not enough by itself: it has name, TextId, qualified name key, and value, but no parameter index/kind split or type-binding payload.
- Existing HIR `NttpTextBindings` is a TextId map and should be treated as compatibility/fast lookup, not the final semantic domain key.
- `TemplateArgRef::debug_text` is safe only for display, diagnostics, or legacy decode fallback; any `*.find(debug_text)` path is semantic authority to migrate.

## Proof

Inventory-only `todo.md` update; no build required and no `test_after.log` generated. Future code-slice proof recorded above exactly as:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$|^frontend_cxx_'
```
