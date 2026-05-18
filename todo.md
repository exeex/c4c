# AArch64 Scalar Expression Control-Value Authority Todo

Status: Active
Source Idea Path: ideas/open/292_aarch64_scalar_expression_control_value_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The First Scalar Publication Primitive

# Current Packet

## Just Finished

Step 2 repaired the regression from the first scalar publication primitive.
The immediate return-publication fallback in
`src/backend/mir/aarch64/codegen/alu.cpp` is now limited to the semantic stale
publication case: a rematerializable immediate return result whose ALU source
depends on a named authoritative immediate that has not been emitted as a
selected scalar register. This preserves selected public `add`/`sub` chains
while keeping the `src/00012.c` repair.

The packet also fixed the directly required AArch64 call-boundary helper in
`src/backend/mir/aarch64/codegen/calls.cpp`: immediate GPR call arguments now
prefer the typed prepared register placement when materializing the ABI
destination, so the internal return-lowering proof selects `w0` for an I32
immediate argument instead of losing the move.

`src/00012.c` remains passing. Generated assembly still contains authoritative
return publication:

```asm
mov w0, #0
add w0, w0, #0
ret
```

The public backend regression cases again print the selected chains:
`return_add.c` emits `mov w0, #2` then `add w0, w0, #3`, and
`return_add_sub_chain.c` emits `mov w0, #2`, `add w0, w0, #3`, then
`sub w0, w0, #1`. The focused subset now has `00012.c` and `00211.c` passing;
`00009.c`, `00056.c`, `00156.c`, and `00161.c` remain failing in other scalar
publication/control-value subfamilies.

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
- `src/00211.c` now passes after the immediate GPR call-argument placement fix;
  do not spend the next packet on that representative unless it regresses.
- `src/00056.c`'s prepared call argument names `source_encoding=frame_slot` and
  `source_stack_offset=24`, but the generated local store for `d` is currently
  at `[sp, #12]`; verify the authoritative stack-slot offset model before
  printing a load, because blindly loading `[sp, #24]` would read the saved
  `x21` slot in the current assembly layout.
- `clang-format` is not installed in this environment.

## Proof

Ran exactly:

```sh
{ cmake --build build -j && ctest --test-dir build -R '^(backend_aarch64_return_lowering|backend_cli_aarch64_asm_external_return_add_smoke|backend_cli_aarch64_asm_external_return_add_sub_chain_smoke)$' -j 4 --output-on-failure; } > test_after.log 2>&1
```

Result: passed, 3/3. Proof log: `test_after.log`.

Also ran exactly:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00009|00012|00056|00156|00161|00211)_c$' -j 4 --timeout 5 --output-on-failure; } > /tmp/c4c_aarch64_scalar_step2_regression_fix_subset.ctest.log 2>&1
```

Result: failed overall with 2/6 passing. `00012` and `00211` passed; `00009`,
`00056`, `00156`, and `00161` still failed. Focused subset log:
`/tmp/c4c_aarch64_scalar_step2_regression_fix_subset.ctest.log`.

Required process check:

```sh
pgrep -af '^/workspaces/c4c/build-aarch64-scan/c_testsuite_aarch64_backend/' || true
```

No generated runtime process remained.
