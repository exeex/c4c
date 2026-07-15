# Current Packet

Status: Active
Source Idea Path: ideas/open/782_lir_vaarg_phi_input_result_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Diagnose and repair the variadic baseline regression

## Just Finished

- The focused frontend-LIR proof passed 1/1 for the Step 1 implementation
  commits `97d305332` and `4830e94a5`, but baseline review rejected the slice:
  `test_baseline.new.log` has 20/3037 failures versus 0/3037 in
  `test_baseline.log`. No new 782 capability claim is accepted.

## Suggested Next

- Diagnose the common mechanism behind the exact 20-test variadic/ABI/stdarg
  family introduced by `97d305332` and `4830e94a5`, then make the smallest
  producer-only repair. Prove either a matching full-suite 0/3037 candidate or
  a credible command that covers the complete named failure family, followed
  by the required full-suite baseline recheck before acceptance.

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

## Proof

- Rejected baseline evidence: `test_baseline.log` is 0/3037 failures;
  `test_baseline.new.log` at `4830e94a5` is 20/3037 failures. The focused
  command `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_call_type_ref$'` passed 1/1, but is not
  sufficient for acceptance. Step 2 requires the matching full-suite candidate
  or the provisional complete-family targeted proof plus later full-suite
  baseline recheck stated in `plan.md`.
