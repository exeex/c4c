# Current Packet

Status: Active
Source Idea Path: ideas/open/762_lir_module_declaration_type_shadow_convergence.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Prove global type consumers use the structured global type ref

## Just Finished

Step 5 complete — aggregate parameter collection now treats paired
`signature_params`/`signature_param_type_refs` as structured authority only
when complete; rendered `signature_text` is the legacy fallback for absent or
incomplete metadata. Backend coverage supplies a syntactically valid,
conflicting parameter shadow while preserving the complete structured ABI.

## Suggested Next

Execute Step 6: audit one exact-identity global consumer seam, correct it only
if a present `llvm_type_ref` can be bypassed by `llvm_type`, and add nearby
stale-shadow coverage with matching before/after proof.

## Watchouts

Keep scope to global type authority. `llvm_type` may remain only for explicit
legacy/no-metadata compatibility or emission; do not absorb signature
rendering, function-reference reachability scans, Raw-BIR, target lowering, or
unrelated global initializer semantics.

## Proof

Required: fresh build plus selected exact global test before and after the
packet; retain matching evidence for supervisor regression comparison when the
selected route merits it.
