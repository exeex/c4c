Status: Active
Source Idea Path: ideas/open/430_rv64_integer_div_rem_lowering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Re-Probe Representatives And Decide Close Readiness

# Current Packet

## Just Finished

Completed Step 4, Re-Probe Representatives And Decide Close Readiness.

Fresh representative probes were recorded under
`build/agent_state/430_step4_close_readiness/` using prepared-BIR dumps and the
RV64 object route for:

- `src/20021120-3.c`
- `src/20030105-1.c`
- `src/20090113-2.c`
- `src/20090113-3.c`
- `src/20100416-1.c`

Focused div/rem acceptance coverage now stands at:

- `UDiv i32` -> RV64 `divuw`
- `UDiv i64` -> RV64 `divu`
- `SDiv i32` -> RV64 `divw`
- `SDiv i64` -> RV64 `div`
- `URem i32` -> RV64 `remuw`
- `URem i64` -> RV64 `remu`
- `SRem i32` -> RV64 `remw`
- `SRem i64` -> RV64 `rem`

Focused fail-closed tests cover missing prepared operand/result homes and
pointer-typed div/rem forms with `unsupported_instruction_fragment`.

Fresh row classification:

- `src/20021120-3.c`: prepared route passes; object route still fails with
  `unsupported_instruction_fragment`; contains 3 `udiv i32` fragments plus
  calls, computed/frame call arguments, loads, and store-local publication.
- `src/20030105-1.c`: prepared route passes; object route still fails with
  `unsupported_instruction_fragment`; contains 1 `udiv i64` fragment plus
  select materialization, calls, loads, and store-local publication.
- `src/20090113-2.c`: prepared route passes; object route still fails with
  `unsupported_instruction_fragment`; contains 8 `udiv i32` and 2 `urem i32`
  fragments plus select materialization, frame/computed call arguments, loads,
  and store-local publication.
- `src/20090113-3.c`: prepared route passes; object route still fails with
  `unsupported_instruction_fragment`; contains 4 `udiv i32` and 2 `urem i32`
  fragments plus select materialization, frame/computed call arguments, loads,
  and store-local publication.
- `src/20100416-1.c`: prepared route passes and RV64 object route passes with
  1 `udiv i64` fragment.

Executor-side close-readiness result: this source idea appears ready for
lifecycle close review. Fresh probes do not expose a specific remaining scalar
integer div/rem opcode/type lowering gap; the still-failing representatives
need separate follow-up ownership for call publication/argument forms, select
materialization, local/store publication, and producer/global-addressing facts.

Step artifacts:

- `build/agent_state/430_step4_close_readiness/probe_summary.tsv`
- `build/agent_state/430_step4_close_readiness/evidence_snippets.md`
- `build/agent_state/430_step4_close_readiness/classification.md`

## Suggested Next

Ask the plan owner to perform lifecycle close review for
`ideas/open/430_rv64_integer_div_rem_lowering.md`, or split any remaining
non-div/rem representative failures into their own source ideas if broader
plan ownership needs them first.

## Watchouts

- Do not use divisor-specific handling, representative filenames, or row-shaped
  shortcuts.
- Preserve signedness: `sdiv`/`srem` must not route through unsigned behavior,
  and `udiv`/`urem` must not route through signed behavior.
- Do not claim the four still-failing representative rows as fixed; they still
  fail object-route probing, but the observed remaining classes are mixed
  non-div/rem residuals.
- Keep floating-point division, F128/I128 helper work, pointer/address,
  select/join, aggregate ABI, call publication, local/store publication, and
  global memory residuals outside this plan.
- Do not change expectations, allowlists, unsupported markers, runtime
  comparison, or pass/fail accounting.

## Proof

Step 4 close-readiness proof passed:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Proof log: `test_after.log`.
