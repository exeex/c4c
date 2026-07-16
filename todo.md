Status: Active
Source Idea Path: ideas/open/846_lir_family_overloaded_verifier_dispatch_printer.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Decide Next 846 Packet Or 847 Handoff

# Current Packet

## Just Finished

Completed `plan.md` Step 5 selected consumer migration for
`LirVaArgOp.type_str` native integer semantic va_arg results.

- Routed selected integer va_arg verifier validation through
  `render_integer_type_ref` when the instruction carries native va_arg result
  authority.
- Updated `LirVaArgOp` printing to render selected integer va_arg result types
  from native width authority instead of mutable display text.
- Preserved generic `require_module_type_ref` / `require_type_ref` behavior for
  floating, pointer, aggregate, vector, compatibility, and other non-integer
  va_arg type refs.
- Added focused stale-display coverage in
  `test_vaarg_helper_result_authority_loss()` proving selected integer va_arg
  printing emits `i32` and does not recover from stale type text.

## Suggested Next

Execute `plan.md` Step 5 inventory again and select the next exact 846-owned
verifier/printer/dispatch consumer, or record the 847 handoff/blocker decision
if no bounded 846 consumer remains.

## Watchouts

This packet intentionally did not broaden va_list memory authority, va_arg
lowering, result-use verification, universal model deletion, Raw-BIR, 734/797
surfaces, LirSwitch selector surfaces, or unrelated LIR consumers. Non-integer
va_arg type refs still rely on the existing generic verifier/printer paths.

## Proof

Completed selected integer va_arg packet proof:

- `cmake --build build`
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure`
  recorded in `test_after.log`
- `git diff --check`
