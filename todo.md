# AArch64 Scalar Expression Control-Value Authority Todo

Status: Active
Source Idea Path: ideas/open/292_aarch64_scalar_expression_control_value_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The First Scalar Publication Primitive

# Current Packet

## Just Finished

Step 2 implemented the first scalar publication primitive in
`src/backend/mir/aarch64/codegen/alu.cpp`: when a prepared scalar result has
authoritative rematerializable-immediate storage and is being published to the
return ABI, lowering now materializes that immediate into the ABI result
register instead of falling back to stale physical-register reconstruction.

`src/00012.c` advanced from `sub w0, w19, #8` and runtime nonzero to passing.
Generated assembly now contains authoritative return publication:

```asm
mov w0, #0
add w0, w0, #0
ret
```

The same proof shows the remaining starter failures are still in other
publication subfamilies: `00056.c` still omits the `x2` call argument for the
frame-slot scalar source, `00211.c` still passes a stale scalar variadic
argument, and `00009.c`/`00156.c`/`00161.c` still fail in broader scalar
result/control-value publication.

## Suggested Next

Continue Step 2 with the next smallest shared primitive in
`src/backend/mir/aarch64/codegen/calls.cpp`: materialize prepared scalar
frame-slot sources feeding call-argument ABI registers. Use `src/00056.c` as
the first representative and prove that the third `printf` call materializes
the authoritative `x2`/`w2` value before `bl printf`.

## Watchouts

- Do not solve this by filename checks, expectation changes, allowlist changes,
  unsupported downgrades, or CTest edits.
- Keep pointer/aggregate address failures, timeout/hang cases, and
  compile-stage printer gaps out of this owner.
- `src/00056.c` confirms the third `printf` format pointer is correct; only the
  scalar second variadic value is missing. Treat it as scalar ABI argument
  materialization, not the closed string/global address owner.
- `src/00056.c`'s prepared call argument names `source_encoding=frame_slot` and
  `source_stack_offset=24`, but the generated local store for `d` is currently
  at `[sp, #12]`; verify the authoritative stack-slot offset model before
  printing a load, because blindly loading `[sp, #24]` would read the saved
  `x21` slot in the current assembly layout.
- `clang-format` is not installed in this environment.

## Proof

Ran exactly:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00009|00012|00056|00156|00161|00211)_c$' -j 4 --timeout 5 --output-on-failure; } > test_after.log 2>&1
```

Result: failed overall with 1/6 passing. `00012` passed; `00009`, `00056`,
`00156`, `00161`, and `00211` still failed. Proof log: `test_after.log`.

Required process check:

```sh
pgrep -af '^/workspaces/c4c/build-aarch64-scan/c_testsuite_aarch64_backend/' || true
```

No generated runtime process remained.
