Status: Active
Source Idea Path: ideas/open/867_lir_memory_va_object_lifetime_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Write direct-local va_start 734 handoff

# Current Packet

## Just Finished

Completed `plan.md` Step 4 for the selected direct-local `va_start` native
pointer authority seam. Added
`docs/lir_memory_va_object_lifetime_authority/handoff_to_734.md`, naming the
exact LIR producer fields, verifier checks, accepted positive case, malformed
rejections, proof command, and one future 734 receiver row.

## Suggested Next

Send the exhausted runbook to plan-owner for semantic disposition of idea 867:
close if the direct-local `LirVaStartOp` handoff satisfies the source
criteria, or repair/switch only if another in-scope requirement remains.

## Watchouts

Do not edit Raw-BIR receiver/importer code, reopen accepted local-object/VLA/
memcpy history, or derive authority from operand spelling, printer output,
LLVM text, rendered names, or testcase identity. Keep `va_start` separate from
`va_end`, `va_copy`, `va_arg`, memset, memcpy, prepared-BIR helper homes, and
Raw-BIR receiver work. The Step 4 document should authorize one future 734 row
only: receive the typed Raw-BIR container for selected direct-local
`LirVaStartOp.ap_authority`.

## Proof

Accepted focused proof:
`{ cmake --build build && ctest --test-dir build -R '^backend_lir_selected_pointer_authority$' --output-on-failure; } > test_after.log 2>&1`

Result: passed; `test_after.log` contains a successful build/no-op build and
1/1 passing test. Step 4 document-only update also requires `git diff --check`.
