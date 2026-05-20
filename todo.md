Status: Active
Source Idea Path: ideas/open/346_aarch64_direct_call_argument_formal_publication.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate And Reclassify For Closure

# Current Packet

## Just Finished

Step 4 validation ran the delegated closure-readiness proof with no code,
test, registration, expectation, `plan.md`, or source-idea changes.

Validation result:

- `^backend_`: passed 141/141.
- Representative set:
  - `00140`: passed.
  - `00159`: passed.
  - `00170`: passed.
  - `00218`: passed.
  - `00175`: failed with the expected residual local conversion store/load
    publication mismatch (`97 17`, `97 -1039755664`,
    `0.000000 0.000000`), which remains split to
    `ideas/open/347_aarch64_local_conversion_store_load_publication.md`.

The active direct-call argument/formal publication owner is closure-ready from
the executor perspective: the broad backend subset is green, the four
direct-call representative guardrails pass, and the only representative
failure is the already-classified `00175` residual outside this owner.

## Suggested Next

Supervisor can hand this to plan-owner for closure or parking decision. Keep
`00175` under idea 347 rather than broadening this direct-call owner.

## Watchouts

- This owner is direct-call ABI argument/formal publication only.
- Do not special-case `x20`, `x21`, `w13`, `d13`, argument 8, or the five
  representative filenames; the repair must consume the prepared move/binding
  contracts generally.
- The completed stack-immediate repair is general over prepared immediate
  stack-slot ABI bindings; do not regress the selected store through the
  outgoing stack base before `bl`.
- The completed immediate-cast direct-call repair is intentionally limited to
  prepared before-call register destinations with same-block immediate scalar
  cast producers; it still consumes prepared move/binding/source-home facts
  and uses reserved MIR scratch registers rather than matching source register
  names.
- The completed fixed-formal repair records register homes as authoritative
  only through entry formal publication and intentionally leaves byval and
  stack formal handling on their existing paths.
- The completed local frame-address repair handles pointer call arguments
  whose stale register home is backed by prepared frame-slot address or local
  stack-object metadata; do not weaken it into a register-name shortcut.
- `00140` now passes, but it still exercises aggregate/byval and pointer
  address publication together; keep nearby aggregate paths under semantic
  coverage rather than relying only on this representative.
- `00170` now passes in the delegated representative subset; keep it as a
  regression guard for outgoing stack-slot immediate publication.
- `00159` now passes and remains the explicit callee-side scalar formal
  consumption guardrail.
- `00218` now passes and is the explicit local frame-address call-argument
  publication guardrail.
- Do not absorb pointer/null result publication, FP comparison
  materialization, aggregate/table memory corruption, libc/file/string
  residuals, semantic `lir_to_bir` admission, or timeout/output-storm buckets
  without fresh first-bad-fact evidence.
- Do not reopen closed ideas 309, 311, 336, 337, or 345 from counts or
  filename recurrence alone.
- Reject filename-specific, register-specific, expectation, unsupported,
  runner, timeout, proof-log, or CTest-registration changes.

## Proof

Delegated proof command was run exactly:
`set +e; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' && ctest --test-dir build -j --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00140|00159|00170|00175|00218)_c'; } > test_after.log 2>&1; rc=$?; printf 'step4_validation_rc=%s\n' "$rc"; exit 0`.

Result: `step4_validation_rc=8`, caused by the expected `00175` residual.
`test_after.log` is the preserved proof log. The broad `^backend_` subset
passed 141/141. The representative c-testsuite subset passed `00140`, `00159`,
`00170`, and `00218`, and failed only `00175` with the residual classified to
idea 347.
