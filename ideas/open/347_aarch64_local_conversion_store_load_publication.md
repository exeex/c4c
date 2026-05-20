# AArch64 Local Conversion Store Load Publication

Status: Open
Created: 2026-05-20
Split From: ideas/open/346_aarch64_direct_call_argument_formal_publication.md

## Goal

Repair the AArch64 local scalar and floating conversion store/load publication
path that remains visible in `c_testsuite_aarch64_backend_src_00175_c` after
direct-call argument publication has advanced.

## Why This Exists

The direct-call owner repaired the original `00175` stale ABI argument
publication failures. The current generated-code and runtime evidence show the
`charfunc`, `intfunc`, and `floatfunc` direct-call series now publish expected
call arguments, while later local conversion outputs still fail:

- `97 17`
- `97 -581795216`
- `0.000000 0.000000`

That residual is local store/load conversion publication, not direct-call
argument/formal publication: the failed lines occur after the direct-call
series has already produced expected call outputs, and the mismatches are the
later local scalar and FP conversion outputs. Continuing it under the
direct-call source idea would broaden that owner beyond its durable intent.

## In Scope

- Localize the first bad local conversion store/load fact in generated
  AArch64 code for `00175` after the direct-call series.
- Identify whether scalar integer conversion results, floating conversion
  results, local homes, stack slots, or reload paths lose the authoritative
  value.
- Repair the general local conversion publication rule that stores and reloads
  conversion results from the correct local home.
- Add focused backend coverage when a narrow local conversion store/load
  contract can pin the behavior without relying only on the c-testsuite file.
- Use `c_testsuite_aarch64_backend_src_00175_c` as representative proof after
  focused coverage or localization establishes the semantic rule.

## Out Of Scope

- Reopening direct-call argument/formal publication work from
  `ideas/open/346_aarch64_direct_call_argument_formal_publication.md` unless
  fresh first-bad-fact evidence shows an ABI argument or formal publication
  regression.
- Reopening closed indirect-call, selected call-boundary printer, return-result
  publication, callee-saved scalar-home, or scalar-select owners from filename
  recurrence alone.
- Repairing unrelated pointer/null condition result publication, aggregate or
  table memory corruption, libc/file/string residuals, semantic `lir_to_bir`
  admission, timeout/output-storm, or runner/proof-log behavior.
- Changing expectations, unsupported classifications, allowlists, CTest
  registration, timeout policy, proof-log policy, or runner behavior.

## Acceptance Criteria

- The local conversion store/load first bad fact is documented with generated
  AArch64 evidence and distinguished from direct-call ABI publication.
- Focused backend coverage proves the local conversion store/load publication
  rule when such a unit-level contract is available.
- `c_testsuite_aarch64_backend_src_00175_c` advances or passes for the local
  scalar and floating conversion outputs without weakening expectations.
- Existing direct-call representative guards from idea 346 remain stable,
  especially `00140`, `00159`, `00170`, and `00218`.
- Any remaining `00175` failure is reclassified by its next first bad fact
  before this idea is closed.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00175`, `charfunc`, `intfunc`, `floatfunc`, one source line,
  one register such as `w13` or `d13`, one stack offset, or one emitted
  instruction sequence instead of repairing local conversion store/load
  publication generally;
- claims progress by changing expected output, unsupported classifications,
  runner behavior, timeout policy, proof-log policy, allowlists, or CTest
  registration;
- treats the residual as direct-call argument/formal publication without fresh
  generated-code evidence that a callsite ABI argument or callee formal home is
  again the first bad fact;
- claims capability progress only through helper renames, dump formatting,
  diagnostic text changes, or classification notes while local conversion
  stores or reloads still read stale, uninitialized, or mismatched homes;
- broadens into pointer/null result publication, aggregate/table memory
  corruption, libc/file IO, timeout/output-storm, semantic `lir_to_bir`
  admission, or closed call-boundary owners without a new first-bad-fact
  record tying that work to this local conversion publication residual;
- leaves the old failure mode behind a renamed abstraction, such that the
  local conversion output still comes from stale or uninitialized scalar or FP
  homes.
