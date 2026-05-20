# AArch64 Non-HFA Aggregate Va Arg Materialization

Status: Open
Created: 2026-05-20
Split From: ideas/open/329_aarch64_variadic_aggregate_va_arg_call_setup.md

## Goal

Repair AArch64 non-HFA aggregate `va_arg` materialization so generated code
copies the selected aggregate bytes from the active `va_list` source into the
temporary aggregate object before later ordinary calls or member accesses
observe that object.

## Why This Exists

Idea 329 repaired post-`va_arg` ordinary-call operand publication. The
focused `00204.c` representative now reaches `myprintf` `%7s` and `%9s`
branches with the following call operands correctly published before
`bl printf`: `.str31` / `.str33` in `x0` and the aggregate text-buffer
address in `x1`.

The remaining first bad fact is earlier than the call boundary. The generated
branches compute or select a non-HFA aggregate `va_arg` source through `x13`,
but the selected bytes are not copied into the stack buffers later passed as
`x1` (`sp + 8` for `%7s`, `sp + 15` for `%9s`). Runtime output after
`stdarg:` is therefore corrupted instead of printing the expected non-HFA
aggregate text such as `ABCDEFGHI` and `lmnopqr`.

This is not remaining idea 329 call-operand publication and is not yet the
parked idea 326 HFA/floating residual. The next owner is the materialization
of non-HFA aggregate `va_arg` values before their storage is observed.

## In Scope

- Localize the generated `myprintf` `%7s` and `%9s` non-HFA aggregate
  `va_arg` paths from `va_list` source selection through the temporary
  aggregate object later passed to `printf`.
- Identify whether the missing copy belongs to register-save-area addressing,
  overflow-area addressing, aggregate byte-copy lowering, temporary aggregate
  destination selection, stack-slot publication, instruction ordering, or
  `va_list` progression.
- Repair the general AArch64 path that materializes non-HFA aggregate
  `va_arg` values into their destination object before member address
  publication or ordinary calls can observe that object.
- Add focused backend coverage for a non-HFA aggregate `va_arg` whose bytes
  must be copied into a temporary object before a following use.
- Preserve the completed call-boundary publication from idea 329 and earlier
  byval aggregate, fixed-formal, local/value-home, frame/formal, and fixed
  HFA guardrails.

## Out Of Scope

- Reopening post-`va_arg` ordinary-call operand publication unless generated
  code again reaches `bl printf` without the fixed format string in `x0` or
  the aggregate-derived operand in its assigned ABI register.
- Reopening idea 326 HFA/floating residuals unless the representative first
  gets past non-HFA aggregate string materialization and fresh evidence
  reaches HFA/floating corruption, floating register-save-area progression,
  overflow-area progression, or HFA lane materialization.
- Reopening idea 328 byval aggregate call-argument lane publication,
  fixed-formal entry publication, local/value-home publication, frame/formal
  publication, global initializer emission, runner, expectation,
  unsupported, timeout, or proof-log behavior without direct generated-code
  evidence tying that surface to the current first bad fact.
- Fixing only `%7s`, `%9s`, `myprintf`, `x13`, `sp + 8`, `sp + 15`, one
  aggregate size, one branch, one string literal, or one emitted copy
  sequence.

## Acceptance Criteria

- The first bad fact is localized to concrete generated-code state: selected
  `va_arg` source, destination aggregate object, expected copied bytes,
  observed missing or wrong copy instructions, and the following use that
  observes the destination.
- Generated AArch64 copies the selected non-HFA aggregate bytes into the
  temporary object before publishing the object or member address to a later
  ordinary call.
- Focused backend coverage proves the repaired materialization owner and
  fails without the repair.
- Adjacent guardrails remain stable, especially idea 329 call-operand
  publication and earlier byval, fixed-formal, local/value-home, frame/formal,
  and fixed HFA repairs.
- The focused `00204.c` representative either passes or advances to a newly
  classified first bad fact for lifecycle handoff.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00204.c`, `myprintf`, `%7s`, `%9s`, `x13`, `sp + 8`,
  `sp + 15`, `ABCDEFGHI`, `lmnopqr`, one aggregate size, one branch, or one
  emitted instruction sequence instead of repairing general AArch64 non-HFA
  aggregate `va_arg` materialization;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, or CTest registration to hide the corrupted
  `stdarg:` string output;
- claims capability progress only through helper renames, dump text changes,
  expectation rewrites, or classification notes without generated code
  copying selected aggregate bytes into the destination object before the
  following use;
- reopens idea 329 call-operand publication, idea 326 HFA/floating residuals,
  idea 328 byval aggregate call lanes, fixed-formal entry, local/value-home,
  frame/formal, or global data behavior without fresh generated-code evidence
  tying that owner to the current first bad fact;
- preserves the exact old failure behind a new abstraction name, such that
  `printf` receives the correct pointer in `x1` but the pointed-to stack
  buffer still contains unmaterialized or unrelated bytes;
- proves only the external `00204.c` run while nearby focused backend output
  lacks assertions for the non-HFA aggregate `va_arg` source-to-destination
  copy being repaired.
