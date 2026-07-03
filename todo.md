Status: Active
Source Idea Path: ideas/open/572_rv64_same_module_call_result_lowering.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement RV64 Same-Module Call Emission

# Current Packet

## Just Finished

Completed Step 3 ordinary same-module GPR call emission repair for the focused
backend object-emission tests.

- The prepared RV64 object route already had the direct same-module call
  scaffold, GPR register argument moves, call relocations, and GPR register
  result publication path in `fragment_for_prepared_call(...)`.
- This packet fixed the remaining observed supported-argument gap by allowing
  `ptr 0` immediate call arguments to materialize as GPR zero, alongside the
  existing integer-immediate materialization path.
- The new immediate/null and prior-result multi-GPR same-module call tests now
  build object modules and prove argument materialization, ordered
  `R_RISCV_CALL_PLT` relocations, `a0` result publication to the owner GPR,
  and later return use of the published result.
- I adjusted only the new Step 2 focused assertions to match the existing RV64
  function epilogue ordering: return-value moves happen before `ra`/`sp`
  restoration and `ret`, so `mv a0, owner` and `ret` are not adjacent.

## Suggested Next

Run the Step 5 representative evidence packet for `src/20000412-2.c` and
`src/20000622-1.c`, unless the supervisor first wants a narrow Step 4 audit to
mark the existing GPR register-result publication path complete.

## Watchouts

- This plan is limited to ordinary same-module RV64 call/result lowering.
- Do not treat `llvm.inline_asm` carriers as ordinary calls.
- Do not implement select, floating-point binary, pointer arithmetic,
  prepared-authority, broad ABI, or runtime-comparison work here.
- Do not add filename-specific matching for `src/20000412-2.c` or
  `src/20000622-1.c`.
- Do not change expectations, unsupported markers, allowlists, or runtime
  comparison behavior.
- Unsupported ordinary call ABI forms still fail closed through the existing
  generic `unsupported_instruction_fragment` surface in this bounded packet;
  precise call-specific diagnostics were not implemented here.
- Register-destination GPR call results are covered by the focused tests.
  Stack-slot call result publication has separate existing coverage and should
  remain distinct from this ordinary register-result slice.
- `emit_riscv_simple_call(...)` also allows `DirectExternFixedArity`; the 572
  slice should keep same-module semantics as the proof focus and avoid
  widening into broad external ABI policy.

## Proof

Run:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'; } > test_after.log 2>&1`

Result: build succeeded; focused `backend_riscv_object_emission` CTest passed.

Proof log: `test_after.log`.
