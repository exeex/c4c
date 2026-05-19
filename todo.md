Status: Active
Source Idea Path: ideas/open/301_aarch64_memory_store_operand_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm store operand failure shapes

# Current Packet

## Just Finished

Step 1 diagnostic confirmation complete for idea 301. The delegated focused
AArch64 subset still has 8/8 compile-route failures, all at AArch64
machine-node printing for `family=memory opcode=store`.

Focused case classification:

| Case | Current failure mode | Classification | Representative operand evidence | Likely implementation surface |
| --- | --- | --- | --- | --- |
| `00173` | `function 0 block 0 instruction 3`: `stack-slot store source scratch is not printable` | source-scratch / frame-slot store value | prepared BIR has `bir.store_local %lv.y, i8 %t1`; storage has `%t1 encoding=frame_slot ... stack88`; access has `stored=%t1 ... size=1` | `src/backend/mir/aarch64/codegen/memory.cpp` publishes a stack-slot `MemoryOperand` for stored frame-slot values; `src/backend/mir/aarch64/codegen/machine_printer.cpp` only has store-source scratch materialization for 4/8-byte widths, so 1-byte stack-source stores are not printable. |
| `00187` | `function 0 block 4 instruction 1`: `stack-slot store source scratch is not printable` | source-scratch / frame-slot store value | prepared BIR has `bir.store_local %lv.ShowChar, i8 %t42`; storage has `%t42 encoding=frame_slot ... stack128`; access has `stored=%t42 ... size=1` | same stack-slot store-value publication plus printer scratch-width boundary as `00173`. |
| `00194` | `function 0 block 5 instruction 2`: `stack-slot store source scratch is not printable` | source-scratch / frame-slot store value | prepared BIR has `bir.store_local %lv.b, i8 %t18`; storage has `%t18 encoding=frame_slot ... stack44`; access has `stored=%t18 ... size=1` | same stack-slot store-value publication plus printer scratch-width boundary as `00173`. |
| `00176` | `function 0 block 0 instruction 3`: `symbol store value is not a register or immediate operand` | symbol/global store value | prepared BIR has global-array stores such as `bir.store_global @array, i32 %t7.outer0.elt0.inner.store`; the selected stored values are stack-slot temporaries rather than register/immediate operands | `memory.cpp::make_store_memory_instruction_record` can publish stored stack-slot values as `MemoryOperand`, but `machine_printer.cpp::print_symbol_memory` only accepts register/immediate store values for symbol addresses. |
| `00181` | `function 0 block 3 instruction 1`: `symbol store value is not a register or immediate operand` | symbol/global store value | prepared BIR has global tower-array stores such as `bir.store_global @A`, `@B`, and `@C` with `.inner.store` values; storage for those selected values is stack-slot based | same symbol-memory printer admission/materialization boundary as `00176`. |
| `00182` | `function 0 block 7 instruction 4`: `symbol store value is not a register or immediate operand` | symbol/global store value | prepared BIR has static-local global stores such as `bir.store_global @__static_local_print_led_3, i32 %t18.outer0.elt0.inner.store`; storage has the `.inner.store` values encoded as frame slots | same symbol-memory printer admission/materialization boundary as `00176`. |
| `00213` | `function 0 block 0 instruction 3`: `symbol store value is not a register or immediate operand` | symbol/global store value | prepared BIR has `bir.store_global @global, i32 %t1`; storage has `%t1 encoding=frame_slot ... stack8`; access has `base=global_symbol stored=%t1 symbol=global size=4` | same symbol-memory printer admission/materialization boundary as `00176`. |
| `00214` | `function 0 block 4 instruction 2`: `symbol store value is not a register or immediate operand` | symbol/global store value | prepared BIR has `bir.store_global @_brk_end, i64 %t19` and later `%t23`; storage has those values encoded as frame slots; access has `base=global_symbol stored=%t19/%t23 symbol=_brk_end size=8` | same symbol-memory printer admission/materialization boundary as `00176`. |

Shared boundary: prepared memory lowering already records frame-slot-backed
stored values, including symbol/global stores, but AArch64 machine printing
only admits register/immediate store values for symbol memory and only
materializes stack-slot source stores for 4/8-byte non-symbol memory. No
focused case fell outside the idea 301 memory-store operand owner.

## Suggested Next

Implement Step 2 first: repair the AArch64 stack-slot store source path for
1-byte local/frame stores (`00173`, `00187`, `00194`) by making the selected
store value printable through a semantic scratch materialization path, then run
the same focused proof command.

## Watchouts

- Do not change expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, or CTest registration.
- Do not reopen closed owners 285 through 300 from failing counts alone.
- Keep runtime nonzero, runtime mismatch/crash, timeout, scalar operand-shape,
  call-boundary, unsigned reduction, invalid scalar cast spelling, and
  `lir_to_bir` residuals outside this owner unless generated-code or
  diagnostic evidence proves they share the memory-store operand failure mode.
- The first implementation slice should not special-case c-testsuite names or
  diagnostic strings. The semantic target is store-value materialization for
  frame-slot operands, starting with 1-byte stores.
- For the follow-on symbol/global slice, `print_symbol_memory` currently accepts
  only register/immediate stored values even when memory lowering supplied a
  prepared stack-slot stored value. Reuse the same materialization model if the
  generated-code evidence supports it.

## Proof

Ran:

```bash
cmake --build --preset default && ctest --test-dir build -j8 -R '^c_testsuite_aarch64_backend_src_(00173|00176|00181|00182|00187|00194|00213|00214)_c$' --output-on-failure > test_after.log 2>&1
```

Result: build succeeded; focused CTest subset failed 8/8 with the expected
memory-store printer diagnostics. `test_after.log` is the proof log for this
packet. This proof is sufficient for Step 1 confirmation and intentionally is
not acceptance proof for an implementation slice.
