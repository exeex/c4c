Status: Active
Source Idea Path: ideas/open/393_rv64_variadic_aggregate_va_arg_cursor_stride.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify The Stride/Layout Rule

# Current Packet

## Just Finished

Step 2 classified the narrow RV64 aggregate `va_arg` cursor stride rule and
the guard set for implementation.

Selected rule: for supported RV64 aggregate `va_arg` payloads that are passed
in variadic GPRs and saved into the RV64 incoming variadic GPR publication
area, cursor progression must use the RV64 ABI GPR save-area slot width from
prepared RV64 incoming-GPR publication layout facts. The aggregate payload copy
size remains the aggregate payload size.

For the `920908-1.c` shape, the payload/copy size is 4 bytes, but the prepared
incoming variadic GPR publications show 8-byte slots: `a2` at offset 56 and
`a3` at offset 64. The bad Step 1 fact is therefore
`progression_stride=4`/`overflow_stride=4`; the supported repair should
publish/use an 8-byte slot stride while still copying 4 bytes.

The implementation owner is `src/backend/prealloc/variadic_entry_plans.cpp`,
specifically the RV64 aggregate helper path. `make_rv64_aggregate_va_arg_access_plan`
currently delegates directly to the generic overflow aggregate plan, which
derives stride from aggregate size/alignment. Step 3 should add the RV64
publication-backed refinement there rather than changing the generic overflow
helper.

Detailed classification and guard set:
`build/agent_state/393_step2_stride_rule.log`.

## Suggested Next

Execute Step 3 by adding focused backend coverage for the selected rule, then
repair `make_rv64_aggregate_va_arg_access_plan` so a unique matching RV64
incoming GPR publication slot provides `source_slot_size_bytes`,
`progression_stride_bytes`, and `overflow_stride_bytes`, while preserving
`payload_size_bytes` and `copy_size_bytes`.

## Watchouts

- Do not clone clang instruction shape; use clang only as ABI confirmation that
  this register-saved aggregate cursor advances by 8 while loading a 4-byte
  payload.
- Keep the route fail-closed unless the prepared RV64 publication facts are
  explicit and unique.
- Preserve generic overflow-memory aggregate behavior when no unique RV64 GPR
  publication slot applies.
- Focused fail-closed coverage should include: missing publications,
  non-8-byte publication size/align, non-contiguous or misaligned publication
  offsets, source/copy crossing the matched slot, missing source payload offset,
  mismatched progression/overflow stride, missing payload write address, and
  non-RV64 targets.
- Object emission is mainly a consumer of prepared facts here; it should keep
  rejecting malformed aggregate access plans through the existing diagnostic.

## Proof

Read-only classification packet. No new proof command was required and
`test_after.log` remains the Step 1 representative/evidence proof log.

Files inspected:
- `plan.md`
- `todo.md`
- `build/agent_state/393_step1_analysis.log`
- `build/agent_state/393_step1_920908-1.prepared.log`
- `build/agent_state/393_step1_920908-1.clang-disasm.log`
- `src/backend/prealloc/variadic_entry_plans.cpp`
- `src/backend/prealloc/variadic.hpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`

Result: route classified; no blocker found.
