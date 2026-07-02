# Prepared Authority Queue Step 1 Reconstruction

Status: Step 1 accounting packet for
`ideas/open/552_prepared_move_bundle_target_shape_authority_gaps.md`.

## Inputs

- Original classification:
  `docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_classification.tsv`
- Carry-in reconciliation:
  `docs/rv64_gcc_torture_post_contract/move_bundle_materialization_residual_20_reconciliation.md`
- Active source idea:
  `ideas/open/552_prepared_move_bundle_target_shape_authority_gaps.md`
- Derived queue:
  `build/agent_state/552_step1_prepared_authority_queue/queue.tsv`
- Focused allowlist:
  `build/agent_state/552_step1_prepared_authority_queue.allowlist`
- Current diagnostics:
  `build/agent_state/552_step1_prepared_authority_queue/current_diagnostics.tsv`
- Proof log: `test_after.log`

## Queue Counts

The reconstructed queue has 43 deduplicated rows:

| Source | Rows |
| --- | ---: |
| Original `prepared_module_target_shape_authority_gap` rows | 31 |
| RV64 materialization carry-ins from 551 closure | 12 |
| Deduplicated total | 43 |

## Earliest Missing Prepared Fact Groups

| Group | Rows |
| --- | ---: |
| Destination home | 16 |
| Source home or move classification | 15 |
| Scalar type, size, or alignment | 9 |
| Return ABI destination home | 2 |
| Select-publication source home | 1 |

Observed family counts preserve the original/carry-in provenance:

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

No duplicate row appeared between the 31 original rows and 12 carry-ins.

## Representative Rows

Destination-home authority:

- `src/20040629-1.c` and `src/20040705-1.c`: current facts say
  `destination_storage=stack_slot`, but the destination home is still
  `rematerializable_immediate`.
- `src/20000717-3.c`: carry-in with the same stack storage versus
  rematerializable-immediate destination-home mismatch.
- `src/strcmp-1.c` and `src/strncmp-1.c`: carry-ins where stack storage is
  paired with `pointer_base_plus_offset` destination-home publication.

Source-home or move-classification authority:

- `src/20000113-1.c` and `src/20011219-1.c`: reason says
  `consumer_stack_to_stack`, but current source home is published as a
  register.
- `src/20020226-1.c`, `src/20020508-1.c`, and the other classifier carry-ins:
  current scan reports
  `prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination`.

Scalar type/size/alignment authority:

- `src/20020402-1.c`: multiple moves include incomplete source type facts.
- `src/20050215-1.c`: register-to-stack move lacks the source type needed by
  RV64 lowering.
- `src/950710-1.c`: stack-to-stack move carries destination type but omits
  source type authority.
- `src/loop-2d.c`: carry-in where the register source lacks scalar type/size
  authority.

Return ABI destination-home authority:

- `src/20001130-2.c` and `src/20080719-1.c`: return destination storage is a
  register while the published destination home remains a stack slot.

Select-publication source-home authority:

- `src/pr58726.c`: select publication is present, but intent remains
  `unsupported_source_home` while publication has stack-slot source and
  destination homes.

## Prepared Facts RV64 Is Waiting For

- Destination-home group: authoritative destination home kind, stack slot id,
  offset, size, alignment, and storage/home agreement.
- Source-home or move-classification group: unambiguous source home and move
  class for stack-destination consumers, including multi-source non-parallel
  bundles.
- Scalar-type group: complete source and destination scalar type, width, and
  alignment facts before RV64 chooses load/store width or extension behavior.
- Return ABI group: return ABI destination-home publication that agrees with
  the ABI register storage.
- Select-publication group: supported source-home intent and publication facts
  for the selected edge publication move.

Rows are not rerouted out of this idea by Step 1. The classifier carry-ins may
split into an earlier semantic producer route if Step 3 proves prepared is only
mirroring missing producer evidence.

## Implementation Surfaces To Inspect Next

- `src/backend/prealloc/regalloc.cpp`: construction and append path for
  prepared move bundles and value homes.
- `src/backend/prealloc/prepared_object_traversal.cpp` and
  `src/backend/prealloc/prepared_object_traversal.hpp`: prepared move-bundle
  consumer classification, including
  `ambiguous_non_parallel_multi_source_stack_destination`.
- `src/backend/prealloc/prepared_lookups.cpp`: indexed value-home, move-bundle,
  return ABI, and select-publication lookup publication.
- `src/backend/prealloc/publication_plans.cpp` and
  `src/backend/prealloc/publication_plans.hpp`: select/source producer
  publication, move-bundle effect classification, and publication placement.
- `src/backend/prealloc/storage_plans.cpp`: typed storage and value-home
  validation.
- `src/backend/prealloc/prepared_contract_verifier.hpp`: contract verifier
  statuses for missing or conflicting source-home/type facts.
- `src/backend/mir/riscv/codegen/object_emission.cpp`: dependent RV64
  diagnostic consumer used to verify repaired prepared facts without guessing.

## Proof

Build freshness was run because the focused proof consumes generated backend
artifacts, even though this packet changed no implementation code:

```text
cmake --build --preset default
```

Focused queue proof:

```text
ALLOWLIST=build/agent_state/552_step1_prepared_authority_queue.allowlist scripts/check_progress_rv64_gcc_c_torture_backend.sh
```

Result:

```text
[rv64-gcc-torture] total=43 passed=0 failed=43
```

All 43 rows remain in the prepared-authority queue, as expected for a
proof/accounting packet.
