Status: Active
Source Idea Path: ideas/open/132_parser_rendered_record_template_lookup_mirror_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Make Instantiation And NTTP Defaults Structured-Primary

# Current Packet

## Just Finished

Step 4 is complete for deferred NTTP default evaluation at known-primary
instantiation sites. `eval_deferred_nttp_default` now has a
`QualifiedNameKey`-aware overload that prefers structured
`nttp_default_expr_tokens_by_key` entries when the key is valid, while the
legacy string overload preserves the prior current-namespace/TextId-derived
behavior and TextId-less rendered fallback. `parse_base_type` default-fill
paths now pass primary-template-derived keys instead of reconstructing identity
from rendered template spelling plus the current namespace.

## Suggested Next

Next coherent packet: continue Step 4 by tracing remaining
`instantiated_template_struct_keys` and template-instantiation substitution
consumers for rendered-string identity, leaving direct helper internals and
compatibility mirrors in place where they are only mirrors.

## Watchouts

Do not broaden Step 4 into a parser template-instantiation redesign. Token text
may remain as source replay/debug data, but semantic template identity should
come from structured carriers where available. The new keyed NTTP default test
uses a manually distinct `QualifiedNameKey` context to prove evaluator identity
selection without depending on namespace fixture setup.

## Proof

Passed:
`cmake --build build --target frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_parser_tests$' --output-on-failure >> test_after.log 2>&1`

Proof log: `test_after.log`.
