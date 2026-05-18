Status: Active
Source Idea Path: ideas/open/279_aarch64_c_testsuite_llvm_stack_helper_symbols.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Implement General Helper Handling

# Current Packet

## Just Finished

Step 4 of `plan.md` implemented general AArch64 dynamic-stack helper handling
in `src/backend/mir/aarch64/codegen/dispatch.cpp`.

The AArch64 block dispatcher now recognizes the retained dynamic-stack helper
family before ordinary prepared call lowering:

- `llvm.stacksave`
- `llvm.dynamic_alloca.*`
- `llvm.stackrestore`

For matching helper calls, dispatch consumes the prepared
`PreparedDynamicStackPlanFunction` operation identity by block/instruction/kind
and emits a deferred-unsupported AArch64 frame machine node with a dynamic-stack
lowering diagnostic. This makes the machine printer fail closed before
assembly/link output instead of letting ordinary extern direct-call lowering
emit unresolved helper calls or silently dropping dynamic-stack semantics. If a
helper spelling appears without matching prepared dynamic-stack operation
authority, dispatch fails closed with a missing authority diagnostic.

The focused backend proof now requires this fail-closed machine-node path; a
side-channel diagnostic without an unprintable node is no longer accepted. The
representative `00207.c` AArch64 backend c-testsuite case now fails as
`[FRONTEND_FAIL]` with the explicit AArch64 dynamic-stack diagnostic before
assembly/link, not as `[RUNTIME_UNAVAILABLE]`.

## Suggested Next

Run Step 5: broader AArch64 backend scan with `-L aarch64_backend`, then record
whether any unresolved LLVM stack helper symbol diagnostics remain and classify
remaining failures without counting `[RUNTIME_UNAVAILABLE]` as pass evidence.

Suggested proof command:

```sh
set -o pipefail; { cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -L aarch64_backend; } 2>&1 | tee test_after.log
```

## Watchouts

- Do not count `[RUNTIME_UNAVAILABLE]` as pass evidence.
- Do not add filename-specific handling for `00207.c`.
- Do not weaken expectations, allowlists, unsupported classifications, or
  stage-specific diagnostics.
- Do not fix this by hiding `llvm.*` labels in the printer or by renaming helper
  symbols. The unresolved calls are a symptom of missing AArch64 dynamic-stack
  lowering or missing truthful rejection.
- This packet chose truthful fail-closed rejection, not full AArch64 dynamic
  stack-pointer lowering.
- The `00207.c` focused subset now fails before runtime with `[FRONTEND_FAIL]`;
  missing `C_TESTSUITE_AARCH64_BACKEND_RUNNER` is not involved in this focused
  proof result.
- The old unresolved-helper spellings were absent from `test_after.log`; stale
  `00207.c.s`/`.bin` artifacts from the earlier runtime-unavailable run were
  removed after this proof so they are not mistaken for successful fresh output.
- Keep completed call-boundary move and `.LBB...` label fixes out of scope
  unless their exact old diagnostics return.

## Proof

Delegated proof command:

```sh
set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' && cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_00207_c$'; } 2>&1 | tee test_after.log
```

Result:

- default build passed
- `^backend_`: 139 selected, 139 passed
- focused backend proof
  `dynamic_stack_helper_calls_do_not_reach_machine_output_unresolved` passed
- AArch64 scan configure/build passed
- focused `c_testsuite_aarch64_backend_src_00207_c` selected 1 test and failed
  as `[FRONTEND_FAIL]`
- failure diagnostic:
  `deferred_unsupported: AArch64 dynamic-stack helper lowering is not implemented; prepared dynamic-stack operations are rejected before unresolved helper calls reach machine output`
- old unresolved helper symbol references were absent from `test_after.log`:
  `llvm.stacksave`, `llvm.dynamic_alloca.i8`, and `llvm.stackrestore`

The delegated proof exits nonzero because the focused c-testsuite route now
truthfully fails closed before assembly/link output. `test_after.log` is the
preserved proof log.
