Status: Active
Source Idea Path: ideas/open/656_20000722_local_memory_access_object_route.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Focused Local-Memory Route Coverage

# Current Packet

## Just Finished

Completed Step 3 for idea 656 by adding focused backend coverage for both the
already-supported simple string-constant label-pointer local load shape and the
currently rejected compound-literal/direct-extern shape.

Added `backend_dump_riscv64_local_string_constant_label_pointer_load` to assert
the prepared addressing contract independently from
`src/20000722-1.c`: `@.str0 = bir.load_local ptr %lv.local, addr .str0`,
`base=string_constant`, `result=@.str0`, `symbol=.str0`, `offset=0`,
`size=8`, `align=8`, `base_plus_offset=yes`,
`layout_authority=string_constant_label_pointer`, and
`range_verdict=unknown_compatible`. The dump test also asserts the prepared
string-constant address materialization record and forbids the representative
GCC torture filename and `%lv._clit_` identifier.

Added `backend_cli_riscv64_local_string_constant_label_pointer_load` to assert
the desired RV64 object-route behavior through ELF object emission for the same
simple focused shape. The simple object-route test passes in the current tree,
which distinguishes it from the rejected compound/direct-extern owner.

Added
`tests/backend/case/riscv64_local_string_constant_label_pointer_compound_arg.c`
with the actual rejected generic shape from `probe_string_compound.c`: a
compound-literal struct whose string literal field is passed by address to a
direct extern fixed-arity callee. Added
`backend_dump_riscv64_local_string_constant_label_pointer_compound_arg` to
assert `@.str0 = bir.load_local ptr %lv._clit_.0, addr .str0`, the
`direct_extern_fixed_arity` call to `consume_payload`, and the prepared
`base=string_constant` / `layout_authority=string_constant_label_pointer`
contract before the call. Added
`backend_cli_riscv64_local_string_constant_label_pointer_compound_arg` as an
object-route expected-failure test for the current
`unsupported_local_memory_access` diagnostic. No unsupported marker, allowlist,
implementation source, plan, or idea file was changed.

## Suggested Next

Execute Step 4 for idea 656 by repairing the RV64 object-route local-memory
materialization/admission hole for the compound-literal direct-extern case,
using `backend_cli_riscv64_local_string_constant_label_pointer_compound_arg` as
the first proof target. Keep
`backend_cli_riscv64_local_string_constant_label_pointer_load` green as the
already-supported simple subset guard.

## Watchouts

- The simple local string-label pointer object route is already supported; the
  Step 4 implementation target is the compound-literal/direct-extern shape
  that currently passes only as an expected-failure diagnostic.
- Do not satisfy Step 4 by weakening
  `backend_cli_riscv64_local_string_constant_label_pointer_compound_arg`; it
  should become a supported object-route test when the implementation is fixed.
- Do not change idea 648 call-argument materialization in this route.
- Do not rely on the historical `mv a0,s2` disassembly as current evidence.
- Do not broaden string-constant local-memory policy or
  `StringConstantLabelPointer` admission; this route is for pointer-sized label
  address materialization only, not arbitrary string byte loads.
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
Proof log: `test_after.log`.
