Status: Active
Source Idea Path: ideas/open/686_private_detail_header_contraction.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Localize Single-Translation-Unit Declarations

# Current Packet

## Just Finished

Completed `plan.md` Step 2 local declaration/body contraction for
`FunctionSymbolSet`.

Changed files:
- `src/backend/bir/lir_to_bir/lowering.hpp`
- `src/backend/bir/lir_to_bir/globals.cpp`
- `todo.md`

Moved the four `FunctionSymbolSet` method bodies (`reserve`,
`insert_function`, `contains_link_name_id`, and
`find_raw_symbol_link_name_id`) out of `lowering.hpp` and into
`globals.cpp`. The header keeps the same data members and method declarations,
and the out-of-line bodies preserve the same lookup, insertion, invalid-id,
and raw-symbol fallback behavior.

## Suggested Next

Next coherent packet: continue Step 2 by selecting another narrow helper family
whose declarations must remain visible across adapter TUs but whose bodies can
be localized to one existing implementation file.

## Watchouts

No include, linkage, or cross-TU dependency blocked this move. Keep the next
packet similarly scoped; do not move private LIR import state into public BIR,
prepared/prealloc, target, MIR, or a broad helper bucket. Avoid structured
layout, initializer semantics, memory/provenance repair, call ABI repair, test
expectation changes, unsupported marker changes, or allowlist edits unless the
supervisor delegates that scope.

## Proof

Ran the delegated proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: passed. The backend subset reported 302 tests passed in
`test_after.log`.
