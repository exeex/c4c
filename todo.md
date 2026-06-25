Status: Active
Source Idea Path: ideas/open/348_vrm_regalloc_mir_and_rv64_assembler_bridge.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Frontier And Call-Boundary Baseline

# Current Packet

## Just Finished

Completed Step 1: Frontier And Call-Boundary Baseline. Existing focused
coverage proves HIR and LIR keep `__c4c_builtin_vrm1/2/4/8` as dedicated VRM
carriers, BIR preserves a source-level VRM inline-asm operand as `c4c.vrm2`,
prepared/regalloc metadata keeps vector carrier class/width facts visible, and
non-expanded VRM call boundaries diagnose for direct argument, call result,
function return, and function-pointer call cases. No expectations were
downgraded and no implementation changes were needed.

## Suggested Next

Delegate Step 2: extend the existing prepared/regalloc frontier from observable
VRM carrier metadata into final contiguous vector-register group allocation and
MIR-consumable base/group identity.

## Watchouts

- Do not add any VRM function-call ABI behavior.
- Do not downgrade supported-path expectations to unsupported.
- Keep proof tied to source-level VRM carriers, not fixture-only prepared
  modules.
- The delegated owned path `tests/backend/bir/backend_prealloc_inline_asm_test.cpp`
  does not exist in this checkout; the `backend_prealloc_inline_asm` CTest entry
  currently reuses `backend_prepared_printer_test`.

## Proof

Passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|frontend_lir_function_signature_type_ref|backend_lir_to_bir_notes|backend_prepare_liveness|backend_prepared_printer|backend_prealloc_inline_asm|backend_riscv_object_emission)$'`

Proof log: `test_after.log`.
