Status: Active
Source Idea Path: ideas/open/261_aarch64_f128_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Move F128 Construction, Transport, And Lowering Bodies

# Current Packet

## Just Finished

Step 3: Move F128 Construction, Transport, And Lowering Bodies continued the
helper-boundary extraction by moving the f128 prepared runtime-helper boundary
record construction cluster into `f128.cpp`. `f128.cpp` now owns
`make_prepared_f128_runtime_helper_boundary_record` plus the narrowly required
f128 helper record/error and register operand builders. `instruction.cpp`
retains the i128 prepared runtime-helper record construction and neutral
non-f128 bodies. `make_f128_register_operand` is no longer declared in
`f128.hpp`; its remaining callers are local to `f128.cpp`. The f128 scalar
register conversion path now selects only the supported GPR/FPR scalar cases
directly and returns no operand for unsupported banks, avoiding a broad neutral
bank-to-class mapper in `f128.cpp`.

No lowering, dispatch, printer spelling, tests, `plan.md`, source idea, or
markdown shard content changed.

## Suggested Next

Continue Step 3 by deciding the narrow f128 lowering handoff: either route the
existing `lower_f128_transport_instruction` ownership through `f128.cpp` with a
small call boundary, or record that memory-owned lowering should remain in
`memory.cpp` while f128 construction stays in `f128.cpp`. Keep dispatch,
machine-printer spelling, and test expectations out of that packet unless the
compile boundary strictly requires a declaration adjustment.

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
- `memory.cpp` already owns `lower_f128_transport_instruction`; Step 3 needs to
  decide whether that lowering is routed through `f128.cpp` or left as neutral
  memory routing that calls f128-owned construction helpers.
- `make_f128_register_operand` no longer has a public `f128.hpp` declaration;
  keep it local to f128 construction unless a real external caller appears.
- Do not add broad neutral bank/class mapping helpers to `f128.cpp`; keep f128
  scalar register conversion explicit and fail-closed for unsupported banks.
- `f128.hpp` currently redeclares an existing surface from `instruction.hpp`;
  duplicate default arguments remain intentionally avoided there. Remove or
  narrow corresponding declarations in `instruction.hpp` only if needed for
  clean ownership and include flow.

## Proof

Exact delegated proof passed after moving the f128 prepared runtime-helper
boundary record construction cluster into `f128.cpp` and narrowing f128 scalar
register-class selection:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`.
`test_after.log` is preserved. The run built the f128 owner split and passed
139/139 backend tests.
