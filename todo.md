Status: Active
Source Idea Path: ideas/open/846_lir_family_overloaded_verifier_dispatch_printer.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Decide Next 846 Packet Or 847 Handoff

# Current Packet

## Just Finished

Completed `plan.md` Step 5 selected consumer repair for `LirShuffleVectorOp`
required native-vector printer/verifier rendering.

- Updated required native-authoritative shuffle printing to render the vector
  type and mask type from the module-owned `LirVectorStoreEntry` and native
  mask lane facts reached through `native_vector_authority.vector_ref`.
- Repaired required native-authoritative shuffle verification so stale
  `vec_type.str()` and `mask_type.str()` display text is check-only when the
  module-owned vector store fact, native result shape, mask lanes, poison
  second vector, owner/result/use facts, and vector operand facts remain valid.
- Preserved the generic `require_type_ref` path for legacy/unselected
  shuffle-vector instructions without required native vector authority.
- Added focused stale-display coverage proving required shuffle printing emits
  the vector-store type and native mask type after mutating stale display text,
  plus a same-feature rejection for native mask lane count mismatch.

## Suggested Next

Execute `plan.md` Step 5 inventory again and select the next exact 846-owned
verifier/printer/dispatch consumer, or record the 847 handoff/blocker decision
if no bounded 846 consumer remains.

## Watchouts

This packet intentionally did not broaden vector verifier policy, vector
lowering, insert/extract/select/cmp/ret consumers, Raw-BIR, generic helper
deletion, or idea 847 deletion work. The stale-display allowance is limited to
required native-authoritative `LirShuffleVectorOp` instances with complete
module-owned `native_vector_authority.vector_ref`; legacy shuffle validation
still treats display mirrors as semantic input.

## Proof

Completed selected shuffle-vector packet proof:

- `cmake --build build`
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure`
  recorded in `test_after.log`
- `git diff --check`
