Status: Active
Source Idea Path: ideas/open/474_prepared_frame_slot_materialization_no_clobber_facts.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Prepared Source-Fact Carrier Requirements

# Current Packet

## Just Finished

Completed Step 1 audit for idea 474. Existing prepared facts identify branch
sites, value homes, frame slots, stack objects, calls, publications, move
bundles, and parallel copies, but there is no durable source-fact carrier that
ties those inputs into an explicit frame-slot materialization/write event plus
path validity and no-clobber proof.

Carrier requirement table:

| Requirement | Existing input | Classification |
| --- | --- | --- |
| Source-fact record identity | Function/block/instruction coordinates, branch condition rows, value ids/names, frame slot ids, stack object ids, offsets/sizes/alignments. | Accepted as identity inputs only. |
| Exact source value | Some source values are available in `store_source`, edge publication, and move/copy facts; `%t23` is visible as the branch condition value. | Required carrier field; not enough without a write/materialization event. |
| Frame-slot materialization/write event | Store-source records cover BIR `store_local`/`store_global` publication; move bundles and parallel copies cover selected transfers; regalloc homes show final stack slots. | Missing for `%t23` slot `#21`; final stack home is not a write event. |
| Destination slot/object match | `PreparedValueHome`, `PreparedFrameSlot`, and `PreparedStackObject` can prove slot/object/offset/size/align identity. | Accepted prerequisite. |
| Path/dominance validity | Prepared CFG, branch conditions, join transfers, and parallel-copy execution sites exist. | Missing as a source-fact status; no path-scoped current-value proof exists. |
| Same-slot write exclusion | Store-source rows, move bundles, block-entry publications, parallel copies, and stack objects are visible. | Missing as a no-clobber classification over a slot/object interval. |
| Call/helper/inline-asm safety | Call plans expose callsites, clobbered registers, preserved values, and wrappers; inline-asm/helper carrier surfaces exist elsewhere. | Missing as slot/object effect safety for the target interval. |
| Publication/move/parallel-copy non-clobber | Prepared publication, move-bundle, and parallel-copy records are printed and queryable. | Missing as clobbering/non-clobbering/unknown status per target slot. |
| Downstream availability | `PreparedBranchStackLoadAuthority` has policy/freshness/clobber booleans. | Out of scope; must stay unavailable until source facts are produced and later consumed. |

First producer disposition:

| Candidate | Classification |
| --- | --- |
| Implement available source facts now | Rejected; explicit materialization/write and no-clobber facts are not present. |
| Implement from raw BIR adjacency, stack homes, object ids, value names, or dump order | Rejected as overfit/raw-shape inference. |
| Define carrier/status contract first | Selected Step 2 packet. |
| Later bounded producer | Only after Step 2 defines statuses: likely create a prepared source-fact carrier that can emit unavailable records for missing materialization/path/no-clobber and available records only when all explicit producer facts exist. |

Artifacts:

- `build/agent_state/474_step1_source_fact_carrier_audit/audit.md`
- `build/agent_state/474_step1_source_fact_carrier_audit/evidence_snippets.md`

## Suggested Next

Execute Step 2 from `plan.md`: Define Prepared Source-Fact Contract.

The contract should define record keys, positive statuses, unavailable statuses,
and fail-closed cases for:

- exact source value plus source producer;
- materialization/write event identity;
- frame slot/object/offset/size/alignment match;
- path/dominance validity;
- same-slot write exclusion;
- call/helper/inline-asm effect safety;
- publication/move-bundle/parallel-copy clobber classification;
- downstream consumer boundary.

## Watchouts

- Do not edit implementation files during Step 2.
- Do not implement RV64 branch-load emission in this producer plan.
- Do not directly populate available branch-stack-load authority records.
- Do not infer source facts from stack homes/storage, offsets, object ids, raw
  BIR, value names, block labels, function names, testcase names, or dump
  order.
- Keep pointer-value/provenance, select-result stack-destination, and
  unsupported-terminator boundaries separate.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Classification proof:

```sh
git diff --check
```

Result: passed.
