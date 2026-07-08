Status: Active
Source Idea Path: ideas/open/612_rv64_instruction_fragment_consumers.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Broaden Within The Same Fragment Family

# Current Packet

## Just Finished

Step 3 from `plan.md` implemented the adjacent narrow integer `BinaryInst`
add-immediate RV64 object consumer for i8/i16 `bir.add` with complete prepared
facts, GPR-backed operands, and GPR or stack result homes. The lowering is
semantic, not testcase-shaped: it gates on `BinaryOpcode::Add`, matching
i8/i16 operand/result types, `+1` or `-1` immediate RHS only, and existing
prepared homes; it emits `addi`, masks back to the narrow width for wraparound,
and publishes through the prepared GPR or stack result home.

Unit coverage added:

- Positive: i8/i16 `+1` and `-1` GPR-result add-immediate rows validate
  zero-extend/add/zero-extend/return publication.
- Positive: i16 stack-result `-1` validates halfword stack publication.
- Guards: add by non-adjacent immediate and mismatched immediate type continue
  to reject through `unsupported_instruction_fragment`.

Direct RV64 gcc_torture backend-object probes:

- `src/doloop-1.c`: passed; moved past `i8 %t4 = bir.add i8 %t3, -1`.
- `src/doloop-2.c`: passed; moved past `i16 %t4 = bir.add i16 %t3, -1`.
- `src/921123-1.c`: passed; moved past `i16 %t2 = bir.add i16 %t1, -1`.
- `src/pr69320-1.c`: passed; moved past `i8 %t11 = bir.add i8 %t10, 1`.
- `src/pr69320-3.c`: passed; moved past `i16 %t7 = bir.add i16 %t6, 1`.

Negative/direct guard probes remained out of scope:

- Pointer `BinaryInst`: `src/20000801-1.c` still stops at owner `ptr %t2`;
  `src/pr43560.c` still stops at owner `ptr %t14`.
- Cast: `src/20030714-1.c` still stops at `CastInst` owner `i32 %t3`;
  `src/pr23467.c` still stops at `CastInst` owner `i32 %t1`.
- Call: `src/920501-9.c` still stops at `CallInst` owner `i32 %t13`.
- Select: `src/950426-1.c` still stops at `SelectInst` owner
  `ptr %t21.outer0.sel3`.
- Separate ashr row: `src/931110-1.c` still stops at `BinaryInst` owner
  `i16 %t13.bf.sext`; this packet did not fold it into add-immediate lowering.

Direct probe log:
`build/agent_state/612_step3_add_immediate_direct_probes.log`.

## Suggested Next

Recommended next packet: either refresh the Step 3 residual set after the
add-immediate movement or take a bounded adjacent narrow integer `BinaryInst`
micro-slice for the remaining i16 `ashr` row only if diagnostics show more than
the single `src/931110-1.c` case or a shared narrow immediate-op abstraction is
available.

## Watchouts

- Pointer `BinaryInst`, cast, call, select, non-instruction policy, and runtime
  rows remained intentionally out of this packet.
- The add-immediate lowering currently supports only immediate RHS `+1` and
  `-1`; widening it to arbitrary immediates should be a separate semantic
  decision because this packet only proved adjacent narrow wraparound movement.
- `src/931110-1.c` remains a separate `ashr` residual and should not be claimed
  as covered by this add-immediate slice.

## Proof

Ran delegated proof command exactly and preserved the output in `test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed; `test_after.log` reports total real test time `2.09 sec`.

Focused validation also run:

- `cmake --build --preset default --target backend_riscv_object_emission_test`
  passed.
- `build/tests/backend/mir/backend_riscv_object_emission_test` passed.
- Direct RV64 gcc_torture backend-object probes for the five positive rows and
  listed guard rows passed/failed as expected; log path:
  `build/agent_state/612_step3_add_immediate_direct_probes.log`.
