# AArch64 Direct-Call Argument And Formal Publication

Status: Closed
Created: 2026-05-20
Split From: ideas/open/295_backend_regex_failure_family_inventory.md

## Goal

Repair the AArch64 direct-call boundary where prepared call operands and
callee formal homes fail to publish into, or read from, the AAPCS64 ABI
argument registers and stack slots for scalar, floating, address, aggregate,
overflow, and variadic direct-call shapes.

## Why This Exists

The post-345 backend-regex inventory classified 24 current
`c_testsuite_aarch64_backend_*` residuals from `/workspaces/c4c/test_after.log`
after Step 1 inventory commit `67736b8b2` and Step 2 classification commit
`c47813313`. Local backend/unit/CLI tests selected by the backend regex were
clean; the remaining failures are external AArch64 c-testsuite runtime
residuals.

The clearest tractable owner is the direct-call argument/formal publication
family represented by `00140`, `00159`, `00170`, `00175`, and `00218`.
Current generated-code evidence points to ABI publication and consumption, not
to the closed selected call-boundary printer diagnostic from idea 311 or the
closed indirect-call preservation owner from idea 309:

- `00159.c.s` `myfunc` uses stale `w20` instead of incoming `w0`.
- `00175.c.s` call/conversion paths use stale `w13`/`d13` for scalar and FP
  arguments.
- `00170.c.s` loses the eighth variadic/overflow integer argument.
- `00140.c.s` passes uninitialized `x20` for an aggregate or address
  argument.
- `00218.c.s` calls `convert_like_real` with uninitialized `x21` instead of
  `&convs`.

The umbrella inventory should remain classification-only. Implementation work
belongs in this focused owner before any backend code edits begin.

## In Scope

- Localize how prepared direct-call operands become AAPCS64 argument
  registers and stack slots at the callsite.
- Localize how callee direct-call formals become usable scalar, floating,
  address, aggregate, overflow, and variadic homes on function entry.
- Repair the general publication/consumption rule for direct-call ABI
  arguments and formals without matching one c-testsuite filename, function,
  register, or emitted instruction string.
- Preserve already closed boundaries for indirect-call argument preservation,
  selected call-boundary move printing, return-result publication, and
  callee-saved scalar-home preservation.
- Add focused backend coverage that exercises scalar integer, FP, address or
  aggregate, and overflow/stack direct-call argument publication.
- Rerun the representative c-testsuite cases and reclassify any remaining
  first bad facts before closure.

## Out Of Scope

- Reopening closed idea 309 indirect-call preservation or closed idea 311
  selected call-boundary move preparation/printing without fresh printer or
  indirect-call first-bad-fact evidence.
- Repairing pointer/null condition result publication, FP comparison result
  materialization, addressable memory/indexed aggregate corruption,
  libc/file/string residuals, timeout/output-storm cases, or semantic
  `lir_to_bir` admission failures from the umbrella inventory.
- Changing c-testsuite expectations, unsupported classifications, runner
  behavior, timeout policy, proof-log policy, allowlists, or CTest
  registration.
- Fixing only `00140`, `00159`, `00170`, `00175`, `00218`, one function name,
  one argument index, one stack offset, one register such as `w13`, `w20`,
  `x20`, `x21`, or `d13`, or one generated assembly sequence.

## Acceptance Criteria

- The first bad fact is localized to concrete direct-call argument/formal
  publication or consumption boundaries with prepared operand, ABI
  classification, destination register/stack-slot, and generated-code
  evidence.
- Focused backend coverage fails without the repair and passes with it for
  direct-call scalar, FP, address or aggregate, and overflow/stack argument
  publication.
- The representative cases `00140`, `00159`, `00170`, `00175`, and `00218`
  either pass or advance beyond the current direct-call argument/formal
  publication failure mode.
- Any remaining representative failure is reclassified by its new first bad
  fact before this idea is closed.
- Adjacent closed-owner guardrails remain stable: indirect-call preservation,
  selected call-boundary printer admission, return-result publication,
  callee-saved scalar-home preservation, and scalar-select result publication.

## Lifecycle Split Note

Post-Step-2 evidence shows the remaining `00175` residual no longer belongs to
this direct-call argument/formal publication owner. The direct-call call series
now publishes its scalar and FP arguments correctly; the remaining bad output
comes from later local scalar/FP conversion store/load publication. That
residual is tracked separately in
`ideas/open/347_aarch64_local_conversion_store_load_publication.md`; it should
not drive more Step 2 implementation here.

## Closure Note

Closed after Step 4 validation. The delegated proof showed `^backend_` passing
141/141 and the representative direct-call guardrails `00140`, `00159`,
`00170`, and `00218` passing. The only remaining representative failure,
`00175`, is the local conversion store/load publication residual split to
`ideas/open/347_aarch64_local_conversion_store_load_publication.md`, not a
direct-call argument/formal publication blocker.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00140`, `00159`, `00170`, `00175`, `00218`, one function
  name, one argument index, one ABI register, one stack slot, one stale
  register such as `w13`, `w20`, `x20`, `x21`, or `d13`, or one emitted
  instruction sequence instead of repairing direct-call argument/formal
  publication generally;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, allowlists, or CTest registration to reduce the
  residual count;
- claims capability progress only through helper renames, dump text changes,
  diagnostic rewrites, expectation changes, or classification notes without
  generated direct-call sites and callee entries publishing/consuming the
  correct ABI argument locations;
- broadens into pointer/null result publication, FP comparison materialization,
  aggregate/table memory corruption, libc/file IO, timeout/output-storm, or
  semantic `lir_to_bir` admission without fresh evidence that those buckets
  share the direct-call argument/formal first bad fact;
- reopens closed ideas 309, 311, 336, 337, or 345 from filename recurrence or
  count changes alone instead of fresh generated-code, printer diagnostic, or
  runtime evidence that contradicts their closure boundaries;
- leaves the old failure mode behind a renamed abstraction, such that direct
  calls still branch with uninitialized or stale ABI argument registers/stack
  slots, or callees still read stale homes instead of incoming formals.
