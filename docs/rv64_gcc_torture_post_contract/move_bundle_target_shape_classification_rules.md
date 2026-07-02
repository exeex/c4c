# Move-Bundle Target-Shape Classification Rules

Status: Step 2 evidence contract for classifying the reconstructed 183-row
`unsupported_move_bundle_target_shape` bucket.

## Input Row Set

Classify only rows from:

```text
docs/rv64_gcc_torture_post_contract/unsupported_move_bundle_target_shape_rows.tsv
```

That table has this schema:

```text
case	log
```

The `log` path is the current per-case evidence anchor. A row must not be
classified from testcase name, expected RV64 output shape, destination register
spelling, or raw BIR shape alone.

## Classification Output

The full-bucket classification should be stored in:

```text
docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_classification.tsv
```

Required schema:

```text
case	log	first_owner_lane	evidence_ref	diagnostic_key	move_shape	first_missing_fact	notes
```

- `case` and `log`: copied exactly from the reconstructed row table.
- `first_owner_lane`: exactly one of the lane values below.
- `evidence_ref`: path plus line range, or a derived artifact path, that lets a
  reviewer audit the row without re-mining unrelated logs.
- `diagnostic_key`: the current `unsupported_move_bundle_target_shape`
  diagnostic tokens used for routing, including event kind, phase, authority,
  move reason, destination storage, and source/destination types when present.
- `move_shape`: a stable normalized shape such as
  `before_instruction/authority_none/consumer_register_to_stack`.
- `first_missing_fact`: the earliest layer that lacks a required fact or
  lowering rule.
- `notes`: short reviewer note; use an empty field when no note is needed.

Lane counts in the final artifact must sum to 183.

## Decision Order

Apply these checks in order and stop at the first matching lane:

1. `f128_primary_quarantine`
2. `bir_semantic_producer_gap`
3. `prepared_module_target_shape_authority_gap`
4. `coherent_rv64_mir_materialization`
5. `evidence_gap`

This order keeps primary-F128 and producer-owned rows out of ordinary RV64
materialization queues, and keeps uncertain rows from becoming testcase-shaped
implementation work.

## Lane: coherent_rv64_mir_materialization

Use this lane only when row evidence proves all required semantic and prepared
authority already exists, and the first missing capability is RV64/MIR
materialization of that prepared move bundle.

Minimum evidence:

- The current case log contains `unsupported_move_bundle_target_shape`.
- The diagnostic records a concrete prepared move bundle coordinate:
  `event_kind`, `phase`, function, block, instruction, bundle block, and bundle
  instruction.
- The diagnostic records concrete move facts: `move_count`, `parallel_copy`,
  `move[N].from_value_id`, `move[N].to_value_id`, destination kind/storage,
  `op_kind`, reason, source type, and destination type.
- The row-level prepared evidence identifies the source and destination value
  homes needed by RV64, such as register bank/name, stack-slot id/offset, ABI
  home, immediate rematerialization, type width, size, and alignment.
- The semantic BIR and prepared facts agree on the value relationship the move
  represents; no missing local-memory, call-argument, aggregate, or object-data
  producer fact is required before RV64 can consume the move.
- The move is not primary-F128.

Examples of implementation-ready families include coherent
register-to-stack, stack-to-register, stack-to-stack, or register-to-register
move materialization where prepared value homes and scalar sizes are complete.
The classification may still split those families later, but they share this
first-owner lane.

## Lane: prepared_module_target_shape_authority_gap

Use this lane when semantic intent is visible, but prepared/module target-shape
authority is missing, inconsistent, or too weak for RV64 to lower safely.

Minimum evidence:

- The current case log contains `unsupported_move_bundle_target_shape`.
- The row has enough context to identify the move-bundle coordinate or the
  specific missing coordinate.
- The first missing fact belongs to prepared/module publication, not RV64
  emission. Examples include absent or inconsistent authority, missing source
  or destination prepared value home, missing stack-slot metadata, missing ABI
  destination metadata, unsupported or ambiguous move classification, missing
  type/size/alignment for the prepared home, or contradictory bundle
  coordinate/phase data.
- The row does not require BIR semantic admission or F128 policy before the
  prepared authority question can be answered.

Rows in this lane must create or feed prepared/module contract work before any
dependent RV64 lowering idea consumes them.

## Lane: bir_semantic_producer_gap

Use this lane when the earliest missing fact is semantic BIR production or
semantic admission, even if the final visible diagnostic is a prepared
move-bundle target-shape failure.

Minimum evidence:

- The row-level evidence identifies a missing or incoherent semantic source
  fact that prepared/module code cannot authoritatively invent.
- Examples include missing local-memory identity, missing aggregate field or
  object-data semantics, missing call-argument or return-value metadata,
  missing inline-asm operand semantics, impossible value provenance, or a
  semantic/prepared mismatch where prepared facts mirror incomplete BIR.
- The evidence shows why RV64 or prepared-module inference would be an
  overfit, such as depending on raw BIR shape, testcase name, or expected
  target register layout instead of an authored semantic fact.

Rows in this lane must feed a BIR semantic producer idea before prepared or
RV64 work resumes for those rows.

## Lane: f128_primary_quarantine

Use this lane when the row is primarily blocked by F128 type, ABI, storage, or
operation policy.

Minimum evidence:

- The row-level log, source-level compiler output, semantic BIR, prepared BIR,
  or prepared object evidence names an F128, `_Float128`, `TFmode`, quad-float,
  F128 ABI, F128 local-memory, or F128 parameter/return-home dependency.
- The F128 dependency is primary for the failing move-bundle route, not merely
  an unrelated type present elsewhere in the testcase.

Primary-F128 rows must remain in the existing F128 quarantine lane unless a
future proof shows broad non-F128 impact.

## Lane: evidence_gap

Use this lane when current artifacts are insufficient to assign one of the
other lanes without guessing.

Minimum evidence:

- The current case log confirms the row belongs to
  `unsupported_move_bundle_target_shape`, but the row lacks enough auditable
  detail to decide whether the first owner is RV64/MIR, prepared/module, BIR
  semantic producer, or F128.
- The missing evidence is named in `first_missing_fact`, such as missing
  prepared dump, missing semantic BIR dump, missing source/destination home
  detail, missing F128 screen, or ambiguous producer/prepared mismatch.

Evidence-gap rows are not implementation-ready. The next packet should gather
or generate the missing row-level evidence instead of assigning ownership from
testcase shape.

## Reviewer Checks

A classification is invalid if it:

- assigns more than one first-owner lane to a row;
- assigns no lane to a row;
- routes a row to RV64/MIR while naming a missing BIR or prepared producer
  fact;
- routes a primary-F128 row into ordinary-C progress;
- relies on testcase names, expected register spelling, raw BIR fragments, or
  final target shape without prepared/semantic authority;
- changes expectations, unsupported markers, allowlists, runtime comparison
  code, or compiler behavior.
