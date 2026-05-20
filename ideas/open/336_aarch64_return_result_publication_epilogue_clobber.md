# AArch64 Return Result Publication Epilogue Clobber

Status: Open
Created: 2026-05-20
Split From: ideas/open/295_backend_regex_failure_family_inventory.md

## Goal

Repair the AArch64 runtime failure family where generated code computes or
loads the intended function result into `w0`/`x0`, then the return path
overwrites `x0` with a stale restored register or non-result home before
returning to the caller.

## Why This Exists

The refreshed umbrella inventory in idea 295 classified 53 current
`c_testsuite_aarch64_backend_*` failures and no local backend/unit/CLI
failures. The largest coherent owner is a 22-case AArch64 return-result
publication / epilogue-clobber bucket:

```text
00004, 00011, 00013, 00014, 00016, 00019, 00020, 00022,
00052, 00087, 00103, 00112, 00116, 00117, 00118, 00121,
00124, 00139, 00153, 00159, 00168, 00170
```

Representative generated assembly shows the same semantic shape across simple
scalar returns, pointer-local returns, and call-result returns:

- `00011.c.s`, `00022.c.s`, and `00052.c.s` load or materialize the intended
  return value, then overwrite `x0` with stale `x13`.
- `00004.c.s` and related pointer-local cases load the correct value into
  `w0`, then overwrite `x0` with restored or stale `x20`.
- `00159.c.s` computes `myfunc` into `w0`, then returns `x13`.
- `00168.c.s` computes recursive factorial into `w0`, then returns `x13`.
- `00087.c.s` and `00124.c.s` preserve indirect call results, then return a
  restored callee-saved register instead of the result.

This owner is separate from closed scalar OPI result publication idea 333 and
closed local-slot publication idea 335. The current evidence is not one
`00204.c` route or one local-slot load; it is a broad final result publication
and return/epilogue ordering problem across many generated functions.

## In Scope

- Localize where the generated function return value authority moves from the
  computed expression, load, or call result into the ABI return register.
- Determine whether the first owner is prepared return home selection,
  result-home publication, call-result preservation, epilogue restore ordering,
  or an AArch64 return emission rule that emits a stale final `mov x0, ...`.
- Repair the classified AArch64 return-result publication rule generally for
  scalar, pointer/local, and call-result representatives.
- Add focused backend coverage that fails without the repair and proves the
  fixed return-result publication path without relying only on external
  c-testsuite cases.
- Validate at least one no-call scalar return case, one pointer/local return
  case, and one call-result return case from the current 22-case bucket.

## Out Of Scope

- Reopening closed ideas 333 or 335 unless fresh generated-code evidence shows
  their exact old owners returned.
- Reopening fixed-formal entry publication, byval/HFA/stdarg publication,
  MOVI materialization, local-slot load publication, frame-layout, runner,
  timeout, CTest registration, expectation, or unsupported-classification
  work without direct evidence that it is the first owner for this bucket.
- Repairing scalar conversion/comparison, addressable-memory materialization,
  switch/loop/control-flow lowering, varargs/libc ABI, indirect-call pointer
  materialization, or scalar-cast machine-printer buckets from idea 295.
- Fixing only one c-testsuite filename, one function, one register such as
  `x13`, `x20`, or `x21`, one stack slot, one source line, or one emitted
  instruction string.

## Acceptance Criteria

- The first bad fact is localized to a concrete return-result publication,
  result-home, call-result preservation, or epilogue ordering owner with
  evidence from generated assembly or focused dumps.
- A general AArch64 repair prevents a correct `w0`/`x0` result from being
  overwritten by a stale restored register on the return path.
- Focused backend coverage fails without the repair and passes with it.
- Representative external cases cover the bucket shape: a small no-call return
  case such as `00011`, `00022`, or `00052`; a pointer/local return case such
  as `00004`; and a call-result case such as `00159`, `00168`, `00087`, or
  `00124`.
- Adjacent AArch64 result-publication, local-slot, call-boundary, frame, and
  scalar backend guardrails remain stable.

## Reviewer Reject Signals

Reject the route if it:

- special-cases any one c-testsuite file, function, register, stack offset,
  return literal, source line, or emitted instruction sequence instead of
  repairing the general AArch64 return-result publication owner;
- claims progress by changing expected output, unsupported classifications,
  runner behavior, timeout policy, CTest registration, proof-log handling, or
  broad baseline selection;
- proves only a single named representative while nearby same-shape scalar,
  pointer/local, and call-result return cases remain unexamined;
- reopens closed ideas 333 or 335, or adjacent byval/HFA/stdarg/frame/MOVI
  owners, without generated-code evidence tying this bucket's first bad fact
  to that exact owner;
- only renames helpers, rewrites diagnostics, changes dump formatting, or
  moves stale-register overwrites behind a new abstraction while a correct
  result can still be clobbered before return.
