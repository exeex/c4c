Status: Active
Source Idea Path: ideas/open/351_aarch64_nested_select_false_value_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Nested Select Value Boundary

# Current Packet

## Just Finished

Lifecycle switch completed. Idea 316 was closed after the frame-layout repair
advanced `00182` past the caller local-array underallocation and reclassified
the remaining seven-`7` output mismatch as AArch64 nested scalar select/value
materialization.

## Suggested Next

Execute Step 1: localize where the accumulated false operand in the nested
select chain is lost, overwritten, or not materialized before the final
`csel`.

## Watchouts

- Do not special-case `00182`, `print_led`, LED line renderers, the digit
  array, `d[0]`, one temporary, one register, or one emitted instruction
  sequence.
- Do not reopen frame-slot/frame-size layout consistency from idea 316 unless
  fresh evidence shows a current frame allocation or stack-slot divergence.
- Do not reopen unsigned div/rem producer publication, direct-call lowering,
  prepared call/address materialization, runner behavior, timeout policy,
  expectation changes, unsupported-classification changes, or CTest
  registration.
- Check whether idea 345's closed stack-home select publication failure has
  actually returned before treating this as the same owner.

## Proof

Lifecycle-only transition. No implementation proof was run in this packet.
Close decision for idea 316 used the existing matching focused
`test_before.log` and `test_after.log` with non-decreasing regression guard
mode, and the delegated proof state reported the broader backend guard passing
141/141.
