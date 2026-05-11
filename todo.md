Status: Active
Source Idea Path: ideas/open/161_hir_template_binding_domain_key_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Convert Forwarding and Lowerer Lookup Paths

# Current Packet

## Just Finished

Completed the delegated Step 5 NTTP forwarding lookup-authority repair from
`plan.md`. `Lowerer::lookup_nttp_binding` now accepts an optional complete
structured NTTP query key and only selects complete structured NTTP bindings
through exact owner/index metadata. Raw TextId/rendered lookup no longer
selects complete structured bindings; it can still use compatibility mirrors
where the raw TextId belongs to the structured domain, while stale/mismatched
complete-key misses fail closed. `resolve_ast_template_value_arg` now passes a
complete owner/index key when its template owner and argument index provide one.
Focused same-spelled different-owner lookup coverage was added.

## Suggested Next

Supervisor should review and commit this coherent Step 5 NTTP lookup-authority
slice, then choose the next packet only if more forwarding/lowerer lookup
bypasses remain.

## Watchouts

- AST consteval forwarding still lacks source-owner/index metadata at the call
  site, so it uses matching TextId compatibility mirrors but no longer selects
  complete structured NTTP bindings by raw spelling alone.
- Explicit complete structured query-key misses fail closed before TextId or
  rendered mirror fallback.
- Do not weaken tests or convert capability work into expectation-only changes.

## Proof

Ran delegated proof command:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_hir_.*|cpp_positive_sema_.*template.*|cpp_positive_sema_.*qualified_.*template.*)$'; } > test_after.log 2>&1`

Result: command exited 0 and `test_after.log` was written. The delegated proof
passed all 323 selected tests, including
`frontend_parser_lookup_authority_tests`, the `cpp_hir_.*` coverage, and the
positive template/qualified-template sema coverage.
