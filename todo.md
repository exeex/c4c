Status: Active
Source Idea Path: ideas/open/436_pr88904_sret_object_home_producer_review.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Produce Follow-Up Eligibility Disposition

# Current Packet

## Just Finished

Completed Step 2: added focused producer-contract coverage for the coherent
`pr88904` aggregate `sret` interpretation.

Coverage added in
`tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`:

- `make_same_module_sret_object_home_contract_module` builds a semantic
  same-module RV64 aggregate `sret` fixture with raw payload ABI
  `size=8 align=4`.
- `check_same_module_sret_object_home_contract` proves the callee hidden
  `sret` parameter object home is pointer-typed/pointer-aligned:
  `type=ptr size=8 align=8`, address-exposed, home-slot required, and
  permanent.
- The same check proves the caller memory-return payload remains aggregate ABI
  `size=8 align=4` and is bound to the caller frame-slot-backed sret storage.
- The aggregate-address argument source selection must identify the caller
  payload frame slot while carrying pointer-sized address facts.
- No producer implementation changes were required.

Artifacts:

- `build/agent_state/436_step2_pr88904_contract_coverage/summary.txt`

## Suggested Next

Execute Step 3: Produce Follow-Up Eligibility Disposition.

First bounded packet:

- Record whether `pr88904` is eligible for later coherent RV64 aggregate
  `sret` lowering now that the object-home alignment contract is covered.
- Keep inline-asm `=*imr`/`*imr`, missing destination access/store-publication,
  stack-slot block-entry publication, and generic target-lowering residuals
  routed outside this producer-review idea.
- Decide whether this source idea is ready for close-readiness review after
  the disposition is recorded.

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

Step 2 focused pre-proof passed:

```sh
cmake --build build -j2 --target backend_prepare_frame_stack_call_contract_test && ctest --test-dir build -j2 --output-on-failure -R '^backend_prepare_frame_stack_call_contract$'
```

Step 2 delegated backend proof passed and is captured in `test_after.log`:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```
