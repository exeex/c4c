Status: Active
Source Idea Path: ideas/open/157_deferred_syntax_text_payload_contract.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Require Structured Carriers Before Semantic Lookup

# Current Packet

## Just Finished

Completed Step 3 audit and repair for HIR typed type-argument
materialization. `resolve_explicit_typed_arg` now handles typed type args
structurally instead of falling through to string fallback, so a structured
type carrier miss cannot reopen `TemplateArgRef::debug_text` lookup. Type-pack
materialization no longer expands `debug_text` pack bindings when structured
type-param metadata is present and misses. Added focused lookup-authority
coverage for scalar typed type args and type packs, including no-carrier
compatibility cases.

## Suggested Next

Continue with the next Step 3 payload-family packet the supervisor selects, or
run a reviewer pass over the structured-carrier semantic lookup changes if this
step is being treated as complete.

## Watchouts

- `debug_text` remains valid for display and no-carrier compatibility. The new
  tests explicitly preserve no-carrier typed scalar and pack lookup behavior.
- `find_bound_type_pack_for_param_ref` now treats any structured type-param
  carrier as authoritative for pack expansion; stale rendered pack names should
  not be reintroduced through foreign-owner index shortcuts.
- The delegated packet did not require changes in `type_resolution.cpp`,
  `templates.cpp`, `hir_types.cpp`, or `tests/cpp/internal/hir_case/*.cpp`.

## Proof

Ran the delegated proof:

`cmake --build --preset default && ctest --test-dir build -R '(frontend_parser_lookup_authority|cpp_hir_.*template|frontend_hir_.*template|hir_case|cpp_positive_sema_variadic_template_arg_sizeof_pack_parse_cpp)' --output-on-failure > test_after.log 2>&1`

Result: passed. `test_after.log` reports 47/47 tests passed.
