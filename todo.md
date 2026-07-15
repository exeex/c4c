# Current Packet

Status: Active
Source Idea Path: ideas/open/798_lir_operand_provenance_authority_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace the concrete provenance-loss boundary

## Just Finished

- Lifecycle switch from 754: its accepted Step 1 audit is preserved in the
  parent resumption record; no blocker implementation packet has run.

## Suggested Next

- Step 1 only: trace real HIR `extractvalue` SSA aggregate flow through the
  surrounding expression APIs to the LIR operand construction boundary and
  select the smallest opt-in provenance carrier.

## Watchouts

- Do not add `LirExtractValueOp` result/use fields or recover an ID from a
  `std::string`; 754 resumes at its unchanged Step 2 only after this handoff.

## Proof

- No implementation proof yet. Preserve 754 references `d8e5ed3a8` and root
  `test_before.log` / `test_after.log` (5/5) as parent audit/prototype evidence
  only; the prototype was rejected and reverted.
