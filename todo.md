Status: Active
Source Idea Path: ideas/open/669_byval_prepared_dump_contract_review.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove Byval Dump And Nearby Regression Safety

# Current Packet

## Just Finished

Step 3 `Prove Byval Dump And Nearby Regression Safety` completed the
supervisor-selected six-row proof after the byval prepared dump snippet
contract alignment. The two focused dump rows and four nearby route/runtime
byval regression surfaces all passed.

## Suggested Next

Hand back to the supervisor for Step 4 close readiness. The current Step 3
proof is green for the requested dump, route, and runtime byval surface, so
plan-owner close review can proceed if the supervisor accepts this as the
closure scope.

## Watchouts

- Treat route and runtime byval rows as regression surfaces, not a reopened
  implementation target.
- Do not reopen the closed idea 659 byval runtime/codegen-route repair without
  fresh focused regression evidence.
- Do not work on object-runtime `BinaryInst`; that belongs to idea 670.
- This packet did not touch implementation, expectations, unsupported markers,
  allowlists, timeout policy, runtime policy, baseline accounting, `plan.md`,
  or the source idea.
- The remaining decision is lifecycle close readiness, not a known
  implementation owner.

## Proof

```sh
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_dump_riscv64_byval_aggregate_fixed_call|backend_dump_riscv64_byval_preserved_pointer_args|backend_codegen_route_riscv64_byval_aggregate_fixed_call|backend_codegen_route_riscv64_byval_preserved_pointer_args|backend_rv64_runtime_riscv64_byval_aggregate_fixed_call|backend_rv64_runtime_riscv64_byval_preserved_pointer_args)$') > test_after.log 2>&1
```

Result: exit `0`; build was up to date and all six tests passed:
`backend_dump_riscv64_byval_aggregate_fixed_call`,
`backend_dump_riscv64_byval_preserved_pointer_args`,
`backend_codegen_route_riscv64_byval_aggregate_fixed_call`,
`backend_codegen_route_riscv64_byval_preserved_pointer_args`,
`backend_rv64_runtime_riscv64_byval_aggregate_fixed_call`, and
`backend_rv64_runtime_riscv64_byval_preserved_pointer_args`. Proof log:
`test_after.log`.
