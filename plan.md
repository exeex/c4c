# LIR-to-New-BIR Fixed Direct-Call Parameter Receipt Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Resumed from: closed 827's accepted fixed-direct-call argument-0 handoff

## Purpose

Receive exactly closed 827's native DirectScalar fixed-direct-call
argument-0 parameter authority into verified Raw BIR.

## Core Rule

Consume only the 827-authorized structured argument-0 authority and its exact
current-function and fixed-callee-parameter-0 coherence. Do not recover any
fact from text, names, signatures, rendered operands, diagnostics, or
compatibility fields.

## Non-Goals

- Any second parameter-use row, generic parameter admission, ABI conversion,
  or producer/schema/verifier change.
- Repeating accepted Steps 1 through 7.39 or receiving memory/VA,
  aggregate/vector, module/type/global, residual instruction/terminator, or
  inline-assembly work.

## Ordered Steps

### Step 7.40 - Receive the 827-authorized fixed-direct-call argument-0 DirectScalar body-parameter authority row

Goal: map only the closed-827 authority tuple into one typed Raw-BIR
call-argument destination and retain all native relations before publication.

Actions:

- map only the parameter value, owner, index, type, DirectScalar ABI, and
  `FixedDirectCallArgument0` role from `LirCallOp.structured_args[0]`;
- require the structured argument-0 SSA value/type to equal the authority and
  to agree with fixed callee parameter 0 of a direct, non-variadic, specified
  call; and
- extend only the reachable importer/verifier path and nearby positive plus
  malformed-authority coverage so missing, invalid, duplicate, foreign,
  owner/index/type/ABI/role-mismatched, or consumer-incoherent input rolls
  back transactionally.

Completion check: fresh build, focused same-feature receiver proof, and a
matching regression guard show only this one 827-authorized row is received;
then return to the source completion gate without claiming whole-source
completion.
