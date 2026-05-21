Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair General Call-Boundary Preparation

# Current Packet

## Just Finished

Step 3 repaired the localized AArch64 call-boundary preparation owner.

The repair keeps prepared frame-address/byval publication ahead of preserved
callee-saved reuse, so indirect byval/register arguments are rematerialized
from their frame homes instead of forwarding stale preserved registers. This
advances `00140`: the focused proof now passes that CTest representative.

Structured f128 call arguments now keep q-register authority even when the
source value also has an FPR/callee-saved home. The call-boundary lowering
finds the prepared f128 carrier by value id when the function-local lookup is
insufficient, and it bypasses the ordinary preserved-source shortcut for
structured f128 register arguments. This removes the previous `00204`
frontend/printer stop at `call-boundary move node requires prepared GPR
registers, scalar FPR registers, or structured f128 q-register authority`.

The focused `backend_aarch64_call_boundary_owner` test now passes. The
supervisor-side `backend_aarch64_instruction_dispatch` follow-up also passes
after updating its byval entry-publication expectations to the semantically
equivalent full-lane store form (`str xN`) introduced by this repair. The
byval callee fixture was narrowed from 32 bytes to 16 bytes because true
AArch64 GPR byval lanes cover up to two eight-byte lanes; the 32-byte path is
the indirect byval pointer representative covered by `00140`.

## Suggested Next

Execute the next packet after supervisor review of the Step 3 residual:
`00204` now reaches runtime and fails with segmentation fault / corrupted HFA
long-double output instead of failing in the call-boundary machine printer.

## Watchouts

Do not treat the remaining `00204` failure as the old call-boundary authority
stop: assembly generation now succeeds, and the current first bad fact is
runtime corruption/nonzero in the HFA long-double/stdarg region.

No expectation, unsupported classification, runner, or CTest contract was
changed. The focused byval lane fixture now uses 16 bytes to preserve the true
GPR-lane contract; 32-byte byval is an indirect pointer path and should stay
represented by `00140`. The `backend_aarch64_instruction_dispatch`
expectation adjustments preserve the existing contract while accepting the
stronger whole-lane byval entry stores.

## Proof

Ran the corrected Step 3 follow-up proof:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_call_boundary_owner|backend_.*call.*boundary.*|backend_.*byval.*|backend_.*f128.*|c_testsuite_aarch64_backend_src_00140_c|c_testsuite_aarch64_backend_src_00204_c)$' > test_after.log 2>&1
```

Result: build succeeded. The proof subset is 7/8 green:
`backend_aarch64_instruction_dispatch`, `backend_aarch64_call_boundary_owner`,
older selected byval/backend focused tests, and
`c_testsuite_aarch64_backend_src_00140_c` pass. The only remaining failure is
`c_testsuite_aarch64_backend_src_00204_c`, which remains the known advanced
`RUNTIME_NONZERO` / segmentation fault with corrupted HFA long-double/stdarg
output. `test_after.log` is the preserved proof log.
