Status: Active
Source Idea Path: ideas/open/612_rv64_instruction_fragment_consumers.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Broaden Within The Same Fragment Family

# Current Packet

## Just Finished

Step 3 from `plan.md` refreshed RV64 gcc_torture backend-object residuals after
commit `8ac2230f3` (`Lower RV64 narrow bitfield fragments`) without code,
expectation, marker, allowlist, plan, idea, or docs edits.

Refresh command:
`scripts/check_progress_rv64_gcc_c_torture_backend.sh`

Refresh result: `total=1467 passed=500 failed=967`; full scan output is in
`build/agent_state/612_step3_residual_refresh_after_8ac2230f3.log`.
Current `unsupported_instruction_fragment` residuals: 115 rows, grouped by
first unsupported instruction kind:

- `BinaryInst`: 42 rows: 23 `ptr`, 10 `i16`, 9 `i8`.
- `CastInst`: 27 rows: 23 `i32`, 2 `f128`, 2 ownerless/unnamed in the first
  diagnostic line.
- `CallInst`: 36 rows.
- `SelectInst`: 7 rows.
- `StoreLocalInst`: 3 rows.

Broader refreshed first-diagnostic groups among failing rows include 144
prepared move-bundle fan-in rows, 115 `unsupported_instruction_fragment` rows,
52 `unsupported_call_abi`, 37 `unsupported_move_bundle_target_shape`, 35
`unsupported_local_memory_access`, 25 `unsupported_terminator_fragment`, 24
`unsupported_global_data`, 21 `unsupported_inline_asm_fragment`, 12
`unsupported_stack_frame`, 8 malformed join-transfer carrier rows, 7
`unsupported_scalar_compare_publication`, 7 branch stack-load authority rows, 5
`unsupported_pointer_arithmetic`, 5 `unsupported_param_home`, 5
`unsupported_floating_cast`, 3 branch stack-load freshness rows, and 3
ambiguous move-bundle source-freshness rows.

Actual Step 2 movement is confirmed in the refreshed object scan:

- `src/20030714-1.c`: moved past `i8 %t2.bf.shr`; current residual is
  `CastInst` owner `i32 %t3`.
- `src/20040709-2.c`: moved past the selected i16 bitfield shifts/recombine;
  current residual is `unsupported_floating_cast`.
- `src/20040709-3.c`: moved past the selected i16 bitfield shifts/recombine;
  current residual is `unsupported_floating_cast`.
- `src/960608-1.c`: moved past `i8 %t10.bf.shr`; current residual is prepared
  move-bundle fan-in to one stack destination at `main`, `entry`,
  `instruction_index=33`.
- `src/pr37882.c`: moved past `i8 %t2.bf.vm` and recombine; current residual is
  runtime mismatch (`[RV64_BACKEND_RUNTIME_MISMATCH]`), not an
  instruction-fragment stop.
- `src/931110-1.c`: moved past `i16 %t12.bf.clr` and recombine; current
  residual is adjacent `BinaryInst` owner `i16 %t13.bf.sext`, prepared as
  `bir.ashr i16 %t13.bf.shl, 13`.

Focused prepared-BIR probes found a larger adjacent narrow integer
`BinaryInst` family with complete prepared homes: i8/i16 `bir.add` by small
immediates feeding local/global store publication and zext/sext consumers.
Representative positive rows: `src/doloop-1.c` (`i8 %t4 = bir.add i8 %t3,
-1`, GPR result), `src/doloop-2.c` (`i16 %t4 = bir.add i16 %t3, -1`, stack
result), `src/921123-1.c` (`i16 %t2 = bir.add i16 %t1, -1`, stack result),
`src/pr69320-1.c` (`i8 %t11 = bir.add i8 %t10, 1`, global-store source), and
`src/pr69320-3.c` (`i16 %t7 = bir.add i16 %t6, 1`, stack result).

## Suggested Next

Recommended next packet: implement a bounded Step 3 adjacent narrow integer
`BinaryInst` add-immediate consumer for i8/i16 GPR-backed operands with complete
prepared facts and supported GPR or stack result homes, covering both `+1` and
`-1` immediate forms only if the existing RV64 object route can emit the result
with correct narrow wraparound and publication semantics.

Positive rows to prove: `src/doloop-1.c`, `src/doloop-2.c`, `src/921123-1.c`,
`src/pr69320-1.c`, and `src/pr69320-3.c`.

Negative guards: keep `ptr` `BinaryInst` rows such as `src/20000801-1.c` and
`src/pr43560.c` out of scope; keep cast rows such as `src/20030714-1.c` and
`src/pr23467.c`, call rows such as `src/920501-9.c`, select rows such as
`src/950426-1.c`, and non-instruction policy rows such as
`unsupported_floating_cast`, move-bundle fan-in, inline asm, global-data, ABI,
and runtime-mismatch rows out of scope. Treat the single remaining bitfield
`ashr` row (`src/931110-1.c`) as adjacent evidence but not enough by itself for
a standalone packet unless the supervisor chooses an `ashr`-only micro-slice.

## Watchouts

- The remaining residual set is mixed. A same-family Step 3 packet is viable
  only if scoped to adjacent narrow integer `BinaryInst` immediate arithmetic;
  otherwise the next lifecycle decision should be Step 4 classification or a
  plan-owner split.
- Pointer `BinaryInst` rows are numerous (23) but are not the same narrow
  integer family and should remain separate from the add-immediate packet.
- `src/pr37882.c` now reaches runtime mismatch, so it should not be claimed as
  object-lowering completion without runtime investigation outside this packet.
- Do not fold the single `bf.sext`/`ashr` row into add-immediate lowering unless
  the implementation naturally supports both under one semantic narrow
  immediate-op abstraction without testcase-shaped matching.

## Proof

Ran delegated proof command exactly and preserved the output in
`test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed; `test_after.log` reports total real test time `2.01 sec`.

Diagnostics also run for this packet:
`scripts/check_progress_rv64_gcc_c_torture_backend.sh`

Result: expected nonzero scan exit because failing torture rows remain;
diagnostic log path:
`build/agent_state/612_step3_residual_refresh_after_8ac2230f3.log`.
