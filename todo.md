Status: Active
Source Idea Path: ideas/open/132_parser_rendered_record_template_lookup_mirror_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Make Instantiation And NTTP Defaults Structured-Primary

# Current Packet

## Just Finished

Step 3 is complete. Commit `58fc4635` added explicit rendered-mirror
visibility for template primary and specialization lookup helpers while keeping
`QualifiedNameKey` lookup authoritative, including structured null misses.
Remaining direct `template_struct_defs` and `template_struct_specializations`
uses are helper internals, registration mirror writes, and tests; call sites
outside helpers use `find_template_struct_primary` /
`find_template_struct_specializations`.

## Suggested Next

Next coherent packet: execute Step 4 by tracing
`instantiated_template_struct_keys`, `nttp_default_expr_tokens`, and their
instantiation/substitution consumers. Prefer structured template identity,
parameter ids, argument identity, or direct parser references over rendered key
strings, and add focused NTTP default plus repeated-instantiation coverage.

## Watchouts

Do not broaden Step 4 into a parser template-instantiation redesign. Token text
may remain as source replay/debug data, but semantic template identity should
come from structured carriers where available. Preserve final artifact spelling
separately from semantic lookup.

## Proof

Passed:
`cmake --build build --target frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_parser_tests$' --output-on-failure >> test_after.log 2>&1`

Supervisor post-commit guard:
`frontend_parser_tests` before/after logs both reported 1 passed and 0 failed.

Supplemental proof:
`ctest --test-dir build -R '^frontend_cxx_' --output-on-failure` passed with
1 test in the current build.

Proof log: `test_after.log`.
