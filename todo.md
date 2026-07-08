Status: Active
Source Idea Path: ideas/open/612_rv64_instruction_fragment_consumers.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh And Sub-Bucket Instruction-Fragment Diagnostics

# Current Packet

## Just Finished

Step 1 from `plan.md` refreshed current RV64 gcc_torture backend-object
diagnostics after the delegated backend proof. Current per-case logs under
`build/rv64_gcc_c_torture_backend/src_*/case.log` contain 104
`unsupported_instruction_fragment` residuals.

Refreshed first-owner / fragment-family groups:

- RV64 consumer candidate, narrow integer bitfield `BinaryInst`: 6 rows.
  `lshr` positives: `src/20030714-1.c`, `src/20040709-2.c`,
  `src/20040709-3.c`, `src/960608-1.c`. Adjacent `and`/clear rows:
  `src/931110-1.c`, `src/pr37882.c`.
- RV64 consumer candidate, pointer `BinaryInst` add/address arithmetic: 20
  rows, including `src/20000801-1.c`, `src/20050502-1.c`,
  `src/pr41395-2.c`, `src/pr41463.c`, `src/ssad-run.c`,
  `src/usad-run.c`.
- Mixed cast/select truth materialization, i32 `CastInst`: 21 rows, including
  `src/20010604-1.c`, `src/20020506-1.c`, `src/20090113-2.c`,
  `src/pr81555.c`, `src/vprintf-chk-1.c`.
- Narrow integer `BinaryInst` other than named bitfield `lshr`/clear: 17
  rows, including `src/20150611-1.c`, `src/921123-1.c`,
  `src/pr39233.c`, `src/pr84524.c`, `src/strcpy-1.c`.
- Call/result lowering or runtime-library policy `CallInst` with value result:
  23 rows, including `src/20030221-1.c`, `src/20030626-1.c`,
  `src/printf-1.c`, `src/va-arg-9.c`.
- Select publication `SelectInst`: 6 rows, including `src/20000815-1.c`,
  `src/950426-1.c`, `src/960402-1.c`, `src/pr21173.c`.
- Inline asm or policy call carrier `CallInst` with `owner=none`: 6 rows,
  including `src/pr54937.c`, `src/pr68381.c`, `src/pr71554.c`.
- Floating-only/policy rows: 5 rows: `StoreLocalInst` float/double
  `src/20050121-1.c`, `src/991019-1.c`, `src/complex-3.c`; f128
  `CastInst` `src/930622-2.c`, `src/ieee_pr29302-1.c`.

Selected Step 2 implementation family: narrow integer bitfield `BinaryInst`
consumer lowering for prepared RV64 GPR-backed i8/i16 `lshr` and adjacent
`and`/clear fragments, requiring complete prepared storage facts and preserving
fail-closed behavior for pointer, cast, select, call, and floating rows.
Focused probes show representative prepared facts:
`src/20030714-1.c` has `%t2.bf.shr = bir.lshr i8 %t2.bf.unit, 4` with GPR
storage; `src/20040709-2.c` / `src/20040709-3.c` have
`%t2.bf.shr = bir.lshr i16 %t2.bf.unit, 5`; `src/pr37882.c` has
`%t2.bf.clr = bir.and i8 %t2.bf.old, -8`; `src/931110-1.c` has
`%t12.bf.clr = bir.and i16 %t12.bf.old, -8`.

## Suggested Next

Execute Step 2 from `plan.md`: implement RV64/MIR object lowering for the
narrow integer bitfield `BinaryInst` family above. Positive candidates:
`src/20030714-1.c`, `src/20040709-2.c`, `src/20040709-3.c`,
`src/960608-1.c`, with adjacent `and`/clear candidates `src/pr37882.c` and
`src/931110-1.c` if the same semantic lowering rule covers them without
guessing.

Exact Step 2 proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

## Watchouts

- Negative guard rows to keep out of the Step 2 patch: pointer `BinaryInst`
  rows such as `src/20000801-1.c`, `src/20050502-1.c`, `src/pr41395-2.c`;
  i32 cast rows such as `src/20010604-1.c`, `src/20020506-1.c`;
  select-publication rows such as `src/960402-1.c`, `src/pr21173.c`;
  call/policy rows such as `src/pr54937.c`, `src/pr68381.c`,
  `src/printf-1.c`, `src/va-arg-9.c`; floating rows such as
  `src/930622-2.c`, `src/ieee_pr29302-1.c`, `src/20050121-1.c`.
- Do not widen Step 2 into pointer arithmetic, casts, select publication, call
  result/publication, inline asm, floating/f128, ABI, runtime, expectations,
  unsupported markers, allowlists, timeout, or accounting.
- The 17 "narrow integer other" `BinaryInst` rows were not selected because the
  operation mix was not inspected deeply enough for the first implementation
  packet; keep them as later adjacent candidates only after the bitfield family
  proves semantic lowering rather than testcase-shaped matching.

## Proof

Ran delegated proof command exactly:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed, 346/346 backend tests passed. Proof log: `test_after.log`.
