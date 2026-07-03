# RV64 Integer Div/Rem Residual Instruction-Fragment Lowering

Status: Open
Type: RV64 object-lowering implementation follow-up
Parent: `ideas/closed/546_rv64_instruction_fragment_current_classification.md`
Owning Layer: RV64/MIR object lowering

## Goal

Clear or reroute the `30` current `integer_div_rem` rows classified from the
refreshed instruction-fragment scan after proving the exact RV64 object
instruction fragment that still owns each representative failure.

## Why This Exists

The completed instruction-fragment classification found `87`
implementation-ready RV64 object-lowering rows. The highest-value and clearest
first follow-up is the `integer_div_rem` bucket: `30` rows with direct BIR
`sdiv`, `udiv`, `srem`, or `urem` evidence and first owner
`rv64_object_lowering`.

Step 1 of the active runbook corrected the route premise: the current tree
already has semantic RV64 object lowering for BIR `sdiv`, `udiv`, `srem`, and
`urem`, with focused object-emission coverage for all four operations at I32
and I64 width. The representative rows still fail with the generic
`unsupported_instruction_fragment` diagnostic, but the next owner must be
proven at the exact later fragment before adding any new lowering.

The durable source evidence is:

- Refreshed scan:
  `build/agent_state/rv64_gcc_torture_backend_current_20260703T015523Z.log`.
- Accepted row set:
  `build/agent_state/unsupported_instruction_fragment_current_rows.tsv`.
- Classification table:
  `build/agent_state/546_step3_instruction_fragment_classification.tsv`.
- Follow-up routing:
  `build/agent_state/546_step5_followup_routing.md` and
  `build/agent_state/546_step5_followup_routing.tsv`.

## In Scope

- Pin the first unsupported instruction fragment in representative
  `integer_div_rem` rows after the existing div/rem object-emission path.
- Repair a generalized RV64 object-lowering gap only when the pinned fragment
  is semantic, route-owned, and not already covered by existing div/rem opcode
  support.
- Preserve signed versus unsigned semantics and operand-width behavior required
  by the existing BIR/prepared facts when any div/rem-adjacent repair is needed.
- Extend focused backend/object-emission coverage only for the newly pinned
  missing semantic path, not to duplicate existing div/rem opcode tests.
- Prove representative gcc_torture rows from the classified `integer_div_rem`
  bucket, including at least a small allowlist drawn from:
  `src/20001026-1.c`, `src/20050215-1.c`, `src/20090113-2.c`,
  `src/20090113-3.c`, and `src/20101013-1.c`.
- Record residual failures as downstream owners when the pinned fragment is not
  an implementation-ready RV64 object-lowering gap for this idea.

## Out Of Scope

- Arithmetic shift-right lowering.
- Pointer/integer casts.
- Heterogeneous scalar integer binary triage beyond div/rem.
- F32/F64 scalar FP cast/op residual work.
- F128 or long-double helper policy.
- Producer/prepared, ABI/call, or evidence-gap rows screened out by idea 546.
- Changing expectations, unsupported markers, allowlists, or pass/fail
  accounting as a substitute for lowering support.

## Acceptance Criteria

- The route records that raw RV64 object lowering for `sdiv`, `udiv`, `srem`,
  and `urem` already exists, including I32/I64 focused backend coverage.
- The first still-unsupported representative instruction fragment is pinned
  with concrete BIR/prepared evidence before implementation proceeds.
- Any implementation patch repairs a generalized semantic RV64 object-lowering
  gap, not a duplicate div/rem opcode encoder or testcase-shaped shortcut.
- Representative `integer_div_rem` gcc_torture rows no longer fail with an
  unowned generic `unsupported_instruction_fragment` diagnostic for this route.
- Existing non-div/rem instruction-fragment buckets are not silently folded
  into this slice.
- Any remaining representative failures name concrete downstream owners.
- Regression proof includes the supervisor-selected backend subset and a
  representative allowlist run for the div/rem rows.

## Reviewer Reject Signals

- Reject testcase-name dispatch, named-case constants, or allowlist filtering
  presented as div/rem lowering progress.
- Reject duplicate div/rem opcode encoders or new div/rem tests presented as
  progress without pinning a missing fragment beyond the existing semantic
  support.
- Reject raw diagnostic matching, opcode-text-only matching, or fragment-text
  string matching instead of semantic BIR/prepared instruction evidence.
- Reject helper-call substitutions that bypass or obscure generalized RV64
  `div`, `divu`, `rem`, or `remu` semantics without explicit route approval.
- Reject expectation rewrites, unsupported downgrades, or weaker test
  contracts without explicit user approval.
- Reject folding F128, producer/prepared, ABI/call, pointer-cast, shift-right,
  or heterogeneous scalar-binary rows into this idea without first-owner proof.
- Reject a patch that passes only one named representative while nearby
  `integer_div_rem` rows remain unexamined or retain the same owned diagnostic.
