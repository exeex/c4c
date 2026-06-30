Status: Active
Source Idea Path: ideas/open/437_rv64_runtime_external_abort_exit_support.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Completed Step 3: implemented the bounded RV64 prepared module runtime external
policy gate for accepted `abort` and `exit`.

Implementation and coverage:

- `src/backend/mir/riscv/codegen/prepared_module_emit.cpp` now keeps existing
  `strlen` support and admits only `abort` with zero arguments plus `exit` with
  one argument through the existing direct fixed-arity external policy.
- Existing fail-closed checks remain in force for variadic external calls,
  indirect external plans, missing callees, memory returns, stack arguments,
  more than eight args, and non-scalar-GPR args/results.
- Focused prepared-module policy tests now prove accepted `abort`/`exit(0)`
  text emission and rejected unknown runtime external plus unsupported `abort`
  signature diagnostics.
- Step 2 object-route coverage still proves the accepted lower-level undefined
  symbol and `R_RISCV_CALL_PLT` relocation behavior.
- Representative probes for `20000917-1.c` and `20020206-1.c` pass
  `--codegen asm` after rebuilding `c4cll`; stderr is empty and
  `unsupported_external_call` is absent.

Artifact:
`build/agent_state/437_step3_runtime_external_policy_gate/summary.txt`.

## Suggested Next

Execute Step 4: Residual Disposition And Close Readiness.

First bounded packet:

- Re-probe the representative rows named by the plan and classify any residuals
  exposed after runtime external support.
- Confirm whether remaining failures, if any, are outside this runtime
  external idea and should route to existing or new source ideas.
- Decide whether the source idea appears ready for lifecycle close review.

Future implementation proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Do not claim aggregate `sret` progress through runtime external support.
- Do not special-case `20000917-1`, `20020206-1`, `main`, `baz`, or one dump
  shape.
- `abort` and `exit` are still deliberately narrow policy entries, not a broad
  unknown-external admission rule.
- Keep unsupported: unknown runtime externals, direct external variadic calls,
  indirect external call plans, external memory returns, external stack
  arguments, more than eight external args, and non-scalar-GPR external args or
  results.
- Keep F128, softfloat, inline asm, select, local-publication, pointer, and
  aggregate ABI work out of this plan.
- Preserve fail-closed diagnostics for unsupported runtime externals outside
  the accepted policy.

## Proof

Step 3 focused pre-proof passed:

```sh
cmake --build build -j2 --target backend_riscv_object_emission_test && ctest --test-dir build -j2 --output-on-failure -R '^backend_riscv_object_emission$'
```

Representative probes passed after rebuilding `c4cll`:

```sh
./build/c4cll -I tests/c/external/gcc_torture --target riscv64-linux-gnu --codegen asm tests/c/external/gcc_torture/src/20000917-1.c
./build/c4cll -I tests/c/external/gcc_torture --target riscv64-linux-gnu --codegen asm tests/c/external/gcc_torture/src/20020206-1.c
```

Step 3 delegated backend proof passed and is captured in `test_after.log`:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```
