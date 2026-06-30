Status: Active
Source Idea Path: ideas/open/431_prepared_aggregate_abi_contract_review.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Produce Follow-Up Disposition

# Current Packet

## Just Finished

Completed Step 2, Classify Prepared ABI Fact Coherence.

The suspicious rows from Step 1 were confirmed as scalar ABI metadata defects,
not coherent aggregate `sret`/`byval` lowering inputs:

- `src/20010224-1.c` raw and prepared BIR contain
  `bir.call void ba_compute_psd(i16 byval(size=14, align=8462384025005154658) 0)`,
  but the prepared call plan consumes the argument as scalar GPR immediate
  `source_encoding=immediate source_literal=0`.
- `src/20020506-1.c` raw and prepared BIR contain scalar `i16` calls to
  `test3`/`test4` with bogus `sret(size=5, align=...)` suffixes, while the
  prepared call plans consume those arguments as scalar GPR frame-slot values.

Producer/verifier boundary:

- `src/backend/bir/bir_printer.cpp` renders `sret(...)` and `byval(...)`
  suffixes from `bir::CallInst::arg_abi`, so the suspicious BIR text is still a
  producer-visible contract issue.
- `src/backend/prealloc/legalize.cpp` already repairs scalar `i16`
  `CallArgAbiInfo` when it sees `byval_copy`, `sret_pointer`, stack passing, or
  a non-integer class.
- `src/backend/prealloc/regalloc/call_return_abi.cpp` also repairs scalar `i16`
  call-argument ABI during call-argument ABI resolution.
- Existing owned prepared verifier tests classify prepared call-boundary moves
  and source-route facts after ABI planning. They do not currently expose a
  precise raw `bir::CallInst::arg_abi` scalar-vs-aggregate coherence contract,
  so no focused verifier test was added in this packet.

Coherent aggregate rows remain parked:

- `src/20000917-1.c` and `src/20020206-1.c` have coherent
  `sret(size=12, align=4)` aggregate returns with `memory_return`,
  `memory_home=frame_slot`, `sret_arg=0`, and field copies at offsets 0, 4,
  and 8.
- `src/pr88904.c` has real `sret(size=8, align=4)` facts but still needs
  producer review because the `sret_param` object metadata reports
  `size=8 align=8` while the call memory record reports
  `memory_size=8 memory_align=4`; the row also has unrelated inline-asm and
  store-publication residuals.

Step artifacts:

- `build/agent_state/431_step2_abi_fact_coherence/classification.md`
- `build/agent_state/431_step2_abi_fact_coherence/proof.txt`

## Suggested Next

Execute Step 3: Produce Follow-Up Disposition.

Recommended Step 3 output:

- create or name a producer follow-up for raw `bir::CallInst::arg_abi`
  coherence on scalar call arguments carrying bogus aggregate flags;
- keep coherent `sret` rows (`20000917-1`, `20020206-1`) as later RV64
  aggregate-lowering candidates only after producer contracts are clean;
- keep `pr88904` in producer review until its `sret` object
  size/alignment/home facts are reconciled.

Primary files for a later producer repair packet:

- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/prealloc/legalize.cpp`
- `src/backend/prealloc/prepared_contract_verifier.cpp`
- `tests/backend/bir/backend_prealloc_prepared_contract_verifier_test.cpp`
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
- Do not add a verifier test that only validates the already-repaired prepared
  call move; the missing contract is raw call ABI metadata coherence.
- Keep F128, long-double, runtime-helper policy, expectation changes, and
  pass/fail accounting out of this plan.

## Proof

Step 2 classification-only proof passed:

```sh
git diff --check
```

No tests were added or run because the existing owned verifier APIs cannot
precisely express raw `bir::CallInst::arg_abi` scalar-vs-aggregate coherence;
prepared call-boundary move contracts already see repaired scalar GPR facts.
