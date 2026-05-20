Status: Active
Source Idea Path: ideas/open/346_aarch64_direct_call_argument_formal_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Narrow Publication Boundary

# Current Packet

## Just Finished

Step 2 repaired the callee-side fixed-formal register publication boundary in
`src/backend/mir/aarch64/codegen/dispatch.cpp`: entry formal publication now
records a scalar register home as authoritative in `BlockScalarLoweringState`
after consuming the ABI source register into that home. Later scalar lowering
therefore retargets operands from stale storage-plan registers to the prepared
formal home instead of re-reading stale carriers.

Focused dispatch coverage now proves a scalar fixed formal arriving in the
ABI source register is published into its prepared register home before a later
multiply, and the multiply consumes that prepared home rather than stale
storage. The repair is general over entry formal register homes and uses the
existing formal publication/prepared-home machinery; it does not match
`myfunc`, `w20`, parameter index 0, or any representative filename.

The delegated representative proof shows `00159` now passes. `00170` also
remains passing, so the outgoing stack-slot immediate guard stayed stable.

Remaining first bad facts after this slice:

- `00140`: still caller-side address or aggregate publication/byval handling;
  the representative still segfaults.
- `00175`: direct-call scalar and FP argument publication advanced; remaining
  failures are local store/load conversion publication after the call series,
  not stale direct-call `w13`/`s13` argument moves.
- `00218`: still caller-side address publication; prepared `%lv.convs` is
  expected in `x21`, but emitted `main` still calls `convert_like_real` with
  stale `x21`.

## Suggested Next

Continue Step 2 with the next narrow publication boundary: repair the
remaining `00175` local store/load conversion publication path if the
supervisor keeps this representative as the next target, or route to `00140`
/ `00218` address or aggregate publication if the next packet should prioritize
those representative failures.

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
- `00140` has an aggregate/byval dimension and should not be reduced to only
  a scalar register shuffle.
- `00170` now passes in the delegated representative subset; keep it as a
  regression guard for outgoing stack-slot immediate publication.
- `00159` now passes and remains the explicit callee-side scalar formal
  consumption guardrail.
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
The representative c-testsuite subset passed `00159` and `00170`, and still
failed `00140`, `00175`, and `00218`. The representative pass count advanced
from 1/5 in `test_before.log` to 2/5 in `test_after.log`.

Supervisor regression guard accepted this as PASS against the prior
`test_before.log`: `00159` was resolved with no new failures. The accepted
after-log was rolled forward to `test_before.log`.
