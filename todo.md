Status: Active
Source Idea Path: ideas/open/345_aarch64_scalar_select_result_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Scalar Select Result Publication Boundary

# Current Packet

## Just Finished

Lifecycle activation created this execution state for `plan.md` Step 1.

## Suggested Next

Delegate Step 1 to an executor: localize the AArch64 scalar select result
publication boundary for `c_testsuite_aarch64_backend_src_00143_c` and record
the first bad owner here.

## Watchouts

- Do not special-case `00143`, `%t9.store0`, `%t13.store0`, `%lv.a.0`, one
  stack offset, one block number, one source line, or one emitted instruction
  string.
- Do not weaken expectations, unsupported classifications, runner behavior,
  timeout policy, proof-log policy, or CTest registration.
- Preserve the idea 344 pointer-local dereference repair and the Duff emission
  repairs from ideas 343 and 342.

## Proof

Lifecycle-only activation; no build or test proof required.
