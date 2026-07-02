# Prepared Move-Bundle Target-Shape Authority Gaps

Status: Open
Type: Prepared/module authority repair queue
Parent: `ideas/open/544_rv64_move_bundle_target_shape_bucket_split.md`
Owning Layer: Prepared/module target-shape authority

## Goal

Repair or refine prepared/module publication for the 31 current
`unsupported_move_bundle_target_shape` rows classified as
`prepared_module_target_shape_authority_gap`.

## Why This Exists

The bucket split review found rows where semantic intent is visible but the
prepared/module target-shape facts are missing, inconsistent, or too weak for
RV64 to lower safely. RV64 must not infer these facts. The prepared layer must
publish coherent move classification, home, type, and ABI authority first.

## Evidence Anchor

- Source splitter: `ideas/open/544_rv64_move_bundle_target_shape_bucket_split.md`
- Classification table:
  `docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_classification.tsv`
- Classification note:
  `docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_classification.md`
- Lane filter: `first_owner_lane=prepared_module_target_shape_authority_gap`
- Current row count: 31

Current missing-fact subqueues from the classification table:

- 11 `prepared_destination_home_shape_authority` rows.
- 9 `prepared_move_classification_or_source_home_authority` rows.
- 8 `prepared_move_type_authority` rows.
- 2 `prepared_return_abi_destination_home_authority` rows.
- 1 `prepared_select_publication_source_home_authority` row.

## In Scope

- Make prepared/module move-bundle publication explicit and coherent for the
  31 classified authority-gap rows.
- Repair source-home, destination-home, type/size/alignment, return ABI, and
  select-publication authority where those are the earliest missing facts.
- Preserve row-level evidence that proves RV64 can consume the repaired facts
  after this prepared queue closes.
- Split further if one authority family requires a separate implementation
  route.

## Out Of Scope

- RV64 materialization for rows whose prepared facts are still missing or
  inconsistent.
- BIR semantic producer changes unless a row proves prepared is mirroring a
  missing semantic fact.
- F128 policy or ordinary-C/F128 accounting changes.
- Expectation rewrites, unsupported marker edits, allowlist filtering, or
  runtime comparison changes.
- Hiding the authority gap behind a renamed diagnostic.

## Acceptance Criteria

- Prepared/module output gives RV64 auditable source and destination homes,
  move classification, scalar type facts, and ABI metadata for a coherent
  subset of the 31 rows.
- Rows that prove to need BIR semantic producer work are explicitly rerouted
  instead of repaired by prepared inference.
- A dependent RV64 implementation idea can consume the fixed prepared facts
  without guessing.
- Fresh proof covers the selected prepared authority family and preserves
  supported-path behavior.

## Reviewer Reject Signals

- Reject RV64 lowering that consumes these rows before prepared authority is
  repaired.
- Reject prepared fixes that invent facts from testcase names, expected RV64
  destination shape, raw BIR text, or register spelling.
- Reject combining this queue with the 151 coherent RV64 materialization rows
  as one implementation slice.
- Reject unsupported downgrades, expectation rewrites, allowlist filtering, or
  weaker runtime comparison as progress.
- Reject diagnostic-only or helper-rename changes that leave the same missing
  source-home, destination-home, type, return ABI, or select-publication
  authority behind a new name.
