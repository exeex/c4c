Status: Active
Source Idea Path: ideas/open/293_aarch64_side_effect_control_value_publication_authority.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Repair Side-Effecting Expression Result Publication

# Current Packet

## Just Finished

Step 1 located the starter authority loss at the prepared value publication
boundary between prealloc move/value-location facts and AArch64 codegen
emission.

Failure evidence:

- `src/00202.c` has correct prepared side-effect semantics:
  `%t2 = bir.mul i32 %t1, 2`, `bir.store_local %lv.bob, i32 %t2`, and the
  first `printf` argument plan reads bob from a prepared frame-slot value.
  Generated assembly never emits the `mul` result publication; it later does
  `ldr w9, [sp, #8]` and stores that stale saved-register slot into bob, so
  output is `bob: 1` instead of `bob: 42`.
- `src/00183.c` has correct prepared control-selected semantics:
  `%t21 = bir.select slt i32 %t7, 5, i32 %t18, %t20` with
  `select_materialization` join transfers for `%t18 -> %t21` and `%t20 -> %t21`.
  Generated assembly computes branch-local values in `w20`, but the join emits
  no prepared block-entry/out-of-SSA copy to the call argument; `printf` uses
  stale `x13`, producing `0..9` instead of squares/triples.
- `src/00164.c` combines both shapes. Prepared scalar call arguments name
  computed values such as `%t3`, `%t8`, `%t29`, `%t50`, and later compare/logical
  materializations, but generated assembly repeatedly prints stale call results
  or old scalar registers instead of the prepared computed values.

First concrete implementation boundary: AArch64 codegen must emit prepared
scalar publication moves, starting with scalar ALU results whose authoritative
home is a frame slot, then block-entry/out-of-SSA `select_materialization`
copies. The files implicated by the boundary are
`src/backend/mir/aarch64/codegen/alu.cpp`,
`src/backend/mir/aarch64/codegen/memory.cpp`, and the dispatch gap for
`PreparedMovePhase::BlockEntry` bundles in
`src/backend/mir/aarch64/codegen/dispatch.cpp`/move lowering.

## Suggested Next

Implement the smallest Step 2 side-effecting expression primitive: prepared
scalar ALU results with frame-slot homes must publish their computed scratch
register back to the prepared frame slot before local-store/call consumers read
that value. Use `src/00202.c` as the primary proof and keep `src/00164.c` as
the mixed-shape guard.

## Watchouts

- Do not fold the `src/00183.c` conditional-expression gap into the first
  side-effecting-expression packet unless the same prepared move emission helper
  naturally covers both; it is the Step 3 block-entry/out-of-SSA
  `select_materialization` half of the same publication boundary.
- `src/00164.c` still shows closed-owner-looking symptoms around call
  arguments and compare/logical values, but prepared call plans already name the
  right argument sources. Treat this as missing publication from prepared values,
  not a reason to reopen closed scalar call, call-argument, switch/control, or
  function-pointer owners.
- Keep pointer/address-heavy cases `src/00172.c` and `src/00217.c` deferred
  unless the same scalar side-effect/control publication primitive owns them.
- Do not touch expected outputs, allowlists, unsupported classifications,
  timeout policy, runner behavior, CTest registration, or build/test
  infrastructure.

## Proof

Ran the delegated proof exactly:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00164|00183|00202)_c$' -j 4 --timeout 5 --output-on-failure; } > test_after.log 2>&1
```

Result: failed as expected, 0/3 passing. All three starter reps are
`RUNTIME_MISMATCH`.

Stale-process check:

```sh
pgrep -af '^/workspaces/c4c/build-aarch64-scan/c_testsuite_aarch64_backend/' || true
```

Result: no generated runtime process remained.

Supporting dumps:

- `/tmp/c4c_aarch64_side_effect_step1_00164.prepared.txt`
- `/tmp/c4c_aarch64_side_effect_step1_00183.prepared.txt`
- `/tmp/c4c_aarch64_side_effect_step1_00202.prepared.txt`
