Status: Active
Source Idea Path: ideas/open/434_bir_call_arg_abi_scalar_metadata_coherence.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Fail-Closed Scalar ABI Coherence Coverage

# Current Packet

## Just Finished

Completed Step 1: audited the scalar call-argument ABI producer and contract
surface.

Exact producer surface:

- `src/backend/bir/lir_to_bir/calling.cpp` builds `lowered_arg_abi` during
  typed-call lowering and assigns it to `bir::CallInst::arg_abi`.
- Scalar typed-call arguments should flow through `compute_call_arg_abi` in
  `src/backend/bir/lir_to_bir/call_abi.cpp`.
- Aggregate-only flags are introduced by the byval/sret construction paths and
  are coherent only for pointer aggregate carriers, not scalar `i16` values.
- `src/backend/bir/bir_printer.cpp` renders `sret(...)`/`byval(...)` directly
  from `CallInst::arg_abi`, so incoherent raw metadata is visible in raw and
  prepared BIR before RV64 object lowering.

Existing later repair is not the producer contract: `src/backend/prealloc/legalize.cpp`
and `src/backend/prealloc/regalloc/call_return_abi.cpp` repair scalar `i16`
call-argument movement facts, while
`src/backend/prealloc/prepared_contract_verifier.cpp` has prepared movement/home
contracts but no raw `CallInst::arg_abi` scalar-vs-aggregate coherence
classifier.

Representative proof inputs are persisted under
`build/agent_state/434_step1_scalar_arg_abi_audit/`:

- `src/20010224-1.c`: raw/prepared BIR renders scalar `i16
  byval(size=14, align=8462384025005154658) 0`; prepared call planning routes
  arg 0 as scalar GPR immediate `0` to `a0`.
- `src/20020506-1.c`: raw/prepared BIR renders scalar `i16 sret(size=5,
  align=...)` for `test3`/`test4` args; prepared call planning routes arg 0 as
  scalar GPR frame-slot/register/immediate facts to `a0`.

## Suggested Next

Execute Step 2: add fail-closed scalar ABI coherence coverage/classification.
First implementation target: introduce a producer-owned raw call-argument ABI
coherence classifier/verifier that rejects scalar call arguments carrying
`sret_pointer` or `byval_copy`, while preserving coherent pointer aggregate
`sret`/`byval` metadata.

Primary files for the next packet:

- `src/backend/prealloc/prepared_contract_verifier.hpp`
- `src/backend/prealloc/prepared_contract_verifier.cpp`
- focused backend/BIR contract tests such as
  `tests/backend/bir/backend_prealloc_prepared_contract_verifier_test.cpp`

Later implementation proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1
```

## Watchouts

- Do not implement RV64 aggregate lowering in this plan.
- Do not rely on later prepared call-plan scalar repair as proof that raw BIR
  `arg_abi` is coherent.
- Do not special-case representative testcase names, function names, or exact
  bogus alignment literals.
- Preserve coherent aggregate `sret`/`byval` pointer metadata.
- Keep F128, long-double, runtime-helper, inline-asm, select, store-publication,
  expectations, allowlists, unsupported markers, runtime comparison, and
  pass/fail accounting out of this packet.

## Proof

Step 1 audit-only slice. Required local check passed:

```sh
git diff --check
```
