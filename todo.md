# Current Packet

Status: Active
Source Idea Path: ideas/open/741_lir_structured_operand_and_terminator_identity_decomposition.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Prove and hand back to idea 734

## Just Finished

- Plan Step 6 completes the final structured-identity audit and exact handoff
  to open idea 734.
- The matrix mechanically matches all 38 current `LirInst` alternatives and
  all 6 `LirTerminator` alternatives with no catch-all or omitted variant. Its
  current-status tables distinguish the four authoritative producer shapes
  from every raw/monostate compatibility row still outside idea 741.
- CC-STORE-1, CC-LOAD-1, CC-GEP-1, and CC-RET-1 each record their exact native
  authority, producer seam, reachable verifier obligation, focused proof, and
  presentation-only display status.
- `docs/lir_structured_identity/handoff_to_734.md` names the exact store, load,
  GEP, and scalar-return receipt subrows now unblocked and the remaining
  receiver-owned containers, mapping, verifier, importer, and transactionality
  work. It does not claim importer implementation or idea-734 completion.

## Suggested Next

- Ask plan-owner for the lifecycle closure/deactivation decision for exhausted
  idea 741 and, if accepted, reactivate open idea 734 at its blocked Step 4
  function-body receipt boundary.

## Watchouts

- Runbook exhaustion does not itself close idea 741; lifecycle disposition
  remains plan-owner/supervisor work.
- “Unblocked” is deliberately subrow-specific. Other modern instruction and
  terminator producers still carry raw/monostate compatibility and must not be
  received by parsing display; they need a separate producer-identity
  initiative if idea 734 reaches them.
- Active new-BIR behavior is unchanged: store/load/GEP remain
  `UnsupportedOrdinaryInstruction`, and scalar return remains
  `InvalidVoidReturn`. Idea 734 owns all receiving implementation.

## Proof

- `cmake --build --preset default` passed.
- `ctest --test-dir build -R
  '^(frontend_lir_call_type_ref|backend_lir_to_bir_interface)$'
  --output-on-failure` passed 2/2.
- The four Step 3 focused `--dump-bir` probes retained exact boundaries:
  store/load/GEP are `UnsupportedOrdinaryInstruction`; scalar return is
  `InvalidVoidReturn`. Focused `--codegen llvm` retained the store/load/GEP/ret
  capability observations, while native authority evidence remains the C++
  structural tests rather than rendered output.
- Exact full proof `ctest --test-dir build -j --output-on-failure >
  test_after.log` passed 3033/3033, matching `test_before.log`. `git diff
  --check` passed.
