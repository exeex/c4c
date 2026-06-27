# Prepared Contract Taxonomy And Fact Family Matrix

Status: Published for idea 413 Step 4
Source Idea: `ideas/open/413_prepared_contract_verifier_and_owner_taxonomy.md`

This document is the row-level vocabulary for the prepared contract verifier
introduced by idea 413. Idea 418 should consume these rows when auditing target
prepared consumers. Ideas 414 through 417 should cite the rows that define the
families they migrate.

## Verifier Report Surface

The shared report API lives in
`src/backend/prealloc/prepared_contract_verifier.hpp` and currently exposes:

- `PreparedContractOwnerClass`: `coherent`, `producer_missing`,
  `producer_incoherent`, `target_unsupported_but_coherent`, and
  `pre_prepared_semantic_failure`.
- `PreparedContractFactFamily`: `value_home_typed_storage`,
  `call_boundary_argument_result_plan`, and
  `variadic_entry_helper_operand_homes`.
- `PreparedContractVerificationReport`: `fact_family`, `owner_class`,
  optional function/value identity fields, `fail_closed`, and diagnostic
  `detail`.

Target diagnostics carry the report through
`ModuleLoweringDiagnostic::prepared_contract_report` in
`src/backend/mir/aarch64/module/module.hpp`. The selected AArch64 consumers
attach the report at existing fail-closed rejection points instead of inferring
missing prepared facts in target code.

## Owner Taxonomy

| Row ID | Owner class | Meaning | Required consumer behavior | Current evidence |
| --- | --- | --- | --- | --- |
| `TAX-OWNER-COHERENT` | `coherent` | The prepared producer supplied a complete fact that the selected consumer can lower. | Continue lowering through the normal prepared consumer path. | `PreparedContractOwnerClass::Coherent` and `fail_closed = false` in `src/backend/prealloc/prepared_contract_verifier.hpp`. |
| `TAX-OWNER-PRODUCER-MISSING` | `producer_missing` | The target-required prepared fact is absent. The producer or prepared publication route owns the defect. | Fail before target recovery or target-local guessing. | Missing value authority, missing call plans/bindings, and missing variadic entry/helper facts classify this way in `src/backend/prealloc/prepared_contract_verifier.cpp`. |
| `TAX-OWNER-PRODUCER-INCOHERENT` | `producer_incoherent` | A prepared fact exists but contradicts itself or lacks required payload for its selected shape. | Fail closed and route the fix to the prepared producer or contract normalizer. | Missing register placement, mismatched call result plans, missing ABI indices, and incomplete variadic helper homes classify this way in `src/backend/prealloc/prepared_contract_verifier.cpp`. |
| `TAX-OWNER-TARGET-UNSUPPORTED-COHERENT` | `target_unsupported_but_coherent` | The producer fact is coherent, but the selected target consumer does not yet support that coherent route. | Report target unsupported status without blaming the producer and without weakening the prepared contract. | Unsupported value-home/storage encodings and unsupported call-boundary move op kinds classify this way in `src/backend/prealloc/prepared_contract_verifier.cpp`. |
| `TAX-OWNER-PRE-PREPARED-SEMANTIC` | `pre_prepared_semantic_failure` | The failure is upstream of prepared publication, such as invalid BIR semantics or source-level admission. | Do not hide it in target lowering; route to the semantic producer that should have rejected or normalized before prepared facts. | Reserved by the enum in `src/backend/prealloc/prepared_contract_verifier.hpp`; no selected Step 2/3 family emits this owner yet. |

## Selected Fact-Family Rows

| Row ID | Fact family | Implemented enum family | Current owner decisions | Example diagnostics/report surfaces | Source evidence | Downstream consumers |
| --- | --- | --- | --- | --- | --- | --- |
| `TAX-FAM-VALUE-HOME-TYPED-STORAGE-001` | Value home / typed storage authority | `value_home_typed_storage` | Missing decoded authority is `producer_missing`; present value-home or storage-plan facts with missing register/frame/immediate/symbol payload are `producer_incoherent`; coherent but unsupported encodings are `target_unsupported_but_coherent`. | AArch64 `resolve_value_operand` verifies decoded home storage before materializing target operands. A fail-closed report is attached to `ModuleLoweringDiagnostic::prepared_contract_report`. | `verify_prepared_decoded_home_storage_contract` in `src/backend/prealloc/prepared_contract_verifier.cpp`; `resolve_value_operand` and `diagnose_decoded_failure` in `src/backend/mir/aarch64/codegen/operands.cpp`; tests in `tests/backend/bir/backend_prealloc_decoded_home_storage_test.cpp` and `tests/backend/mir/backend_aarch64_operand_resolution_test.cpp`. | Idea 418 audit row input; idea 415 consumes for value materialization contract boundaries; idea 417 may cite where local storage facts feed value-home storage. |
| `TAX-FAM-CALL-BOUNDARY-ARG-RESULT-001` | Call-boundary argument/result plans | `call_boundary_argument_result_plan` | Missing call argument plans, result plans, and ABI bindings are `producer_missing`; missing ABI index and mismatched result plan authority are `producer_incoherent`; unsupported move op kinds are `target_unsupported_but_coherent`. | AArch64 before-call and after-call move rejection attaches the shared report through `failed_call_boundary_contract_report` on the normal call diagnostic path. | `verify_prepared_call_boundary_move_contract` in `src/backend/prealloc/prepared_contract_verifier.cpp`; report attachment in `src/backend/mir/aarch64/codegen/calls.cpp`; tests in `tests/backend/bir/backend_prealloc_call_boundary_classification_test.cpp` and `tests/backend/mir/backend_aarch64_call_boundary_owner_test.cpp`. | Idea 418 audit row input; idea 414 must consume for typed call argument routes and call-result plan cleanup. |
| `TAX-FAM-VARIADIC-HELPER-OPERAND-HOMES-001` | Variadic entry helper operand homes and access plans | `variadic_entry_helper_operand_homes` | Missing variadic entry plans, missing required entry facts, and missing helper operand-home/access-plan records are `producer_missing`; present but incomplete or wrong-helper records are `producer_incoherent`. | AArch64 variadic entry helper lowering attaches a report for missing/incomplete entry plans. AArch64 call lowering attaches a report when helper operand-home/access-plan facts are missing or incomplete before helper lowering proceeds. | `verify_prepared_variadic_entry_plan_contract` and `verify_prepared_variadic_entry_helper_operand_homes_contract` in `src/backend/prealloc/prepared_contract_verifier.cpp`; report attachment in `src/backend/mir/aarch64/codegen/variadic.cpp` and `src/backend/mir/aarch64/codegen/calls.cpp`; tests in `tests/backend/bir/backend_prealloc_call_boundary_classification_test.cpp` and `tests/backend/mir/backend_aarch64_operand_resolution_test.cpp`. | Idea 418 audit row input; idea 416 must consume for typed variadic/helper operand contracts. |

## Placeholder Fact-Family Rows

These rows reserve stable IDs for in-scope families that were inventoried by
idea 413 but not fully implemented by the selected Step 2/3 verifier slice.
Downstream ideas may refine these rows after idea 418 audits concrete target
consumers.

| Row ID | Placeholder family | Intended owner boundary | Initial classification rule | Expected follow-up |
| --- | --- | --- | --- | --- |
| `TAX-FAM-CALL-ARG-TYPED-ROUTES-PLACEHOLDER-001` | Typed call-argument source routes, including preservation, frame-slot address/value, local address materialization, byval lanes, and outgoing call storage. | Prepared call-route producer owns required route fields; target consumers own only lowering coherent route payloads. | Missing route payload is `producer_missing`; contradictory optional combinations are `producer_incoherent`; route present but unsupported on a target is `target_unsupported_but_coherent`. | Idea 418 should audit target call consumers, then idea 414 should cite this row and `TAX-FAM-CALL-BOUNDARY-ARG-RESULT-001`. |
| `TAX-FAM-VALUE-MATERIALIZATION-PLACEHOLDER-001` | Rematerialized immediates, pointer base-plus-offset facts, and selected same-block producer chains. | Prepared materialization producers own width, signedness, source identity, and scheduling authority. | Missing materialization fact is `producer_missing`; width/signedness/source contradictions are `producer_incoherent`; unsupported but explicit materialization forms are `target_unsupported_but_coherent`; invalid BIR expression semantics are `pre_prepared_semantic_failure`. | Idea 418 should identify raw BIR expression recovery, then idea 415 should cite this row and `TAX-FAM-VALUE-HOME-TYPED-STORAGE-001`. |
| `TAX-FAM-MEMORY-ACCESS-PLACEHOLDER-001` | Prepared memory accesses: base, offset, size, alignment, provenance, selected load/store authority, and access plan shape. | Prepared addressing/storage producers own object identity and byte-range authority; targets own only supported addressing-mode selection over coherent facts. | Missing base/range/provenance is `producer_missing`; contradictory size/alignment/range facts are `producer_incoherent`; valid but unsupported addressing forms are `target_unsupported_but_coherent`. | Idea 418 should audit target memory consumers; idea 417 should cite this row for storage and memory-access normalization. |
| `TAX-FAM-HELPER-OPERAND-TYPED-PLACEHOLDER-001` | Typed helper and intrinsic operand contracts beyond the initial variadic helper surface. | Prepared helper producers own helper-specific operand homes and access plans. | Missing helper operand homes are `producer_missing`; wrong helper kind or incomplete per-helper payloads are `producer_incoherent`; coherent helper forms without target lowering are `target_unsupported_but_coherent`. | Idea 418 should audit helper lowering paths; idea 416 should cite this row and `TAX-FAM-VARIADIC-HELPER-OPERAND-HOMES-001`. |
| `TAX-FAM-PUBLICATION-FACTS-PLACEHOLDER-001` | Prepared publication facts: control-flow edge/block publication, selected node publication, object-data publication, and global initializer admission. | Prepared publication producers own publication completeness and consistency before target consumers read published tables. | Missing publication entries are `producer_missing`; stale or contradictory publication entries are `producer_incoherent`; coherent published facts that a target cannot lower are `target_unsupported_but_coherent`. | Idea 418 should audit publication consumers. Ideas 415 and 417 should cite this row when materialization, storage, object-data, or initializer facts depend on publication completeness. |
| `TAX-FAM-GLOBAL-INITIALIZER-STORAGE-PLACEHOLDER-001` | Global storage and initializer facts: emitted bytes, relocations, zero-fill, symbol references, section ownership, and unsupported object-data forms. | Prepared global/object-data producers own emitted data facts; targets own object emission for coherent data facts only. | Missing emitted bytes/relocations/zero-fill facts are `producer_missing`; contradictory range/relocation facts are `producer_incoherent`; known coherent initializer forms without target support are `target_unsupported_but_coherent`; invalid initializer semantics are `pre_prepared_semantic_failure`. | Idea 418 should audit global/object-data target consumers; idea 417 should cite this row for storage and initializer contract work. |

## Diagnostic Surface Rules

- A non-coherent report must set `fail_closed = true` before the target
  consumer tries to recover the fact.
- `producer_missing` and `producer_incoherent` must not be reclassified as
  target unsupported just because the failing diagnostic is emitted from target
  lowering.
- `target_unsupported_but_coherent` is reserved for facts that are complete and
  internally consistent, but whose selected route is not implemented by the
  target consumer.
- A target consumer may attach a report at an existing fail-closed rejection
  site, but it must not synthesize ABI homes, helper operand addresses,
  aggregate/storage layout, initializer bytes, or BIR expression semantics.

## Downstream Row Handoff

| Downstream idea | Required taxonomy rows to cite first | Notes |
| --- | --- | --- |
| Idea 418: target consumer boundary audit | All rows in this document. | Use these row IDs to classify audited RV64/AArch64 target helpers before producing `target_consumer_boundary_audit.md`. |
| Idea 414: typed prepared call argument contracts | `TAX-FAM-CALL-BOUNDARY-ARG-RESULT-001`, `TAX-FAM-CALL-ARG-TYPED-ROUTES-PLACEHOLDER-001`, and any idea 418 audit rows for call-source recovery. | Do not add target fallback inference for formal homes, ABI indices, stack offsets, or register banks. |
| Idea 415: prepared value materialization contracts | `TAX-FAM-VALUE-HOME-TYPED-STORAGE-001`, `TAX-FAM-VALUE-MATERIALIZATION-PLACEHOLDER-001`, `TAX-FAM-PUBLICATION-FACTS-PLACEHOLDER-001`, and any idea 418 audit rows for BIR expression recovery. | Distinguish target scheduling over explicit facts from target reconstruction of missing producer facts. |
| Idea 416: prepared helper operand home contracts | `TAX-FAM-VARIADIC-HELPER-OPERAND-HOMES-001`, `TAX-FAM-HELPER-OPERAND-TYPED-PLACEHOLDER-001`, and any idea 418 audit rows for helper address/home recovery. | Preserve the initial variadic missing/incoherent classification while splitting typed per-helper payloads. |
| Idea 417: prepared storage layout and initializer contracts | `TAX-FAM-MEMORY-ACCESS-PLACEHOLDER-001`, `TAX-FAM-GLOBAL-INITIALIZER-STORAGE-PLACEHOLDER-001`, `TAX-FAM-PUBLICATION-FACTS-PLACEHOLDER-001`, and storage-relevant uses of `TAX-FAM-VALUE-HOME-TYPED-STORAGE-001`. | Keep layout, aliasing, range, initializer bytes, relocations, and zero-fill as prepared facts, not target-side source reconstruction. |

## Current Proof References

- Step 2/3 backend proof: accepted backend validation recorded 326/326 backend
  tests passed after the selected verifier/report wiring before supervisor
  regression-log roll-forward.
- Docs-only Step 4 proof is this artifact plus file-reference evidence from
  `plan.md`, `todo.md`, `src/backend/prealloc/prepared_contract_verifier.*`,
  AArch64 diagnostic attachment sites, and focused backend tests.
