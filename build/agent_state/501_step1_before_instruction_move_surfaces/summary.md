# 501 Step 1 Before-Instruction Prepared Move Surfaces

## Packet Scope

This packet inspected prepared before-instruction move-bundle publication and
RV64 consumption surfaces for idea 501. It made no implementation changes.

## Input Evidence

- Step 2 classifier from idea 495:
  `build/agent_state/495_step2_move_bundle_coherence/`
- Focused representative matrix:
  `build/agent_state/501_step1_before_instruction_move_surfaces/representative_rows.tsv`
- Representative case logs:
  - `build/rv64_gcc_c_torture_backend/src_20010123-1.c/case.log`
  - `build/rv64_gcc_c_torture_backend/src_20040709-1.c/case.log`

The focused idea-501 family is:

```text
278 before_instruction_copies / before_instruction / authority none / stack_slot / consumer_register_to_stack
 50 before_instruction_copies / before_instruction / authority none / stack_slot / consumer_stack_to_stack
```

Both families are non-parallel-copy consumer moves with coherent prepared
authority. Source storage is not printed by the current case-log token stream,
but the prepared move carries `from_value_id`, and RV64 has existing lookup
helpers to resolve that value to a prepared home.

## AST-Backed Surface Queries

`c4c-clang-tools` found:

- RV64 consumer:
  `fragment_for_prepared_move_bundle` at
  `src/backend/mir/riscv/codegen/object_emission.cpp:1966`.
- Caller:
  `prepared_function_to_object_function` at
  `src/backend/mir/riscv/codegen/object_emission.cpp:8979`.
- Producer helpers:
  `classify_prepared_move_phase` at
  `src/backend/prealloc/regalloc.cpp:71` and
  `append_prepared_move_bundle` at
  `src/backend/prealloc/regalloc.cpp:106`.

## Prepared Publication Surface

The relevant records/helpers are:

- `PreparedMoveResolution`
  (`src/backend/prealloc/regalloc.hpp:199`): publishes `from_value_id`,
  `to_value_id`, destination kind/storage, destination register/stack metadata,
  block/instruction coordinate, op kind, authority kind, and reason.
- `PreparedMoveBundle`
  (`src/backend/prealloc/value_locations.hpp:161`): groups move resolutions by
  function, phase, authority, block index, instruction index, and optional
  parallel-copy predecessor/successor labels.
- `classify_prepared_move_phase`
  (`src/backend/prealloc/regalloc.cpp:71`): maps ordinary value-destination
  moves with `authority=none` to `PreparedMovePhase::BeforeInstruction`.
- `append_prepared_move_bundle`
  (`src/backend/prealloc/regalloc.cpp:106`): groups matching phase/authority/
  coordinate move resolutions into one prepared bundle.
- `append_consumer_move_resolution`
  (`src/backend/prealloc/regalloc/consumer_moves.cpp:197`): publishes consumer
  moves for named instruction results and named operands, with
  `PreparedMoveAuthorityKind::None` and reason from
  `storage_transfer_reason("consumer", source, destination)`.

## RV64 Consumption Surface

The current RV64 hook is `fragment_for_prepared_move_bundle`:

- It already suppresses authorized select-edge and cast-dependency bundles
  before normal move handling.
- It currently rejects every move whose `destination_storage_kind` is not
  `PreparedMoveStorageKind::Register`, which is the direct rejection for the
  idea-501 stack-slot destination rows.
- It already resolves `source_home` from `move.from_value_id` through
  `prepared_value_home_for_id`.
- It already has helpers for register and stack homes:
  `gpr_register_number_for_home`, `prepared_stack_slot_home_offset`,
  `prepared_bir_value_type_for_name`, and `append_rv64_store_register_to_stack`.

## Step 2 Readiness

Current prepared facts are complete enough for a narrow Step 2
register-to-stack implementation:

- Required source identity: `move.from_value_id`, resolved through
  `prepared_value_home_for_id`.
- Required destination identity: `move.to_value_id`, resolved through the same
  prepared value-home lookup.
- Required destination route: `move.destination_storage_kind == StackSlot` plus
  the destination value home's stack-slot metadata.
- Required coordinate and event guard: bundle phase `BeforeInstruction`,
  authority `None`, matching block/instruction coordinates, op kind `Move`,
  non-parallel-copy labels absent.
- Required RV64 emission helper: `append_rv64_store_register_to_stack`.

The first implementation packet should be limited to
`consumer_register_to_stack`: source home resolves to a GPR, destination home
resolves to a stack slot of a scalar RV64 size, then emit the store at the
prepared before-instruction site. It should not implement stack-to-stack in the
same packet; that shape needs a temporary register/load-then-store policy.

## Boundaries

- Do not implement out-of-SSA, before-return, or select-publication move
  handling under idea 501.
- Do not infer source storage from case-log text. Use `move.from_value_id` and
  prepared value homes.
- Do not infer destination stack offsets from raw testcase shape. Use the
  destination prepared value home and `prepared_stack_slot_home_offset`.

## Proof

Delegated proof command:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed > test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
