# Current Packet

Status: Active
Source Idea Path: ideas/open/782_lir_vaarg_phi_input_result_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove the bounded handoff and return to 751

## Just Finished

- Step 2 is complete and accepted at `11a45f173`: AArch64 FP vaarg stack
  alignment publishes an authoritative `inttoptr` result, and
  `verify_cast_op_authority` now validates its integer-to-pointer endpoints
  and exact integer source width. The exact named regression family passed
  20/20 after a fresh build; the matching full suite passed 3037/3037.

## Suggested Next

- Execute Step 3, `Prove the bounded handoff and return to 751`: add nearby
  positive and malformed authority coverage for the AArch64 GP, AArch64 FP,
  and AMD64 input chains, then record the accepted field names, constructors,
  proof, and commit needed to reactivate 751 at Step 1.

## Watchouts

- `LirVaArgOp.result` is a later result and does not identify helper PHI inputs.
- AMD64 overflow authority belongs to the final non-pointer result load, not
  the pointer load used as the memcpy source.
- Do not change `LirPhiOp`, PHI verification, predecessor/edge authority, CFG,
  Raw-BIR/importer, backend, target lowering, MIR, or emission.
- Do not recover IDs from names, labels, rendered text, instruction order, or
  testcase text; do not introduce side tables or result-name maps.
- The FP alignment route uses the existing cast opt-in for the <=8-byte path;
  the new producer opt-ins remain restricted to GEP/load/call.
- Do not claim PHI carrier/verifier completion; this packet publishes only the
  producer-side handoff that lets 751 resume at Step 1.

## Proof

- Accepted Step 2 proof: `cmake --build --preset default && ctest --test-dir
  build -j --output-on-failure -R
  '^(positive_sema_ok_call_variadic_aggregate_runtime_c|positive_sema_ok_fn_returns_variadic_fn_ptr_c|abi_abi_variadic_forward_wrapper_c|abi_abi_variadic_va_copy_accumulate_c|llvm_gcc_c_torture_src_(pr56205|980205|pr44942|pr64979|stdarg_[1234]|va_arg_(12|15|16|17|26|5|6|trap_1))_c)$'`
  passed 20/20 after a fresh build.
- Supervisor acceptance proof: `cmake --build --preset default && ctest
  --test-dir build -j --output-on-failure` passed 3037/3037.
