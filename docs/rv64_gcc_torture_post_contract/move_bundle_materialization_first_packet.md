# Move-Bundle Materialization First Packet

Status: Step 1 packet reconstruction for
`ideas/open/551_rv64_move_bundle_materialization_from_classified_bucket.md`.

## Inputs

- Queue source:
  `docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_classification.tsv`
- Lane filter: `first_owner_lane=coherent_rv64_mir_materialization`
- Current per-case logs under `build/rv64_gcc_c_torture_backend/*/case.log`
- RV64 implementation surface identified with `c4c-clang-tool-ccdb`:
  `src/backend/mir/riscv/codegen/object_emission.cpp`

## Subqueue Counts

The 151-row coherent RV64/MIR lane splits by `move_shape` as:

| Move shape | Rows | Step owner |
| --- | ---: | --- |
| `before_instruction/authority_none/consumer_register_to_stack/register_to_stack_slot` | 130 | First implementation packet |
| `before_instruction/authority_none/consumer_register_to_stack/rematerializable_immediate_to_stack_slot` | 15 | Later immediate packet |
| `block_entry/out_of_ssa_parallel_copy/phi_join_register_to_register/rematerializable_immediate_to_register` | 3 | Later immediate packet |
| `before_instruction/authority_none/consumer_stack_to_stack/stack_slot_to_stack_slot` | 2 | Later stack-to-stack packet |
| `block_entry/out_of_ssa_parallel_copy/phi_join_register_to_register/select_publication_immediate_to_register` | 1 | Later select-publication packet |

The selected first packet is the 130-row
`consumer_register_to_stack/register_to_stack_slot` family under the coherent
lane. The packet is defined by both `first_owner_lane` and `move_shape`, not by
`move_shape` alone: five prepared-authority rows share the same textual
`move_shape` and remain excluded. The packet excludes all 31
`prepared_module_target_shape_authority_gap` rows and the single `evidence_gap`
row.

## Selected Packet Shape

All selected rows publish:

- `event_kind=before_instruction_copies`
- `phase=before_instruction`
- `authority=none`
- `parallel_copy=no`
- `destination_kind=value`
- `destination_storage=stack_slot`
- `source_home_kind=register`
- `destination_home_kind=stack_slot`
- `fragment_status=generic_move_bundle_materialization_failed`

Selected row move-count spread:

| Move count | Rows |
| ---: | ---: |
| 1 | 96 |
| 2 | 21 |
| 3 | 13 |

Selected row scalar type pairs:

| Source to destination type | Rows |
| --- | ---: |
| `i32_to_i16` | 27 |
| `i32_to_i64` | 24 |
| `i64_to_i32` | 20 |
| `i8_to_i32` | 19 |
| `i32_to_i32` | 17 |
| `i64_to_i16` | 6 |
| `ptr_to_i32` | 5 |
| `i8_to_i16` | 4 |
| `i32_to_i8` | 3 |
| `i64_to_i8` | 2 |
| `i8_to_i64` | 2 |
| `ptr_to_ptr` | 1 |

The first code packet should therefore treat the family as semantic
register-home to stack-slot-home materialization, not as a same-width raw store
only. The source register and destination stack offset must come from prepared
homes/storage facts, while scalar width and extension/truncation behavior must
come from the prepared/BIR type facts.

## Representative Evidence

| Case | Evidence | Why it is representative |
| --- | --- | --- |
| `src/pr78438.c` | `build/rv64_gcc_c_torture_backend/src_pr78438.c/case.log:6-18` | Single move, `i32` register source to `i16` stack destination. |
| `src/20000121-1.c` | `build/rv64_gcc_c_torture_backend/src_20000121-1.c/case.log:6-18` | Single move, small integer source widened to an `i64` stack destination. |
| `src/20000801-2.c` | `build/rv64_gcc_c_torture_backend/src_20000801-2.c/case.log:6-18` | Pointer register source published to an integer stack destination. |
| `src/20000422-1.c` | `build/rv64_gcc_c_torture_backend/src_20000422-1.c/case.log:6-28` | Three-move bundle where the first move is register-to-stack and the bundle also contains stack-to-stack moves. |
| `src/20000914-1.c` | `build/rv64_gcc_c_torture_backend/src_20000914-1.c/case.log:6-23` | Two-move bundle in the selected family without the later prepared-authority blocker seen in `src/20000717-3.c`. |

These representatives are intentionally not filename contracts. They cover the
shape variation that the next implementation packet should prove before the
remaining 130-row family is reconciled.

`src/20000717-3.c` was the original two-move representative, but after the
register-to-stack materialization advanced it reached a later move bundle whose
move says `destination_storage=stack_slot` while the prepared destination home
is `rematerializable_immediate`. That later missing destination stack-slot
authority makes it unsuitable as a clean Step 2 proof representative.

## Implementation Surface For Next Packet

Use `src/backend/mir/riscv/codegen/object_emission.cpp` as the first
implementation surface:

- `fragment_for_prepared_move_bundle` is the RV64 prepared move-bundle lowering
  entry point for traversal events.
- The existing `consumer_register_to_stack` branch already resolves source and
  destination homes through `prepared_value_home_for_id`, stack offsets through
  `prepared_stack_slot_home_absolute_offset`, source registers through
  `gpr_register_number_for_home`, and stores through
  `append_rv64_store_register_to_stack_offset`.
- The current branch rejects several selected packet shapes, including
  width-changing source/destination type pairs and bundles with more than one
  `consumer_register_to_stack` move. The next packet should inspect whether
  those checks are intentionally conservative or need to become semantic
  register-to-stack materialization.
- Adjacent helper surface:
  `prepared_storage_plan_endpoint_is_coherent_gpr_register`,
  `prepared_storage_plan_endpoint_is_coherent_gpr_frame_slot`,
  `rv64_scalar_memory_size_for_type`, and the existing stack load/store helpers.

The next implementation must not infer homes from case names, final register
spelling, target text, or expected assembly. Rows whose prepared homes or stack
slot offsets are not explicit should be rerouted to the prepared-authority idea.

## Expected Proof Subset

After the first code packet, run a build proof plus the RV64 gcc torture object
runner for at least:

```text
src/pr78438.c
src/20000121-1.c
src/20000801-2.c
src/20000422-1.c
src/20000914-1.c
```

For each case, the relevant proof condition is that the failure no longer ends
at `unsupported_move_bundle_target_shape` with
`fragment_status=generic_move_bundle_materialization_failed` for the selected
register-to-stack shape. If a representative advances to a later independent
failure, record that separately from full case success.

## Reroutes And Exclusions

No row is rerouted out of `ideas/open/551_rv64_move_bundle_materialization_from_classified_bucket.md`
by this Step 1 packet. Excluded rows are already owned outside the selected
first packet:

- 21 coherent RV64/MIR rows remain for later steps in this same idea.
- 31 prepared/module authority-gap rows, including 5 rows with the same textual
  register-to-stack `move_shape`, remain owned by
  `ideas/open/552_prepared_move_bundle_target_shape_authority_gaps.md`.
- 1 evidence-gap row remains owned by
  `ideas/open/553_move_bundle_target_shape_evidence_gap_src_960209_1.md`.
