Status: Active
Source Idea Path: ideas/open/846_lir_family_overloaded_verifier_dispatch_printer.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Decide Next 846 Packet Or 847 Handoff

# Current Packet

## Just Finished

Completed `plan.md` Step 5 selected consumer repair for `LirExtractValueOp`
native aggregate verifier/printer rendering.

- Updated selected native-authoritative extractvalue printing to render the
  aggregate type from ordered native anonymous-struct field facts instead of
  stale `agg_type.str()` display text.
- Repaired selected native-authoritative extractvalue verification so stale
  aggregate display text is check-only while native field facts, selected
  result element facts, result/aggregate SSA authority, and matching
  current-function aggregate producer facts remain fail-closed.
- Preserved legacy/unselected extractvalue validation and printing through the
  generic `require_type_ref` path, and left insertvalue behavior unchanged.
- Added focused stale-display coverage proving selected extractvalue accepts a
  stale aggregate display mirror only when native facts remain valid and
  `print_llvm` emits the native aggregate type, not the stale text.

## Suggested Next

Execute `plan.md` Step 5 inventory again and select the next exact 846-owned
verifier/printer/dispatch consumer, or record the 847 handoff/blocker decision
if no bounded 846 consumer remains.

## Watchouts

This packet intentionally did not broaden insertvalue behavior, vector/select/
cmp/ret/switch consumers, Raw-BIR/backend/lowering, generic helper deletion, or
idea 847 deletion work. The stale-display allowance is limited to selected
native-authoritative `LirExtractValueOp` instances with complete ordered native
aggregate fields and matching current-function producer facts; legacy
extractvalue validation still treats display mirrors as semantic input.

## Proof

Completed selected extractvalue packet proof:

- `cmake --build build`
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure`
  recorded in `test_after.log`
- `git diff --check`
