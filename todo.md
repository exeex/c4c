Status: Active
Source Idea Path: ideas/open/157_deferred_syntax_text_payload_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Make Payload API Boundaries Explicit

# Current Packet

## Just Finished

Completed Step 2 first repair: `$expr:` raw string replay is no longer used as
semantic authority after structured carriers miss. Removed the parser
`lex_template_expr_text(actual_args[pi].nttp_name + 6, ...)` fallback and the
HIR mirrors that evaluated `$expr:` from `template_arg_nttp_names`, explicit
string refs, or `TemplateArgRef::debug_text`. Structured expression nodes,
captured tokens, TextId forwarding, and keyed deferred-default token evaluation
remain intact. Added stale-payload coverage for a direct NTTP instantiation
whose only `$expr:` payload is display text without captured tokens,
expression nodes, or TextId carriers. Follow-up regression repair preserved
parsed NTTP expression nodes into `TemplateArgRef` carriers so
`sizeof...(pack)` remains a structured expression path rather than falling
back to `$expr:` text, and taught HIR template value-arg evaluation to handle
`NK_SIZEOF_PACK` from that structured expression carrier.

## Suggested Next

Continue Step 2 by auditing remaining non-`$expr:` rendered-string bridges that
still participate in semantic lookup after a structured carrier miss, starting
with the string-key NTTP/name compatibility maps identified in Step 1.

## Watchouts

- HIR keyed default evaluation still calls `eval_deferred_nttp_expr_hir` with
  `expr_override == nullptr`; that is the structured default-token path and
  should stay separate from display-string replay.
- Plain forwarded NTTP names still have compatibility lookup behavior when no
  TextId carrier is available. This packet only demoted `$expr:` syntax-text
  replay, not all legacy name fallback.
- Do not remove `debug_text` fields wholesale; they remain useful for display
  and legacy materialization paths that do not perform expression evaluation.

## Proof

Ran the delegated proof:

`cmake --build --preset default && ctest --test-dir build -R '(frontend_parser_lookup_authority|cpp_hir_.*template|frontend_hir_.*template|hir_case)' --output-on-failure > test_after.log 2>&1`

Result: passed. `test_after.log` reports 46/46 tests passed.

Targeted regression repair proof also passed:

`cmake --build --preset default && ctest --test-dir build -R '^(cpp_positive_sema_variadic_template_arg_sizeof_pack_parse_cpp)$' --output-on-failure`
