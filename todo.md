Status: Active
Source Idea Path: ideas/open/475_prepared_frame_slot_source_fact_population.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Population Contract

# Current Packet

## Just Finished

Completed Step 2 contract for idea 475. The real source-fact population
contract is defined for scalar compare-result rows, but no bounded
implementation packet is justified from current evidence.

Decision: route Step 3 as blocker-only unless a lower-level producer first
publishes explicit semantic instruction-result materialization/write events and
interval no-clobber facts. The existing idea 474 carrier can represent the
facts, but current prepared data cannot populate them for `%t23` slot `#21`
without raw-shape inference.

Positive contract for a real scalar compare-result source fact:

| Required fact | Contract |
| --- | --- |
| Target identity | Function, block, branch role, target value id/name/type, frame slot id, stack object id, offset, size, and alignment must match the existing source-fact carrier. |
| Semantic producer | A prepared producer record must identify the scalar compare instruction result, e.g. `%t23 = ne i32 %t22, 0`, with opcode/type/operands and result value id. |
| Materialization/write event | A prepared event must explicitly write or materialize that exact result value into the target frame slot/object. |
| Path validity | A path/dominance or edge-scope fact must prove the event reaches the branch consumer site. |
| Same-slot exclusion | The interval from materialization to consumer must classify same-slot writes as absent. |
| Effect safety | Calls, helpers, inline asm, publications, move bundles, and parallel copies in the interval must be classified non-clobbering for the target slot/object. |

Negative / fail-closed cases:

| Shape | Disposition |
| --- | --- |
| `from_value_id=16 -> to_value_id=17` stack move | Rejected. It is a plain `%t22` to `%t23` storage copy with `authority=none`, not semantic materialization of `%t23 = ne i32 %t22, 0`; using it would be source/value mismatch or raw-shape inference. |
| Final stack home/storage only | Rejected as insufficient. |
| Raw BIR adjacency only | Rejected as insufficient without prepared materialization/write event. |
| Visible calls/moves/copies/publications without slot classifier | Fail closed as missing effect/non-clobber proof. |
| `%t22` select-result stack destination | Protected `unsupported_boundary`; separate owner. |
| `%t1` / `%t7` pointer/provenance rows | Protected `unsupported_boundary`; separate owner. |
| `%t2` / `%t8` unsupported-terminator rows | Protected `unsupported_boundary`; separate owner. |
| Downstream branch-stack-load availability or RV64 consumption | Out of scope until available source-fact records exist. |

Exact Step 3 blocker:

- no current prepared record models semantic scalar instruction-result
  materialization/write into a frame slot;
- no current path/dominance fact ties such an event to `f.logic.end.14`;
- no current same-slot write exclusion or effect non-clobber classifier exists
  for slot `#21`;
- the only nearby move into value `#17` is `authority=none` and carries source
  value `#16`, so it cannot populate the `%t23` current-value source fact.

Artifacts:

- `build/agent_state/475_step2_source_fact_population_contract/contract.md`

## Suggested Next

Execute Step 3 as a routing/blocker packet for idea 475. Record that no real
source-fact population implementation is selected until a separate lower-level
producer supplies semantic instruction-result frame-slot materialization/write
events plus path and no-clobber interval facts. Do not edit implementation in
that route packet unless the supervisor explicitly supplies a new producer
surface.

## Watchouts

- Do not reuse the `%t22 -> %t23` stack move as a source fact.
- Do not infer materialization/current-value/no-clobber from BIR adjacency,
  final stack homes, storage, offsets, value ids, value names, block names,
  testcase shape, or dump order.
- Keep select-result stack-destination, pointer/provenance, and
  unsupported-terminator rows as protected boundaries.
- Keep downstream `PreparedBranchStackLoadAuthority` unavailable and do not
  edit RV64 target lowering.

## Proof

Delegated proof:

```sh
git diff --check
```

Result: passed.
