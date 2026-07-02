# Move-Bundle Target-Shape Representatives

Status: Step 3 representative classification from current per-case logs.

## Inputs

- Row source:
  `docs/rv64_gcc_torture_post_contract/unsupported_move_bundle_target_shape_rows.tsv`
- Evidence rules:
  `docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_classification_rules.md`
- Derived inspection artifacts:
  - `build/agent_state/544_step3_representatives/diagnostic_context.tsv`
  - `build/agent_state/544_step3_representatives/normalized_keys.tsv`
  - `build/agent_state/544_step3_representatives/normalized_keys_with_header.tsv`

The derived key scan grouped the 183 current bucket rows by
`event_kind`, `phase`, `authority`, and `move[0].reason`:

```text
160 before_instruction_copies  before_instruction  none                     consumer_register_to_stack
 15 before_instruction_copies  before_instruction  none                     consumer_stack_to_stack
  4 pre_terminator_copies      block_entry         out_of_ssa_parallel_copy phi_join_register_to_register
  2 pre_terminator_copies      before_return       none                     return_stack_to_register
  1 pre_terminator_copies      block_entry         out_of_ssa_parallel_copy phi_join_stack_to_stack
  1 missing detailed coordinate
```

No current bucket log matched `f128`, `_Float128`, `TFmode`, or `quad`, so
there is no observed F128-primary representative in the current log evidence.

## Output

Representative classifications are stored in:

```text
docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_representatives.tsv
```

The TSV uses the Step 2 classification schema:

```text
case	log	first_owner_lane	evidence_ref	diagnostic_key	move_shape	first_missing_fact	notes
```

## Classification Summary

| Lane | Representative rows |
| --- | ---: |
| `coherent_rv64_mir_materialization` | 2 |
| `prepared_module_target_shape_authority_gap` | 4 |
| `bir_semantic_producer_gap` | 0 |
| `f128_primary_quarantine` | 0 |
| `evidence_gap` | 1 |
| Total | 7 |

## Representative Coverage

`src/20080519-1.c` covers the plain before-instruction
`consumer_register_to_stack` shape where current prepared facts name the
coordinate, one move, value ids, rematerializable-immediate source home,
stack-slot destination home, scalar types, and generic RV64 materialization
failure.

`src/20000113-1.c` covers the before-instruction `consumer_stack_to_stack`
shape where the current diagnostic says stack-to-stack but publishes a
register source home. That is routed to prepared/module authority because RV64
would otherwise have to choose which prepared fact to believe.

`src/20001130-2.c` covers the before-return ABI shape. The diagnostic names a
function-return ABI register destination while the destination home remains a
stack slot, so the first owner is prepared/module return-home authority.

`src/pr37924.c` covers a select-publication phi join with complete intent and
publication evidence. The rejection is `unsupported_source_immediate_i32_range`
with intent/publication agreement, so the first missing fact is RV64
materialization of that select-publication immediate move.

`src/pr58726.c` covers select-publication phi stack-to-stack with
`intent_status=unsupported_source_home`. The row is not RV64-ready because the
current intent cannot publish a supported source home.

`src/pr36339.c` covers the pointer-style prepared-home subshape under the
large before-instruction register-to-stack family. The destination storage is
`stack_slot`, but the destination home is `pointer_base_plus_offset` and the
current log omits source type, so the prepared destination-home authority is
the first owner.

`src/960209-1.c` covers the one current row whose log only confirms
`unsupported_move_bundle_target_shape` bucket membership. It remains
`evidence_gap` because the current log has no event kind, phase, authority,
move ids, homes, types, or other auditable first-owner facts.

## Reviewer Notes

This representative pass does not classify any row from testcase name,
register spelling, raw BIR shape, or expected target shape. Each selected row
uses the current per-case `case.log` line range as the evidence anchor.

The representative pass did not find a current F128-primary shape. Step 4
should still keep the F128 lane in the full-bucket schema and route any row to
`f128_primary_quarantine` if a row-level artifact names a primary F128
dependency.
