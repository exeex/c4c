Status: Active
Source Idea Path: ideas/open/337_aarch64_callee_saved_scalar_home_preservation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Preservation Owner

# Current Packet

## Just Finished

Step 2 repaired the AArch64 prepared-to-codegen scalar callee-saved placement
owner. `gpr:callee_saved#0/w1` and `gpr:callee_saved#1/w1` now resolve through
the prepared AArch64 callee-saved scalar profile used by regalloc/frame
planning, so operand emission maps them to `w20/x20` and `x21` instead of
indexing the full ABI callee-saved array from `x19`.

The focused conversion test now pins the handoff: saved-register frame records
with `gpr:callee_saved#0/w1` keep the published frame register `x20`, direct
placement conversion maps slot 0 to `w20`, and assignment conversion still
prefers structured placement over stale spelling. The `00168` AArch64
c-testsuite case now passes in the delegated proof.

## Suggested Next

Supervisor should review/commit this Step 2 slice, then decide whether the
active plan needs a Step 3 closure packet or a broader validation packet around
AArch64 prepared scalar homes.

## Watchouts

The repair is intentionally scoped to scalar GPR/AggregateAddress callee-saved
prepared placements. Caller-saved prepared slots and FP/vector callee-saved
placement policy were left unchanged because the delegated residual was the
live scalar-home mismatch. Saved-register records now convert from their
explicit frame register name, while physical value assignments still prefer
structured placement.

## Proof

`cmake --build build --target c4cll backend_aarch64_prepared_register_conversion_test backend_aarch64_return_lowering_test backend_prepare_frame_stack_call_contract_test backend_prepare_liveness_test -j 2 && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_(prepared_register_conversion|return_lowering)|backend_prepare_(frame_stack_call_contract|liveness)|c_testsuite_aarch64_backend_src_00168_c' > test_after.log 2>&1`
passed. `git diff --check` also passed. Proof log: `test_after.log`.

Supervisor follow-up validation:
`ctest --test-dir build -j --output-on-failure -R '^backend_'` passed 141/141.
