Status: Active
Source Idea Path: ideas/open/437_rv64_runtime_external_abort_exit_support.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Bounded Runtime External Support

# Current Packet

## Just Finished

Completed Step 2: added focused lower-level RV64 object-route coverage for the
accepted runtime external policy before implementing the upstream policy gate.

Coverage added in `tests/backend/mir/backend_riscv_object_emission_test.cpp`:

- `builds_prepared_runtime_abort_external_call_object`: proves a complete
  prepared `direct_extern_fixed_arity` zero-argument `abort` call emits an
  undefined function symbol plus `R_RISCV_CALL_PLT` relocation.
- `builds_prepared_runtime_exit_external_call_object`: proves a complete
  prepared `direct_extern_fixed_arity` one-scalar-GPR `exit(0)` call emits an
  undefined function symbol, call relocation, and `a0` status materialization.
- Unknown-runtime rejection was not added here because this object-route test
  surface accepts complete direct external calls uniformly; the reject/allow
  runtime symbol policy is owned by prepared module text emission and would be
  intentionally red for accepted `abort`/`exit` until Step 3.

Coverage artifact:
`build/agent_state/437_step2_runtime_external_coverage/summary.txt`.

## Suggested Next

Execute Step 3: implement the narrow runtime external policy gate.

First bounded packet:

- Primary implementation file:
  `src/backend/mir/riscv/codegen/prepared_module_emit.cpp`.
- Admit only accepted ordinary runtime termination helpers `abort` and `exit`
  through the existing direct fixed-arity scalar-GPR external call contract,
  preserving `strlen`.
- Add/keep focused policy coverage for rejected unknown runtime external
  symbols at the prepared module emission surface.
- Keep coverage semantic: no representative filename, caller name, block index,
  or dump-shape matching.

Future implementation proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Do not claim aggregate `sret` progress through runtime external support.
- Do not special-case `20000917-1`, `20020206-1`, `main`, `baz`, or one dump
  shape.
- Step 2 only proves the lower-level object consumer can emit complete
  accepted runtime external call facts; it does not implement or test the
  upstream prepared module allowlist change.
- Keep unsupported: unknown runtime externals, direct external variadic calls,
  indirect external call plans, external memory returns, external stack
  arguments, more than eight external args, and non-scalar-GPR external args or
  results.
- Keep F128, softfloat, inline asm, select, local-publication, pointer, and
  aggregate ABI work out of this plan.
- Preserve fail-closed diagnostics for unsupported runtime externals outside
  the accepted policy.

## Proof

Step 2 focused pre-proof passed:

```sh
cmake --build build -j2 --target backend_riscv_object_emission_test && ctest --test-dir build -j2 --output-on-failure -R '^backend_riscv_object_emission$'
```

Step 2 delegated backend proof passed and is captured in `test_after.log`:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```
