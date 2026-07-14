# Current Packet

Status: Active
Source Idea Path: ideas/open/747_frontend_source_representation_for_unresolved_external_direct_calls.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Author the required research delivery

## Just Finished

- Plan Step 2 is complete: authored exactly the required index and numbered
  answer under `docs/frontend_unresolved_external_direct_call_representation/`.
  The answer records the parser/sema/HIR/LIR evidence, classifies each legal
  direct-call candidate, and reaches only the supported-surface absence
  conclusion: no production-facing plain direct fixed-empty scalar-call
  carrier combines native global/link identity with absent `target_fn`.

## Suggested Next

- Hand the completed research evidence to the supervisor for a separate
  lifecycle decision on idea 746. This delivery does not itself unblock,
  complete, supersede, retire, or close ideas 746 or 744.

## Watchouts

- Idea 746 remains open and blocked. Do not treat this research activation as
  an implementation authorization or as proof that idea 746 or idea 744 is
  unblocked, complete, retired, replaced, or superseded.
- Do not change source, tests, expectations, runtime behavior, HIR, prototype
  placement, authority tables, unsupported markers, allowlists, or lifecycle
  history. Do not derive any route from text/name/type/result recovery.
- Evidence anchors for the Step 2 citations: direct-call `DeclRef` creation
  and link-ID attachment are in `src/frontend/hir/impl/expr/call.cpp`;
  function lookup/indexing is in `src/frontend/hir/hir_ir.hpp` and
  `src/frontend/hir/hir_functions.cpp`; local/parameter function-pointer
  signatures are confined to `lower_call_expr`; and LIR target/signature and
  extern-declaration handling are in `src/codegen/lir/hir_to_lir/call/target.cpp`,
  `core.cpp`, `ir.hpp`, and `hir_to_lir.cpp`.

## Proof

- Passed: `git diff --check && test "$(find
  docs/frontend_unresolved_external_direct_call_representation -maxdepth 1
  -type f -name '*.md' | wc -l)" -eq 2 && test -f
  docs/frontend_unresolved_external_direct_call_representation/index.md &&
  test -f docs/frontend_unresolved_external_direct_call_representation/01_legal_source_to_hir_representation.md
  && rg -n "01_legal_source_to_hir_representation\.md"
  docs/frontend_unresolved_external_direct_call_representation/index.md`.
  No build, test, or root proof log was required for this documentation-only
  packet.
