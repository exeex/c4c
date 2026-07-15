Status: Active
Source Idea Path: ideas/open/799_lir_amd64_vaarg_overflow_aggregate_carrier_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish and verify the native carrier boundary

# Current Packet

## Just Finished

- 799 Step 2 complete: selected AMD64 SysV aggregate
  `layout.needs_memory` overflow lowering now publishes one optional
  `LirAmd64SysVOverflowAggregateCarrier` on its non-volatile memcpy.  It
  binds the direct-local `va_list` authority, typed field-2 GEP result, ptr
  load/source identity, overflow-area storage kind, destination alloca,
  aggregate payload type, typed positive i64 byte size, and final load.
- The LIR verifier fails closed for carrier partial/unselected fields,
  malformed or non-derived field/load chains, foreign/dead local facts,
  destination disagreement, and type/size incoherence.  Focused coverage
  includes the accepted direct-local aggregate route plus malformed source,
  owner, liveness, type, and size mutations.

## Suggested Next

- Supervisor should select the next bounded active-plan packet; this carrier
  slice intentionally leaves all other va_arg and memory routes compatibility-only.

## Watchouts

- The selection gate remains AMD64 SysV + aggregate + positive
  `layout.needs_memory` + checked direct-local `va_list`; registers, scalars,
  other targets, and indirect/nonlocal va_list routes remain compatibility-only.

## Proof

- Focused: `cmake --build --preset default &&
  ./build/tests/backend/bir/backend_lir_selected_pointer_authority_test` passed.
  Matching backend baseline is recorded in `test_after.log`.
