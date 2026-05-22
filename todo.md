Status: Active
Source Idea Path: ideas/open/358_aarch64_recursive_scalar_formal_post_call_preservation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Current First Bad Fact

# Current Packet

## Just Finished

Step 1 refreshed the current first bad fact for idea 358. The delegated proof
is green, including `c_testsuite_aarch64_backend_src_00181_c`, so there is no
current representative failure showing scalar-formal post-call preservation is
still live.

## Suggested Next

Return to lifecycle routing for idea 358: `00181` is currently green under the
focused proof, so this plan should be closed, parked, or replaced rather than
expanded into implementation work.

## Watchouts

- The source idea is parked and says the scalar `%p.n` post-call owner was
  previously repaired; the fresh proof is consistent with that note.
- No implementation files, tests, expectations, CTest registration, runner
  behavior, `plan.md`, or source idea files were touched in this packet.
- Do not reopen pointer-formal callee-saved home publication, stack-preserved
  pointer formal overwrite handling, address-valued publication, semantic
  string-load work, or frontend admission inside this plan without lifecycle
  routing.

## Proof

Command run exactly:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_cli_aarch64_asm_external_return_add_smoke|c_testsuite_aarch64_backend_src_001(70|81|89)_c)$'; } > test_after.log 2>&1
```

Result: passed. `test_after.log` records `ninja: no work to do` followed by
4/4 passing tests:
`backend_cli_aarch64_asm_external_return_add_smoke`,
`c_testsuite_aarch64_backend_src_00170_c`,
`c_testsuite_aarch64_backend_src_00181_c`, and
`c_testsuite_aarch64_backend_src_00189_c`.

Classification: `00181` is green, so scalar-formal post-call preservation is
not a live owner for this representative on the current tree. No generated BIR
or AArch64 failure artifact needed further inspection because the delegated
subset did not produce a failing first bad fact.
