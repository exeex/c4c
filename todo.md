Status: Active
Source Idea Path: ideas/open/556_prepared_move_bundle_ambiguous_stack_destination_src_960209_1.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce And Capture Bundle Facts

# Current Packet

## Just Finished

Step 1 - Reproduce And Capture Bundle Facts completed for
`src/960209-1.c` without semantic repairs.

Fresh proof reproduces:

```text
prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination:
prepared move-bundle classifier rejected ambiguous non-parallel
multi-source stack-destination authority
```

First ambiguous bundle:

- Function: `f`
- Prepared block: `tern.end.38`
- Event: `BeforeInstructionCopies`
- Bundle site: `phase=before_instruction`, `block_index=16`,
  `instruction_index=4`
- BIR consumer at the site: `%t45 = bir.sub i64 %t43, %t44`
- Scalar type/width: `i64`, 8 bytes
- Move count and ordering:
  1. `from_value_id=24` (`%t43`) to `to_value_id=26` (`%t45`)
  2. `from_value_id=25` (`%t44`) to `to_value_id=26` (`%t45`)
- Source homes:
  - `%t43`, value id 24, register home `s1`,
    `encoding=register bank=gpr placement=gpr:callee_saved#0/w1`
  - `%t44`, value id 25, register home `s2`,
    `encoding=register bank=gpr placement=gpr:callee_saved#1/w1`
- Destination home:
  - `%t45`, value id 26, stack-slot home `slot_id=#21`, offset 136
  - Storage plan: `encoding=frame_slot bank=gpr spill_slot=slot#21+stack136`
  - Frame slot: `slot_id=#21 offset=136 size=8 align=8`
- Move facts:
  - Both moves have `destination_kind=value`,
    `destination_storage=stack_slot`, `op_kind=move`,
    `uses_cycle_temp_source=no`, `reason=consumer_register_to_stack`
  - Bundle authority is `authority=none`
  - Per-move authority is `none`
  - No `source_parallel_copy_step_index` is present
  - No `source_parallel_copy=...` owner is present on the bundle
- Parallel-copy facts:
  - Existing authoritative parallel copies for surrounding phi/select carriers
    are predecessor-terminator bundles, not this before-instruction bundle.
  - This bundle is non-parallel-copy consumer work: event
    `parallel_copy_bundle == nullptr` and bundle authority is not
    `out_of_ssa_parallel_copy`.
- Classifier branch:
  - `classify_prepared_object_move_bundle_consumer` reaches the
    `BeforeInstructionCopies` guard and calls
    `prepared_move_bundle_has_ambiguous_multi_source_stack_destination`.
  - The helper returns true because the bundle has `authority=none`,
    phase `BeforeInstruction`, at least two moves, both sources are register
    homes, both destinations are stack-slot homes, and both moves target the
    same destination value id 26 / same stack slot #21.

Classification: this row matches the existing fail-closed contract exercised by
`verify_move_bundle_consumer_rejects_ambiguous_multi_source_stack_destination`.
The captured row does not prove missing producer authority by itself; it proves
that RV64 is being asked to consume a non-parallel, unordered, multi-register
source bundle targeting one stack slot, with no published producer authority
for source ownership or sequencing.

## Suggested Next

Execute Step 2 from `plan.md`: decide whether to retain this precise
fail-closed rejection with focused coverage for the row shape or repair the
producer/classifier boundary to publish explicit sequencing, split moves, or
parallel-copy authority.

## Watchouts

- Do not continue this work as RV64 local-memory addressing.
- Do not weaken gcc_torture expectations, unsupported markers, allowlists, or
  runtime comparison behavior.
- Do not special-case `src/960209-1.c` or materialize ambiguous bundles in
  RV64 by guessing source ownership.
- The row already has nearby valid `out_of_ssa_parallel_copy` authority for
  select/phi carriers, but the rejected bundle is a separate
  `authority=none` before-instruction consumer bundle.
- Treat any acceptance route that picks `%t43` or `%t44`, drops one move, or
  infers ordering from dump order as testcase-overfit.

## Proof

Proof command run exactly as delegated, with combined output recorded in
`test_after.log`:

```sh
cmake --build --preset default
cmake -DCOMPILER="$PWD/build/c4cll" -DCLANG="$(command -v clang)" -DQEMU_RISCV64="$(command -v qemu-riscv64)" -DSRC="$PWD/tests/c/external/gcc_torture/src/960209-1.c" -DROOT="$PWD/tests/c/external/gcc_torture" -DTARGET_TRIPLE="riscv64-linux-gnu" -DSYSROOT="/usr/riscv64-linux-gnu" -DOUT_CLANG_BIN="$PWD/build/rv64_gcc_c_torture_backend/src_960209-1.c/clang.bin" -DOUT_OBJECT="$PWD/build/rv64_gcc_c_torture_backend/src_960209-1.c/c4c.o" -DOUT_C4C_BIN="$PWD/build/rv64_gcc_c_torture_backend/src_960209-1.c/c4c.bin" -DCASE_TIMEOUT_SEC="20" -P "$PWD/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake"
```

Result: expected fail-closed one-row compile failure at
`[RV64_C4C_OBJ_COMPILE_FAIL]` with
`prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination`.
The build step reported `ninja: no work to do.`
