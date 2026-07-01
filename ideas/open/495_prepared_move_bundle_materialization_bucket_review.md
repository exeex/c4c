# Prepared Move-Bundle Materialization Bucket Review

Status: Open
Type: Fresh RV64 gcc_torture high-impact bucket review and follow-up splitter
Parent: `ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md`
Handoff Directory: `docs/rv64_gcc_torture_post_contract/`
Owning Layer: Prepared move-bundle authority first, RV64 object lowering only for coherent prepared bundles

## Goal

Classify the fresh `unsupported_move_bundle_target_shape` RV64 gcc_torture
failures and split them into producer/authority follow-ups versus RV64
materialization follow-ups.

## Why This Exists

The 2026-07-01 supervisor scan recorded `1467` total cases, `314` pass, and
`1153` fail on the RV64 gcc_torture backend object path. A first-owner bucket
pass found `423` failures dominated by `unsupported_move_bundle_target_shape`,
larger than both `semantic_lir_to_bir` and `unsupported_instruction_fragment`.

Move bundles are ordinary-C control-flow, phi, preservation, publication, and
copy traffic. They should be handled before lower-frequency F128 or narrow
instruction-fragment work, but only after determining whether the prepared
bundle facts are coherent enough for target lowering.

## In Scope

- Reproduce the current move-bundle bucket from
  `build/agent_state/rv64_gcc_c_torture_backend_summary.full.tsv`.
- Classify move bundles by `event_kind`, `authority`, `parallel_copy`,
  execution site, destination storage, source storage, and move reason.
- Separate coherent prepared bundles that need RV64 materialization from
  missing or confused prepared authority.
- Create follow-up ideas for producer/authority gaps before any RV64 consumer
  work that would otherwise infer facts.
- Create RV64 lowering ideas only for coherent bundle families.
- Preserve the external-test policy: RV64 gcc_torture is not default CTest
  harness coverage.

## Out Of Scope

- Implementing move-bundle lowering inside this review idea.
- Reconstructing missing move authority, predecessor/successor identity,
  storage class, or value identity in RV64 from raw BIR or testcase shape.
- F128-specific move bundles unless they are explicitly quarantined and do not
  drive the priority order.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline churn.

## Acceptance Criteria

- The handoff directory records fresh move-bundle counts, representative rows,
  and first-owner classifications.
- Producer/prepared gaps are split into separate BIR/prepared ideas.
- Coherent RV64-lowerable bundle families are split into separate RV64 ideas
  with concrete proof subsets.
- Default `ctest --test-dir build -j --output-on-failure` must not regress for
  lifecycle close.

## Reviewer Reject Signals

- Reject RV64 code that guesses move source/destination authority, edge
  execution site, storage identity, or phi/join semantics from raw shape.
- Reject combining producer repair and RV64 materialization in one
  implementation slice.
- Reject testcase-named move handling or one-file move recipes.
- Reject progress claims based primarily on expectation, allowlist, or
  unsupported-marker changes.
