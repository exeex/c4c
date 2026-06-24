Status: Active
Source Idea Path: ideas/open/342_rv64_ev_insn_d_inline_asm_stage3.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Integrate With Object Emission And Existing Inline Asm Behavior

# Current Packet

## Just Finished

Step 4 added focused integration coverage proving the prepared RV64 object path
can emit existing scalar `.insn r` inline asm and the new EV `.insn.d` inline
asm in one function without breaking the existing return path.

- Added mixed prepared object coverage that emits `.insn r` bytes
  `0x007302b3`, EV `.insn.d` bytes `0x0000030b10620a0a`, the preserved return
  move `0x00028513`, and `ret` in order.
- Reused the existing vector-constraint `.insn.d` carrier and the existing
  scalar `.insn r` prepared fixture so the test covers route integration rather
  than a separate hand-built object fragment.
- Existing `.insn r`, tied-input `.insn r`, vector substitution, and standalone
  `.insn.d` object coverage still pass in the focused object-emission
  executable.

## Suggested Next

Supervisor should review and commit the Step 4 test-only integration slice, or
move to the next lifecycle packet if broader route review is desired.

## Watchouts

- No production implementation change was needed; the mixed prepared object
  route passed with the existing `.insn.d`-first then `.insn r` fallback
  dispatch.
- Named operands, `%c[...]`, masks, consteval strings, relocations, and broad
  EV operation semantics remain rejected or out of scope.
- The backend proof still shows the known unrelated baseline failure
  `backend_codegen_route_riscv64_pointer_typed_select_publication`.

## Proof

Ran exactly:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`

Result: build succeeded and the backend subset ran. `test_after.log` contains
one failing test, the known unrelated baseline failure
`backend_codegen_route_riscv64_pointer_typed_select_publication`; the new mixed
`backend_riscv_object_emission` `.insn r` plus `.insn.d` coverage passed.
