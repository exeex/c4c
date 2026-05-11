Status: Active
Source Idea Path: ideas/open/168_compatibility_bridge_retirement.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Retire BIR and Backend Raw Symbol Fallbacks

# Current Packet

## Just Finished

Step 6 - Retire BIR and Backend Raw Symbol Fallbacks fenced the retained
BIR/LIR-to-BIR raw function/global symbol tables as raw-import/no-id
compatibility lookups behind `LinkNameId` authority.

This packet was behavior-preserving: `GlobalTypes` and
`FunctionSymbolSet::raw_symbol_link_name_ids` are now documented as producer
final-spelling compatibility indices for no-id imported payloads, while
present `LinkNameId` metadata remains semantic authority and fails closed on
misses instead of falling through raw symbol lookup. The backend note test
diagnostics now name raw-import/no-id compatibility explicitly for function
initializers, global loads/stores, and initializer symbols.

## Suggested Next

Continue Step 6 by reviewing whether any remaining imported BIR name-table or
backend verifier diagnostics still describe raw spellings as semantic
authority. Preserve raw-import/no-id compatibility and keep route-local
prepared identity work out of this idea.

## Watchouts

- This packet was behavior-preserving by design; it fenced an already-proven
  raw function/global symbol-table path rather than changing lowering
  semantics.
- Raw-import/no-id compatibility remains valid when no `LinkNameId` metadata is
  present.
- Present `LinkNameId` metadata must remain authoritative and fail closed on
  miss for function initializers, global references, direct calls, and
  initializer symbols.
- Keep route-local value, block, slot, prepared, and generated-name cleanup out
  of Step 6 unless a raw spelling is acting as source/link-visible bridge
  authority; those local identity families belong to idea 169.
- The pre-existing untracked `review/168_step4_hir_bridge_route_review.md`
  remains transient and was not modified.
- No current blockers.

## Proof

Step 6 focused proof:
`cmake --build build --target backend_lir_to_bir_notes_test && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_notes$'`

Result: passed in `test_after.log`.

Additional check: `git diff --check` passed.
