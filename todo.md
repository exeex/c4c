Status: Active
Source Idea Path: ideas/open/867_lir_memory_va_object_lifetime_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Write direct-local va_start 734 handoff

# Current Packet

## Just Finished

Completed `plan.md` Step 3 for the selected direct-local `va_start` native
pointer authority seam as a no-code verification evidence step. Existing
`verify_native_memory_va_authority` coverage already enforces the selected
contract through `verify_pointer`: current-function local object ownership,
`ap_ptr` value-id agreement with `ap_authority.local_pointer.pointer_definition`,
modeled pointer definition, canonical pointer fact agreement, pointer type,
pointee type, and liveness. Existing focused tests cover direct-local
`va_start` positive population, missing selected authority rejection, and
rejection of authority fields on unselected `va_start`.

## Suggested Next

Start `plan.md` Step 4 for the selected seam only: write the handoff document
naming the exact future 734 receiver row for `LirVaStartOp` direct-local
destination `va_list` authority. The handoff must name the LIR fields,
producer, verifier checks, accepted positive case, rejected malformed cases,
and exclude Raw-BIR receiver/importer work.

## Watchouts

Do not edit Raw-BIR receiver/importer code, reopen accepted local-object/VLA/
memcpy history, or derive authority from operand spelling, printer output,
LLVM text, rendered names, or testcase identity. Keep `va_start` separate from
`va_end`, `va_copy`, `va_arg`, memset, memcpy, prepared-BIR helper homes, and
Raw-BIR receiver work. The Step 4 document should authorize one future 734 row
only: receive the typed Raw-BIR container for selected direct-local
`LirVaStartOp.ap_authority`.

## Proof

No-code Step 3 verification evidence. Focused proof run:
`{ cmake --build build && ctest --test-dir build -R '^backend_lir_selected_pointer_authority$' --output-on-failure; } > test_after.log 2>&1`

Result: passed; `test_after.log` contains a successful build/no-op build and
1/1 passing test. Run `git diff --check` before committing.
