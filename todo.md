Status: Active
Source Idea Path: ideas/open/369_rv64_object_route_terminator_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit First Terminator Fragment

# Current Packet

## Just Finished

Lifecycle activation created this executor-compatible scratchpad for Step 1 of
`plan.md`.

## Suggested Next

Audit the first prepared terminator fragment behind the current
`unsupported_terminator_fragment` failures for `src/20000224-1.c` and
`src/20000112-1.c`.

## Watchouts

- Do not key behavior on testcase names, source syntax, instruction indexes,
  block numbering accidents, labels, or prepared-BIR text matching.
- Do not reconstruct CFG, branch targets, edge order, or value publication
  outside the prepared module contract.
- Diagnostic-only changes are not capability progress unless followed by
  semantic lowering.
- Keep data-section, global, string, relocation, byval, variadic, and
  non-register parameter-home ownership out of this plan unless the audited
  terminator shape directly requires it.

## Proof

Lifecycle-only activation; no build required.
