Status: Active
Source Idea Path: ideas/open/158_sema_validate_string_authority_audit.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Template Parameter And Local Scope Visibility
你該做test baseline review了

# Current Packet

## Just Finished

Step 4: Template Parameter And Local Scope Visibility repaired the full-suite
baseline regression in local symbol lookup authority. A structured local
reference miss now retains rendered compatibility only when the rendered local
binding has no structured metadata; stale rendered fallback remains rejected
when the rendered local binding itself has structured metadata. The over-broad
legacy-local test was corrected to assert intended no-metadata compatibility.

## Suggested Next

Supervisor can review and commit this Step 4 repair slice, then decide whether
Step 4 has any remaining template/local visibility bridge work.

## Watchouts

- Do not weaken tests or mark supported paths unsupported.
- Rendered local lookup remains available after a metadata-bearing reference
  miss only for legacy/no-metadata local bindings.
- Do not weaken `test_sema_unqualified_symbol_lookup_rejects_stale_rendered_local_spelling`;
  it still covers the structured local binding stale-rendered rejection.
- Existing template type-parameter rendered-name compatibility remains limited
  to no-metadata carriers; TypeSpec metadata misses fail closed.
- This slice did not broaden into consteval/HIR storage migration.

## Proof

Passed: `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|positive_sema_|cpp_positive_sema_|llvm_gcc_c_torture_src_930603_1_c|llvm_gcc_c_torture_src_loop_2_c)') > test_after.log 2>&1`

Proof log: `test_after.log` reports 923/923 matching parser, positive sema, and
representative GCC torture tests passed.
