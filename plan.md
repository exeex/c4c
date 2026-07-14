# LIR-To-New-BIR Container And Import Completeness Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Resumed from: closed idea 748 selected memcpy pointer/object authority handoff
(`6a12cddab`, `dac9c8f81`)

## Purpose

Resume the bounded target-independent Raw-BIR receiver route at the first
post-Step-7.19 receiver-ready row. Receive exactly the closed-748 selected
non-volatile fixed-aggregate byval `LirMemcpyOp`; do not repeat accepted work
or broaden memory support.

## Goal

Import each structured-authority LIR fact into one verified Raw-BIR module
without loss or partial publication. Never recover a fact from presentation.

## Core Rule

The selected `LirMemcpyOp::selected_authority` is the sole semantic input:
destination/source `LirValueId`, i64 immediate size, destination/source
`LirObjectId`, current-function object owners, and live-at-site facts. Display
operands remain display-only. The row maps directly to typed Raw-BIR container,
importer, reachable verifier, and transactional proof.

## Read First

- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `ideas/closed/748_lir_memcpy_selected_pointer_object_authority_publication.md`
- `docs/lir_remaining_ordinary_value_identity/authority_matrix.md`
- existing Raw-BIR memory/container builders, views, verifier, and LIR importer

## Landed Progress

- Steps 1 through 5.3.5 established the coverage foundation and accepted typed
  module/type/value, metadata, direct-call, and ordinary-value rows through
  normalized i32 `Mul` (`ea4b63135`).
- Steps 6.1 through 6.5 accepted selected inline-assembly bindings, direct
  branch receipt, `SExt`, and `SLT` rows (`ad82d1456`, `37014f013`,
  `97efe9c38`, `39518d27a`, `03448676f`).
- Steps 7.1 through 7.19 are accepted historical work, including dispatcher
  rollback, bounded scalar/intrinsic/cast/binary rows, builtin ffs/ctz/clz,
  and builtin-popcount receipt (`565be6932`). Do not repeat them.

## Non-Goals

- no LIR schema/producer edits, presentation-text recovery, target
  interpretation, allocation, canonicalization, MIR, emission, or legacy-BIR
  revival
- no second memcpy row, volatile or dynamic-size memcpy, alias/overlap model,
  generic pointer/object model, stack/local, globals, parameters outside this
  fixed byval row, va-list, memset, loads/stores/GEPs, aggregate/vector, CFG,
  inline-assembly, or other memory/object family
- no Raw-BIR receipt beyond exactly the closed-748 selected row

## Execution Rules

1. Implement only the closed-748 selected row and retain every other memcpy
   row's prior unsupported/fail-closed disposition.
2. Add the typed Raw-BIR receiving container, importer dispatch, reachable
   Raw-BIR verifier, and positive/negative transactional coverage together.
3. Require exactly one selected descriptor when the current-function selected
   pointer carrier exists; preserve pointer/object identity, owner, and live
   relation without parsing display operands.
4. Reject missing, duplicate, invalid, cross-function, mismatched, dead,
   non-i64/nonpositive-size, or otherwise incoherent authority with no partial
   Raw-BIR module publication.
5. Keep all producer authority from closed 748 authoritative; do not alter or
   revalidate producer scope through an LIR change.

## Ordered Steps

### Step 7.20 - Receive the selected `LirMemcpyOp` authority row

Goal: receive exactly the closed-748 selected fixed-aggregate byval memcpy row
in typed Raw-BIR, importer dispatch, reachable Raw-BIR verification, and
transactional positive/negative proof.

Primary targets:

- the smallest typed Raw-BIR memcpy/memory receiving container, builder, and
  immutable view required for this single row
- LIR-to-Raw-BIR instruction dispatch and reachable Raw-BIR verifier
- focused backend receiver coverage plus the closed-748 producer regression
  neighbor

Actions:

- consume only `LirMemcpyOp::selected_authority` from the selected non-volatile
  fixed aggregate byval producer in `src/codegen/lir/hir_to_lir/lvalue.cpp`;
  map its source/destination IDs, i64 positive immediate size, object IDs,
  current-function owners, and live-at-site facts without using display
  operands
- verify exactly one selected descriptor under the selected pointer carrier,
  current-function pointer/object ownership, distinct source/destination
  relation, i64-positive size, and live-site coherence; reject missing,
  duplicate, cross-owner/function, type/kind, object-link, size, and lifetime
  failures before Raw-BIR publication
- prove one selected-row success and neighboring malformed authority failures
  with whole-module rollback; preserve unselected memcpy rows and every other
  memory/object family as unsupported and fail-closed

Completion check:

- fresh build, focused receiver plus producer-neighbor proof, supervisor-owned
  matching regression guard, and the required broader checkpoint establish one
  verified transactional selected memcpy receipt without presentation recovery
  or scope expansion.

### Source completion gate (not an executor packet)

Do not execute this as a placeholder. After Step 7.20, return the runbook to
plan-owner for an explicit source-completion, repair, replacement, or
conclusion decision. This source remains open until every valid current-LIR
row has an evidenced typed receiver disposition and all source acceptance
criteria are met.
