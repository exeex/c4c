Status: Active
Source Idea Path: ideas/open/430_rv64_integer_div_rem_lowering.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Lower Or Validate Scalar Division

# Current Packet

## Just Finished

Completed Step 1, Audit Div/Rem Evidence And Choose Packet.

Audit artifacts are under `build/agent_state/430_step1_div_rem_audit/`:
`probe_summary.tsv`, `evidence_snippets.txt`, `classification.md`, and per-row
prepared/object logs.

Current RV64 object lowering state:

- `object_emission.cpp` already maps `SDiv` to `divw`/`div`, `UDiv` to
  `divuw`/`divu`, `SRem` to `remw`/`rem`, and `URem` to `remuw`/`remu`.
- The focused backend test `builds_prepared_scalar_divrem_object` covers
  signed division plus signed/unsigned remainder, but it does not include
  unsigned division cases.

Representative row classification:

- `src/20021120-3.c`: three `udiv i32` fragments; object route still rejects
  with `unsupported_instruction_fragment`; row also has calls and
  stack/local-publication residuals.
- `src/20030105-1.c`: one `udiv i64 32, 4` fragment; object route still rejects
  with `unsupported_instruction_fragment`; row also has select materialization,
  calls, and many local publications.
- `src/20090113-2.c`: eight `udiv i32` and two `urem i32` fragments; object
  route still rejects with `unsupported_instruction_fragment`; row also has
  select, frame-slot call-arg, and local-publication residuals.
- `src/20090113-3.c`: four `udiv i32` and two `urem i32` fragments; object
  route still rejects with `unsupported_instruction_fragment`; same residual
  class as `20090113-2.c`.
- `src/20100416-1.c`: one `udiv i64 80, 16` fragment; RV64 object route
  succeeds, showing at least one representative `UDiv` row already lowers.

No representative in this packet exposed `sdiv` or `srem`; signed division and
signed remainder are represented by focused backend tests rather than these
rows.

Chosen first packet: Step 2, a code-changing focused coverage/validation
packet rather than a known capability-repair packet. Add `UDiv` `i32` and
`i64` cases to focused RV64 object-emission div/rem coverage, and add or
tighten fail-closed scalar div/rem tests for missing homes or unsupported
forms if the local fixture supports it. Touch `object_emission.cpp` or
`prepared_function_emit.cpp` only if that focused coverage exposes a real
semantic bug.

Primary files for the next packet:
`tests/backend/mir/backend_riscv_object_emission_test.cpp`, `todo.md`,
`test_after.log`, and `src/backend/mir/riscv/codegen/object_emission.cpp` only
if focused coverage exposes an implementation defect.

Future implementation proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1
```

## Suggested Next

Execute Step 2: validate scalar division lowering by adding focused unsigned
division coverage for `i32`/`i64`, preserving existing signed division
coverage, and keeping unsupported/missing-home div/rem shapes fail-closed.

## Watchouts

- Do not use divisor-specific handling, representative filenames, or row-shaped
  shortcuts.
- Preserve signedness: `sdiv`/`srem` must not route through unsigned behavior,
  and `udiv`/`urem` must not route through signed behavior.
- Do not claim row-level representative progress from focused div/rem coverage;
  the failing representatives also contain call, select, frame-slot argument,
  local-publication, or global/select residuals.
- Keep floating-point division, F128/I128 helper work, pointer/address,
  select/join, aggregate ABI, call publication, and global memory residuals
  outside this plan.
- Do not change expectations, allowlists, unsupported markers, runtime
  comparison, or pass/fail accounting.

## Proof

Step 1 audit-only check:

```sh
git diff --check
```
