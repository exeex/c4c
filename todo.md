Status: Active
Source Idea Path: ideas/open/800_lir_amd64_vaarg_unselected_alloca_compatibility_regression.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Restore typed alloca-result compatibility on unselected overflow routes

# Current Packet

## Just Finished

- Switched from 753 Step 3 after its required full baseline failed 3036/3037.
  The failure is outside 753's source scope and is now owned by this separate
  800 regression blocker; no 753 receiver handoff was performed.

## Suggested Next

- Execute Step 1 only: restore a verifier-compatible typed result operand for
  the unselected AMD64 overflow `LirAllocaOp` route, while preserving selected
  carrier selection and authority exactly as accepted in `c4e820a48`.

## Watchouts

- Do not weaken `LirAllocaOp` verification, turn the unselected route into a
  selected carrier, recover facts from text, or modify closed 799. Keep the
  correction limited to the unselected compatibility construction seam.

## Proof

- Required for this blocker: fresh `cmake --build --preset default` plus
  `./build/tests/frontend/frontend_lir_call_type_ref_test`; after that passes,
  the supervisor must run a matching full baseline before returning to 753.
