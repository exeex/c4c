# RV64 gcc_torture Failure Bucket Map

Status: Step 4 current reset-main evidence refresh complete.

## Current Evidence Anchor

Use the stable 2026-07-02 reset-main/post-cleanup RV64 gcc_torture
backend-object scans as the current planning anchor:

- `1467` total cases
- `349` pass
- `1118` fail
- `build/agent_state/rv64_gcc_torture_backend_current_20260702T032151Z.log`
- `build/agent_state/rv64_gcc_torture_backend_current_20260702T151551Z.log`

Those two scans had the same totals and the same pass/fail case set. This map
therefore treats the timestamped 2026-07-02 evidence, the current mutable
summary, and the current per-case logs as the source of truth:

- `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`
- `build/agent_state/rv64_gcc_c_torture_backend_failed.txt`
- `build/rv64_gcc_c_torture_backend/<case-id>/case.log`

Older handoff summaries and instruction-fragment-only classifications are
historical support only. They must not be used as the current priority anchor.

## Evidence Limits

The current summary TSV records `status`, `case`, and `log` only. It does not
carry row-level first-owner annotations. The bucket counts below are therefore
limited to diagnostics directly visible in the current per-case logs plus the
source idea's explicit stable-scan fact for the move-bundle bucket.

The current per-case logs support these directly counted diagnostic groups:

| Diagnostic group | Current rows | First-owner confidence | Routing decision |
| --- | ---: | --- | --- |
| `unsupported_move_bundle_target_shape` | 183 | High; source idea explicitly records this as the largest current explicit prepared/module-shape bucket. | First expected-value ordinary-C follow-up candidate. Split coherent RV64 move materialization from prepared/BIR authority gaps before implementation. |
| `unsupported_instruction_fragment` | 137 | Medium; explicit RV64 unsupported diagnostic, but no refreshed row-level sub-bucket table exists for the current 2026-07-02 rows. | RV64 instruction-fragment follow-up after move-bundle and producer-admission work. Reclassify current rows before implementation. |
| `unsupported_stack_frame` | 85 | Medium; explicit infrastructure diagnostic, but current rows are not split by ordinary scalar, FPR, F128, or producer authority. | Prepared/RV64 frame infrastructure review. Keep broad FPR/F128 expansion out of the ordinary-C route unless row evidence requires it. |
| `unsupported_local_memory_access` | 44 | Medium; explicit prepared module-shape diagnostic, but first-owner split is not verified row by row. | BIR/prepared local-memory authority and RV64 memory-lowering boundary review. Do not guess missing prepared address facts in RV64. |
| `unsupported_global_data` | 43 | Medium; explicit infrastructure diagnostic. | Prepared/global-data infrastructure review before RV64 global-address lowering consumes the facts. |
| `unsupported_terminator_fragment` | 27 | Medium; explicit RV64 unsupported diagnostic. | RV64 terminator-lowering review after higher-count ordinary-C buckets. |
| `unsupported_prepared_move_bundle_classification` | 26 | Medium; explicit prepared classification diagnostic. | Prepared/module classification owner first; dependent RV64 move lowering should wait for coherent prepared authority. |
| `unsupported_scalar_compare_publication` | 3 | Medium; explicit scalar publication diagnostic. | Small RV64 scalar compare/publication residual; not a primary queue driver. |
| `unsupported_floating_cast` | 2 | Medium; explicit scalar-FP diagnostic. | Tiny non-F128 scalar FP salvage candidate; keep separate from F128. |
| `unsupported_variadic_helper_lowering` | 1 | Medium; explicit helper diagnostic. | Runtime/helper ABI review, low priority. |
| Compile failures with no explicit `unsupported_*` diagnostic | 503 | Low; current logs expose compile failure but not a first-owner row reason. | Evidence gap. Re-run or enrich row-level diagnostics before assigning to BIR, prepared, RV64, runtime, or test infrastructure owners. |
| Other non-unsupported failures | 57 | Low; includes subprocess aborts/timeouts and other non-explicit failures. | Evidence gap. Needs targeted reproduction before ownership assignment. |
| Segmentation-fault exits | 7 | Low; crash signal only. | Evidence gap. Treat as crash triage, not ordinary-C capability progress, until reproduced. |

The directly counted rows above account for the current `1118` failures. Only
the rows with explicit current diagnostics should be used for current
first-owner planning. The 567 rows without explicit current ownership evidence
must not be silently folded into a named implementation bucket.

## First-Owner Classification Rules

Classify rows by the first layer that must supply a semantic fact or lowering
rule:

- F128 rows are screened into the F128 quarantine lane before ordinary non-F128
  work. F128 remains lowest priority unless fresh evidence proves it blocks a
  broad non-F128 owner.
- Prepared/module-shape diagnostics own missing or incoherent prepared facts
  before RV64 lowering is asked to consume them.
- BIR semantic producer gaps own missing semantic admission, local-memory,
  call metadata, or aggregate facts before MIR/RV64 repair work.
- RV64 object lowering owns coherent prepared/BIR facts that are rejected only
  because the object route lacks a semantic lowering rule.
- Runtime mismatch and crash rows require targeted reproduction before they can
  be treated as ordinary lowering progress.

## Current Priority Order

1. `unsupported_move_bundle_target_shape` is the current highest expected-value
   ordinary-C candidate with 183 verified current rows. The next idea should
   split those rows into coherent RV64 move-materialization work versus
   prepared/BIR authority gaps.
2. BIR semantic producer admission remains a high-priority owner family from
   the source idea, but this file does not have a verified current row count for
   it. Step 5 should avoid inventing one unless it creates or cites current
   row-level evidence.
3. `unsupported_instruction_fragment` has 137 current explicit rows. The older
   detailed instruction-fragment sub-bucket work is useful as a taxonomy, but
   its stale row counts are not current evidence.
4. `unsupported_local_memory_access`, `unsupported_global_data`,
   `unsupported_stack_frame`, and
   `unsupported_prepared_move_bundle_classification` should become
   prepared/BIR/RV64 boundary reviews before implementation slices consume
   those facts.
5. Runtime, crash, timeout, and compile-failure rows without explicit current
   diagnostics are evidence gaps. They need refreshed row-level classification
   before being ranked ahead of explicit ordinary-C buckets.
6. F128 is quarantined and lowest priority. Primary-F128 testcase rows should
   be screened away from ordinary-C progress accounting unless a future proof
   shows broad non-F128 impact.

## Historical Supporting Evidence

The previous instruction-fragment classification remains a useful taxonomy for
future re-bucketing, but its counts came from an older scan and do not describe
the current 2026-07-02 row set. When the queue reaches current
`unsupported_instruction_fragment` work, rerun row-level classification before
turning those historical categories into implementation ideas.

Historical categories to consider during that rerun:

- select and join materialization
- call-adjacent scalar publication and inline-asm materialization
- pointer cast and address materialization
- aggregate `sret`/`byval` call-storage
- integer div/rem lowering
- integer arithmetic shift right
- large literal and materialization
- global memory/addressing residual
- scalar F32/F64 conversion/op residual
- F128 quarantine

Do not reuse the historical category counts as current row counts without a
fresh 2026-07-02 row table.
