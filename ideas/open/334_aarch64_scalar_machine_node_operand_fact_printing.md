# AArch64 Scalar Machine-Node Operand Fact Printing

Status: Open
Created: 2026-05-20
Split From: ideas/open/295_backend_regex_failure_family_inventory.md

## Goal

Repair AArch64 scalar machine-node operand fact preservation and printing so
selected scalar arithmetic nodes carry printable, coherent source facts when
their operands live in registers, frame slots, or prepared spill homes.

## Why This Exists

The fresh backend-regex inventory selected 354 tests, with all 55 failures in
`c_testsuite_aarch64_backend_*`. The crisp current compile/printer failures are
`c_testsuite_aarch64_backend_src_00164_c` and
`c_testsuite_aarch64_backend_src_00214_c`.

`00164.c` fails before runtime while printing a selected scalar `mul` node:

```text
cannot print AArch64 machine node family=scalar opcode=mul:
scalar mul/div/rem node has incomplete printable rhs facts
```

Prepared evidence for `00164.c` shows scalar arithmetic such as
`%t159 = bir.mul i32 %t157, %t158` and
`%t168 = bir.mul i32 %t166, %t167`, with late operands/results assigned to
prepared spill/frame homes including `%t158` at `stack232`, `%t159` at
`stack236`, `%t167` at `stack244`, and `%t168` at `stack248`.

`00214.c` fails before runtime while printing a selected scalar `add` node:

```text
cannot print AArch64 machine node family=scalar opcode=add:
scalar add/sub/bitwise memory operands require prepared frame-slot sources
```

This owner is about selected scalar machine nodes reaching the AArch64 printer
with enough structured operand facts to print or materialize their operands
generally. It is not a request to special-case those two files.

## In Scope

- Localize how selected AArch64 scalar `mul`/`div`/`rem` and
  `add`/`sub`/`bitwise` machine nodes record source operands after
  preparation, liveness, register allocation, spill/reload, and frame-slot
  assignment.
- Repair missing or incoherent printable operand facts for scalar arithmetic
  nodes whose operands are prepared registers, frame slots, spill slots, or
  rematerializable values.
- Preserve semantic operation width and signedness for scalar arithmetic while
  making operand spelling or operand materialization explicit.
- Add focused backend coverage that fails without the operand-fact repair and
  proves both the scalar `mul` RHS-fact path and scalar `add` frame-slot or
  memory-operand path.
- Validate the focused representatives
  `c_testsuite_aarch64_backend_src_00164_c` and
  `c_testsuite_aarch64_backend_src_00214_c`, then classify any next first bad
  fact.

## Out Of Scope

- Reopening parked idea 316 frame-slot/frame-layout consistency unless fresh
  localization proves frame allocation is the current first owner after, or
  inside, this scalar-printer path.
- Repairing runtime mismatch, runtime nonzero, timeout, output-storm, runner,
  expectation, unsupported-classification, CTest-registration, or broad
  backend-regex behavior.
- Fixing only `00164.c`, `00214.c`, one function, one instruction index, one
  stack offset, one literal, one register, or one emitted instruction
  sequence.
- Reopening fixed-formal, byval, HFA/floating, stdarg cursor, MOVI,
  large-offset spelling, or local/value publication owners without direct
  current evidence tying them to this scalar machine-node printer failure.

## Acceptance Criteria

- The first scalar machine-node printer divergence is localized to concrete
  prepared operands, selected machine-node records, storage homes, and printer
  requirements.
- A general AArch64 scalar operand-fact repair makes selected scalar
  `mul`/`div`/`rem` and scalar `add`/`sub`/`bitwise` nodes printable when their
  operands come from prepared spill/frame-slot/register homes.
- Focused backend coverage proves the repaired operand-fact path without
  relying only on the external c-testsuite representatives.
- The focused representatives get past their current printer diagnostics, or
  any next first bad fact is explicitly classified for lifecycle handoff.
- Adjacent AArch64 scalar, memory operand, frame-slot, and machine-printer
  guardrails remain stable.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00164.c`, `00214.c`, `main`, `extend_brk`, one instruction
  index, one stack offset such as `stack232`, one scalar opcode, one register,
  or one emitted instruction string instead of repairing general scalar
  operand-fact preservation/printing;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, or CTest registration to improve counts;
- claims progress only through helper renames, diagnostic text changes, dump
  formatting, expectation rewrites, or classification notes while selected
  scalar nodes can still reach the printer without required operand facts;
- broadens into frame-layout repair, runtime mismatch repair, byval/HFA/stdarg
  work, or large stack behavior without fresh generated-code evidence that the
  scalar printer first bad fact has moved there;
- hides the exact old printer failure behind a new abstraction while
  `00164.c` can still report incomplete scalar `mul` RHS facts or `00214.c`
  can still report missing scalar `add` frame-slot operand facts.
