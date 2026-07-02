# RV64 Move-Bundle Materialization Reconciliation

Status: Step 6 reconciliation for the 151-row
`coherent_rv64_mir_materialization` lane.

## Inputs

- Source lane:
  `docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_classification.tsv`
  filtered to `first_owner_lane=coherent_rv64_mir_materialization`.
- Generated allowlist:
  `build/agent_state/551_step6_reconciliation/coherent_rv64_mir_materialization.allowlist`.
- Fresh scan summary:
  `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`.
- Row reconciliation:
  `build/agent_state/551_step6_reconciliation/reconciliation_rows.tsv`.
- Remaining generic materialization rows:
  `build/agent_state/551_step6_reconciliation/remaining_generic_move_bundle_rows.tsv`,
  derived from reconciliation rows whose Step 6 category is
  `same_generic_move_bundle_materialization_failed`.
- Proof log: `test_after.log`.

The generated allowlist contains 151 rows. The fresh RV64 gcc torture backend
subset reports 45 pass and 106 fail.

## Reconciliation Counts

| Category | Rows | Meaning |
| --- | ---: | --- |
| `pass` | 45 | Row now builds, links, runs, and matches the clang RV64 reference. |
| `same_generic_move_bundle_materialization_failed` | 20 | Row still reaches `fragment_status=generic_move_bundle_materialization_failed`. |
| `later_explicit_unsupported_diagnostic` | 48 | Row advanced past generic move-bundle materialization into a later explicit unsupported backend diagnostic. |
| `later_runtime_mismatch` | 10 | Row advanced into runtime comparison and currently mismatches or aborts. |
| `reroute_prepared_move_bundle_classification` | 23 | Fresh evidence now points at prepared move-bundle authority or classifier ownership, not RV64 materialization. |
| `reroute_prepared_select_publication_authority` | 4 | Fresh select-publication evidence lacks source-home or source-stack-offset authority. |
| `reroute_move_bundle_evidence_gap` | 1 | Fresh log reports move-bundle target shape without enough published detail for a semantic RV64 materialization packet. |

Diagnostic counts:

| Current diagnostic | Rows |
| --- | ---: |
| `pass` | 45 |
| `fragment_status=generic_move_bundle_materialization_failed` | 20 |
| `unsupported_instruction_fragment` | 22 |
| `unsupported_terminator_fragment` | 9 |
| `unsupported_local_memory_access` | 7 |
| `unsupported_global_data` | 10 |
| `RV64_BACKEND_RUNTIME_MISMATCH` | 10 |
| `prepared_move_bundle_authority_or_classification` | 23 |
| `unsupported_source_home` | 2 |
| `unsupported_source_stack_offset` | 2 |
| `unsupported_move_bundle_target_shape_without_published_detail` | 1 |

## Remaining RV64 Materialization Rows

These 20 rows still need an RV64 materialization packet or narrower triage
inside this source idea:

- `src/20000717-3.c`
- `src/20020226-1.c`
- `src/20020508-1.c`
- `src/20020508-2.c`
- `src/20020508-3.c`
- `src/20020510-1.c`
- `src/20100316-1.c`
- `src/920721-1.c`
- `src/920908-2.c`
- `src/bf-pack-1.c`
- `src/loop-2d.c`
- `src/pr25125.c`
- `src/pr40386.c`
- `src/pr48197.c`
- `src/pr81281.c`
- `src/pr82192.c`
- `src/pr89195.c`
- `src/strcmp-1.c`
- `src/strncmp-1.c`
- `src/usmul.c`

Shape split:

- 17 rows:
  `before_instruction/authority_none/consumer_register_to_stack/register_to_stack_slot`
- 3 rows:
  `before_instruction/authority_none/consumer_register_to_stack/rematerializable_immediate_to_stack_slot`

## Residual Ownership

Rows in `later_explicit_unsupported_diagnostic` and `later_runtime_mismatch`
should not block this move-bundle materialization route. They have advanced to
later backend capabilities or runtime correctness work.

Rows in the three `reroute_*` categories should not be fixed by inferring
missing facts in RV64. They should be handed to the prepared authority,
prepared classifier, or evidence-gap follow-up queues unless the supervisor
asks for a dedicated review.

## Suggested Next

This source idea is not ready to close because 20 rows still fail with the same
generic move-bundle materialization fragment status. The next implementation
packet should inspect those 20 rows as a smaller semantic group, starting with
the 17 remaining register-to-stack rows and explicitly separating the three
rematerializable-immediate rows.
