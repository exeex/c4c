# RV64 Move-Bundle Residual Register-To-Stack Triage

Status: Step 7 triage for the 17 residual rows from
`consumer_register_to_stack/register_to_stack_slot`.

## Inputs

- Allowlist:
  `build/agent_state/551_step7_register_to_stack_residual.allowlist`
- Row status artifact:
  `build/agent_state/551_step7_register_to_stack_residual/row_status.tsv`
- Generic reroute artifact:
  `build/agent_state/551_step7_register_to_stack_residual/rerouted_generic_failures.tsv`
- Proof log: `test_after.log`

## Code Change

`fragment_for_prepared_move_bundle` now lets coherent rematerializable integer
immediate sources use the existing RV64 load-immediate helper before storing to
an authorized stack-slot destination. The rule still requires:

- a coherent rematerializable integer immediate prepared fact
- an available scratch GPR
- coherent destination frame-slot authority from prepared storage facts
- a stack-slot destination home and destination scalar size

This removes the previous 12-bit-only restriction from the stack-store path.
RV64 still does not infer destination homes, stack offsets, pointer-base stack
slots, or missing source size authority.

## Proof Counts

The 17-row subset scan reported `total=17 passed=2 failed=15`.

Row-status reconciliation:

| Status | Rows |
| --- | ---: |
| `pass` | 2 |
| `reroute_prepared_move_bundle_classifier` | 6 |
| `generic_move_bundle_materialization_failed` | 6 |
| `later_runtime_mismatch` | 2 |
| `later_explicit_unsupported_diagnostic` | 1 |

Every remaining generic move-bundle materialization failure is listed in the
derived reroute artifact with row-level evidence.

## Passing Rows

- `src/bf-pack-1.c`
- `src/pr25125.c`

These rows advanced through the wider rematerializable-immediate-to-stack
materialization path and passed the RV64 backend object comparison.

## Prepared Classifier Reroutes

These rows now fail before RV64 materialization with
`ambiguous_non_parallel_multi_source_stack_destination`:

- `src/20020226-1.c`
- `src/20020508-1.c`
- `src/20020508-2.c`
- `src/20020508-3.c`
- `src/pr40386.c`
- `src/pr81281.c`

They belong to prepared move-bundle classifier or authority ownership, not RV64
home inference.

## Generic Reroutes

The remaining generic failures are not safe for RV64 to repair by inference:

| Row | Reroute Owner | Evidence |
| --- | --- | --- |
| `src/20000717-3.c` | prepared destination authority | `destination_storage=stack_slot` but `destination_home_kind=rematerializable_immediate` |
| `src/20100316-1.c` | prepared destination authority | `destination_storage=stack_slot` but `destination_home_kind=rematerializable_immediate` |
| `src/920908-2.c` | prepared destination authority | `destination_storage=stack_slot` but `destination_home_kind=rematerializable_immediate` |
| `src/loop-2d.c` | prepared source authority | register source lacks the source scalar type/size authority required by the existing GPR-to-stack contract |
| `src/strcmp-1.c` | prepared destination authority | `destination_storage=stack_slot` but `destination_home_kind=pointer_base_plus_offset` |
| `src/strncmp-1.c` | prepared destination authority | `destination_storage=stack_slot` but `destination_home_kind=pointer_base_plus_offset` |

## Later Residuals

- `src/pr48197.c` advanced to `unsupported_terminator_fragment`.
- `src/20020510-1.c` and `src/pr89195.c` advanced to
  `RV64_BACKEND_RUNTIME_MISMATCH`.

These are later-route failures, not current move-bundle materialization
blockers.
