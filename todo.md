Status: Active
Source Idea Path: ideas/open/395_rv64_object_route_instruction_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Classify Instruction Fragment Families

# Current Packet

## Just Finished

Step 1 classified the current RV64 object-route instruction-fragment
representatives without implementation edits.

Current probe results:
- `src/20000223-1.c` now passes the delegated two-case probe, so it is not the
  current first repair family.
- `src/20020225-2.c` still fails with
  `unsupported_instruction_fragment: BIR instruction requires unsupported RV64
  object lowering`.
- Nearby same-diagnostic artifact selected from the current full backend work
  directory: `src/pr48973-2.c`, which also fails with the same public
  diagnostic and has the same scalar `bir.ashr` instruction family in a fresh
  prepared dump.

Rejected instruction family:
- Prepared scalar arithmetic right shift, `bir.ashr i32`.
- `src/20020225-2.c`: in `test`, `%t5 = bir.ashr i32 %p.x, %t4`.
  Operand homes are `%p.x` in GPR `a0` and `%t4` in GPR `s1`; result `%t5`
  is an `i32` in GPR `s2`. `%t4` is loaded from local frame slot #0
  (`offset=0 size=4 align=4`, proven in bounds). There are no immediates on
  the rejected `ashr`; required publication is the ordinary result home plus a
  before-return move from `%t5` to return ABI `a0`.
- `src/pr48973-2.c`: in `main`, `%t5.bf.sext = bir.ashr i32 %t5.bf.shl, 31`
  and `%t7.bf.sext = bir.ashr i32 %t7.bf.shl, 31`. Operand homes are GPR
  registers for the shifted values and an immediate `31`; results are `i32`
  GPR homes (`s1`). Memory facts around the family are global-symbol i32
  loads/stores with proven in-bounds addressing; the `ashr` result feeds
  later scalar ops and branch comparison through normal prepared homes.

Code-path mapping:
- Prepared traversal reaches
  `src/backend/mir/riscv/codegen/object_emission.cpp`:
  `fragment_for_prepared_instruction()` detects `bir::BinaryInst`, then calls
  `fragment_for_prepared_binary()`.
- `fragment_for_prepared_binary()` accepts `I32`/`I64` homes and currently
  handles add/sub/and/or/xor/shl/lshr/mul/udiv and compare opcodes, but has no
  `BinaryOpcode::AShr` case. It returns `std::nullopt`.
- The traversal then emits the generic
  `unsupported_instruction_fragment: BIR instruction requires unsupported RV64
  object lowering` diagnostic.

No producer-fact gap was found for this family: operand homes, result homes,
immediates, memory facts, and publication requirements are present in the
prepared dumps.

## Suggested Next

Delegate Step 2 to implement semantic RV64 object lowering for prepared scalar
`bir.ashr` binary fragments, starting with `src/20020225-2.c` and proving the
same family against `src/pr48973-2.c` if the supervisor wants a nearby
same-family representative.

## Watchouts

- Do not absorb stack-frame or parameter-home work owned by idea 398.
- Do not infer missing producer facts inside RV64 object emission.
- Do not claim progress from filename-specific lowering, expectation rewrites,
  allowlist filtering, or diagnostic-only churn.
- Keep this separate from other same-diagnostic families in current artifacts,
  such as frame-slot call-argument publication in `src/20000622-1.c`.
- The AShr packet should preserve fail-closed behavior for unsupported widths,
  non-GPR homes, and invalid shift-amount shapes; do not repair by treating
  the bitfield `ashr ..., 31` pattern as a filename or source-shape shortcut.

## Proof

Ran the delegated proof command exactly:

```sh
cmake --build --preset default && mkdir -p build/agent_state/395_step1_dumps && printf '%s\n' 'src/20000223-1.c' 'src/20020225-2.c' > build/agent_state/395_step1_instruction_fragments.allowlist.txt && { ALLOWLIST=build/agent_state/395_step1_instruction_fragments.allowlist.txt STOP_ON_FAILURE=0 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/395_step1_instruction_fragments_probe.log 2>&1 || true; } && rg -n 'unsupported_instruction_fragment|unsupported_stack_frame|unsupported_local_memory_access|unsupported RV64 object lowering|\[rv64-gcc-torture\]|pass\t|fail\t|RV64_C4C_OBJ|error:' build/agent_state/395_step1_instruction_fragments_probe.log && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result: build was already up to date; probe total was 2 with
`src/20000223-1.c` passing and `src/20020225-2.c` failing with
`unsupported_instruction_fragment`; `ctest -R '^backend_'` passed and
`test_after.log` is the canonical proof log.

Fresh dump artifacts:
- `build/agent_state/395_step1_dumps/20000223-1.prepared-bir.txt`
- `build/agent_state/395_step1_dumps/20020225-2.{semantic-bir,prepared-bir}.txt`
- `build/agent_state/395_step1_dumps/pr48973-2.{semantic-bir,prepared-bir}.txt`
