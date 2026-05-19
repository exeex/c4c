Status: Active
Source Idea Path: ideas/open/301_aarch64_memory_store_operand_materialization.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Focused closure and broader backend-regex proof

# Current Packet

## Just Finished

Step 4 reran the focused memory-store owner closure proof and the broader
backend regex inventory after the Step 2 and Step 3 repairs. The focused
subset is stable at 2/8 passing: `00194` and `00213` pass; `00173`, `00181`,
and `00214` fail with runtime nonzero/segfault behavior; `00176` fails with a
runtime mismatch; `00182` fails in backend assembly on non-encodable
`mov x0, #1234567`; and `00187` times out.

No focused case still fails with the old memory-store operand printer
diagnostics, and the broader `backend` regex log contains no `memory store`,
`store source`, `store value is not`, `symbol store value is not`, or `not a
register or immediate operand` diagnostic.

The broad backend regex run reports 300/352 passing and 52/352 failing. The
residual buckets are: 26 `RUNTIME_NONZERO`, 15 `RUNTIME_MISMATCH`, 6
`FRONTEND_FAIL`, 2 `BACKEND_FAIL`, and 3 timeouts. The non-runtime residuals
are outside the old memory-store printer mode: scalar operand-shape printer
failures (`00064`, `00139`), call-boundary deferred unsupported (`00140`),
unsigned reduction printer failure (`00205`), `lir_to_bir` admission failures
(`00204`, `00216`), backend assembler operand failures (`00104`, `00182`), and
timeouts (`00143`, `00187`, `00220`).

## Suggested Next

From the validation evidence, idea 301 can close if its intended scope is the
memory-store operand materialization/printer diagnostics. The remaining
focused and broad failures should be owned by fresh packets for runtime
incorrectness/segfaults, timeout behavior, non-encodable immediate materialization,
scalar operand-shape printing, call-boundary moves, unsigned reductions, or
`lir_to_bir` admission rather than extending this owner.

## Watchouts

- No stale qemu, c-testsuite, ctest, or related runtime processes were found
  before or after the broad backend run; nothing was terminated.
- The focused subset still fails overall, but those failures no longer carry
  the old memory-store operand printer diagnostics. Do not treat those residual
  failures as Step 2/Step 3 memory-store blockers without fresh generated-code
  evidence tying them back to store source/value materialization.
- Do not change expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, CTest registration, or implementation files from
  this closure packet.

## Proof

Ran:

```bash
cmake --build --preset default
ctest --test-dir build -j8 -R '^c_testsuite_aarch64_backend_src_(00173|00176|00181|00182|00187|00194|00213|00214)_c$' --output-on-failure
timeout 900s ctest --test-dir build -j10 -R backend --output-on-failure > test_after.log 2>&1
```

Result: build succeeded. Focused CTest failed 6/8 with the residual buckets
listed above. Broad backend regex CTest failed 52/352 and preserved its full
output in `test_after.log`; it did not hit the 900s timeout wrapper. The proof
is sufficient for closure classification of the memory-store owner because the
old memory-store operand diagnostics are absent in both the focused outcome and
the broad backend log, while remaining failures classify into separate runtime,
assembler, timeout, scalar-printer, call-boundary, unsigned-reduction, and
`lir_to_bir` buckets.
