Status: Active
Source Idea Path: ideas/open/461_rv64_move_bundle_coordinate_diagnostics.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Diagnostic Coordinate Gap

# Current Packet

## Just Finished

Completed Step 1 for idea 461: audited the RV64 move-bundle diagnostic
coordinate gap and saved the table under
`build/agent_state/461_step1_diagnostic_coordinate_gap/audit.md`.

| Missing field / decision point | Current evidence | Candidate event families | Possible diagnostic/probe surface |
| --- | --- | --- | --- |
| Traversal event kind and move-bundle phase | Object stderr only reports broad `unsupported_move_bundle_target_shape`. | Block-entry select copies, before-instruction cast stack publication, before-instruction suppression/register setup. | Event-site rejection in `prepared_function_to_object_function` can print `event.kind` and `move_bundle.phase`. |
| Function/block/instruction coordinate | Prepared output has rows such as `4:1` and `4:2`, but the object route omits the first rejecting coordinate. | `4:1` cast stack publication, `4:2` idea-459 suppression target, later `8:1`/`12:1` register setup, `3/2`/`7/6`/`11/10` select copies. | Event-site rejection already has `event.block_index`, `event.instruction_index`, and the classified move bundle. |
| Move index and move identity | Multi-move bundles exist; stderr does not say which move failed. | Register setup bundles and select-result parallel copies. | Either print all bundle moves at the event-site rejection or make `fragment_for_prepared_move_bundle` return a move-level failure record. |
| Destination storage and move reason | Prepared rows distinguish `consumer_register_to_stack`, `consumer_stack_to_register`, and `phi_join_*`; stderr drops them. | Cast-dependency stack publication vs select publication vs generic consumer moves. | Add a move-bundle coordinate formatter using `destination_storage_kind` and `reason`. |
| Authority match status | Step 460 could not tell whether cast stack publication or idea-459 suppression matched the object-route guards. | `4:1` cast-dependency stack publication; `4:2`, `8:1`, `12:1` suppression/register rows. | Event-site diagnostic can call the existing suppression and cast-stack authority predicates before fragment emission. |
| Fragment failure reason | `fragment_for_prepared_move_bundle` returns only `std::nullopt`. | Any admitted bundle that reaches generic RV64 move materialization. | A second-stage structured fragment result can carry failing move index and guard reason if event-site coordinates are insufficient. |

## Suggested Next

Execute Step 2: define the coordinate diagnostic contract. The first coherent
implementation packet should be diagnostic/probe-only at the event-site
rejection for `unsupported_move_bundle_target_shape`, reporting event kind,
phase, function/block/instruction coordinate, move identities, destination
storage, reason, parallel-copy context, and cast/suppression authority match
status. Refine `fragment_for_prepared_move_bundle` only if the event-site
fields still cannot identify the failing guard.

## Watchouts

- This is diagnostics/probe support, not semantic lowering.
- Do not add generic stack-to-register or register-to-register move support.
- Do not consume `load_from_stack_slot missing_stack_freshness`.
- Do not reopen ideas 456, 458, or 459 without coordinate-bearing evidence.
- Do not infer ownership from value ids, block indexes, instruction indexes,
  raw BIR shape, filenames, function names, or one prepared dump alone.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, baseline logs, or
  `review/`.

## Proof

Step 1 proof:

```sh
git diff --check
```

Result: passed.
