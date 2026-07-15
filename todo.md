# Current Packet

Status: Active
Source Idea Path: ideas/open/809_lir_ternary_phi_incoming_operand_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace the returned-operand to PHI-incoming construction seam

## Just Finished

- Active plan switched from 807 Step 2 after its unaccepted `fresh_value(ctx)`
  probe showed native `fneg` authority is dropped when ternary lowering
  constructs the matching PHI incoming. No blocker implementation has begun.

## Suggested Next

- Execute Step 1: trace the returned `LirOperand` to the ternary
  `LirPhiOp::incoming` construction and identify the exact omitted `value_id`
  handoff plus nearby preservation/rejection coverage.

## Watchouts

- Keep the parked 807 `src/codegen/lir/hir_to_lir/expr/misc.cpp` hunk intact
  but do not accept, enlarge, or move it. Do not alter producer creation, PHI
  schema/verifier behavior, other unary families, CFG semantics, or rendered
  text identity.

## Proof

- Discovery evidence only: fresh `cmake --build --preset default` succeeded;
  the focused `ctest --test-dir build -j --output-on-failure -R
  '^frontend_lir_call_type_ref$'` failed only a temporary positive assertion,
  and temporary test edits were removed. Before acceptance, require a fresh
  build and focused blocker proof selected by the supervisor.
