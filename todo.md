Status: Active
Source Idea Path: ideas/open/161_hir_template_binding_domain_key_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Convert Forwarding and Lowerer Lookup Paths

# Current Packet

## Just Finished

Completed the delegated Step 5 NTTP forwarding lookup-authority repair from
`plan.md`. Function lowering now records the AST owner used to populate
structured template bindings in `FunctionCtx`, and consteval NTTP forwarding
uses that owner plus the forwarded parameter `TextId` to pass a complete
owner/index query key into `lookup_nttp_binding` when available. Focused
coverage now verifies that lowerer consteval forwarding prefers the complete
structured NTTP binding over stale TextId/rendered mirrors, while stale
structured misses still fail closed.

## Suggested Next

Supervisor should review and commit this coherent Step 5 consteval NTTP
forwarding metadata slice, then choose the next packet only if more
forwarding/lowerer lookup bypasses remain.

## Watchouts

- `template_binding_owner_node` must stay aligned with the owner used to
  populate `ctx.structured_nttp_bindings`; function lowering uses the function
  node and struct-method lowering uses the structured owner selected for method
  template bindings.
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
