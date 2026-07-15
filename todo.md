# Current Packet

Status: Active
Source Idea Path: ideas/open/827_lir_direct_scalar_binary_lhs_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace the binary-LHS producer and authority seam

## Just Finished

- Switched from Idea 825 Step 2 after recording its accepted Step 1 and the
  unaccepted shared-worktree selector slice. No semantic implementation is
  accepted by this lifecycle transition.

## Suggested Next

- Execute Step 1 only: trace the native DirectScalar `LirBinOp.lhs` producer
  and `verify_scalar_binary_lhs_authority` seam without changing preserved
  Idea 825 files.

## Watchouts

- Do not touch or credit the preserved unaccepted Idea 825 `core.cpp`,
  `ir.hpp`, `verify.cpp`, or focused-test changes.
- Do not expand into direct selector, Raw-BIR/importer, RHS, return, pointer,
  generic parameter, or presentation-derived authority routes.

## Proof

- Before acceptance, require a fresh build and exact focused
  `^frontend_lir_call_type_ref$` CTest after nearby positive/malformed
  binary-LHS coverage.
