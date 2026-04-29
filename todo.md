Status: Active
Source Idea Path: ideas/open/132_parser_rendered_record_template_lookup_mirror_cleanup.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Consolidate Compatibility Mirrors And Final Proof

# Current Packet

## Just Finished

Step 4 is complete. Commit `e772691f` keyed deferred NTTP default evaluation by
template identity: `eval_deferred_nttp_default` now has a
`QualifiedNameKey`-aware overload, the legacy string overload preserves the
prior current-namespace/TextId-derived behavior and TextId-less rendered
fallback, and `parse_base_type` known-primary default-fill paths now pass
primary-template-derived keys instead of reconstructing identity from rendered
template spelling plus the current namespace.

## Suggested Next

Next coherent packet: execute Step 5 by consolidating compatibility mirrors and
final proof. Re-check remaining rendered lookup mirrors after the structured
record, template primary/specialization, instantiation, and NTTP default paths
are in place; remove dead mirrors where safe, and document or visibly guard any
remaining compatibility-only fallback.

## Watchouts

Do not broaden Step 5 into a parser template-instantiation redesign. Rendered
strings may remain for diagnostics, generated spelling, debug output, source
replay, or explicit compatibility fallback, but they should not silently decide
parser semantic lookup or substitution once structured identity is available.
Supervisor review found remaining direct uses of
`instantiated_template_struct_keys`, `make_template_struct_instance_key`,
`nttp_default_expr_tokens`, and `eval_deferred_nttp_default` are structured or
legacy mirror helper internals, parser cache writes, and tests.

## Proof

Passed:
`cmake --build build --target frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_parser_tests$' --output-on-failure >> test_after.log 2>&1`

Supervisor guard:
matching before/after `frontend_parser_tests` passed with before=1 passed/0
failed and after=1 passed/0 failed; equal pass counts allowed.

Supplemental proof:
`ctest --test-dir build -R '^frontend_cxx_' --output-on-failure` passed, 1 test
in the current build.

Proof logs: `test_before.log` and `test_after.log`.
