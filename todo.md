# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Shift Module Entry And Legacy Dispatch To The Replacement Graph
Plan Review Counter: 5 / 6
# Current Packet

## Just Finished

Completed a step 4 dump-surface classification packet by making
`src/backend/backend.cpp` route `BackendDumpStage::SemanticBir` and
`BackendDumpStage::PreparedBir` through explicit generic backend dump helpers,
while routing `BackendDumpStage::MirSummary` and `BackendDumpStage::MirTrace`
through an explicit target-local x86 `route_debug` helper. Added focused
backend-front-door assertions in `backend_x86_route_debug_test.cpp` proving
semantic/prepared dumps still match the generic backend printers while MIR
summary/trace still match the x86 route-debug surfaces.

## Suggested Next

Audit the remaining step 4 public entrypoints in `src/backend/backend.cpp` and
`src/backend/backend.hpp` for any legacy x86-only dispatch that still sits
behind generic naming, especially around module entry helpers that still mix
bootstrap behavior with target-local ownership.

## Watchouts

- `dump_module(...)` still lowers LIR to semantic/prepared BIR before both the
  generic and x86 MIR branches; this packet only classified ownership of the
  public dump surfaces, not the lowering pipeline itself.
- `assemble_target_lir_module(...)` remains a separate bootstrap contract from
  this dump-surface work; do not conflate the dump ownership split with object
  emission ownership without a new packet.

## Proof

Step 4 backend-front-door dump classification packet on
2026-04-23:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
