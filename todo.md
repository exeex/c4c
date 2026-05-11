Status: Active
Source Idea Path: ideas/open/161_hir_template_binding_domain_key_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Convert Forwarding and Lowerer Lookup Paths

# Current Packet

## Just Finished

Completed the delegated Step 5 lowerer generic-control lookup packet from
`plan.md`. `Lowerer::infer_generic_ctrl_type` now resolves template typedef
bindings through `find_template_type_binding_for_call`, so TypeSpecs with
complete structured template-parameter metadata use structured binding authority
and complete structured misses fail closed instead of falling back to text or
rendered binding maps. Incomplete metadata still reaches the existing
compatibility lookup paths through the shared helper.

## Suggested Next

Supervisor should review and commit this coherent Step 5 lookup-authority
slice, then choose the next packet only if more direct forwarding or lowerer
lookup bypasses remain.

## Watchouts

- `find_template_type_binding_for_call` is now the single authority for this
  lowerer path; avoid reintroducing direct `tpl_bindings_by_text` or rendered
  `tpl_bindings` probes in `infer_generic_ctrl_type`.
- No focused test was added in this packet because the existing delegated HIR
  and template lookup subset covers the shared helper and passed unchanged.
- Do not weaken tests or convert capability work into expectation-only changes.

## Proof

Ran delegated proof command:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_hir_.*|cpp_positive_sema_.*template.*|cpp_positive_sema_.*qualified_.*template.*)$'; } > test_after.log 2>&1`

Result: command exited 0 and `test_after.log` was written. The delegated proof
passed all 323 selected tests, including
`frontend_parser_lookup_authority_tests`, the `cpp_hir_.*` coverage, and the
positive template/qualified-template sema coverage.
