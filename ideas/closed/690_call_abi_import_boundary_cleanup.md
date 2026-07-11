# Call ABI Import Boundary Cleanup

Status: Closed
Type: Implementation idea
Order: 6 of 6 in the `LIR -> BIR` adapter boundary first wave
After: `ideas/open/689_memory_address_provenance_import_cleanup.md`
Parent: `ideas/open/678_lir_to_bir_adapter_boundary_umbrella.md`
First Owning Layer: Call ABI import
Consumes:
- `docs/lir_bir_adapter_boundary/ordered_followup_plan.md`
- `docs/lir_bir_adapter_boundary/interface_inventory_handoff.md`
- `docs/lir_bir_adapter_boundary/responsibility_classification_handoff.md`
- `docs/lir_bir_adapter_boundary/step1_adapter_boundary_inventory.md`
- `docs/lir_bir_adapter_boundary/step2_responsibility_classification.md`
Related Evidence:
- `docs/bir_core_cleanup/implementation_inventory.md`
- `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`
- `docs/bir_prealloc_fusion/phase_e5_prepared_bir_module_demotion_or_retirement_gate.md`
- `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_classification.md`
- `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`

## Goal

Clean up the call and return ABI import boundary so signature, direct call,
call return, byval, vararg, HFA, inline asm, runtime call, and call/intrinsic
metadata are admitted from LIR into semantic BIR without taking ownership of
prepared call plans, physical register placement, outgoing stack layout,
wrappers, helper protocols, or target emission.

## Why This Exists

The Step 3 classification identifies call ABI import as an adapter bridge. It
must remain distinct from downstream prepared/prealloc and target call
implementation so future backend progress is not claimed through target-shaped
shortcuts inside the adapter or through prepared call-plan changes hidden under
import cleanup.

## In Scope

- Own `src/backend/bir/lir_to_bir/call_abi.cpp`,
  `src/backend/bir/lir_to_bir/calling.cpp`, call and return ABI metadata
  import paths, inline asm admission, runtime call admission, intrinsic call
  metadata import, and related adapter-private declarations.
- Narrow or clarify adapter-owned ABI admission contracts.
- Preserve existing BIR output, prepared call behavior, target emission,
  diagnostics, object output, and runtime behavior.

## Out Of Scope

- Prepared call plans, physical register placement, outgoing stack layout,
  aggregate lane transport, wrappers, runtime helper protocols, target
  emission, MIR consumers, tests, expectations, unsupported markers,
  allowlists, runtime behavior, or harness policy.
- Combining call ABI import cleanup with BIR route schema changes, prepared
  publication changes, target ABI repair, or named backend case repair.

## Behavior-Preserving Proof Surface

Use compile proof plus existing call/ABI adapter coverage selected by the
active runbook. If the future packet touches prepared call plans or target ABI
placement, split that downstream work into a separate idea and escalate proof.

## Acceptance Criteria

- Call ABI import contracts are narrower or clearer inside the adapter.
- Adapter code admits stable semantic BIR call facts without owning prepared or
  target call-placement policy.
- Prepared call plans, wrappers, helper protocols, target emission, object
  output, and runtime behavior remain unchanged.
- Proof is recorded in `todo.md`.

## Completion Notes

Closed after Step 5 final proof. The completed slice narrowed or clarified
adapter-owned call ABI import boundaries in `call_abi.cpp` and `calling.cpp`
without changing prepared call plans, wrappers, helper protocols, target
emission, object output, runtime behavior, tests, expectations, unsupported
markers, allowlists, or harness policy.

Close proof used the focused call/ABI adapter subset recorded in
`test_before.log` and refreshed in `test_after.log`; the close-time regression
guard passed with no new failures and no pass-count loss.

## Reviewer Reject Signals

- Reject physical register placement, outgoing stack layout, aggregate lane
  transport, wrapper, helper protocol, target emission, tests, expectations,
  unsupported-marker, allowlist, runtime, or object-output changes in this
  idea.
- Reject testcase-shaped ABI matching, named-case-only fixes, expectation
  rewrites, unsupported downgrades, or weaker proof commands.
- Reject moving prepared call-plan authority into the adapter or moving raw LIR
  call compatibility maps into public BIR/prepared/target/MIR ownership.
- Reject helper renames or classification-only edits that retain the same call
  ABI import coupling.
- Reject proof that omits call or ABI adapter coverage when those paths were
  touched.
