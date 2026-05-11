Status: Active
Source Idea Path: ideas/open/161_hir_template_binding_domain_key_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Convert Forwarding and Lowerer Lookup Paths

# Current Packet

## Just Finished

Completed the delegated Step 5 forwarding/lowerer lookup packet from
`plan.md`. `Lowerer::lookup_nttp_binding` now accepts an explicit query
`TextId` and a rendered-mirror fallback switch, so forwarded NTTP value args and
consteval template calls can route through the structured-aware authority
without reopening rendered fallback when a forwarded `TextId` carrier is
present. Added focused lookup-authority coverage for structured wins over stale
TextId/rendered mirrors and complete structured miss behavior in the affected
value-argument and consteval-call paths.

## Suggested Next

Supervisor should review and commit this coherent Step 5 lookup-authority
slice, then choose the next Step 5 packet only if more direct forwarding or
lowerer lookup bypasses remain.

## Watchouts

- Forwarded NTTP lookup with a valid `TextId` now passes the rendered spelling
  only for structured pack matching; rendered mirror fallback is deliberately
  disabled for that metadata-bearing query.
- No-carrier forwarded NTTP compatibility still uses rendered names. Keep that
  path separate from complete structured miss behavior.
- `Lowerer::lookup_nttp_binding` still treats any complete structured NTTP
  binding channel as miss-authoritative over stale TextId/rendered mirrors.
- Do not weaken tests or convert capability work into expectation-only changes.

## Proof

Ran delegated proof command:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(llvm_gcc_c_torture_src_20011008_3_c|frontend_parser_lookup_authority_tests|cpp_c4_static_assert_if_constexpr_branch_unlocks_later|cpp_c4_static_assert_multistage_shape_chain|cpp_hir_.*|cpp_positive_sema_.*template.*|cpp_positive_sema_.*qualified_.*template.*)$'; } > test_after.log 2>&1`

Result: command exited 0 and `test_after.log` was written. The delegated proof
passed all 326 selected tests, including `llvm_gcc_c_torture_src_20011008_3_c`,
`frontend_parser_lookup_authority_tests`, the consteval/static-assert tests,
the `cpp_hir_.*` coverage, and the positive template/qualified-template sema
coverage.
