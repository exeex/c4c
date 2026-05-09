Status: Active
Source Idea Path: ideas/open/157_deferred_syntax_text_payload_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Make Payload API Boundaries Explicit

# Current Packet

## Just Finished

Completed Step 2 audit continuation: demoted remaining HIR forwarded-NTTP
rendered-name bridges in `build_call_nttp_bindings` and consteval call
lowering so a present `template_arg_nttp_text_ids` carrier is authoritative.
If the TextId carrier misses, HIR no longer reopens
`template_arg_nttp_names` lookup against enclosing `nttp_bindings`. The
rendered-name path remains documented as no-metadata compatibility when no
TextId carrier is present. Added stale-payload coverage for scalar and pack
forwarded NTTP arguments, including the retained no-carrier compatibility
case.

## Suggested Next

Continue Step 2 by auditing remaining `TemplateArgRef::debug_text` and
explicit string argument materialization paths in HIR template
materialization; classify each as display/literal compatibility or demote it
if it can still influence semantic lookup after a structured carrier miss.

## Watchouts

- HIR keyed default evaluation still calls `eval_deferred_nttp_expr_hir` with
  `expr_override == nullptr`; that is the structured default-token path and
  should stay separate from display-string replay.
- Plain forwarded NTTP names still have compatibility lookup behavior only
  when no TextId carrier is available. Do not remove that without checking
  legacy no-metadata forwarding coverage.
- Do not remove `debug_text` fields wholesale; they remain useful for display
  and legacy materialization paths that do not perform expression evaluation.

## Proof

Ran the delegated proof:

`cmake --build --preset default && ctest --test-dir build -R '(frontend_parser_lookup_authority|cpp_hir_.*template|frontend_hir_.*template|hir_case|cpp_positive_sema_variadic_template_arg_sizeof_pack_parse_cpp)' --output-on-failure > test_after.log 2>&1`

Result: passed. `test_after.log` reports 47/47 tests passed.
