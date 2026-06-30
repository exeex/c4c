Status: Active
Source Idea Path: ideas/open/421_rv64_instruction_fragment_bucket_classification.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Validate And Prepare Lifecycle Close Decision

# Current Packet

## Just Finished

Step 5 validated and summarized the RV64 instruction-fragment classification
runbook for supervisor close review. The completed classification used the
fresh matching Step 2 scan (`total=1467 passed=404 failed=1063`) and split 190
`unsupported_instruction_fragment` rows into these disjoint first-owner buckets:

- select and join materialization: 54
- call-adjacent scalar publication and inline-asm materialization: 38
- pointer cast and address materialization: 21
- aggregate `sret`/`byval` call-storage: 19
- integer div/rem lowering: 17
- F128 quarantine: 16
- integer arithmetic shift right: 11
- large literal and materialization: 10
- global memory/addressing residual: 2
- scalar F32/F64 conversion/op residual: 2

Step 4 created durable open follow-up ideas for the highest-priority
evidence-backed candidates:

- `ideas/open/427_rv64_scalar_select_join_materialization.md`
- `ideas/open/428_rv64_call_adjacent_scalar_inline_asm_materialization.md`
- `ideas/open/429_rv64_pointer_address_materialization.md`
- `ideas/open/430_rv64_integer_div_rem_lowering.md`
- `ideas/open/431_prepared_aggregate_abi_contract_review.md`

F128 remains explicitly quarantined and lowest priority for this runbook. It
should not displace ordinary non-F128 implementation work unless future
evidence proves F128 blocks a broader non-F128 owner.

No unresolved row-level evidence gap blocks close review. Remaining route
boundaries are captured in the follow-up ideas: pointer/address work must reject
incomplete provenance, and aggregate `sret`/`byval` work must validate prepared
ABI producer facts before RV64 aggregate lowering.

## Suggested Next

Supervisor close review: decide whether
`ideas/open/421_rv64_instruction_fragment_bucket_classification.md` is complete
and should be closed, or whether another classification-only runbook is needed.

## Watchouts

- Keep F128 quarantined and lowest priority unless future evidence proves it
  blocks a broader ordinary non-F128 owner.
- Keep producer-owned work separate: aggregate `sret`/`byval` review lives in
  `ideas/open/431_prepared_aggregate_abi_contract_review.md`, and pointer rows
  with incomplete provenance must remain outside
  `ideas/open/429_rv64_pointer_address_materialization.md`.
- This was a lifecycle/docs classification runbook; no RV64 lowering,
  expectations, allowlists, pass/fail accounting, or implementation files were
  changed.

## Proof

Delegated proof: `git diff --check > test_after.log 2>&1`.

Result: passed. `test_after.log` exists and is the proof log for this Step 5
packet.
