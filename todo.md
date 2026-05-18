Status: Active
Source Idea Path: ideas/open/293_aarch64_side_effect_control_value_publication_authority.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Repair Side-Effecting Expression Result Publication

# Current Packet

## Just Finished

Step 2 sampled the remaining `src/00164.c` side-effecting expression surface
after the `src/00202.c` repair. The sample shows Step 2 is not complete:
`src/00202.c` now passes, but `src/00164.c` still fails before the Step 3
control/select cases because the assignment-like `c + d` value is not
published for either the plain call argument or `(y = c + d)`.

Prepared-code evidence for `src/00164.c`:

```text
%t1 = bir.load_local i32 %lv.c
%t2 = bir.load_local i32 %lv.d
%t3 = bir.add i32 %t1, %t2
%t4 = bir.call i32 printf(ptr @.str0, i32 %t3)
%t6 = bir.load_local i32 %lv.c
%t7 = bir.load_local i32 %lv.d
%t8 = bir.add i32 %t6, %t7
bir.store_local %lv.y, i32 %t8
%t9 = bir.call i32 printf(ptr @.str0, i32 %t8)
```

Prepared metadata says `%t3` is a frame-slot value consumed as the first
`printf` argument from `stack+32`, and `%t8` is a register value with a
prepared store-local memory access for `%lv.y`.

Generated AArch64 currently emits:

```asm
ldr w13, [sp, #8]
mov x0, x20
bl printf
mov x13, x0
str w13, [sp, #24]
mov x0, x20
mov x1, x13
bl printf
```

That sequence loads only `c`, never materializes `c + d` into `%t3`/`x1`, and
stores the first `printf` return value into `y`'s local slot before the second
print. The runtime confirms the first two lines are still wrong:
expected `134`, `134`; actual garbage, `12`.

## Suggested Next

Stay in Step 2 for one more code packet: repair AArch64 publication/order for
prepared scalar `add` results whose consumers include both a frame-slot
variadic call argument and a following `store_local` assignment result, using
`src/00164.c` plus already-green `src/00202.c` as the proof subset.

Proposed proof command:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00164|00202)_c$' -j 4 --timeout 5 --output-on-failure; } > test_after.log 2>&1
```

## Watchouts

- Do not fold the `src/00183.c` conditional-expression gap into the next Step 2
  packet; it is still the Step 3 block-entry/out-of-SSA
  `select_materialization` half of the publication boundary.
- `00164` has later logical/select, compare, bitwise, and arithmetic symptoms,
  but the first actionable Step 2 bug is earlier: the `c + d` result for
  `%t3`/`%t8` is not computed/published before its call-argument and
  assignment consumers.
- The first callsite metadata wants `arg1 bank=gpr from=frame_slot:stack+32 to=x1`;
  current assembly does not emit the frame-slot publication or the `x1`
  materialization for that argument.
- The prepared store-local access for `%t8` targets `%lv.y`; current assembly
  instead stores `x0` from the previous `printf` call to `y`'s slot.
- Keep pointer/address-heavy cases `src/00172.c` and `src/00217.c` deferred
  unless the same scalar side-effect/control publication primitive owns them.
- Do not touch expected outputs, allowlists, unsupported classifications,
  timeout policy, runner behavior, CTest registration, or build/test
  infrastructure.

## Proof

Ran the delegated focused sample exactly under `/tmp`:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00164|00202)_c$' -j 4 --timeout 5 --output-on-failure; } > /tmp/c4c_aarch64_side_effect_step2_00164_sample.log 2>&1
```

Result: failed overall, 1/2 passing. `00202` passed. `00164` remains
`RUNTIME_MISMATCH`.

Stale-process check:

```sh
pgrep -af '^/workspaces/c4c/build-aarch64-scan/c_testsuite_aarch64_backend/' || true
```

Result: no generated runtime process remained.

Additional artifacts:

- `/tmp/c4c_aarch64_side_effect_step2_00164_sample.log`
- `/tmp/c4c_aarch64_side_effect_step2_00164_prepared.txt`
- `/tmp/c4c_aarch64_side_effect_step2_00164_mir.txt`
- `/tmp/c4c_aarch64_side_effect_step2_00164_direct.s`
