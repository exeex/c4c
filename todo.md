Status: Active
Source Idea Path: ideas/open/346_aarch64_direct_call_argument_formal_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Narrow Publication Boundary

# Current Packet

## Just Finished

Step 2 repaired the next caller-side direct-call publication subcase in
`src/backend/mir/aarch64/codegen/calls.cpp`: prepared register/FPR
`CallArgumentAbi` moves whose source value is produced by an immediate scalar
cast in the same block now lower to a selected call-boundary publication
sequence that materializes/converts directly into the ABI argument register
before the direct `bl`. Focused dispatch coverage now proves both an immediate
FP-to-integer source published to `w0` and an immediate integer-to-FP source
published to `s0` through the prepared move/binding records, without matching
specific filenames or stale source registers.

The delegated representative proof shows `00175` advanced but did not pass:
its direct `charfunc`, `intfunc`, and `floatfunc` calls now print the expected
`c`, `99`, and `99.000000` values. The remaining `00175` mismatch is after
the direct-call sequence in local store/load conversion publication:
`97 17`, `97 -160045680`, and `0.000000 0.000000` are still printed where
`97 97`, `97 97`, and `97.000000 97.000000` are expected.

Remaining first bad facts after this slice:

- `00140`: still caller-side address or aggregate publication/byval handling;
  the representative still segfaults.
- `00159`: still callee-side scalar formal consumption; prepared `myfunc`
  publishes `%p.x` in incoming `x0`, but emitted `myfunc` still reads stale
  `w20` in `mul w0, w20, w20`. This is not resolved by caller-side
  `CallArgumentAbi` publication.
- `00175`: direct-call scalar and FP argument publication advanced; remaining
  failures are local store/load conversion publication after the call series,
  not stale direct-call `w13`/`s13` argument moves.
- `00218`: still caller-side address publication; prepared `%lv.convs` is
  expected in `x21`, but emitted `main` still calls `convert_like_real` with
  stale `x21`.

## Suggested Next

Continue Step 2 with the next narrow publication boundary: either repair the
remaining `00175` local store/load conversion publication path if the
supervisor keeps this representative as the next target, or split the
callee-side fixed formal consumption gap for `00159` if the next packet should
leave `calls.cpp`.

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
- The `00159` callee formal gap appears to require the fixed-formal consumer
  path to make incoming `x0` authoritative for later scalar lowering; a
  caller-side move repair alone will not change `mul w0, w20, w20`.
- `00140` has an aggregate/byval dimension and should not be reduced to only
  a scalar register shuffle.
- `00170` now passes in the delegated representative subset; keep it as a
  regression guard for outgoing stack-slot immediate publication.
- `00159` is the explicit callee-side scalar formal consumption guardrail.
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
`set +e; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64|backend_cli_aarch64' && ctest --test-dir build -j --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00140|00159|00170|00175|00218)_c'; } > test_after.log 2>&1; rc=$?; printf 'proof_rc=%s\n' "$rc"; exit 0`.

Result: `proof_rc=8`. The AArch64 backend/backend CLI subset passed 31/31.
The representative c-testsuite subset passed `00170` and still failed `00140`,
`00159`, `00175`, and `00218`. `00175` advanced from stale direct-call
argument outputs to correct direct-call outputs, with only the later local
store/load conversion outputs still mismatching as listed above.

Supervisor review accepted this as valid partial route progress after reviewer
acceptance, no new failing tests, backend/backend CLI staying 31/31, `00170`
staying green, and `00175` direct-call output advancement. The strict
monotonic regression guard did not pass because the representative pass count
remained 1/5, so the before/after logs are retained rather than rolled forward.
