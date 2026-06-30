Status: Active
Source Idea Path: ideas/open/458_select_edge_source_producer_move_bundle_placement_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Select-Edge Placement Evidence

# Current Packet

## Just Finished

Completed Step 1 audit for idea 458. Evidence is recorded under
`build/agent_state/458_step1_select_edge_placement_audit/`.

Bucket table for the `20010329-1` target bundle:

| Field | Evidence | Classification |
| --- | --- | --- |
| Target bundle | `move_bundle phase=before_instruction authority=none block_index=4 instruction_index=2` | In-scope target; before-instruction register-destination bundle with no explicit placement authority. |
| Move 1 | `from_value_id=7 to_value_id=10 destination_storage=register placement=gpr:caller_saved#0/w1 reason=consumer_register_to_register` | `%t15 -> %t18`; source home is register `s1`. |
| Move 2 | `from_value_id=9 to_value_id=10 destination_storage=register placement=gpr:caller_saved#0/w1 reason=consumer_stack_to_register` | `%t17 -> %t18`; source home is stack slot, but `%t17` has explicit `rematerialize_cast_from_source status=available` authority. |
| Source producer | `%t18 = bir.ule ptr %t15, %t17` at `logic.end.14` instruction index `2` | Existing source-producer facts identify the binary producer. |
| Select carrier | `%t22 = bir.select uge ptr %t5, %t7, i32 %t18, 0`; `select_chain ... root_is_select=yes source_producer=select_materialization` | `%t18` is an incoming value for a select-materialization carrier, not an ordinary same-block temporary. |
| Edge transfer | `logic.rhs.end.13 -> logic.end.14 incoming=%t18 destination=%t22` | Explicit edge relationship exists. |
| Edge-owned publication | `parallel_copy logic.rhs.end.13 -> logic.end.14 execution_site=predecessor_terminator`; block-entry publication for `%t22` at block index `2` | Edge destination publication exists, but it is separate from the target before-instruction producer bundle. |
| Homes | `%t15` value id `7` register `s1`; `%t16` value id `8` rematerializable immediate; `%t17` value id `9` stack slot; `%t18` value id `10` register `t0`; `%t22` value id `11` register `t0` | Homes describe values but do not prove placement or suppression. |
| Existing placement facts | Target bundle has `authority=none` and no printed `source_parallel_copy=...` | First missing fact is explicit placement/source-producer authority on or for the target bundle. |

Classification:

- Accepted target-consumable placement: none.
- Coherent candidate evidence: target bundle plus binary source producer,
  select carrier, edge transfer, block-entry edge publication, and RHS
  cast-rematerialization authority.
- First missing authority: producer-owned placement semantics distinguishing
  same-block setup, edge-owned materialization with predecessor/successor
  identity, or suppression because predecessor-edge publication already
  consumes the source producer.
- Fail-closed: `load_from_stack_slot status=missing_stack_freshness`, raw
  placement inference, generic stack/register moves, and RV64 lowering before
  placement authority exists.

## Suggested Next

Execute Step 2: `Define Placement Authority Contract`.

Define the prepared facts required to distinguish:

- same-block compare-operand setup that should emit at the instruction;
- edge-owned materialization with explicit predecessor/successor identity; and
- suppression because predecessor-edge publication already consumes the source
  producer.

The contract should require semantic source-producer identity, select carrier
identity, edge identity when edge-owned, source/destination value ids and homes,
and an explicit placement/suppression decision. Keep RV64 lowering out until
that producer authority exists.

Re-read:

- `ideas/open/458_select_edge_source_producer_move_bundle_placement_authority.md`
- `build/agent_state/458_step1_select_edge_placement_audit/audit.md`

Classification-only proof:

```sh
git diff --check
```

## Watchouts

- Do not route to RV64 lowering before placement authority exists.
- Do not infer predecessor/successor identity or suppression authority from
  value ids, block indexes, instruction indexes, raw BIR shape, filenames,
  function names, or one prepared dump.
- Do not reopen idea 456 cast-dependency consumption.
- Do not consume `load_from_stack_slot missing_stack_freshness`.
- Keep generic stack-home branch consumer work routed to
  `ideas/open/451_stack_home_branch_operand_materialization.md`.
- Keep pointer-value provenance and generic instruction-side lowering out of
  this plan.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, baseline logs, or
  `review/`.

## Proof

Step 1 classification-only validation:

```sh
git diff --check
```

Result: passed.
