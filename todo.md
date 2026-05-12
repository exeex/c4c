Status: Active
Source Idea Path: ideas/open/189_direct_call_no_prototype_variadic_signature_mismatch.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Locate Signature Construction And Failing Shape

# Current Packet

## Just Finished

Lifecycle switched from idea 188 to the narrow direct-call
no-prototype/variadic signature mismatch blocker. No implementation, tests, or
logs were edited by the plan owner.

## Suggested Next

Execute Step 1: locate direct-call callee signature construction and reproduce
or inspect at least one no-prototype/old-style failure and one variadic failure
under the supervisor-delegated proof command.

## Watchouts

- Idea 188 is parked, not closed. Return to its milestone gate after this
  blocker is fixed.
- The 9 known failing tests are:
  `clang_c_external_C_drs_dr206_c`,
  `llvm_gcc_c_torture_src_20010605_2_c`,
  `llvm_gcc_c_torture_src_20051012_1_c`,
  `llvm_gcc_c_torture_src_920501_1_c`,
  `llvm_gcc_c_torture_src_921202_1_c`,
  `llvm_gcc_c_torture_src_921208_2_c`,
  `llvm_gcc_c_torture_src_pr28289_c`,
  `llvm_gcc_c_torture_src_pr34982_c`, and
  `llvm_gcc_c_torture_src_va_arg_2_c`.
- Do not downgrade expectations, suppress signature verification broadly, or
  replace structured metadata with rendered callee text.

## Proof

Lifecycle-only switch. No build or test proof required for this plan-owner
operation.
