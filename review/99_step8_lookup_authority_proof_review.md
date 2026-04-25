# Idea 99 Step 8 Lookup Authority Proof Review

## Scope

- Delegated role: `c4c-reviewer`
- Requested diff: `863b2fea..HEAD`
- Chosen review base: `863b2fea` (`[plan] Gate idea 99 fallback demotion on targeted proof`)
- Reason: this is the plan checkpoint that explicitly split Step 8 targeted proof from any Step 9 fallback demotion and updated `todo.md` to require authority distinction before demotion.
- Commits since base: 1 (`7fb45211 instrument hir module lookup authority proof`)
- Report path: `review/99_step8_lookup_authority_proof_review.md`

I used direct diff review rather than `c4c-clang-tools`; the changed resolver bodies and tests are compact enough for direct inspection.

## Findings

No blocking implementation findings.

Step 8 adds explicit authority classification without changing resolver precedence:

- `ModuleDeclLookupAuthority` distinguishes `Structured`, `LegacyRendered`, `ConcreteGlobalId`, and `LinkNameId` in `src/frontend/hir/hir_ir.hpp:329`.
- Function resolution records `LinkNameId`, structured, and legacy-rendered hits while preserving the existing function precedence of local/param/global exclusion, link-name bridge, structured lookup, then legacy rendered fallback in `src/frontend/hir/hir_ir.hpp:1224` and `src/frontend/hir/hir_ir.hpp:1251`.
- Global resolution records concrete `GlobalId`, `LinkNameId`, structured, and legacy-rendered hits while preserving concrete ID first, link-name bridge next, then structured/legacy parity behavior in `src/frontend/hir/hir_ir.hpp:1323` and `src/frontend/hir/hir_ir.hpp:1357`.
- HIR dump output now prints accumulated lookup hits and qualified-ref inline annotations in `src/frontend/hir/impl/inspect/printer.cpp:174` and `src/frontend/hir/impl/inspect/printer.cpp:545`.
- The direct frontend test exercises structured function/global, legacy-rendered function/global, concrete global-ID, and function/global `LinkNameId` resolver hits in `tests/frontend/frontend_hir_lookup_tests.cpp:72`.
- The HIR fixture assertion now requires hit output for the real namespace-qualified case and still fails on parity mismatches in `tests/cpp/internal/hir_case/CMakeLists.txt:331`.

## Assessment

Step 8 provides enough targeted evidence to answer the Step 9 route question, but that answer is not "demote rendered fallback now."

The proof is targeted and useful: it distinguishes structured, legacy-rendered, concrete/global-ID, and `LinkNameId` hits for module function/global paths, and the implementation does not downgrade expectations or add testcase-shaped semantic shortcuts. The new frontend-only unit test is synthetic, but it tests resolver authority mechanics directly rather than tailoring production lowering to a named testcase. The HIR dump fixture gives a supervisor-reviewable artifact for real lowered module refs and preserves the parity-mismatch guard.

The evidence also shows rendered fallback remains an active compatibility path. `todo.md` still records known metadata gaps: hoisted compound-literal globals lack declaration metadata, and refs with incomplete qualifier text IDs intentionally fall back to rendered lookup. Because Step 8 directly proves legacy-rendered function and global hits are still valid behavior, immediate fallback removal would be premature unless Step 9 is explicitly narrowed to auditing/remediating those legacy-only cases first.

## Judgments

- Plan alignment: on track
- Idea alignment: matches source idea
- Technical debt: acceptable
- Validation sufficiency: narrow proof sufficient for Step 8 review; broader proof needed before any actual fallback demotion
- Testcase-overfit: not present

## Recommendation

Narrow next packet.

Keep Step 9 fallback demotion parked for now. If the supervisor chooses to start Step 9, it should begin as a narrow audit/demotion-readiness packet that enumerates remaining legacy-rendered hits and metadata gaps, not a direct removal or weakening of rendered fallback.
