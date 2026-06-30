Status: Active
Source Idea Path: ideas/open/431_prepared_aggregate_abi_contract_review.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Prepared ABI Fact Coherence

# Current Packet

## Just Finished

Completed Step 1, Audit Aggregate ABI Evidence And Choose First Review Packet.

Representative raw BIR, prepared BIR, and RV64 object-route probes were recorded
under `build/agent_state/431_step1_aggregate_abi_audit/` for:

- `src/pr88904.c`
- `src/20000917-1.c`
- `src/20010224-1.c`
- `src/20020206-1.c`
- `src/20020506-1.c`

Classification summary:

- `src/pr88904.c`: real aggregate `sret(size=8, align=4)` return for `foo`;
  prepared call facts expose `memory_return`, `memory_home=frame_slot`, and
  `sret_arg=0`, but prepared object metadata reports the `sret_param` object as
  `size=8 align=8`. Treat as a producer-contract size/alignment/object-home
  review point before any RV64 lowering. The row also contains separate inline
  asm unsupported facts and store-publication residuals.
- `src/20000917-1.c`: coherent aggregate `sret(size=12, align=4)` returns for
  `one` and `zero`, with frame-slot memory returns and field copies at offsets
  0, 4, and 8. This looks like a later RV64 aggregate-lowering follow-up
  candidate after producer baseline review.
- `src/20010224-1.c`: suspicious producer fact: scalar call
  `ba_compute_psd(i16 byval(size=14, align=8462384025005154658) 0)` with
  immediate call source. This is not aggregate storage and should be reviewed as
  bogus scalar `byval` metadata. The RV64 object route currently passes despite
  the suspicious marker.
- `src/20020206-1.c`: coherent aggregate `sret(size=12, align=4)` return for
  `bar`, with frame-slot memory return and field copies at offsets 0, 4, and 8.
  This looks like a later RV64 aggregate-lowering follow-up candidate after
  producer baseline review.
- `src/20020506-1.c`: suspicious producer facts: scalar `i16` calls to `test3`
  and `test4` carry bogus `sret(size=5, align=...)` markers while the prepared
  call sources are register scalar values. This is producer-contract review
  scope, not aggregate lowering.

Step artifacts:

- `build/agent_state/431_step1_aggregate_abi_audit/probe_summary.tsv`
- `build/agent_state/431_step1_aggregate_abi_audit/classification.tsv`
- `build/agent_state/431_step1_aggregate_abi_audit/evidence.md`
- per-row `.bir.out`, `.prepared.out`, `.object.err`, and related probe files

## Suggested Next

Execute Step 2: Classify Prepared ABI Fact Coherence as a producer-contract
review packet focused first on bogus scalar ABI attributes, using
`src/20010224-1.c` and `src/20020506-1.c` as representative evidence.

Expected Step 2 output:

- confirm where scalar `byval`/`sret` attributes are introduced or preserved in
  the prepared ABI producer path;
- add a focused fail-closed contract/verifier test only if the existing contract
  surface can express the bogus scalar ABI metadata precisely;
- otherwise record the exact producer gap and keep coherent `sret` aggregate
  rows parked for a later RV64 aggregate-lowering source idea.

Primary files for the next packet if tests or source inspection are needed:

- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/bir/lir_to_bir/call_abi.cpp`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/prepared_contract_verifier.cpp`
- `tests/backend/bir/backend_prealloc_prepared_contract_verifier_test.cpp`
- `tests/backend/bir/backend_prealloc_call_boundary_classification_test.cpp`
- `todo.md`
- `test_after.log` if the packet changes tests

## Watchouts

- Do not implement RV64 aggregate lowering in this review plan.
- Do not treat aggregate rows as scalar call publication.
- Do not infer missing aggregate size/alignment/storage/return slot/byval copy
  facts in RV64.
- Keep coherent aggregate `sret` rows (`20000917-1`, `20020206-1`) separate
  from suspicious scalar ABI metadata rows (`20010224-1`, `20020506-1`).
- `pr88904` has both real `sret` facts and separate inline-asm/store
  publication residuals; do not collapse those into aggregate ABI lowering.
- Keep F128, long-double, runtime-helper policy, expectation changes, and
  pass/fail accounting out of this plan.

## Proof

Step 1 audit/classification proof passed:

```sh
git diff --check
```
