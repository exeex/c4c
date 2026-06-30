Status: Active
Source Idea Path: ideas/open/436_pr88904_sret_object_home_producer_review.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Or Confirm Producer Contract Coverage

# Current Packet

## Just Finished

Completed Step 1: audited `pr88904` prepared aggregate `sret` object-home and
call memory-return facts.

Classification: coherent, with a focused producer-contract coverage gap.

- The apparent mismatch is not one object carrying contradictory alignment:
  `object #5 func=foo name=%ret.sret source_kind=sret_param type=ptr size=8
  align=8` describes callee-side pointer storage for the hidden `sret`
  parameter.
- The call record `memory_return=%t0 memory_home=frame_slot ... memory_size=8
  memory_align=4` describes the caller-side aggregate payload passed by
  address, matching raw BIR `sret(size=8, align=4)`.
- Object-home ownership is separated: `%ret.sret` is owned by callee `foo`
  slot `#3` as pointer storage, while `%t0` is the caller memory-return payload
  addressed by the aggregate-address call argument and copied through 4-byte
  lanes at offsets `0` and `4`.
- Refreshed probes reproduced the inherited evidence: BIR and prepared dumps
  pass; object route still fails with a generic `unsupported_instruction_fragment`.
- Unrelated residuals stay out of scope: inline-asm `=*imr`/`*imr` carriers,
  missing destination access/store-publication facts, stack-slot block-entry
  publication residuals, generic RV64 object lowering gaps, and target
  aggregate lowering.

Artifacts:

- `build/agent_state/436_step1_pr88904_sret_fact_audit/classification.md`
- `build/agent_state/436_step1_pr88904_sret_fact_audit/extracted_evidence.md`
- `build/agent_state/436_step1_pr88904_sret_fact_audit/probe_status.txt`

## Suggested Next

Execute Step 2: Add Or Confirm Producer Contract Coverage.

First bounded packet:

- Add or confirm focused producer-contract coverage for the coherent
  interpretation: hidden `sret` parameter object homes are pointer-sized and
  pointer-aligned, while caller memory-return payload facts keep aggregate ABI
  size/alignment.
- Primary candidate files:
  `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp` and
  `tests/backend/bir/backend_prealloc_prepared_contract_verifier_test.cpp`.
- Touch producer/verifier implementation only if a focused test exposes a real
  contract gap.
- Keep the test semantic; do not key behavior to `pr88904`, function names, or
  dump line numbers.

## Watchouts

- Do not implement RV64 aggregate lowering for `pr88904` in this plan.
- Do not reinterpret `object #5` as the aggregate payload object; it is the
  hidden pointer parameter object.
- Do not fold inline-asm, clobber, store-publication, select, or
  local-publication residuals into the `sret` producer review.
- Do not infer object layout, alignment, or home ownership in RV64 from source
  or testcase identity.
- Keep expectation, allowlist, unsupported-marker, runtime-comparison, and
  pass/fail accounting changes out of scope.

## Proof

Step 1 delegated backend proof passed and is captured in `test_after.log`:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```
