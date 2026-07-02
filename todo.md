Status: Active
Source Idea Path: ideas/open/513_rv64_stack_to_stack_prepared_move_materialization.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Reclassify Representative Rows

# Current Packet

## Just Finished

Completed Step 5 from `plan.md`: reran focused prepared dumps and RV64 object
probes for the representative rows under
`build/agent_state/513_step5_row_reclassification/`.

Commands run for each row:

- `build/c4cll --target riscv64-unknown-elf --dump-prepared-bir tests/c/external/gcc_torture/src/<row>.c`
- `build/c4cll --target riscv64-unknown-elf --codegen obj tests/c/external/gcc_torture/src/<row>.c -o build/agent_state/513_step5_row_reclassification/<row>/out.o`

Probe results:

- `20010518-1`: prepared dump status 0; object probe status 2. The first
  failure is still `unsupported_move_bundle_target_shape` at `add` instruction
  0, but it is not a coherent single stack-to-stack move: the bundle has
  `move_count=2`, both moves target value 13, and source values 0 and 1 have
  homes/storage in registers `a0` and `a1`. Classification: still old gate for
  a separate multi-source/register-source move-bundle owner, not cleared by the
  owned same-scalar stack-slot path.
- `pr27073`: prepared dump status 0; object probe status 2. The first failure
  is still `unsupported_move_bundle_target_shape` at `foo` instruction 1, with
  move 4 -> 10, but value 4 `%p.count` has home/storage in register `a4` while
  destination value 10 `%t0` is a GPR frame slot. Classification: still old gate
  for a separate register-source-to-stack reason/publication owner.
- `pr69447`: prepared dump status 0; object probe status 2. The first failure
  is still `unsupported_move_bundle_target_shape` at `foo` instruction 14, with
  move 15 -> 16. Both homes are stack slots, but source value 15 `%t8` has
  storage-plan `encoding=frame_slot bank=none`, while the destination is
  `bank=gpr`; the BIR source is the i16 input to a later `zext i16 to i64`.
  Classification: still old gate for a separate non-GPR/converted-value owner,
  not a coherent same-type GPR frame-slot copy.

## Suggested Next

Execute Step 6 from `plan.md`: validate and hand off the runbook with the
focused coverage and representative-row evidence recorded here.

## Watchouts

No row in this packet cleared the old move-bundle diagnostic, but that is not
evidence against the Step 3 materializer. The remaining representative first
failures are malformed or broader than the accepted shape: multi-move
register-source bundle, register-source-to-stack reason mismatch, and
`bank=none`/conversion-adjacent source storage.

Do not expand idea 513 to infer stack locations or conversion semantics for
these rows. Any follow-up should be separated from the coherent same-scalar
stack-slot materialization scope.

## Proof

Ran the delegated row-probe proof:

- `build/c4cll --target riscv64-unknown-elf --dump-prepared-bir tests/c/external/gcc_torture/src/20010518-1.c`
- `build/c4cll --target riscv64-unknown-elf --codegen obj tests/c/external/gcc_torture/src/20010518-1.c -o build/agent_state/513_step5_row_reclassification/20010518-1/out.o`
- `build/c4cll --target riscv64-unknown-elf --dump-prepared-bir tests/c/external/gcc_torture/src/pr27073.c`
- `build/c4cll --target riscv64-unknown-elf --codegen obj tests/c/external/gcc_torture/src/pr27073.c -o build/agent_state/513_step5_row_reclassification/pr27073/out.o`
- `build/c4cll --target riscv64-unknown-elf --dump-prepared-bir tests/c/external/gcc_torture/src/pr69447.c`
- `build/c4cll --target riscv64-unknown-elf --codegen obj tests/c/external/gcc_torture/src/pr69447.c -o build/agent_state/513_step5_row_reclassification/pr69447/out.o`
- `git diff --check -- todo.md`

Prepared dumps passed for all three rows. Object probes intentionally preserve
their failing diagnostics in the row artifact directories; this packet did not
overwrite `test_after.log`.
