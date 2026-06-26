Status: Active
Source Idea Path: ideas/open/378_rv64_object_route_20000112_instruction_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit First Unsupported Instruction Fragment

# Current Packet

## Just Finished

Activation created this execution state from
`ideas/open/378_rv64_object_route_20000112_instruction_fragment_lowering.md`.

## Suggested Next

Execute Step 1: audit the first ordinary prepared instruction-fragment shape
rejected by the RV64 object route for `src/20000112-1.c`, using prepared and
object-route evidence rather than testcase-shaped matching.

## Watchouts

- Do not reopen idea 369 terminator-fragment lowering or CFG reconstruction.
- Do not treat diagnostic-only changes, allowlist edits, expectation rewrites,
  or named-case shortcuts as capability progress.
- Keep any implementation scoped to semantic instruction-fragment lowering and
  fail-closed adjacent shapes.

## Proof

No activation-time code proof was run. The first executor packet should record
its audit/proof artifacts and update `test_after.log` if it runs a delegated
proof command.
