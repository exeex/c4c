Status: Active
Source Idea Path: ideas/open/437_rv64_runtime_external_abort_exit_support.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Runtime External Coverage

# Current Packet

## Just Finished

Completed Step 1: classified the RV64 runtime external `abort`/`exit` policy
and selected the first bounded coverage packet.

Classification summary:

- Representative prepared facts show `abort` as a direct fixed-arity external
  runtime call with zero arguments in both `20000917-1.c` and `20020206-1.c`.
- Representative prepared facts show `exit` as a direct fixed-arity external
  runtime call with one scalar GPR immediate argument `0` to `a0`.
- The precise prepared asm residual is the module-level runtime external
  symbol policy: `unsupported_external_call ... callee='abort' ...
  reason=unsupported runtime external symbol`.
- `prepared_module_emit.cpp` currently admits only `strlen` in the runtime
  external symbol policy; existing prepared/object call emitters already lower
  scalar-compatible direct fixed-arity calls through normal call relocation
  once the symbol policy admits them.
- Accepted policy: explicitly admit ordinary C runtime termination helpers
  `abort` and `exit` under the existing direct fixed-arity scalar-GPR external
  call contract. Keep `strlen` unchanged.
- This is not a broad unknown-external policy and not runtime-helper
  infrastructure.
- Classification artifact:
  `build/agent_state/437_step1_runtime_external_policy/classification.md`.

## Suggested Next

Execute Step 2: Add Focused Runtime External Coverage.

First bounded packet:

- Primary file: `tests/backend/mir/backend_riscv_object_emission_test.cpp`.
- Add focused coverage for accepted `abort` zero-argument direct fixed-arity
  runtime external calls.
- Add focused coverage for accepted `exit` one-scalar-GPR direct fixed-arity
  runtime external calls.
- Add or preserve a focused rejected unknown runtime external symbol case.
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
- Keep unsupported: unknown runtime externals, direct external variadic calls,
  indirect external call plans, external memory returns, external stack
  arguments, more than eight external args, and non-scalar-GPR external args or
  results.
- Keep F128, softfloat, inline asm, select, local-publication, pointer, and
  aggregate ABI work out of this plan.
- Preserve fail-closed diagnostics for unsupported runtime externals outside
  the accepted policy.

## Proof

Original Step 1 delegated backend proof passed:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Metadata-only correction proof:

```sh
git diff --check
```
