Status: Active
Source Idea Path: ideas/open/161_hir_template_binding_domain_key_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Convert Forwarding and Lowerer Lookup Paths

# Current Packet

## Just Finished

Completed the delegated Step 5 lvalue cast template-binding lookup-authority
repair from `plan.md`. `Lowerer::is_ast_lvalue` now routes typedef cast
classification through structured-aware type-binding authority: complete
structured type-parameter carriers bind through `ctx.structured_tpl_bindings`
and complete structured misses fail closed instead of falling through to raw
TextId lookup. Incomplete carriers still use the existing TextId/name mirror
compatibility path.

## Suggested Next

Supervisor should review and commit this coherent Step 5 lvalue cast
lookup-authority slice, then choose the next packet only if more
forwarding/lowerer lookup bypasses remain.

## Watchouts

- The local helper in `expr.cpp` mirrors the structured-key policy from
  callable lookup because `is_ast_lvalue` has no `Module*` and only needs the
  structured map plus existing TextId compatibility mirror.
- Complete structured lvalue cast type-parameter carriers now depend on
  `ctx.structured_tpl_bindings`; stale complete structured misses intentionally
  do not use the TextId mirror.
- Do not weaken tests or convert capability work into expectation-only changes.

## Proof

Ran delegated proof command:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_hir_.*|cpp_positive_sema_.*template.*|cpp_positive_sema_.*qualified_.*template.*)$'; } > test_after.log 2>&1`

Result: command exited 0 and `test_after.log` was written. The delegated proof
passed all 323 selected tests, including
`frontend_parser_lookup_authority_tests`, the `cpp_hir_.*` coverage, and the
positive template/qualified-template sema coverage.
