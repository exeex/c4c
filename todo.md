Status: Active
Source Idea Path: ideas/open/656_20000722_local_memory_access_object_route.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Implement Or Split The Narrow Owner

# Current Packet

## Just Finished

Completed Step 4 for idea 656 by repairing the RV64 object-route local-memory
owner for the compound-literal direct-extern string-label pointer case.

The immediate object-route rejection was the compound literal's prepared
one-byte zero-fill frame-slot stores before the string-label pointer store. The
object route now derives a bounded stack offset from the prepared `StoreLocalInst`
slot id when the indexed prepared access fact is unavailable for such a local
frame-slot store, then reuses the existing RV64 stack-offset store encoder. This
keeps the repair scoped to prepared local frame-slot stores and does not broaden
string-byte loading or string-constant policy.

Converted
`backend_cli_riscv64_local_string_constant_label_pointer_compound_arg` from an
object-route expected-failure diagnostic test to a supported RV64 ELF object
emission test. The existing simple load object-route guard and both prepared
dump tests remain green.

## Suggested Next

Run the supervisor acceptance decision for idea 656. If more confidence is
needed, use a broader RV64 object/local-memory subset around prepared local
frame-slot stores and the original `src/20000722-1.c` object route.

## Watchouts

- The Step 4 implementation did not edit idea 648 call-argument materialization
  code; any renewed call-argument mismatch belongs back to that route.
- The repair is intentionally not a string-byte load expansion. Preserve
  fail-closed behavior for unsupported broad string local-memory shapes.
- Do not change idea 648 call-argument materialization in this route.
- Do not rely on the historical `mv a0,s2` disassembly as current evidence.
- Do not special-case `src/20000722-1.c`, `.str0`, `%lv._clit_`, `bar`, `foo`,
  `s2`, or `a0`.
- Do not edit expectations, unsupported markers, allowlists, timeouts,
  runtime-comparison policy, or pass/fail accounting.
- If diagnostics reach a renewed call-argument mismatch after this blocker,
  hand that back to idea 648 instead of fixing it here.

## Proof

Ran the focused proof:
`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_(dump|cli)_riscv64_local_string_constant_label_pointer"; } 2>&1 | tee test_after.log'`.

Result: passed. All four focused tests passed:
`backend_dump_riscv64_local_string_constant_label_pointer_load` and
`backend_cli_riscv64_local_string_constant_label_pointer_load`, plus
`backend_dump_riscv64_local_string_constant_label_pointer_compound_arg` and
`backend_cli_riscv64_local_string_constant_label_pointer_compound_arg`.
The supervisor-selected proof was sufficient for this bounded code packet.
Proof log: `test_after.log`.
