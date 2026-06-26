Status: Active
Source Idea Path: ideas/open/386_rv64_object_route_same_module_byval_aggregate_call_args.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Prepared Callsite Facts

# Current Packet

## Just Finished

Lifecycle switched from closed idea 384 to the next owner for the representative
boundary exposed by `src/20030914-2.c`.

## Suggested Next

Execute Step 1 from `plan.md`: audit the prepared callsite facts for
`main` / `entry` / inst `36`, especially the same-module wrapper, callee,
`arg0 bank=aggregate_address from=register:s1 to=none`, byval size/alignment,
and `arg1 bank=gpr from=immediate:4660 to=a1`.

## Watchouts

- Do not reopen prepared global-symbol publication; idea 384 closed that
  boundary.
- Do not treat closed byval parameter-home idea 370 as owning this callsite
  movement boundary.
- Keep aggregate `va_arg` and non-register parameter-home work routed to ideas
  371 and 374 respectively.
- Do not modify or rely on unrelated EV64 idea 385 for this lifecycle route.

## Proof

Close gate for idea 384 used matching focused backend logs:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_stack_layout|backend_riscv_object_emission)$'; } > test_before.log 2>&1
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_stack_layout|backend_riscv_object_emission)$'; } > test_after.log 2>&1
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Result: passed, `2/2` before and after with no newly failing tests.
