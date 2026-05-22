Status: Active
Source Idea Path: ideas/open/362_aarch64_pointer_derived_load_address_scaling_timeout.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Refresh Current First Bad Fact

# Current Packet

## Just Finished

Step 1: Refresh Current First Bad Fact is complete. The delegated focused
proof is green: build was up to date and all 7 selected tests passed,
including `c_testsuite_aarch64_backend_src_00181_c`, nearby `00170` / `00189`,
and the memory-operand / pointer-value writeback backend contracts. Current
classification: pointer-derived load/address scaling is not a live first bad
fact in this tree; the old `00181` timeout owner appears already repaired.

## Suggested Next

Supervisor should route lifecycle close/deactivation consideration for idea
362, or choose a separate source idea if current broader evidence points to a
different first bad fact.

## Watchouts

- Artifact note: `build/c_testsuite_aarch64_backend/src/00181.c.s` has current
  `Move` pointer-derived address calculations using `sxtw` plus `lsl #2`
  before adding to pointer carriers, and no `mul` occurrence matching the old
  suspicious immediate-scale pattern.
- This packet did not inspect or change expectations, unsupported
  classifications, runner behavior, timeout policy, CTest registration,
  implementation files, tests, `plan.md`, or the source idea.

## Proof

Ran exactly:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_pointer_value_named_scalar_writeback_uses_computed_store_value|backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_contract|c_testsuite_aarch64_backend_src_00170_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00189_c)$' > test_after.log 2>&1
```

Result: passed, 7/7 tests green. Proof log: `test_after.log`.
