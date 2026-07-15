# Current Packet

Status: Active
Source Idea Path: ideas/open/762_lir_module_declaration_type_shadow_convergence.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Prove general function-signature lowering is structured-first

## Just Finished

Step 6 complete — `lower_minimal_global` now derives scalar, array, aggregate,
and initializer type handling from `llvm_type_ref::render_llvm()` whenever
metadata is present; `llvm_type` remains the explicit absent-metadata
compatibility path. Nearby global coverage rejects a stale rendered scalar
shadow against complete structured metadata transactionally.

## Suggested Next

Execute Step 7: audit general signature return/parameter lowering and its
declaration/definition callers; correct only a present-metadata text bypass
and add a nearby non-aggregate stale-signature-shadow case with matching
before/after proof.

## Watchouts

Keep `signature_text` only as output or an explicit absent-metadata
compatibility path. Do not absorb signature rendering changes,
function-reference reachability scans, globals, Raw-BIR, target lowering, or
unrelated call semantics.

## Proof

Required: fresh build plus selected exact signature frontend/backend tests
before and after the packet; retain matching evidence for supervisor regression
comparison when the selected route merits it.
