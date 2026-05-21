Status: Active
Source Idea Path: ideas/open/378_aarch64_scalar_fp_expression_constant_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Scalar FP Publication

# Current Packet

## Just Finished

Step 3 repaired the shared AArch64 scalar FP publication boundary for non-call
FP binary producers assigned to FPR storage. `emit_fp_value_to_register` now
materializes scalar FP `BinaryInst` operands recursively, including FP
immediates, and emits the matching `fadd`/`fsub`/`fmul`/`fdiv` into the
destination FPR before later consumers read it.

The dispatch path also publishes prepared FPR-backed scalar FP binary results
at the producer instruction when the prepared storage plan authorizes FPR
storage, while preserving the existing fail-closed behavior for missing FPR
storage facts. Store-local publication now covers scalar FP binary producers
and skips duplicate publication when the producer already filled the same FPR.

## Suggested Next

Execute Step 4 by proving the external `00174` representative and recording
whether it passes or advances to a new first bad fact under the repaired scalar
FP publication path.

## Watchouts

Do not special-case `00174`, one output row, one FP register, or one emitted
instruction sequence. Keep `00216`, `00200`, and `00207` parked unless fresh
evidence proves they share this scalar FP owner.

The repair intentionally stays on scalar FP literal/arithmetic publication. It
does not reopen aggregate, timeout, HFA/variadic, external call-result, or
expectation buckets. The next packet should use `00174` only as representative
proof, not as a reason to absorb unrelated residuals.

## Proof

Ran the delegated proof:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`.
The build completed and the backend subset passed: 145/145 tests green.
The focused route
`backend_codegen_route_aarch64_scalar_fp_literal_add_publishes_fpr_result`
now passes; generated AArch64 materializes both double literals, emits
`fadd d13, d13, d16`, stores/reloads that value, and passes it to `printf`.
`test_after.log` is the proof log.
