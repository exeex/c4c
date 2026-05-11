Status: Active
Source Idea Path: ideas/open/161_hir_template_binding_domain_key_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Convert Forwarding and Lowerer Lookup Paths

# Current Packet

## Just Finished

Completed the delegated Step 5 template-global argument lookup-authority repair
from `plan.md`. The remaining `tpl_bindings_by_text` probes inside
`ensure_template_global_instance` in `global.cpp` now route through
`find_template_type_binding_for_call`, so complete structured type-parameter
keys bind through shared authority and complete structured misses do not fall
through to raw TextId lookup. Empty owner-text metadata is still treated as
incomplete so the existing compatibility path remains available for
legacy/incomplete carriers.

## Suggested Next

Supervisor should review and commit this coherent Step 5 template-global lookup
authority slice, then choose the next packet only if more forwarding/lowerer
lookup bypasses remain.

## Watchouts

- `global.cpp` now depends on `ctx->structured_tpl_bindings` for complete
  structured carriers; stale complete structured misses fail closed inside
  `find_template_type_binding_for_call`.
- Some template-global argument carriers can arrive with an owner TextId that
  resolves to empty text; those remain compatibility/incomplete metadata cases.
- Do not weaken tests or convert capability work into expectation-only changes.

## Proof

Ran delegated proof command:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_hir_.*|cpp_positive_sema_.*template.*|cpp_positive_sema_.*qualified_.*template.*)$'; } > test_after.log 2>&1`

Result: command exited 0 and `test_after.log` was written. The delegated proof
passed all 323 selected tests, including
`frontend_parser_lookup_authority_tests`, the `cpp_hir_.*` coverage, and the
positive template/qualified-template sema coverage.
