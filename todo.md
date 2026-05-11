Status: Active
Source Idea Path: ideas/open/168_compatibility_bridge_retirement.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Retire BIR and Backend Raw Symbol Fallbacks

# Current Packet

## Just Finished

Step 5 - Retire LIR and HIR-to-LIR Final-Output Bridges is sufficiently
exhausted for this runbook and should advance to Step 6.

Latest committed Step 5 packets covered the planned LIR/HIR-to-LIR bridge
families:

| Commit | Step 5 result |
| --- | --- |
| `cedb4c205` | Fenced `extern_decl_name_map` as a legacy raw/rendered-name compatibility and output boundary behind `LinkNameId` extern declaration dedup. |
| `2d6d63d47` | Fenced initializer reachability so structured initializer function ids seed reachability before any retained `init_text` scan. |
| `93256206f` | Fenced `type_decls` as legacy verifier/printer shadows behind structured `LirStructDecl` / `StructNameId` declarations. |
| `c86025ed3` | Fenced `signature_text` as final LLVM/output spelling plus legacy no-metadata compatibility payload, with structured signature mirrors as verifier/backend authority when present. |

Retained Step 5 bridges are now explicit compatibility, output, parity-shadow,
legacy no-metadata, or unresolved producer-boundary fallbacks. The remaining
function-reference scan of `signature_text` is not accepted as ordinary
semantic authority; it is documented as a classified compatibility fallback
because no structured producer carrier exists yet for function references
embedded in attributes/metadata.

## Suggested Next

Proceed to Step 6 - Retire BIR and Backend Raw Symbol Fallbacks.

Recommended Step 6 packet:

1. Re-read the idea 167 BIR/backend bridge classification before editing:
   raw function/global symbol maps, initializer raw symbol parsing,
   runtime/intrinsic invalid-id placeholders, imported BIR name tables,
   prepared backend link tables, and raw-only handoff points.
2. Pick one narrow BIR/backend family where present id metadata already exists.
   Prefer a call/global/initializer path that can prove present-id mismatch
   fails instead of falling back to raw spelling.
3. Inspect production callers before changing tests, and classify any retained
   raw path as invalid-id, raw-import, runtime/intrinsic, display, or explicit
   compatibility boundary.
4. Run a focused BIR/backend proof selected by the supervisor for the touched
   family.

## Watchouts

- Preserve BIR/backend runtime and intrinsic spelling where it is an explicit
  invalid-id or display/producer boundary.
- Keep route-local value, block, slot, prepared, and generated-name cleanup out
  of Step 6 unless a raw spelling is acting as source/link-visible bridge
  authority; those local identity families belong to idea 169.
- Do not claim progress through expectation weakening or grep-count reduction.
- Step 5 did not close every possible future producer-carrier gap; it made the
  retained final-output bridges explicit enough for this runbook to move on.
- The pre-existing untracked `review/168_step4_hir_bridge_route_review.md`
  remains transient and was not modified.
- No current blockers.

## Proof

Step 5 latest focused proof:
`cmake --build build --target frontend_lir_function_signature_type_ref_test backend_prepare_structured_context_test && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_function_signature_type_ref|backend_prepare_structured_context)$'`

Result: passed in `test_after.log` for commit `c86025ed3`.

Full-suite baseline reminder was accepted after the relevant Step 5 commits;
latest accepted full-suite candidate reported by the supervisor is 3135/3135 at
`c86025ed3`.

Lifecycle-only plan-owner review; no implementation validation was rerun for
this `todo.md` metadata advance.
