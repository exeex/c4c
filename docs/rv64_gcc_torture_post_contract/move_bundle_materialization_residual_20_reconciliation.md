# RV64 Move-Bundle Residual 20-Row Reconciliation

Status: Step 9 reconciliation for the residual rows from Step 6.

## Inputs

- Allowlist:
  `build/agent_state/551_step9_residual_20.allowlist`
- Row-status artifact:
  `build/agent_state/551_step9_residual_20/row_status.tsv`
- Fresh scan summary:
  `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`
- Proof log: `test_after.log`

## Proof Counts

The focused 20-row subset scan reported `total=20 passed=5 failed=15`.

Row-status reconciliation:

| Status | Rows |
| --- | ---: |
| `pass` | 5 |
| `prepared_authority_reroute_classifier` | 6 |
| `prepared_authority_reroute_generic_fragment` | 6 |
| `later_runtime_mismatch` | 2 |
| `later_explicit_unsupported_diagnostic` | 1 |
| `evidence_gap` | 0 |
| `same_generic_move_bundle_materialization_failure_without_reroute` | 0 |

Six rows still surface
`fragment_status=generic_move_bundle_materialization_failed`, but their row
evidence points at missing prepared authority rather than an RV64
materialization rule that can be implemented without inference.

## Passing Rows

- `src/bf-pack-1.c`
- `src/pr25125.c`
- `src/920721-1.c`
- `src/pr82192.c`
- `src/usmul.c`

## Prepared Authority Reroutes

These rows now fail before RV64 materialization with
`ambiguous_non_parallel_multi_source_stack_destination`:

- `src/20020226-1.c`
- `src/20020508-1.c`
- `src/20020508-2.c`
- `src/20020508-3.c`
- `src/pr40386.c`
- `src/pr81281.c`

These rows still print the generic move-bundle fragment status, but the
published move facts show missing or incompatible prepared authority:

| Row | Evidence |
| --- | --- |
| `src/20000717-3.c` | `destination_storage=stack_slot` but `destination_home_kind=rematerializable_immediate` |
| `src/20100316-1.c` | `destination_storage=stack_slot` but `destination_home_kind=rematerializable_immediate` |
| `src/920908-2.c` | `destination_storage=stack_slot` but `destination_home_kind=rematerializable_immediate` |
| `src/loop-2d.c` | Register source lacks source scalar type/size authority needed by the GPR-to-stack contract |
| `src/strcmp-1.c` | `destination_storage=stack_slot` but `destination_home_kind=pointer_base_plus_offset` |
| `src/strncmp-1.c` | `destination_storage=stack_slot` but `destination_home_kind=pointer_base_plus_offset` |

## Later Residuals

- `src/pr48197.c` advanced to `unsupported_terminator_fragment`.
- `src/20020510-1.c` and `src/pr89195.c` advanced to
  `RV64_BACKEND_RUNTIME_MISMATCH`.

These failures are later-route or runtime-correctness work, not blockers for
the RV64 move-bundle materialization source idea.

## Lifecycle Recommendation

No row remains in a clearly implementable RV64 move-bundle materialization
bucket. Because six rows still expose the old generic fragment status, the
supervisor should route closure through reviewer or plan-owner judgment rather
than silently closing from executor output alone. The narrow follow-up, if the
supervisor continues instead of closing, is a prepared-authority/evidence
packet that replaces those six generic diagnostics with explicit prepared
authority diagnostics.
