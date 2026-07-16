Status: Active
Source Idea Path: ideas/open/846_lir_family_overloaded_verifier_dispatch_printer.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Decide Next 846 Packet Or 847 Handoff

# Current Packet

## Just Finished

Completed `plan.md` Step 5 selected consumer repair for `LirExtractElementOp`
native-vector printer/verifier rendering.

- Updated native-vector extract-element printing to render the vector type from
  the module-owned `LirVectorStoreEntry` reached through
  `native_vector_authority.vector_ref`.
- Repaired native-vector extract-element verification so stale `vec_type.str()`
  display text is check-only when the module-owned vector store fact,
  native result shape, index type, owner/result/use facts, and vector operand
  facts remain valid.
- Preserved the generic `require_type_ref` path for legacy/unselected
  extract-element instructions without native vector authority.
- Added focused stale-display coverage proving extract-element printing emits
  the vector-store type after mutating stale display text, plus a same-feature
  rejection for vector-store/result-shape mismatch.

## Suggested Next

Execute `plan.md` Step 5 inventory again and select the next exact 846-owned
verifier/printer/dispatch consumer, or record the 847 handoff/blocker decision
if no bounded 846 consumer remains.

## Watchouts

This packet intentionally did not broaden vector verifier policy, vector
lowering, insert/shuffle/select/cmp/ret consumers, Raw-BIR, generic helper
deletion, or idea 847 deletion work. The stale-display allowance is limited to
`LirExtractElementOp` instances with complete module-owned
`native_vector_authority.vector_ref`; legacy extract-element validation still
treats display mirrors as semantic input.

## Proof

Completed selected extract-element packet proof:

- `cmake --build build`
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure`
  recorded in `test_after.log`
- `git diff --check`
