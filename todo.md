Status: Active
Source Idea Path: ideas/open/846_lir_family_overloaded_verifier_dispatch_printer.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Decide Next 846 Packet Or 847 Handoff

# Current Packet

## Just Finished

Completed `plan.md` Step 5 selected consumer repair for `LirInsertElementOp`
required native-vector authority.

- Updated required native-vector insert-element printing to render vector and
  element types from the module-owned `LirVectorStoreEntry` reached through
  `native_vector_authority.vector_ref`.
- Repaired required native-vector insert-element verification so stale
  `vec_type.str()` / `elem_type.str()` display mirrors are check-only when the
  module-owned vector store fact, native element type fact, vector shape,
  owner/result/use facts, and zero index all match.
- Preserved the generic `require_type_ref` path for legacy/unselected
  insert-element instructions without required native-vector authority.
- Added focused stale-display coverage proving required insert-element printing
  emits the vector-store shape/element type after mutating stale display text.

## Suggested Next

Execute `plan.md` Step 5 inventory again and select the next exact 846-owned
verifier/printer/dispatch consumer, or record the 847 handoff/blocker decision
if no bounded 846 consumer remains.

## Watchouts

This packet intentionally did not broaden vector verifier policy, vector
lowering, extract/shuffle/select/cmp/ret consumers, Raw-BIR, generic helper
deletion, or idea 847 deletion work. The stale-display allowance is limited to
the selected `requires_native_vector_authority` insert-element path with a
module-owned complete `native_vector_authority.vector_ref`; legacy
insert-element validation still treats display mirrors as semantic input.

## Proof

Completed selected required insert-element packet proof:

- `cmake --build build`
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure`
  recorded in `test_after.log`
- `git diff --check`
