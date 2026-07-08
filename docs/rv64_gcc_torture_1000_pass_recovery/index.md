# RV64 gcc_torture 1000-Pass Recovery Handoff Index

Status: Step 6 closure handoff for the `470/1467` RV64 backend scan.

This index gathers the umbrella outputs from
`ideas/open/601_rv64_gcc_torture_1000_pass_recovery_umbrella.md`. It does not
claim compiler progress or change implementation, tests, expectations,
unsupported markers, allowlists, runtime behavior, timeout policy, or
pass/fail accounting.

## Evidence Baseline

Authoritative current evidence is documented in
[`current_scan_summary.md`](current_scan_summary.md):

- command: `BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh`
- total rows: `1467`
- passing rows: `470`
- failing rows: `997`
- missing rows: `0`

The current row artifacts are the July 8 mutable summary and failed list under
`build/agent_state/`, with per-case logs under
`build/rv64_gcc_c_torture_backend/`. The older `349/1467`, `425/1467`, and
`438/1467` results are historical movement markers only. Follow-up work should
not use `349/1467` as current evidence.

## Required Handoff Documents

- [`current_scan_summary.md`](current_scan_summary.md): records the current
  `470/1467` baseline, `997` failures, `0` missing rows, scan artifacts, and
  stale-count guidance.
- [`failure_bucket_map.md`](failure_bucket_map.md): classifies the current
  `997` failures by first owner and capability family using row diagnostics
  and representative logs.
- [`high_yield_followup_plan.md`](high_yield_followup_plan.md): ranks the
  follow-up families by breadth, owner clarity, dependency order, and
  testcase-overfit risk.
- [`dependency_order_to_1000.md`](dependency_order_to_1000.md): converts the
  generated idea queue into the recommended activation order toward `1000+`
  passing rows.

All handoff docs agree that the current evidence is `470/1467`, with `997`
failed rows and `0` missing rows. Counts from `349/1467`, `425/1467`, and
`438/1467` appear only as stale or historical context.

## Bucket Method

The bucket map uses first stopping diagnostics from
`build/agent_state/rv64_gcc_c_torture_backend_failed.txt` and matching
`build/rv64_gcc_c_torture_backend/<case-id>/case.log` files. Representative
case names are examples of a diagnostic family, not implementation targets.

The first-owner rollup is:

| First owner | Count | Route meaning |
| --- | ---: | --- |
| BIR semantic producer | `311` | Producer gaps before prepared handoff. |
| RV64/MIR consumer | `252` | Prepared module exists, but target lowering lacks a consumer. |
| prepared/prealloc authority | `146` | Move-bundle source or destination authority is incomplete. |
| runtime | `72` | Object emits and links, but runtime behavior mismatches clang. |
| ABI/RV64 consumer | `60` | Call, return, or frame ABI shape is unsupported by RV64 object route. |
| prepared/RV64 authority | `48` | Shared authority exists or is close, but RV64-side use needs wiring. |
| prepared/global authority | `40` | Global data authority is missing before RV64 emission. |
| RV64/global consumer | `30` | RV64 global symbol or access-width emission gap. |
| policy/unsupported | `18` | Inline asm carrier fragments remain unsupported. |
| timeout/policy | `9` | Object compile timeout lane. |
| architecture research | `4` | Pointer arithmetic selected-authority question. |
| runtime/policy | `3` | Object run timeout lane. |
| prepared authority | `3` | Scalar compare publication tail. |
| runtime/link | `1` | Single link failure after object generation. |

## Generated Follow-Up Ideas

Step 4 generated the ordered open idea queue `602` through `618`:

| Order | Idea | Owning layer | Breadth |
| ---: | --- | --- | --- |
| 1 | [`602` BIR local-memory load semantics](../../ideas/open/602_bir_local_memory_load_semantics.md) | BIR semantic producer | `82` load rows |
| 2 | [`603` BIR local-memory store semantics](../../ideas/open/603_bir_local_memory_store_semantics.md) | BIR semantic producer | `56` store rows |
| 3 | [`604` BIR local-memory GEP and address semantics](../../ideas/open/604_bir_local_memory_gep_address_semantics.md) | BIR semantic producer | `43` GEP rows |
| 4 | [`605` BIR local-memory alloca and scalar semantics](../../ideas/open/605_bir_local_memory_alloca_and_scalar_semantics.md) | BIR semantic producer | `12` alloca plus `26` mixed rows |
| 5 | [`606` BIR global initializer bootstrap](../../ideas/open/606_bir_global_initializer_bootstrap.md) | BIR semantic producer | `38` initializer plus `2` string-pool rows |
| 6 | [`607` destination fan-in authority research](../../ideas/open/607_destination_fan_in_authority_research.md) | prepared/prealloc authority | `125` research-gated rows |
| 7 | [`608` prepared global data authority](../../ideas/open/608_prepared_global_data_authority.md) | prepared/global authority | `40` rows |
| 8 | [`609` RV64 global data consumer](../../ideas/open/609_rv64_global_data_consumer.md) | RV64/global consumer | `30` rows |
| 9 | [`610` RV64 move-bundle target materialization](../../ideas/open/610_rv64_move_bundle_target_materialization.md) | RV64/MIR consumer | `75` rows |
| 10 | [`611` RV64 terminator fragment lowering](../../ideas/open/611_rv64_terminator_fragment_lowering.md) | RV64/MIR consumer | `70` rows |
| 11 | [`612` RV64 instruction fragment consumers](../../ideas/open/612_rv64_instruction_fragment_consumers.md) | RV64/MIR consumer | up to `97` rows after sub-bucketing |
| 12 | [`613` ABI call, result, and stack-frame lowering](../../ideas/open/613_abi_call_result_stack_frame_lowering.md) | ABI/RV64 consumer | `60` rows |
| 13 | [`614` RV64 pointer local-memory consumption](../../ideas/open/614_rv64_pointer_local_memory_consumption.md) | prepared/RV64 boundary | `27` rows |
| 14 | [`615` branch stack-source residual audit and repair](../../ideas/open/615_branch_stack_source_residual_audit.md) | prepared/RV64 authority | `10` rows |
| 15 | [`616` select publication source wiring](../../ideas/open/616_select_publication_source_wiring.md) | prepared/RV64 authority | `11` rows |
| 16 | [`617` scalar compare publication](../../ideas/open/617_scalar_compare_publication.md) | prepared authority | `3` rows |
| 17 | [`618` runtime mismatch ownership investigation](../../ideas/open/618_runtime_mismatch_ownership_investigation.md) | runtime ownership mapping | `72` mismatches plus `3` run timeouts |

Each generated idea includes owning layer, prerequisites, estimated evidence
breadth, proof surface, acceptance criteria, and reviewer reject signals.

## High-Yield Route

The route toward `1000+` is producer-before-consumer:

1. Activate `602` through `606` first to repair or refine the largest BIR
   local-memory and initializer producer blockers.
2. Run `607` early as research because destination fan-in controls `125`
   prepared/prealloc authority rows, but do not implement those rows until the
   authority rule is settled.
3. Activate `608` before `609` so RV64 global emission consumes prepared
   global facts rather than reconstructing them.
4. Activate `610`, `611`, and then sub-bucketed `612` only where prepared
   authority is present.
5. Activate `613` after memory, global, and major RV64/MIR fragment
   prerequisites are explicit.
6. Activate `614` through `617` as recent-architecture-close wiring tails.
7. Run `618` after enough compile-time noise has moved that runtime symptoms
   can be mapped to real owners.

This order is intended to recover broad ordinary-C capability. It rejects
testcase-shaped shortcuts and expectation rewrites as progress.

## Architecture Weak Points

- Destination fan-in authority: `607` must define or reject a rule for
  non-parallel multi-source stack destinations before implementation work
  chooses among candidate destinations.
- Runtime mismatch ownership: `618` must map abort, segfault, wrong-output,
  and timeout rows to ABI, layout, memory, call, or true runtime support owners
  before runtime fixes are attempted.
- Pointer/address boundary: `604` and `614` must preserve the distinction
  between BIR GEP/address production and RV64 consumption of selected
  pointer/local-memory authority from ideas `599` and `600`.
- Branch, select, and compare publication tails: `615`, `616`, and `617` must
  keep source freshness, alias evidence, destination legality, and target
  consumption separately observable.

## Deferred And Quarantined Families

Deferred families are outside the first route unless later evidence changes
their breadth or ownership:

- memcpy and memset intrinsic semantics;
- unordered floating compare and floating cast rows;
- string-pool constants, direct-call semantic tails, small scalar/vector tails,
  and the single post-object link failure;
- compile timeouts and run timeouts until performance, harness policy, or
  runtime ownership is explicitly investigated.

Quarantined lanes must not be counted as recovery progress inside this
umbrella route:

- inline asm carrier fragments;
- F128-, library-, builtin-, or environment-dependent rows;
- any lane requiring expectation downgrades, unsupported-marker changes,
  allowlist filtering, timeout tuning, weaker runtime comparison, or
  pass/fail accounting changes.

## Recommended Next Lifecycle Activation

After Step 7 closure readiness review, the recommended next lifecycle
activation is [`602` BIR local-memory load semantics](../../ideas/open/602_bir_local_memory_load_semantics.md).
That idea is the first producer-owned gate in the dependency order, has
current `470/1467` evidence, and does not require RV64 target work before BIR
load facts are repaired or more accurately diagnosed.

If the supervisor wants to settle architecture risk before implementation,
[`607` destination fan-in authority research](../../ideas/open/607_destination_fan_in_authority_research.md)
is the early research gate, but it should not replace `602` as the first
ordinary implementation activation unless route policy changes.
