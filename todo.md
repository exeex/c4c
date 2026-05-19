Status: Active
Source Idea Path: ideas/open/306_aarch64_symbol_offset_address_materialization_width.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate The Symbol+Offset Address Width Owner

# Current Packet

## Just Finished

Lifecycle switch completed from umbrella inventory idea 295 to focused owner
idea 306. No implementation work has started under the new owner.

## Suggested Next

Execute Step 1: locate where the AArch64 backend selects or preserves a 32-bit
`wN` register for symbol+offset address temporaries in `00050.c`, `00176.c`,
and `00182.c`.

## Watchouts

- Keep `00189.c` out of scope unless generated-code evidence proves the same
  repair owns externally binding GOT/PIC symbol references such as `stdout`.
- Reject testcase-shaped fixes, exact emitted-instruction matching,
  expectation rewrites, allowlist changes, unsupported classification changes,
  timeout policy changes, runner behavior changes, CTest registration changes,
  or proof-log policy changes.
- Preserve the distinction between 64-bit address temporaries/memory bases and
  32-bit scalar data values loaded from or stored to memory.

## Proof

Lifecycle-only switch. No build or tests were run, and canonical proof logs
were not modified.
