# LIR Import Context Extraction

Status: Open
Type: Implementation idea
Order: 1 of 6 in the `LIR -> BIR` adapter boundary first wave
Parent: `ideas/open/678_lir_to_bir_adapter_boundary_umbrella.md`
First Owning Layer: LIR import
Consumes:
- `docs/lir_bir_adapter_boundary/ordered_followup_plan.md`
- `docs/lir_bir_adapter_boundary/interface_inventory_handoff.md`
- `docs/lir_bir_adapter_boundary/responsibility_classification_handoff.md`
- `docs/lir_bir_adapter_boundary/step1_adapter_boundary_inventory.md`
- `docs/lir_bir_adapter_boundary/step2_responsibility_classification.md`
Related Evidence:
- `docs/bir_core_cleanup/implementation_inventory.md`
- `docs/bir_core_cleanup/destination_map.md`
- `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`
- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- `docs/bir_prealloc_fusion/phase_e5_prepared_bir_module_demotion_or_retirement_gate.md`
- `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_classification.md`
- `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`

## Goal

Extract the public and private `LIR -> BIR` import context into narrower
adapter-owned contracts without changing lowering semantics, result envelope
behavior, diagnostics, tests, expectations, unsupported markers, allowlists, or
runtime output.

## Why This Exists

The umbrella handoff identifies the public adapter entry and broad
`lowering.hpp` detail surface as the first dependency in the cleanup order.
Before structured layout, initializer, memory/provenance, call ABI, or
downstream BIR/prealloc work can be isolated safely, the adapter needs a clear
import context boundary for options, notes, prescan analysis, diagnostics,
module orchestration, and route-local state currently visible through broad
split-TU declarations.

## In Scope

- Own `src/backend/bir/lir_to_bir.hpp`,
  `src/backend/bir/lir_adapter_error.hpp`, root
  `src/backend/bir/lir_to_bir.cpp`, `analysis.cpp`, `context.cpp`,
  `module.cpp`, and selected import-local state currently declared in
  `src/backend/bir/lir_to_bir/lowering.hpp`.
- Separate public import entry behavior from private split-TU state where that
  reduces declaration width.
- Keep `ValueMap`, CFG/phi scratch maps, raw producer spelling maps, import
  diagnostics, notes, and prescan facts adapter-local.
- Preserve existing `BirLoweringOptions`, `BirLoweringResult`, notes,
  unsupported/malformed diagnostics, optional BIR result behavior, and throwing
  convenience entry semantics.

## Out Of Scope

- Editing BIR route schemas, public BIR query surfaces, printer, validator, or
  canonical semantic records.
- Editing prepared/prealloc module shape, lookup bundles, frame/stack/call or
  storage products, MIR consumers, target emission, tests, expectations,
  unsupported markers, allowlists, runtime behavior, or harness policy.
- Combining this extraction with structured layout, initializer,
  memory/provenance, or call ABI semantic repair.

## Behavior-Preserving Proof Surface

The active runbook should choose a narrow proof that at minimum compiles the
adapter and runs the focused BIR or `LIR -> BIR` test target covering the public
lowering entry and result envelope. The proof must preserve lowering notes,
diagnostics, and result envelope behavior. Broader backend proof is optional
unless the implementation touches shared non-adapter surfaces.

## Acceptance Criteria

- The public adapter entry remains the same behaviorally, including notes,
  diagnostics, optional result, and throwing convenience behavior.
- Import context state is narrower or more private than before; the change is
  not just a helper rename.
- Import-local maps and scratch state remain adapter-owned and do not move into
  public BIR, prepared/prealloc, target, or MIR ownership.
- The proving command is recorded in `todo.md` and is behavior-preserving.

## Reviewer Reject Signals

- Reject testcase-shaped shortcuts, named-case-only fixes, expectation
  rewrites, unsupported downgrades, allowlist filtering, or weaker contracts
  claimed as import-context progress.
- Reject moving raw LIR spelling maps, CFG/phi scratch state, or lowering notes
  into public BIR, prepared/prealloc, target, or MIR ownership.
- Reject a new abstraction that preserves the same broad `lowering.hpp`
  responsibility pile behind a different name.
- Reject BIR route schema, prepared publication, MIR consumer, target emission,
  or runtime behavior changes in this idea.
- Reject proof that only demonstrates a named failing case while nearby public
  adapter entry behavior is unexamined.
