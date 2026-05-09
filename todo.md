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
same-owner type-param metadata is present and misses. The bounded
foreign-owner pack bridge remains for nested owner handoff cases where the
current primary cannot resolve a different template owner's pack carrier
directly. Added focused lookup-authority coverage for scalar typed type args
and type packs, including no-carrier compatibility cases.

## Suggested Next

Continue with the next Step 3 payload-family packet the supervisor selects, or
run a reviewer pass over the structured-carrier semantic lookup changes if this
step is being treated as complete.

## Watchouts

- `debug_text` remains valid for display and no-carrier compatibility. The new
  tests explicitly preserve no-carrier typed scalar and pack lookup behavior.
- `find_bound_type_pack_for_param_ref` treats same-owner structured type-param
  carriers as authoritative for pack expansion; the foreign-owner/index bridge
  is retained for nested owner handoff compatibility.
- The delegated packet did not require changes in `type_resolution.cpp`,
  `templates.cpp`, `hir_types.cpp`, or `tests/cpp/internal/hir_case/*.cpp`.

## Proof

Ran the delegated proof:

`cmake --build --preset default && ctest --test-dir build -R '(frontend_parser_lookup_authority|cpp_hir_.*template|frontend_hir_.*template|hir_case|cpp_positive_sema_variadic_template_arg_sizeof_pack_parse_cpp)' --output-on-failure > test_after.log 2>&1`

Result: passed. `test_after.log` reports 47/47 tests passed.
