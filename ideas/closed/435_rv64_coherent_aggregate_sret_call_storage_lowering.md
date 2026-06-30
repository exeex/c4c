# RV64 Coherent Aggregate SRet Call Storage Lowering

Status: Closed
Type: Target lowering idea
Parent: `ideas/closed/431_prepared_aggregate_abi_contract_review.md`
Source Evidence:
- `build/agent_state/431_step1_aggregate_abi_audit/evidence.md`
- `build/agent_state/431_step1_aggregate_abi_audit/classification.tsv`
- `build/agent_state/431_step2_abi_fact_coherence/classification.md`
Owning Layer: RV64 aggregate call-storage lowering after producer contracts are clean
Closed: Step 4 residual disposition and close-readiness review

## Goal

Lower coherent RV64 aggregate `sret` call-storage facts using prepared ABI
metadata, starting from representatives with consistent `sret(size=12,
align=4)` memory-return records.

## Why This Exists

The prepared aggregate ABI review parked `src/20000917-1.c` and
`src/20020206-1.c` as coherent aggregate `sret` candidates. Both rows carry
prepared callsites with `memory_return`, `memory_home=frame_slot`,
`sret_arg=0`, and field-copy facts at offsets `0`, `4`, and `8`. They are not
scalar call publication work and should not be implemented until producer
contracts are clean.

## In Scope

- Consume prepared aggregate `sret` call-storage facts for coherent rows only.
- Lower caller-side frame-slot memory-return setup and post-call aggregate
  value materialization for RV64 object emission.
- Preserve field-copy and memory-return authority from prepared facts rather
  than reconstructing aggregate layout in the target.
- Use `src/20000917-1.c` and `src/20020206-1.c` as representatives after
  producer ABI contracts are accepted.
- Keep any remaining select, local-publication, or pointer residuals as
  separate disposition if they are exposed after aggregate lowering.

## Out Of Scope

- Repairing raw BIR scalar call ABI metadata defects.
- Reviewing or consuming `src/pr88904.c` until its size/alignment/object-home
  producer facts are reconciled.
- Inferring aggregate size, alignment, field offsets, return slots, or byval
  copy homes from source or raw BIR in RV64.
- Scalar call publication, inline-asm, select lowering, F128, long-double,
  runtime-helper policy, or expectation/pass-fail accounting work.

## Acceptance Criteria

- Coherent prepared `sret` aggregate call-storage rows are lowered from
  prepared memory-return facts without target-side ABI guessing.
- Focused tests or probes show the aggregate `sret` unsupported fragment is
  removed or replaced by a smaller, correctly classified non-aggregate
  residual.
- `src/20000917-1.c` and `src/20020206-1.c` are used only as representatives
  of a semantic prepared-aggregate lowering rule.
- No producer-contract defect rows are admitted into this target-lowering
  scope.

## Completion Notes

Step 3 re-probed `src/20000917-1.c` and `src/20020206-1.c` with raw BIR,
prepared BIR, object, and asm routes. The coherent aggregate `sret`
memory-return facts remain complete:

- `memory_return`
- `memory_encoding=frame_slot`
- `sret_arg_index=0`
- `memory_size=12`
- `memory_align=4`
- aggregate-address arg0 selected through local frame-slot address
  materialization
- explicit prepared frame-slot field-copy accesses at offsets `0`, `4`, and
  `8`

The RV64 backend already has a prepared-fact-driven same-module `sret`
memory-return path, and focused coverage now validates the representative
12-byte / align-4 contract plus fail-closed shapes. No Step 3 implementation
change was justified for aggregate `sret` memory-return call storage.

Final residual disposition:

- `--codegen asm` resolves the first remaining representative blocker to
  unsupported runtime external `abort`, not missing aggregate `sret`
  call-storage lowering.
- Runtime external `abort`/`exit` support is split to
  `ideas/open/437_rv64_runtime_external_abort_exit_support.md`.
- `src/pr88904.c` remains excluded until
  `ideas/open/436_pr88904_sret_object_home_producer_review.md` reconciles its
  producer facts.
- Any later select, local-publication, pointer, or post-call-copy residuals
  remain outside this source idea until exposed and classified separately.

Close evidence:

- Step 3 summary:
  `build/agent_state/435_step3_sret_memory_return/summary.txt`
- Backend regression guard passed with existing canonical logs:
  `test_before.log` and `test_after.log` both reported `327 passed, 0 failed`.
- `git diff --check` passed at close.

## Reviewer Reject Signals

- Reject RV64 lowering that infers missing aggregate size, alignment, field
  offsets, return-slot, or storage-home facts from source or testcase shape.
- Reject including `src/20010224-1.c`, `src/20020506-1.c`, or `src/pr88904.c`
  as coherent target-lowering proof before their producer issues are resolved.
- Reject named-case shortcuts for `one`, `zero`, `bar`, `20000917-1`, or
  `20020206-1`.
- Reject expectation downgrades, unsupported-marker changes, or pass/fail
  accounting edits as aggregate lowering progress.
- Reject broad rewrites that merge scalar call publication, inline-asm, select,
  F128, or unrelated ABI work into this idea.
