Status: Active
Source Idea Path: ideas/open/346_aarch64_direct_call_argument_formal_publication.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate And Reclassify For Closure

# Current Packet

## Just Finished

Step 2 repaired the caller-side direct-call local frame-address publication
boundary in `src/backend/mir/aarch64/codegen/calls.cpp`: before-call pointer
arguments whose prepared source is a register home now prefer prepared
frame-slot address materialization or the matching local stack object when the
BIR call argument is a pointer. The call-boundary move then materializes that
frame address directly into the destination ABI register before `bl` instead
of moving a stale register source.

Focused dispatch coverage now proves a register-homed local pointer call
argument with a matching prepared local stack object is lowered as a frame
address materialization into the ABI register. The generated representative
assembly for the repaired shapes now publishes local frame addresses with
`add xN, sp, #...` before the direct calls. The repair is general over
prepared pointer call arguments, frame-slot address materializations, and
local stack-object metadata; it does not match `x20`, `x21`, `f1`,
`convert_like_real`, function names, filenames, or the representative set.

The delegated representative proof now shows `00140` and `00218` pass.
Existing guards `00159` and `00170` remain passing.

Remaining first bad facts after this slice:

- `00175`: still fails with local store/load conversion publication residuals
  after the direct-call series. The direct-call scalar and FP argument moves
  remain advanced; the remaining mismatch is the later local conversion output
  (`97 17`, `97 -581795216`, `0.000000 0.000000`) rather than stale direct-call
  ABI argument publication.

Lifecycle review classified the remaining `00175` failure as outside the
active direct-call argument/formal publication owner: the direct-call call
series now publishes expected scalar and FP arguments, and the remaining bad
output is from later local conversion stores/reloads. Continuing Step 2
implementation for that residual would broaden this idea into local conversion
store/load publication, so Step 2 direct-call implementation is stopped. The
residual is split to
`ideas/open/347_aarch64_local_conversion_store_load_publication.md`.

## Suggested Next

Proceed with Step 4 validation/classification for the active direct-call owner.
Use the accepted proof showing `00140`, `00159`, `00170`, and `00218` pass and
`00175` has advanced beyond direct-call ABI publication. If closure is
requested, run the close-time regression guard before moving the active source
idea out of `ideas/open/`.

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
`set +e; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64|backend_cli_aarch64' && ctest --test-dir build -j --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00140|00159|00170|00175|00218)_c'; } > test_after.log 2>&1; rc=$?; printf 'proof_rc=%s\n' "$rc"; exit 0`.

Result: `proof_rc=8`. The AArch64 backend/backend CLI subset passed 31/31.
The representative c-testsuite subset passed `00140`, `00159`, `00170`, and
`00218`, and still failed `00175`. The representative pass count advanced from
2/5 in `test_before.log` to 4/5 in `test_after.log`.

Supervisor regression guard accepted this as PASS against the prior
`test_before.log`: `00140` and `00218` were resolved with no new failures. The
accepted `test_after.log` was rolled forward to `test_before.log`.

Focused pre-proof check also passed:
`ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_instruction_dispatch|backend_aarch64_machine_printer|backend_aarch64_target_instruction_records'`.
