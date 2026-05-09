Status: Active
Source Idea Path: ideas/open/157_deferred_syntax_text_payload_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Make Payload API Boundaries Explicit

# Current Packet

## Just Finished

Completed Step 2 audit of HIR template materialization
`TemplateArgRef::debug_text` and explicit string/value argument paths.
Demoted `resolve_ast_template_value_arg` so a present
`template_arg_nttp_text_ids` carrier is authoritative: if the TextId carrier
misses, or no TextId environment map exists, HIR no longer reopens
`template_arg_nttp_names` rendered-name lookup against enclosing
`nttp_bindings`. Added lookup-authority coverage for this explicit AST value
argument path and preserved no-carrier rendered-name compatibility. The
remaining audited `debug_text` uses in template materialization are either
guarded behind structured carriers before semantic binding or retained as
literal/display/no-metadata compatibility.

## Suggested Next

Continue Step 2 with a reviewer/supervisor pass over the accumulated payload
API boundary changes, or move to the next payload-family packet the supervisor
selects.

## Watchouts

- HIR keyed default evaluation still calls `eval_deferred_nttp_expr_hir` with
  `expr_override == nullptr`; that is the structured default-token path and
  should stay separate from display-string replay.
- Plain forwarded NTTP names and explicit string template argument refs still
  have compatibility lookup behavior only when no structured carrier is
  available. Do not remove that without checking legacy no-metadata forwarding
  coverage.
- Do not remove `debug_text` fields wholesale; they remain useful for display
  and legacy materialization paths that do not perform expression evaluation.

## Proof

Ran the delegated proof:

`cmake --build --preset default && ctest --test-dir build -R '(frontend_parser_lookup_authority|cpp_hir_.*template|frontend_hir_.*template|hir_case|cpp_positive_sema_variadic_template_arg_sizeof_pack_parse_cpp)' --output-on-failure > test_after.log 2>&1`

Result: passed. `test_after.log` reports 47/47 tests passed.
