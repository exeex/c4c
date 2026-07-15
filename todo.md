# Current Packet

Status: Active
Source Idea Path: ideas/open/782_lir_vaarg_phi_input_result_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Diagnose and repair the variadic baseline regression

## Just Finished

- Step 2 repaired the exact 20-test variadic baseline family: AArch64 FP
  vaarg stack alignment publishes an authoritative `inttoptr` result, but the
  selected native-result cast verifier accepted only scalar integer casts.
  `verify_cast_op_authority` now validates the `inttoptr` integer-to-pointer
  endpoints and exact integer source width; the producer authority remains
  native and adjacent to the defining cast.

## Suggested Next

- Supervisor-owned full-suite baseline recheck against the 0/3037 candidate;
  keep Step 2 pending until that matching comparison accepts the repair.

## Watchouts

- Treat 783's accepted source-to-immediate-consumer contracts as upstream
  authority, not as a 782 helper-field or PHI-completion claim.
- `LirVaArgOp.result` is a later result and does not identify helper PHI inputs.
- AMD64 overflow authority belongs to the final non-pointer result load, not
  the pointer load used as the memcpy source.
- Do not change `LirPhiOp`, PHI verification, predecessor/edge authority, CFG,
  Raw-BIR/importer, backend, target lowering, MIR, or emission.
- Do not recover IDs from names, labels, rendered text, instruction order, or
  testcase text; do not introduce side tables or result-name maps.
- The FP alignment route uses the existing cast opt-in for the <=8-byte path;
  the new producer opt-ins remain restricted to GEP/load/call.
- The rejected family is four positive/ABI variadic tests, four named
  `llvm_gcc_c_torture` cases, `stdarg_{1,2,3,4}`, and
  `va_arg_{12,15,16,17,26,5,6,trap_1}`. Do not hide it with named-case logic,
  expectation downgrades, or weaker malformed-authority checks.
- Do not start Step 3/handoff from this packet; this repair only unblocks the
  required Step 2 full-suite baseline recheck.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^(positive_sema_ok_call_variadic_aggregate_runtime_c|positive_sema_ok_fn_returns_variadic_fn_ptr_c|abi_abi_variadic_forward_wrapper_c|abi_abi_variadic_va_copy_accumulate_c|llvm_gcc_c_torture_src_(pr56205|980205|pr44942|pr64979|stdarg_[1234]|va_arg_(12|15|16|17|26|5|6|trap_1))_c)$'`
  passed 20/20 after a fresh build. Per packet scope, no root test log was
  written; the supervisor owns the required full-suite baseline logs.
