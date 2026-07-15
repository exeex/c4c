# LIR-to-New-BIR Truthiness-Comparison Parameter Receipt Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Resumed from: closed 826's accepted truthiness-comparison LHS handoff

## Purpose

Receive exactly closed 826's native DirectScalar truthiness-comparison LHS
parameter authority into verified Raw BIR.

## Core Rule

Consume only the 826-authorized LIR comparison authority and its exact LHS,
type, integer-`ne`, and authoritative-zero-RHS relations. Do not treat
`LirCondBr.condition` as direct parameter authority or recover facts from text.

## Non-Goals

- Any second parameter-use row, generic parameter admission, ABI conversion,
  or producer/schema/verifier change.
- Repeating accepted Steps 1 through 7.38 or receiving memory/VA,
  aggregate/vector, module/type/global, residual instruction/terminator, or
  inline-assembly work.

## Ordered Steps

### Step 7.39 - Receive the 826-authorized DirectScalar truthiness-comparison-LHS parameter authority row

Goal: map only the closed-826 authority tuple into one typed Raw-BIR
comparison destination and retain all native relations before publication.

Actions:

- map only the parameter value, owner, index, type, DirectScalar ABI, and
  truthiness-comparison LHS role;
- require exact `LirCmpOp.lhs == authority.value`,
  `LirCmpOp.type_str == authority.type`, integer predicate `ne`, and
  authoritative integer-zero RHS with current-function ownership; and
- extend only the reachable importer/verifier path and nearby positive plus
  malformed-authority coverage so missing, invalid, duplicate, foreign,
  owner/index/type/ABI/role-mismatched, or consumer-incoherent input rolls
  back transactionally.

Completion check: fresh build, focused same-feature receiver proof, and a
matching regression guard show only this one 826-authorized row is received;
then return to the source completion gate without claiming whole-source
completion.
