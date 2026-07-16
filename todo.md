Status: Active
Source Idea Path: ideas/open/867_lir_memory_va_object_lifetime_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Verify direct-local va_start native pointer authority

# Current Packet

## Just Finished

Completed `plan.md` Step 2 for the selected direct-local `va_start` native
pointer authority seam as a no-code publication confirmation. The required LIR
carrier already exists in `LirVaStartOp.ap_authority`, the producer already
populates it through `StmtEmitter::native_direct_local_va_pointer`, and the
selected facts are native fields rather than operand spelling: `ap_ptr` value
id, current-function local object id, owner `LinkNameId`, pointer type `ptr`,
native `va_list` pointee type, and liveness.

The selected producer boundary remains only `BuiltinId::VaStart` with a direct
local `TB_VA_LIST` decl-ref. Adjacent `va_end`, `va_copy`, `va_arg`, memcpy,
memset, prepared-BIR helper homes, and Raw-BIR receiver/importer work remain
outside this selected row.

## Suggested Next

Start `plan.md` Step 3 for the selected seam only: verify that
`verify_native_memory_va_authority` enforces current-function local object
ownership, pointer-definition agreement with `ap_ptr`, canonical pointer
facts, type/liveness suitability, malformed rejection, and rejection of
authority fields on unselected `va_start`.

## Watchouts

Do not edit Raw-BIR receiver/importer code, reopen accepted local-object/VLA/
memcpy history, or derive authority from operand spelling, printer output,
LLVM text, rendered names, or testcase identity. Keep `va_start` separate from
`va_end`, `va_copy`, `va_arg`, memset, memcpy, prepared-BIR helper homes, and
Raw-BIR receiver work. If Step 3 coverage is already sufficient, record that
as evidence and move directly to the Step 4 handoff document.

## Proof

No-code Step 2 confirmation. Focused proof run:
`{ cmake --build build && ctest --test-dir build -R '^backend_lir_selected_pointer_authority$' --output-on-failure; } > test_after.log 2>&1`

Result: passed; `test_after.log` contains a successful build/no-op build and
1/1 passing test. Run `git diff --check` before committing.
