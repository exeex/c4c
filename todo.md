Status: Active
Source Idea Path: ideas/open/261_aarch64_f128_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Move F128 Construction, Transport, And Lowering Bodies

# Current Packet

## Just Finished

Step 3: Move F128 Construction, Transport, And Lowering Bodies resolved the
f128 transport-lowering ownership handoff as a todo-only decision. The existing
boundary should stay in place: `memory.cpp` keeps
`lower_f128_transport_instruction` because that body is mostly load/store
classification, structured `MemoryOperand` preparation, memory diagnostics, and
`MachineInstruction` wrapping, while `f128.cpp` owns the f128-specific carrier
transport record construction and `F128Transport` instruction construction that
the memory lowering already calls.

No code changed. Moving the lowering body itself would require pulling
memory-owned operand preparation/diagnostic helpers into `f128.cpp` or exposing
new memory helper seams just for this route, which would widen the Step 3 shard
instead of clarifying f128 ownership.

## Suggested Next

Continue Step 3 with the f128 runtime-helper lowering handoff. The live lowering
body is in `dispatch.cpp`, so the next packet should either explicitly include
that owned file for a narrow f128-owner route or record that dispatch-owned
runtime-helper lowering remains the correct router while f128 construction stays
in `f128.cpp`.

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

## Proof

Audit-only todo update; no code changed, so the delegated packet did not require
a build or test run. No new `test_after.log` was created by this packet.
