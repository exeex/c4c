Status: Active
Source Idea Path: ideas/open/846_lir_family_overloaded_verifier_dispatch_printer.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Decide Next 846 Packet Or 847 Handoff

# Current Packet

## Just Finished

Completed `plan.md` Step 5 selected consumer repair for `LirInsertValueOp`
native aggregate verifier/printer rendering.

- Updated selected native-authoritative insertvalue verification so stale
  aggregate and element display text are check-only while native aggregate
  result facts, ordered fields, selected element facts, and result authority
  remain fail-closed.
- Updated selected native-authoritative insertvalue printing to render the
  aggregate and element types from `aggregate_result_type` native facts instead
  of stale `agg_type.str()` and `elem_type.str()` display text.
- Preserved legacy/unselected insertvalue validation and printing through the
  generic `require_type_ref` path, and left extractvalue behavior unchanged.
- Added focused stale-display coverage proving selected insertvalue accepts
  stale aggregate/element display mirrors only when native facts remain valid
  and `print_llvm` emits the native aggregate/element types, not stale text.

## Suggested Next

Execute `plan.md` Step 5 inventory again and select the next exact 846-owned
verifier/printer/dispatch consumer, or record the 847 handoff/blocker decision
if no bounded 846 consumer remains.

## Watchouts

This packet intentionally did not broaden extractvalue behavior, vector/select/
cmp/ret/switch consumers, Raw-BIR/backend/lowering, generic helper deletion, or
idea 847 deletion work. The stale-display allowance is limited to selected
native-authoritative `LirInsertValueOp` instances with complete ordered native
aggregate result fields and matching selected element facts; legacy insertvalue
validation still treats display mirrors as semantic input.

## Proof

Completed selected insertvalue packet proof:

- `cmake --build build`
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure`
  recorded in `test_after.log`
- `git diff --check`
