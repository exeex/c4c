Status: Active
Source Idea Path: ideas/open/261_aarch64_f128_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Move F128 Spelling And Printer Helpers

# Current Packet

## Just Finished

Step 4: Move F128 Spelling And Printer Helpers moved
`print_f128_transport`, `print_f128_runtime_helper`, and their f128-only
register/marshal spelling helpers from generic `machine_printer.cpp` into
`f128.cpp`/`f128.hpp`. `machine_printer.cpp` keeps the generic payload
traversal and dispatches f128 transport/helper payloads through the f128-owned
printer entry points.

## Suggested Next

Continue Step 4 by auditing any remaining f128-specific printer spelling in
`machine_printer.cpp` that can move into the f128 owner without dragging broad
generic traversal or non-f128 printer helpers with it.

## Watchouts

- Keep the redistribution behavior-preserving.
- Preserve fail-closed and deferred-helper boundaries.
- Do not weaken tests, rewrite expectations, or add named-case f128 shortcuts.
- Do not redesign `instruction.hpp` solely to split this shard.
- Current live f128 support is prepared-fact driven and intentionally
  fail-closed: missing prepared carriers/helpers, incomplete 16-byte size or
  alignment, non-full-width carrier kinds for helper ABI moves, unsupported
  helper families, missing clobber/resource/live-preservation policies, missing
  selected-call ownership, unsupported scalar bridge widths, and unprintable
  memory/register facts all become diagnostics or target-unsupported output.
  Do not convert these checks into fallback scalar F64 behavior.
- `f128.md` describes legacy address/temp/constant/source hooks that are not
  currently implemented as live C++ selected nodes. Moving code must not
  synthesize those old hooks or claim semantic completion by recreating names.
- Transport, runtime-helper boundary instruction construction, and prepared
  runtime-helper boundary record construction are now owned by `f128.cpp`.
- `memory.cpp` should continue to own `lower_f128_transport_instruction`; it is
  the neutral memory router that prepares structured memory operands and calls
  f128-owned construction helpers.
- A narrow f128 transport lowering move was rejected for this packet because it
  would drag broad memory helper seams into `f128.cpp`.
- `make_f128_register_operand` no longer has a public `f128.hpp` declaration;
  keep it local to f128 construction unless a real external caller appears.
- Do not add broad neutral bank/class mapping helpers to `f128.cpp`; keep f128
  scalar register conversion explicit and fail-closed for unsupported banks.
- `f128.hpp` currently redeclares an existing surface from `instruction.hpp`;
  duplicate default arguments remain intentionally avoided there. Remove or
  narrow corresponding declarations in `instruction.hpp` only if needed for
  clean ownership and include flow.
- `f128_vector_register_name` is now f128-owned and declared in `f128.hpp`
  because the existing vector intrinsic printers still need the same q/vector
  spelling while remaining in generic printer dispatch.
- `clang-format` is not installed in this environment; the touched C++ files
  were left in the existing local style after manual review.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
The build completed and all 139 selected backend tests passed. Full output is
in `test_after.log`.
