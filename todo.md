Status: Active
Source Idea Path: ideas/open/110_aarch64_remaining_baseline_failure_recovery.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Current Failure Facts

# Current Packet

## Just Finished

Step 1 refreshed the targeted AArch64 failure facts with the delegated build and
CTest subset. Current result: `00204` is stale-only/passing; live failures are
`backend_aarch64_instruction_dispatch`, `00172`, `00180`, `00216`, and `00220`.

Pass/fail and symptoms:
- PASS `c_testsuite_aarch64_backend_src_00204_c`.
- FAIL `backend_aarch64_instruction_dispatch`: the f64 global-select call
  route emits `fcsel d20` and later `fcsel d0` before `bl consume_f64`, but the
  test still expects the selected readback in `d20` to feed an explicit
  `fmov d0, d20` call-ABI move.
- FAIL `00172`: pointer equality/inequality runtime output is wrong after
  `d = e`; expected `... 1 0`, actual ends with `0 -1 -1417132352`, pointing
  at pointer compare/materialization or pointer-value reload lowering.
- FAIL `00180`: `printf("%s\n", &a[1])` prints `abcdef` instead of `bcdef`,
  pointing at local array element address/decay lowering for byte buffers.
- FAIL `00216`: aggregate initializer/copy stress case is mostly correct, but
  local aggregate copies differ at `ls` (`6 5 4 3` vs `1 2 3 4`) and downstream
  copied/compound-literal aggregate fields such as `lv2`/`lv3`, pointing at
  local aggregate copy/address lowering rather than global initializer data.
- FAIL `00220`: wide-string test exits with segmentation fault before output,
  pointing at wide string literal/local wide array materialization or traversal
  address lowering.

Likely shared backend capability groups:
- Highest priority shared cause: local addressable object lowering for arrays
  and aggregate copies. This covers `00180`, the local-copy portion of `00216`,
  and may also cover the `00220` crash if the wide string local array is
  addressed from the wrong base/offset.
- Separate scalar capability: pointer comparison/value materialization for
  `00172`.
- Separate f64 route capability: selected f64 value publication into the call
  ABI destination for `backend_aarch64_instruction_dispatch`.

## Suggested Next

Repair the local addressable object path first, starting from `00180` as the
smallest repro for array element address/decay. Then rerun the same targeted
subset to see whether `00216` and `00220` move with it before treating them as
separate aggregate or wide-string initiatives.

## Watchouts

Do not spend repair effort on `00204` in this route; the current targeted rerun
proves it is stale-only. Avoid expectation rewrites: all live failures are
runtime or internal-route symptoms that need lowering repairs. The f64 dispatch
failure may be either a real missing `fmov d0, d20` publication or a stale test
contract if the selected value is intentionally recomputed directly into `d0`;
review the call-argument publication invariant before changing code.

## Proof

Ran:
`cmake --build --preset default; ctest --test-dir build -j1 --output-on-failure -R '^(backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00(172|180|216|220|204)_c)$' > test_after.log 2>&1`

Build completed with no work to do. CTest result is the expected fact-refresh
failure set: 1/6 passed, 5/6 failed. Proof log: `test_after.log`.
