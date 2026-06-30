Status: Active
Source Idea Path: ideas/open/427_rv64_scalar_select_join_materialization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Emit One Coherent Scalar Select In The Object Route

# Current Packet

## Just Finished

Step 2 implemented ordinary small-integer scalar select materialization in the
RV64 object route. `fragment_for_prepared_select` now admits `I8` and `I16`
results alongside the existing `I32`/`I64` cases, then validates stack-slot
destinations with the selected result byte width instead of the previous
4-byte default. This keeps non-scalar, pointer, F128, and missing-home cases
fail-closed through the existing scalar-size and destination-home checks.

AST-backed inspection was used before the edit. The direct callees confirmed
that the selected path already uses the existing scalar memory-size,
value-home, branch, jump, value-to-register, and stack-store helpers.
Follow-up proof added `backend_riscv_object_emission_test` coverage for
prepared ordinary `I8` and `I16` select materialization. The test constructs
prepared object-route select modules directly, avoiding C lowering shape drift,
and checks the emitted compare/branch/materialize sequence plus byte-width
`sb`/`sh` stack stores.

## Suggested Next

Execute Step 3: inspect and implement the next coherent select materialization
packet for join-transfer carriers, starting from the existing classification
gate and published edge-copy checks. Keep pointer/address materialization
separate unless the supervisor chooses the pointer-address follow-up idea.

## Watchouts

- The attempted `i8` local-array C sibling split into two non-owned shapes:
  dynamic-index `char` array stores require byte-offset pointer/address
  materialization, while fixed-element ternaries lower as join-transfer
  carriers. Neither belongs in this ordinary-select packet.
- The focused proof for this packet is now the lower-level prepared-object
  test in `backend_riscv_object_emission_test`; it would have rejected before
  the Step 2 `I8`/`I16` type admission and stack-home width fix, and now emits
  the expected `sb`/`sh` object bytes.
- `src/pr43236.c` remains evidence, not a filename-shaped special case. If it
  still fails after this slice, inspect the next unsupported fragment before
  assuming the select type gate is still the blocker.
- Keep join-transfer carrier materialization for the next select packet unless
  the supervisor redirects to pointer/address materialization.
- Do not edit expectations, allowlists, unsupported markers, runtime comparison, or pass/fail accounting.
- Keep F128, call-adjacent publication, aggregate ABI, pointer provenance, and global memory repair out of this route.
- Reject filename-shaped fixes for representative gcc_torture rows.

## Proof

Ran the delegated proof command:
`(cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_') > test_after.log 2>&1`.

Result: passed, including the focused prepared-object `I8`/`I16` ordinary
select materialization assertion. `test_after.log` is the canonical proof log
for this Step 2 code packet.
