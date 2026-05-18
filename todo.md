Status: Active
Source Idea Path: ideas/open/293_aarch64_side_effect_control_value_publication_authority.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Repair Side-Effecting Expression Result Publication

# Current Packet

## Just Finished

Step 2 repaired the first assignment-like `src/00164.c` scalar `add`
publication/order primitive while preserving `src/00202.c`. AArch64 now
computes `c + d`, publishes the frame-slot result before the first variadic
call argument, and keeps the register result live for the following
`store_local` assignment.

The continuation extended the same publication path to prepared scalar
bitwise and signed arithmetic chains: fallback scalar lowering can now
materialize prepared frame-slot operands for `and`/`or`/`xor` and
`mul`/`sdiv`/`srem`, and before-call argument moves can consume an already
emitted scalar register instead of reloading that value from a logical spill
slot. Generated assembly now reaches the later `00164` bitwise/arithmetic
forms and emits native operations such as:

```asm
ldr w9, [sp, #20]
ldr w10, [sp, #20]
orr w9, w9, w10
...
ldr w9, [sp, #8]
ldr w10, [sp, #12]
and w9, w9, w10
...
eor w9, w9, w9
...
mul w9, w20, w10
sdiv w9, w9, w20
```

Generated AArch64 now emits the first two `c + d` forms as:

```asm
ldr w13, [sp, #8]
ldr w10, [sp, #12]
add w9, w13, w10
str w9, [sp, #32]
mov x0, x20
ldr w1, [sp, #32]
bl printf
ldr w9, [sp, #8]
ldr w10, [sp, #12]
add w13, w9, w10
str w13, [sp, #24]
mov x0, x20
mov x1, x13
bl printf
```

The focused proof still fails overall because `00164` now reaches an
in-block compare/select materialization boundary after the first six passing
lines. In `logic.end.100`, the emitted `orr` for `%t103` is followed by
unlowered `%t104 = ne %t103, 0` and a large in-block `%t106 = select ...`
chain; the following `printf` still consumes stale `x13`. Later comparison
arguments (`a == a`, `a == b`, `a != a`, `a != b`) and arithmetic call results
also still require the same compare/select or expression-chain result
publication, rather than another add/bitwise frame-slot source repair.
`00202` remains passing.

## Suggested Next

Hand the remaining `00164` failure to the compare/select materialization owner
or split Step 3: the exact first blocker is `%t104 = bir.ne i32 %t103, 0`
followed by `%t106 = bir.select ...` in prepared block `logic.end.100`. The
current Step 2 scalar publication slice can compute and publish the direct
bitwise/arithmetic operations, but it does not own lowering in-block select
trees or compare-result materialization.

Proposed proof command:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00164|00202)_c$' -j 4 --timeout 5 --output-on-failure; } > test_after.log 2>&1
```

## Watchouts

- Do not fold broad `src/00183.c` conditional-expression work into this Step 2
  packet. The remaining `00164` blocker is the same class of in-block
  compare/select materialization, and should be owned by the Step 3 route.
- The initial `c + d` publication/order bug is repaired. Do not rework that
  path unless a broader proof shows a real regression.
- The focused proof still reports `00164` as `RUNTIME_MISMATCH`; current output
  has the first six expected lines passing (`134`, `134`, `0`, `1`, `1`, `1`)
  before the first stale select result.
- Frame-slot call-argument sources now prefer prepared memory-access facts
  when present, and only fall back to the value-home stack offset for
  published scalar results such as `%t3` that have no separate load access.
- Keep pointer/address-heavy cases `src/00172.c` and `src/00217.c` deferred
  unless the same scalar side-effect/control publication primitive owns them.
- Do not touch expected outputs, allowlists, unsupported classifications,
  timeout policy, runner behavior, CTest registration, or build/test
  infrastructure.

## Proof

Ran the delegated focused proof exactly:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00164|00202)_c$' -j 4 --timeout 5 --output-on-failure; } > test_after.log 2>&1
```

Result: failed overall, 1/2 passing. `00202` passed. `00164` remains
`RUNTIME_MISMATCH`, but materially advanced: the first six expected outputs
now pass, scalar `or`/`and`/`xor` and `mul`/`sdiv` are emitted, and direct
before-call moves can use emitted scalar registers. The first remaining bad
form is the in-block `%t104` compare plus `%t106` select chain in
`logic.end.100`.

Because shared AArch64 backend code changed, also ran:

```sh
ctest --test-dir build -R '^backend_aarch64_' -j 8 --output-on-failure > /tmp/c4c_aarch64_side_effect_step2_00164_backend_aarch64.log 2>&1
```

Result: passed, 27/27.

Stale-process check:

```sh
pgrep -af '^/workspaces/c4c/build-aarch64-scan/c_testsuite_aarch64_backend/' || true
```

Result: no generated runtime process remained.

Additional check: `git diff --check` passed.

Log paths:

- `test_after.log`
- `/tmp/c4c_aarch64_side_effect_step2_00164_backend_aarch64.log`
