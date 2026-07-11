Status: Active
Source Idea Path: ideas/open/686_private_detail_header_contraction.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Localize Single-Translation-Unit Declarations

# Current Packet

## Just Finished

Completed `plan.md` Step 2 declaration-surface contraction for
`is_known_function_link_name_id`.

Changed files:
- `src/backend/bir/lir_to_bir/lowering.hpp`
- `src/backend/bir/lir_to_bir/globals.cpp`
- `todo.md`

Removed the `is_known_function_link_name_id` declaration from `lowering.hpp`
and made its definition private to `globals.cpp` with internal linkage. The
cross-TU `is_known_function_global_address` declaration remains in
`lowering.hpp`; it still calls the private link-name helper from `globals.cpp`.

Reference checks:
- `rg` found `is_known_function_link_name_id` only in `globals.cpp` and
  `lowering.hpp`.
- `c4c-clang-tool-ccdb function-callers` found only `globals.cpp` callers:
  `is_known_function_global_address` and `resolve_known_global_address`.

## Suggested Next

Continue Step 2 with one more `lowering.hpp` declaration family that can be
removed or narrowed after reference proof, or hand back to plan owner if no
safe single-translation-unit declaration candidate remains.

## Watchouts

`is_known_raw_function_symbol` was not removed in this packet because
`memory/provenance.cpp` still has a namespace using-declaration for it, so that
would require touching another implementation file. `is_known_function_global_address`
is cross-TU and must remain declared unless a broader packet moves its callers.

## Proof

Ran the delegated proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: passed. `test_after.log` reports 100% tests passed, 0 tests failed out
of 302.
