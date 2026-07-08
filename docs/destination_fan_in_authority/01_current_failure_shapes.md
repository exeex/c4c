# Current Failure Shapes

## Evidence Base

This document classifies the non-parallel multi-source stack-destination
family from the July 8 RV64 gcc_torture backend evidence, without selecting a
destination authority rule.

Current scan authority:

- `docs/rv64_gcc_torture_1000_pass_recovery/current_scan_summary.md` records
  the July 8 backend-object scan as `1467` total rows, `470` passed rows,
  `997` failed rows, and `0` missing rows. It names the mutable scan artifacts:
  `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`,
  `build/agent_state/rv64_gcc_c_torture_backend_failed.txt`, and per-case logs
  under `build/rv64_gcc_c_torture_backend/<case-id>/case.log`.
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md` classifies
  `125` rows as `non-parallel multi-source stack destination`, assigns first
  owner `prepared/prealloc authority`, and uses `src/pr43236.c` as the
  representative case.
- The active research source,
  `ideas/open/607_destination_fan_in_authority_research.md`, treats those
  `125` rows as an architecture research population, not an implementation
  target. It requires a destination legality answer before RV64 target
  materialization work can proceed.

Current per-case workdir note:

- A raw grep of the present workdir finds `128` logs containing
  `ambiguous_non_parallel_multi_source_stack_destination`. Within those,
  `106` logs contain the detailed
  `unsupported_prepared_move_bundle_classification: non-parallel` payload and
  `22` logs contain the shorter classifier rejection text. The July 8 recovery
  map remains the authoritative planned family count for this research packet:
  `125` rows. The extra current-log hits are treated here as representative
  shape evidence, not as a count rewrite.

## Representative Rows

| Case | Log | Shape | Key evidence |
| --- | --- | --- | --- |
| `src/pr43236.c` | `build/rv64_gcc_c_torture_backend/src_pr43236.c/case.log` | Detailed two-source fan-in | `prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination`; `unsupported_prepared_move_bundle_classification: non-parallel`; `move_count=2`; `parallel_copy=no`; both sources are registers and both destinations are the same stack slot value. |
| `src/20000113-1.c` | `build/rv64_gcc_c_torture_backend/src_20000113-1.c/case.log` | Entry-block detailed fan-in | Same category and classification, with `function=foobar`, `phase=before_instruction`, `authority=none`, and two register sources targeting one stack slot destination. |
| `src/20040409-1w.c` | `build/rv64_gcc_c_torture_backend/src_20040409-1w.c/case.log` | Early instruction detailed fan-in | Same category and classification, with `function=test`, `instruction_index=1`, `move_count=2`, and `parallel_copy=no`. |
| `src/20021010-2.c` | `build/rv64_gcc_c_torture_backend/src_20021010-2.c/case.log` | Current downstream detailed fan-in | Recent focused log with the same category, `function=main`, `block_label=block_10`, and two register sources to `to_value_id=33`. |
| `src/20021120-3.c` | `build/rv64_gcc_c_torture_backend/src_20021120-3.c/case.log` | Current downstream detailed fan-in | Recent focused log with the same category, `function=foo`, `block_label=entry`, and two register sources to `to_value_id=3`. |
| `src/pr48814-2.c` | `build/rv64_gcc_c_torture_backend/src_pr48814-2.c/case.log` | Short classifier rejection | Same prepared consumer category, but the log stops at `prepared move-bundle classifier rejected ambiguous non-parallel multi-source stack-destination authority` instead of printing per-move fields. |

The representative rows show two emitted forms of the same family:

- detailed diagnostic payloads that expose move count, phase, source and
  destination home kinds, and missing producer authority; and
- shorter classifier rejections that identify the same prepared consumer
  category but do not dump the individual move entries.

## Diagnostic Vocabulary

The family is identified by the following log vocabulary:

- `prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination`
- `unsupported_prepared_move_bundle_classification: non-parallel`
- `register-source fan-in to one stack destination has no ordering or mutually-exclusive authority`
- `event_kind=before_instruction_copies`
- `phase=before_instruction`
- `authority=none`
- `move_count=2` in the detailed representative rows
- `parallel_copy=no`
- `move[*].source_home_kind=register`
- `move[*].destination_home_kind=stack_slot`
- `diagnostic_owner=rv64_prepared_move_bundle_consumer`
- `fragment_status=producer_authority_missing_for_register_fan_in_stack_destination`
- shorter-form rows: `prepared move-bundle classifier rejected ambiguous non-parallel multi-source stack-destination authority`

That vocabulary matters because the rejection is not simply "RV64 does not know
how to copy a register to a stack slot." The detailed rows show an authority
gap: multiple register sources propose the same stack destination in a
non-parallel bundle, and the prepared/prealloc layer has not published an
ordering, mutual-exclusion, or merge fact that makes one destination write
legal for target consumption.

## Prepared And Prealloc Surfaces Involved

The relevant surfaces are prepared/prealloc authority surfaces, not testcase
names:

- `ideas/closed/587_prepared_value_freshness_authority_mvp.md` introduced
  first-class source freshness vocabulary and explicitly warns that
  move-bundle destination authority does not automatically prove source
  freshness.
- `ideas/closed/588_shared_prealloc_move_operand_source_freshness_inventory.md`
  records that `classify_prepared_object_move_bundle_consumer` already uses
  `PreparedValueFreshnessUseKind::MoveBundleSource` through
  `find_prepared_value_freshness_authority`, while direct edge-publication
  moves and several adjacent shared-prealloc consumers remain blocked on
  contract design.
- The current logs name the target-side prepared consumer as
  `rv64_prepared_move_bundle_consumer`, but the first missing fact is producer
  authority for register fan-in to one stack destination. The consumer is
  failing closed because the prepared/prealloc authority surface does not
  prove which destination write is legal.

For Step 1, source freshness authority is evidence context only. The selected
source freshness work from ideas `587` and `588` can tell a consumer whether a
source is fresh enough to trust for a use. It does not, by itself, decide
whether two non-parallel register sources may target the same stack destination
or which destination write is authoritative.

## Adjacent Owner Boundaries

These nearby buckets from
`docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md` should not be
merged into the `125` destination fan-in family:

| Adjacent family | Count | First owner | Boundary |
| --- | ---: | --- | --- |
| `generic move-bundle target shape` | `16` | `prepared/prealloc authority` | Representative `src/20000422-1.c` reports `unsupported_move_bundle_target_shape` and `generic_move_bundle_materialization_failed`; it includes mixed register-to-stack and stack-to-stack materialization, not the specific same-stack-destination fan-in category. |
| `ambiguous move-bundle source freshness` | `2` | `prepared/prealloc authority` | Source freshness ambiguity belongs to the idea `587` vocabulary. It is adjacent, but its question is source validity, not same-destination legality. |
| `local memory frame-slot or pointer base+offset` | `27` | `prepared/RV64 authority` | Pointer/base-plus-offset and frame-slot wiring rows are selected-address or local-memory consumer questions, not move-bundle destination fan-in. |
| `branch stack-load authority` and `branch stack-load source freshness` | `10` combined | `prepared/RV64 authority` | These rows ask whether branch operands have selected stack-load/source freshness authority. They should be audited against branch-specific use kinds, not destination fan-in. |
| `select publication stack-offset source wiring` and `select publication move-bundle wiring` | `11` combined | `prepared/RV64 authority` | Select publication rows are publication/source wiring questions. Alias or publication evidence is not destination fan-in authority. |
| `scalar compare publication` | `3` | `prepared authority` | Compare publication is a prepared publication gap, not a move-bundle same-destination rule. |
| `malformed join transfer carrier` | `3` | `prepared/prealloc authority` | Join-transfer carrier completeness is a carrier-structure gap, not a destination fan-in legality decision. |

## Conclusion

The Step 1 family boundary is:

- included: rows whose first stopping diagnostic is
  `prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination`
  or the equivalent short classifier rejection for ambiguous non-parallel
  multi-source stack-destination authority;
- excluded: generic move-bundle materialization failures, source freshness
  ambiguity rows, branch/select publication wiring rows, pointer/base-plus-
  offset local-memory rows, scalar compare publication rows, and malformed
  join-transfer carrier rows.

The `125` row family is therefore a destination fan-in authority problem. It
should remain blocked from implementation until a later answer file selects or
rejects an ordering, mutual-exclusion, merge-authority, or explicit-rejection
rule. Source freshness authority is necessary context, but it is not the
destination legality rule.
