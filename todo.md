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

Lifecycle close review rejected closure for now.

Remaining packet 1: continue Step 2 with one declaration family whose
`lowering.hpp` declaration surface can actually be removed or narrowed, not
only moved out-of-line. The completed `FunctionSymbolSet` slice moved method
bodies into `globals.cpp` but intentionally preserved the data members and
method declarations in `lowering.hpp`, so it is useful contraction progress but
not enough by itself to exhaust the source idea.

Remaining packet 2: after Step 2 has no more viable single-translation-unit
localization candidates, run Step 3 only if a real cross-TU family still needs
a narrower adapter-owned helper boundary than `lowering.hpp`.

Remaining packet 3: complete Step 4 by auditing that import-local state
authority stayed inside LIR import and did not leak into public BIR,
prepared/prealloc, target, or MIR layers.

Remaining packet 4: complete Step 5 with fresh proof recorded here. Closure
also needs a usable matching `test_before.log` / `test_after.log` regression
guard pair; this review found `test_after.log` absent from the worktree and
did not recreate root proof logs because the delegation marked them out of
scope.

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
