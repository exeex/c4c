Status: Active
Source Idea Path: ideas/open/434_bir_call_arg_abi_scalar_metadata_coherence.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Raw Call Argument ABI Production

# Current Packet

## Just Finished

Completed Step 2: added fail-closed raw scalar call-argument ABI coherence
coverage/classification.

Implementation summary:

- Added `RawCallArgumentAbiMetadata` as a producer-owned contract fact family.
- Added `PreparedRawCallArgumentAbiCoherenceContractStatus` plus
  `classify_prepared_raw_call_argument_abi_coherence_contract` and
  `verify_prepared_raw_call_argument_abi_coherence_contract`.
- The new classifier fails closed when raw `bir::CallInst::arg_abi` is missing
  or when a scalar non-pointer argument carries aggregate-only `sret_pointer` or
  `byval_copy`.
- Focused verifier tests cover scalar bad `sret`, scalar bad `byval`, coherent
  scalar metadata, coherent pointer aggregate `sret`/`byval` metadata, missing
  ABI metadata, status spelling, and fact-family spelling.
- Lookup/build artifacts for this packet are under
  `build/agent_state/434_step2_scalar_abi_coherence_coverage/`.

## Suggested Next

Execute Step 3: repair raw call argument ABI production so scalar call
arguments no longer publish aggregate-only `sret_pointer` or `byval_copy`
metadata.

Primary files for the next packet:

- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/bir/lir_to_bir/call_abi.cpp` if the scalar ABI producer helper
  itself needs narrowing
- focused BIR lowering/printing tests that prove scalar calls no longer render
  bogus aggregate suffixes

Recommended proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Do not implement RV64 aggregate lowering in this plan.
- Do not rely on later prepared call-plan scalar repair as proof that raw BIR
  `arg_abi` is coherent.
- Do not special-case representative testcase names, function names, or exact
  bogus alignment literals.
- Preserve coherent aggregate `sret`/`byval` pointer metadata.
- Step 2 intentionally added fail-closed classification only; it does not repair
  the raw BIR producer yet.
- Keep F128, long-double, runtime-helper, inline-asm, select, store-publication,
  expectations, allowlists, unsupported markers, runtime comparison, and
  pass/fail accounting out of this packet.

## Proof

Focused pre-proof:

```sh
ctest --test-dir build -j2 --output-on-failure -R '^backend_prealloc_prepared_contract_verifier$'
```

Delegated proof passed:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Proof log: `test_after.log`.
