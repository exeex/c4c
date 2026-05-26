Status: Active
Source Idea Path: ideas/open/40_riscv_prepared_edge_publication_typed_stack_source_policy.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement Or Preserve Fail-Closed Selected Typed Form

# Current Packet

## Just Finished

Step 2 preserved and made explicit the fail-closed policy for typed scalar
RISC-V `StackSlot -> Register` edge-publication forms that lack prepared
signedness/register-bank authority.

Added focused negative coverage in
`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp` for:
- sub-word stack sources, including an explicitly typed `I16 -> I16` shape,
  staying fail-closed without signedness authority and without emitting
  `lb`/`lbu`/`lh`/`lhu`
- unsigned-I32-shaped `I32 -> I64` stack sources staying fail-closed without
  zero-extension authority and without emitting `lwu`
- `F32 -> F32` stack sources with raw `fa0` spelling staying fail-closed
  without prepared FPR destination authority and without emitting `flw` or
  integer `lw fa0`

Added a narrow RISC-V lowering guard in `emit.cpp` for register-destination
stack sources: the existing size-based `lw`/`ld` policy is preserved only for
same-type non-floating values. Floating stack-source forms and source/dest type
mismatches now return `UnsupportedSourceHome` instead of selecting an opcode
from size or raw register spelling. Existing I32, I64-size, and large-offset
stack-source behavior remains covered by the existing tests.

## Suggested Next

Proceed to Step 3 to harden neighboring unsupported cases only if the
supervisor wants more negative coverage. Otherwise Step 4 validation can run a
backend bucket and decide whether this source idea is complete as a documented
fail-closed blocker.

## Watchouts

This packet intentionally does not add RISC-V typed load support. A future
support packet still needs shared prepared facts such as explicit stack-source
extension kind and destination register bank/view before target-local opcode
selection. Do not broaden this into dynamic-address, aggregate, pointer-base,
or source-to-stack behavior.

## Proof

Proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_prepared_edge_publication|backend_prepared_lookup_helper)$' | tee test_after.log`

Result: passed, 2/2 tests. Proof log: `test_after.log`.

Additional checks:
`rg -n "\b(lwu|flw|lb|lbu|lh|lhu)\b" src/backend/mir/riscv/codegen/emit.cpp || true`
returned no matches.

`git diff --check` passed.
