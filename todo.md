Status: Active
Source Idea Path: ideas/open/678_lir_to_bir_adapter_boundary_umbrella.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Closure Readiness Check

# Current Packet

## Just Finished

Executed `plan.md` Step 5 closure readiness check for
`ideas/open/678_lir_to_bir_adapter_boundary_umbrella.md`.

The umbrella content is closure-ready: the handoff directory contains the
required interface inventory, responsibility classification, and ordered
follow-up plan, all using the same evidence set and separating durable docs
from transient `build/` scan artifacts. The generated follow-up ideas cover
the documented first wave in dependency order:
`685_lir_import_context_extraction.md`,
`686_private_detail_header_contraction.md`,
`687_structured_layout_bridge_isolation.md`,
`688_initializer_lowering_bridge_isolation.md`,
`689_memory_address_provenance_import_cleanup.md`, and
`690_call_abi_import_boundary_cleanup.md`.

Close rejected for now: the required close-time regression guard did not pass.
Existing canonical logs are mismatched scopes: `test_before.log` is a full
`3397`-test run with `3395` passed and `2` failed, while `test_after.log` is a
`368`-test backend subset with `368` passed and `0` failed. The guard command
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`
returned `FAIL` because passed count decreased by `3027`. Root-level proof
logs are explicitly out of scope for this delegated packet, so no new matching
logs were generated.

## Suggested Next

Supervisor should either provide or authorize a matching close-scope
`test_before.log` / `test_after.log` pair, then re-run the close gate. If the
matching regression guard passes, close the umbrella by moving
`ideas/open/678_lir_to_bir_adapter_boundary_umbrella.md` to `ideas/closed/`
and removing `plan.md` plus `todo.md`.

## Watchouts

- No umbrella acceptance-content gap was found. The remaining blocker is the
  close-gate proof-log mismatch, not a generated-idea ordering or coverage
  defect.
- Evidence used for the readiness decision:
  `docs/lir_bir_adapter_boundary/interface_inventory_handoff.md`,
  `docs/lir_bir_adapter_boundary/responsibility_classification_handoff.md`,
  `docs/lir_bir_adapter_boundary/ordered_followup_plan.md`,
  `docs/lir_bir_adapter_boundary/step1_adapter_boundary_inventory.md`,
  `docs/lir_bir_adapter_boundary/step2_responsibility_classification.md`,
  `docs/bir_core_cleanup/`, `docs/bir_prealloc_fusion/`,
  `docs/rv64_gcc_torture_post_contract/`, and transient RV64 scan pointers
  `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv` plus
  `build/agent_state/rv64_gcc_c_torture_backend_failed.txt`.
- Handoff docs written:
  `interface_inventory_handoff.md`,
  `responsibility_classification_handoff.md`, and
  `ordered_followup_plan.md`.
- Intentionally deferred responsibilities remain assigned to future ideas:
  canonical BIR route records and query surfaces, prepared/prealloc homes and
  frame/stack/call/storage products, carriers, wrappers, MIR consumers, target
  emission policy, and any RV64/AArch64/x86-specific facts.

## Proof

Passed: umbrella acceptance content review against
`ideas/open/678_lir_to_bir_adapter_boundary_umbrella.md`.
Passed: generated follow-up order and payload review for
`ideas/open/685_lir_import_context_extraction.md` through
`ideas/open/690_call_abi_import_boundary_cleanup.md`.
Failed close gate: `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`
because the existing canonical logs compare full-suite before output against a
backend-subset after output.
