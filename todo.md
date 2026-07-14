# Current Packet

Status: Active
Packet State: Blocked on source-representation research idea 747
Source Idea Path: ideas/open/746_lir_unresolved_external_direct_call_signature_result_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace and bind the plain-DeclRef unresolved-external producer seam

## Just Finished

- Step 1 trace work established the plain-`DeclRef` unresolved-external seam in
  `StmtEmitter::resolve_call_target_info`: the `DeclRef` branch publishes the
  callee `LinkNameId`, then a missing `target_fn` leaves
  `resolve_callee_fn_ptr_sig` null, `record_extern_call_decl` populates
  `extern_decl_link_name_map`, and finalization copies it to `extern_decls`.
  `emit_call_with_result` consequently receives no structured signature and
  falls back to `fresh_tmp(ctx)`, so the first bad facts are no retained
  `FnPtrSig` and no `fresh_value(ctx)` result identity. No code authority was
  changed.

## Suggested Next

- Do not begin Step 2. Run the separate, documentation-only
  `ideas/open/747_frontend_source_representation_for_unresolved_external_direct_calls.md`
  investigation to determine whether a legal production-facing source form can
  simultaneously preserve direct global/link identity and leave `target_fn`
  absent. Resume Step 1 only if that investigation supplies a concrete native
  source/HIR carrier contract; otherwise retire or replace this runbook without
  claiming the prerequisite or idea 744 complete.

## Watchouts

- A normal fixed-empty source declaration is retained as `target_fn`, so it
  takes the local-target branch instead of the unresolved-external route. An
  undeclared source call did not retain the direct global/link identity needed
  for the probe. The idea forbids moving prototypes into external rows or
  mutating HIR to fabricate this route; do not replace this with text/printer
  assertions or testcase-shaped matching. Step 2 remains out of bounds until
  a production-facing source probe exists.

## Blocker

- Step 1 is incomplete, not structurally complete: its trace portion passed,
  but the required positive source probe did not. The declared fixed-empty
  fixture has the needed native declaration facts but follows the local-target
  branch; the undeclared fixture reaches the unresolved-external branch but
  lacks direct global/link identity. No current evidence authorizes an HIR
  fabrication, prototype relocation, or text-derived substitute. The exact
  unblocker is one production-facing source fixture satisfying both properties,
  or evidence that the frontend has no such legal source representation. That
  question is now isolated in open research idea 747. The current evidence is
  insufficient to conclude that no legal production-facing source
  representation exists: it shows only that the declared and undeclared forms
  tried so far split the required properties. A distinct source-language and
  frontend-representation investigation is required before a valid production
  route, retirement, or replacement can be selected.

## Proof

- Attempted the exact proof while evaluating the minimal probe:
  `cmake --build --preset default && ctest --test-dir build -R
  '^frontend_lir_call_type_ref$' --output-on-failure > test_after.log 2>&1`.
  The declared fixture missed `extern_decls`; the undeclared fixture missed
  native direct identity, confirming the source-probe blocker. The speculative
  test was removed. The same exact command was rerun after removal and passed;
  `test_after.log` is the passing focused-suite log. This packet remains
  blocked without a production-facing replacement probe.
