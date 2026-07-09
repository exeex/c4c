Status: Active
Source Idea Path: ideas/open/656_20000722_local_memory_access_object_route.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Locate The Local-Memory Rejection Boundary

# Current Packet

## Just Finished

Completed Step 2 for idea 656 by locating the RV64 object-route local-memory
boundary for `tests/c/external/gcc_torture/src/20000722-1.c`.

The failing prepared/BIR owner remains `bar:entry` instruction 7:
`@.str0 = bir.load_local ptr %lv._clit_.0, addr .str0`. The consumed prepared
fact from
`build/agent_state/656_step1_local_memory_access_evidence/summary.md` is the
string-constant label-pointer local-memory access:
`base=string_constant`, `result=@.str0`, `symbol=.str0`, `offset=0`,
`size=8`, `align=8`, `base_plus_offset=yes`,
`layout_authority=string_constant_label_pointer`, and
`range_verdict=unknown_compatible`.

The exact rejection boundary is the RV64 object `LoadLocalInst` fallback in
`diagnose_unsupported_prepared_instruction_fragment` in
`src/backend/mir/riscv/codegen/object_emission.cpp`. The diagnostic is emitted
by its `local_memory_diagnostic` helper after
`fragment_for_prepared_instruction` cannot produce a fragment for the
instruction. The relevant admission predicates are:
`prepared_memory_access_for_local_instruction(...)` to retrieve the prepared
access; `string_constant_local_load_is_supported()` for ptr-sized string-label
loads; and
`prepare::prepared_string_constant_label_pointer_has_authority(...)` in
`src/backend/prealloc/addressing.hpp` for the authority contract.

The prepared authority helper itself is not a policy blocker for this shape: it
accepts string-constant bases with a symbol name, base-plus-offset authority,
offset `0`, size/align `8`, `layout_authority=StringConstantLabelPointer`, and
`range_verdict=UnknownCompatible`. The directly related object materialization
path in `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp`,
`fragment_for_prepared_load_local`, already has a string-constant pointer-load
branch that materializes the label address with
`make_rv64_pcrel_address_fragment(...)` when the prepared access and destination
home are available. The current failure is therefore a narrow object-route
materialization/admission hole around this already-prepared string-label
pointer local load, not a request to broaden string-constant byte-load policy.

No lifecycle split or review is needed before the next focused coverage packet:
the route is within idea 656 and outside idea 648 call-argument materialization.

## Suggested Next

Execute Step 3 for idea 656 by adding focused backend coverage for
`tests/backend/case/riscv64_local_string_constant_label_pointer_load.c`. The
test should isolate the compound-literal string-label pointer load shape and
assert the RV64 object route no longer fails with
`unsupported_local_memory_access`; if object disassembly is available in the
existing harness, also assert the expected PC-relative string-label address
materialization for the loaded pointer.

## Watchouts

- Do not change idea 648 call-argument materialization in this route.
- Do not rely on the historical `mv a0,s2` disassembly as current evidence.
- Do not broaden string-constant local-memory policy or
  `StringConstantLabelPointer` admission; this route is for pointer-sized label
  address materialization only, not arbitrary string byte loads.
- Do not special-case `src/20000722-1.c`, `.str0`, `%lv._clit_`, `bar`, `foo`,
  `s2`, or `a0`.
- Do not edit expectations, unsupported markers, allowlists, timeouts,
  runtime-comparison policy, or pass/fail accounting.
- Do not treat the later `foo` pointer-value access as the first blocker; the
  focused `probe_pointer_value.c` object route succeeds.
- If diagnostics reach a renewed call-argument mismatch after this blocker,
  hand that back to idea 648 instead of fixing it here.

## Proof

No build or ctest proof was required for this boundary-location packet. Read
`build/agent_state/656_step1_local_memory_access_evidence/summary.md`; used
`c4c-clang-tool-ccdb`/`c4c-clang-tool` for symbol and callee lookup; inspected
`src/backend/mir/riscv/codegen/object_emission.cpp`,
`src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp`,
`src/backend/mir/riscv/codegen/prepared_module_emit.cpp`,
`src/backend/prealloc/addressing.hpp`, and directly related prepared lookup
helpers. Did not create or overwrite `test_after.log`.
