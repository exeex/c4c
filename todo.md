Status: Active
Source Idea Path: ideas/open/552_prepared_move_bundle_target_shape_authority_gaps.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reconstruct The 43-Row Prepared Authority Queue

# Current Packet

## Just Finished

Step 1, "Reconstruct The 43-Row Prepared Authority Queue," completed as a
proof/accounting packet. Initial `git status --short` was clean. The queue was
rebuilt from 31 original
`first_owner_lane=prepared_module_target_shape_authority_gap` rows plus 12
carry-ins from the closed 551 RV64 materialization lane, then deduplicated to
43 rows.

Artifacts:

- `build/agent_state/552_step1_prepared_authority_queue/queue.tsv`
- `build/agent_state/552_step1_prepared_authority_queue/group_counts.tsv`
- `build/agent_state/552_step1_prepared_authority_queue/observed_family_counts.tsv`
- `build/agent_state/552_step1_prepared_authority_queue/representatives.tsv`
- `build/agent_state/552_step1_prepared_authority_queue/current_diagnostics.tsv`
- `build/agent_state/552_step1_prepared_authority_queue.allowlist`
- `docs/rv64_gcc_torture_post_contract/prepared_authority_queue_step1_reconstruction.md`

Deduplicated group counts by earliest missing prepared fact:

| Group | Rows |
| --- | ---: |
| Destination home | 16 |
| Source home or move classification | 15 |
| Scalar type, size, or alignment | 9 |
| Return ABI destination home | 2 |
| Select-publication source home | 1 |

Observed family counts:

| Observed Family | Rows |
| --- | ---: |
| `prepared_destination_home_shape_authority` | 11 |
| `prepared_move_classification_or_source_home_authority` | 9 |
| `prepared_move_type_authority` | 8 |
| `prepared_return_abi_destination_home_authority` | 2 |
| `prepared_select_publication_source_home_authority` | 1 |
| `carry_in_classifier_ambiguous_non_parallel_multi_source_stack_destination` | 6 |
| `carry_in_generic_fragment_destination_home_mismatch` | 5 |
| `carry_in_generic_fragment_source_type_size_authority` | 1 |

Representative rows:

- Destination home: `src/20040629-1.c`, `src/20040705-1.c`,
  `src/20000717-3.c`, `src/strcmp-1.c`, `src/strncmp-1.c`.
- Source home or move classification: `src/20000113-1.c`,
  `src/20011219-1.c`, `src/20020226-1.c`.
- Scalar type, size, or alignment: `src/20020402-1.c`,
  `src/20050215-1.c`, `src/950710-1.c`, `src/loop-2d.c`.
- Return ABI destination home: `src/20001130-2.c`, `src/20080719-1.c`.
- Select-publication source home: `src/pr58726.c`.

Prepared/module facts RV64 is waiting for: explicit source and destination home
kind, stack/register coordinates, scalar type/size/alignment, return ABI
destination-home agreement, and select-publication source-home intent. No row
was rerouted out of this idea in Step 1; the six classifier carry-ins may need
an earlier semantic producer split only if Step 3 proves prepared is mirroring
missing producer evidence.

## Suggested Next

Executor should run Step 2, "Repair Destination-Home Shape Authority," starting
with the 16 destination-home rows. Inspect prepared value-home publication,
move-bundle construction, typed storage validation, and RV64 diagnostic
consumption before editing implementation code.

## Watchouts

- Do not infer prepared facts from RV64 destination spelling or expected
  assembly.
- Keep the 12 carry-in rows from the closed 551 materialization lane inside
  this queue unless row-level evidence reroutes them to an earlier owner.
- Do not edit implementation files unless the delegated packet explicitly
  includes a code change.
- Prepared/module implementation surfaces to inspect next:
  `src/backend/prealloc/regalloc.cpp`,
  `src/backend/prealloc/prepared_object_traversal.cpp`,
  `src/backend/prealloc/prepared_object_traversal.hpp`,
  `src/backend/prealloc/prepared_lookups.cpp`,
  `src/backend/prealloc/publication_plans.cpp`,
  `src/backend/prealloc/publication_plans.hpp`,
  `src/backend/prealloc/storage_plans.cpp`,
  `src/backend/prealloc/prepared_contract_verifier.hpp`, and dependent
  consumer diagnostics in `src/backend/mir/riscv/codegen/object_emission.cpp`.

## Proof

- Build freshness:

```text
cmake --build --preset default
```

Result: passed; ninja reported no work to do.

- Focused queue proof:

```text
ALLOWLIST=build/agent_state/552_step1_prepared_authority_queue.allowlist scripts/check_progress_rv64_gcc_c_torture_backend.sh
```

Result: `total=43 passed=0 failed=43`, as expected for a reconstruction-only
packet. Proof output is preserved in `test_after.log`.
