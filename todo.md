Status: Active
Source Idea Path: ideas/open/659_rv64_byval_prepared_call_boundary.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Representative Runtime And Backend Regression Proof

# Current Packet

## Just Finished

Step 4 evidence remains active after the committed RV64 byval runtime repair:
the focused runtime subset and supervisor same-scope `^backend_` guard both
passed for the committed runtime slice.

Step 3 focused byval/prepared call-boundary evidence was also recorded after
that repair. The delegated focused subset completed with 6/9 rows passing.
Passing target rows:
`backend_codegen_route_riscv64_byval_preserved_pointer_args`,
`backend_codegen_route_riscv64_byval_formal_gpr_publication`,
`backend_codegen_route_riscv64_byval_aggregate_fixed_call`,
`backend_rv64_runtime_riscv64_byval_preserved_pointer_args`,
`backend_rv64_runtime_riscv64_byval_formal_gpr_publication`, and
`backend_rv64_runtime_riscv64_byval_aggregate_fixed_call`.

Remaining non-Step-2 / separate-owner Step 3 rows:
`backend_dump_riscv64_byval_aggregate_fixed_call` still fails because the
expected dump snippet asks for `move from_value_id=20 to_value_id=20
destination_kind=call_argument_abi destination_storage=stack_slot`, while the
current prepared output has the call-argument stack move for value id 22.
`backend_dump_riscv64_byval_preserved_pointer_args` still fails on a stale
aggregate-address dump snippet whose current output records frame-slot
call-argument sources. `backend_obj_runtime_rv64_frame_slot_pointer_arg_preserves_payload`
still fails in the separate object-runtime route with `unsupported_instruction_fragment`
for `instruction_kind=BinaryInst`.

## Suggested Next

Supervisor should decide close, split, or lifecycle routing after the Step 4
same-scope backend evidence, with the Step 3 focused proof recorded as
supporting evidence. No implementation, tests, expectations, unsupported
markers, allowlists, runtime policy, timeout settings, baseline files, or logs
were changed by this correction.

## Watchouts

- Do not edit expectations, unsupported markers, allowlists, runtime policy,
  timeout settings, or baseline acceptance files.
- Do not merge pointer-local, stack fan-in, AArch64, CLI, static object-data,
  callee-saved GPR, packed-member, or LLVM torture work into this route.
- Reject named-case or final-assembly-shape fixes.
- Do not treat the two dump failures as permission to rewrite expectations;
  their current output is useful positive evidence that prepared facts exist.
- Keep the object-runtime `BinaryInst` unsupported-fragment row as a separate
  split unless the supervisor explicitly assigns object-route coverage.
- The focused route/runtime Step 2 rows passed in this proof. The focused
  subset remains red only because the two dump rows and the separate
  object-runtime row are still failing.
- Avoid broad scalar `StoreLocalInst` fallbacks. A previous attempt that
  allowed mismatched or missing prepared accesses to fall back to the raw store
  slot made many unrelated local-memory routes fail.
- The object-runtime row was not part of this packet and should remain a
  separate owner unless the supervisor explicitly assigns shared helper work.
- A probe of the aggregate dump showed mismatched prepared memory-access rows
  for `main` inst 0-3, so the text emitter must not trust those rows unless
  they match the destination store slot name.
- A non-delegated inspection run of
  `ctest --test-dir build -j --output-on-failure -R 'backend_dump_riscv64_byval_aggregate_fixed_call|backend_codegen_route_riscv64_byval_aggregate_fixed_call'`
  still leaves the dump row red because its expected snippet is stale relative
  to the current prepared call-argument move shape; no test expectations were
  changed.

## Proof

Step 3 focused proof command ran exactly as delegated:

```sh
cmake --build --preset default && (ctest --test-dir build -j --output-on-failure -R 'backend_(dump_riscv64_byval_aggregate_fixed_call|codegen_route_riscv64_byval_aggregate_fixed_call|dump_riscv64_byval_preserved_pointer_args|codegen_route_riscv64_byval_preserved_pointer_args|codegen_route_riscv64_byval_formal_gpr_publication|rv64_runtime_riscv64_byval_aggregate_fixed_call|rv64_runtime_riscv64_byval_preserved_pointer_args|rv64_runtime_riscv64_byval_formal_gpr_publication|obj_runtime_rv64_frame_slot_pointer_arg_preserves_payload)' > test_after.log; test -s test_after.log)
```

Result: build succeeded (`ninja: no work to do`), the focused CTest subset
reported 67% tests passed with 6 passed and 3 failed out of 9, and
`test_after.log` is the preserved proof log. The shell wrapper returned success
because `test_after.log` was nonempty after the CTest run.

Prior supervisor Step 4 acceptance ran a stash-based same-scope backend guard:

```sh
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R '^backend_' > test_before.log
ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Result: PASS. Before `passed=343 failed=25 total=368`; after
`passed=349 failed=19 total=368`; resolved 6 backend rows; new failing tests:
0.
