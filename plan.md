# LIR-To-New-BIR AMD64 Aggregate VA-Arg Receiver Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Resumed from: closed 753 selected AMD64 SysV aggregate-overflow `va_arg`
receiver handoff.

## Purpose

Receive exactly closed 753's selected AMD64 SysV aggregate
`layout.needs_memory` overflow `va_arg` memcpy row in Raw-BIR. This is one
bounded receiver packet; it does not claim completion of the remaining
memory/VA or source-wide coverage matrix.

## Historical Progress

Steps 1 through 7.31 are accepted historical 734 receiver work. Closed 753
completed the selected producer/verifier handoff and the source requires this
resume at Step 7.32. Do not repeat those earlier receiver steps or producer
authority work.

## Core Rule

Consume only the checked native fields from closed 753's Step 3 receiver-ready
handoff. Do not derive pointer, object, owner, liveness, type, size, storage,
or row-selection authority from names, formatted operands, rendered LIR/LLVM,
testcase shape, `monostate`, or unclassified operands.

## Read First

- `ideas/closed/753_lir_memory_va_pointer_authority_convergence.md` (Step 3
  receiver-ready handoff and closure record)
- `ideas/closed/799_lir_amd64_vaarg_overflow_aggregate_carrier_authority.md`
- `docs/lir_to_new_bir_remaining_coverage/successor_queue.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md` (post-Step 7.31
  exhaustion decision)

## Non-Goals

- Any other memory/VA, aggregate/vector, local/VLA, parameter, module/type,
  global/metadata, instruction/terminator, or inline-assembly authority row;
- producer/schema/verifier republishing, target lowering, MIR, emission, or a
  broad importer/dispatcher sweep;
- presentation-derived recovery or weaker verifier/test contracts.

## Ordered Steps

### Step 7.32 - Receive selected AMD64 aggregate VA-arg overflow authority

Goal: transactionally import closed 753's one selected AMD64 SysV aggregate
`layout.needs_memory` overflow `va_arg` memcpy row into a typed Raw-BIR
destination.

Actions:

- consume only the selected direct current-function `va_list` local, typed
  field-2 GEP address, overflow-pointer load, `Amd64SysVOverflowArgArea`
  storage kind, live aggregate temporary, final load identity, struct payload
  `LirTypeRef`, and positive typed i64 byte size;
- bind the selected `LirMemcpyOp` source, destination, and immediate size to
  those identities; add only the minimum Raw-BIR destination, importer
  dispatch, reachable verification, and nearby positive/negative receiver
  coverage;
- reject partial, unselected, mixed, foreign, dead, non-derived,
  destination-disagreeing, or type/size-disagreeing input transactionally;
- run a fresh build and focused receiver proof before the supervisor-selected
  broader proof.

Completion check: exactly the selected non-volatile direct-local AMD64 SysV
aggregate-overflow memcpy row imports and verifies without presentation
recovery. All other rows remain fail closed; source completion is reassessed
after this bounded receipt.
