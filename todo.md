Status: Active
Source Idea Path: ideas/open/261_aarch64_f128_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Move F128 Construction, Transport, And Lowering Bodies

# Current Packet

## Just Finished

Step 3: Move F128 Construction, Transport, And Lowering Bodies continued the
helper-boundary extraction by moving f128 runtime-helper boundary validation
and `make_f128_runtime_helper_boundary_instruction` into `f128.cpp`. The
relocated body preserves the same diagnostics, selection status, instruction
family/opcode, register and prepared-value effects, call clobbers, call side
effect, payload, and source tracking.
`instruction.cpp` no longer owns f128 transport or runtime-helper boundary
instruction construction. No helper-boundary lowering, printer logic, tests,
`plan.md`, source idea, or markdown shard content changed.

## Suggested Next

Continue Step 3 by moving the f128 prepared runtime-helper boundary record
construction cluster into `f128.cpp`, including
`make_prepared_f128_runtime_helper_boundary_record` and only the narrowly
required f128-specific helpers. Keep lowering, dispatch, printer spelling, and
neutral instruction helpers out of the packet.

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
- Transport and runtime-helper boundary instruction construction are now owned
  by `f128.cpp`; keep the next move focused on prepared f128 runtime-helper
  record construction before touching memory lowering or printer bodies.
- `memory.cpp` already owns `lower_f128_transport_instruction`; Step 3 needs to
  decide whether that lowering is routed through `f128.cpp` or left as neutral
  memory routing that calls f128-owned construction helpers.
- `make_f128_register_operand` is still declared in `f128.hpp` because the
  unmoved prepared runtime-helper record body in `instruction.cpp` still depends
  on it. Narrow or hide that bridge only after that body moves.
- `f128.hpp` currently redeclares an existing surface from `instruction.hpp`;
  duplicate default arguments remain intentionally avoided there. Remove or
  narrow corresponding declarations in `instruction.hpp` only if needed for
  clean ownership and include flow.

## Proof

Exact delegated proof passed after moving f128 runtime-helper boundary
validation and instruction construction into `f128.cpp`:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`.
`test_after.log` is preserved. The run built the f128 owner split and passed
139/139 backend tests.
