Status: Active
Source Idea Path: ideas/open/161_hir_template_binding_domain_key_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Convert Forwarding and Lowerer Lookup Paths

# Current Packet

## Just Finished

Completed the delegated Step 5 NTTP value-argument forwarding repair from
`plan.md`. `resolve_ast_template_value_arg` now derives forwarded NTTP query
keys from `ctx.template_binding_owner_node` plus the forwarded `TextId` instead
of from the target template owner. Direct NTTP lookup bridges in scalar
lowering, value-arg expression evaluation, and generic control type inference
now use complete source-owner keys when the `FunctionCtx` owner and expression
`TextId` can identify the source NTTP parameter, while incomplete-metadata cases
remain on the existing compatibility path.

## Suggested Next

Supervisor should review and commit this coherent Step 5 NTTP forwarding and
direct-lookup authority slice, then decide whether Step 5 is ready for Step 6
validation.

## Watchouts

- The focused `cpp_hir_template_parameter_binding_key_test` case proves a
  forwarded source-owner NTTP binding resolves even when the target owner key
  would be a structured miss.
- The existing lookup-authority test fixture now explicitly installs
  `ctx.template_binding_owner_node` for structured HIR value materialization;
  this keeps complete-key behavior source-owned instead of restoring target
  owner fallback.
- No direct NTTP site was converted without a complete source owner plus
  expression `TextId`; those remain compatibility bridges.

## Proof

Ran delegated proof command:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_hir_.*|cpp_positive_sema_.*template.*|cpp_positive_sema_.*qualified_.*template.*)$'; } > test_after.log 2>&1`

Result: command exited 0 and `test_after.log` was written. The delegated proof
passed all 323 selected tests, including
`frontend_parser_lookup_authority_tests`, the `cpp_hir_.*` coverage, and the
positive template/qualified-template sema coverage.
