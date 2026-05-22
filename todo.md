Status: Active
Source Idea Path: ideas/open/360_aarch64_hanoi_starting_state_output_mismatch.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh the Current `00181` First Bad Fact

# Current Packet

## Just Finished

Step 1 refreshed the current `00181` first bad fact for idea 360. The
delegated focused proof passed 7/7, including
`c_testsuite_aarch64_backend_src_00181_c`, so there is no live observable Hanoi
starting-state output mismatch in this subset and no current first bad fact
that still belongs to idea 360.

## Suggested Next

Supervisor should route the next lifecycle decision for this active refresh:
either advance to the boundary-preservation handoff/close decision or ask the
plan owner to retire/deactivate the refresh if no additional proof is needed.

## Watchouts

- Do not reactivate idea 362 as the immediate next route unless fresh evidence
  requires it.
- Treat expectation, timeout, runner, unsupported-classification, CTest, and
  proof-log-policy changes as out of scope.
- Do not claim progress through a filename-specific or literal-value shortcut.
- The proof output contains no `00181` mismatch details because the test passes;
  classification is therefore "no live in-scope first bad fact", not a localized
  new failure.

## Proof

Ran:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_pointer_value_named_scalar_writeback_uses_computed_store_value|backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_contract|c_testsuite_aarch64_backend_src_00170_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00189_c)$' > test_after.log 2>&1
```

Result: build succeeded (`ninja: no work to do`), CTest passed 7/7 with
`c_testsuite_aarch64_backend_src_00181_c`, `00170`, `00189`, and the focused
backend guardrails all passing. `test_after.log` is the preserved proof log.
