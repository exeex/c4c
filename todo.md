Status: Active
Source Idea Path: ideas/open/846_lir_family_overloaded_verifier_dispatch_printer.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Decide Next 846 Packet Or 847 Handoff

# Current Packet

## Just Finished

Completed `plan.md` Step 5 selected consumer migration for
`LirPhiOp.boundary_value_type` native scalar integer/floating PHI boundary
carriers.

- Added a PHI boundary verifier helper that routes scalar integer carriers
  through `render_integer_type_ref` and scalar floating carriers through
  `render_floating_type_ref`.
- Preserved generic `require_module_type_ref` behavior for vector, aggregate,
  pointer, function, void, runtime text, opaque, and other non-selected PHI
  boundary families.
- Updated PHI printing to render selected scalar integer/floating boundary
  carriers from native width/builtin authority after the existing
  `boundary_value_type->type` / `op.type_str` mirror check.
- Added focused PHI stale-display coverage proving integer and floating scalar
  boundary printing uses native carrier authority and still rejects stale
  mirror text.

## Suggested Next

Execute `plan.md` Step 5 inventory again and select the next exact 846-owned
verifier/printer/dispatch consumer, or record the 847 handoff/blocker decision
if no bounded 846 consumer remains.

## Watchouts

This packet intentionally did not touch universal model deletion, Raw-BIR,
734/797 surfaces, LirSwitch selector surfaces, PHI incoming authority, or
unrelated LIR consumers. PHI vector, aggregate, pointer, function, void,
runtime text, opaque, and other non-selected boundary families still rely on
the existing generic verifier/printer paths.

## Proof

Completed PHI boundary scalar packet proof:

- `cmake --build build`
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure`
  recorded in `test_after.log`
- `git diff --check`
