# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 4.2
Current Step Title: Receive direct selected-global scalar loads

## Just Finished

- Added the minimal typed Raw-BIR `StoreNode`: one resolved
  `GlobalObjectId`, its exact integer `Type`, one ordinary constant `ValueId`
  operand, no result, immutable view exposure, builder ownership/type checks,
  and defensive foundation verification.
- Imported only authoritative direct-global integer stores through a native
  `LinkNameId` to imported-global registry and the existing typed
  constant/value model. Display-only/raw rows, SSA/local pointers, wrong
  authority alternatives, non-integers, overflow, mismatches, unresolved or
  ambiguous globals, and later unsupported instructions remain transactional.
- Proved two neighboring store shapes with misleading displays, structural
  builder Raw/Canonical publication, post-store `InvalidVoidReturn`, unchanged
  load/GEP rejection, fresh focused proof, and full 3033/3033 regression proof.

## Suggested Next

- Implement only Step 4.2's authoritative direct selected-global scalar
  `LirLoadOp` row, reusing the same imported-global registry and registering
  exactly one ordinary result by the current function's source `LirValueId`.

## Watchouts

- Reuse typed `GlobalObjectId` and ordinary `ValueId` authority; do not add a
  display/name side table or reinterpret raw load rows.
- Keep SSA/local pointer loads, raw result/pointer identities, GEP, scalar
  return, remaining function/CFG/local-object work, and other instruction
  families fail-closed.

## Proof

- Fresh focused build and executable:
  `cmake --build build --target backend_lir_to_bir_interface_test -j2` and
  `./build/tests/backend/bir/backend_lir_to_bir_interface_test`.
- Focused CTest: `backend_lir_to_bir_interface` passed 1/1.
- Fresh full build passed; full CTest passed 3033/3033 into
  `test_after.log`.
- Canonical regression comparison against `test_before.log` passed with
  equal 3033/3033 scope and no new failures or suspicious timeouts.
