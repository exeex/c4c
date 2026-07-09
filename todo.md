Status: Active
Source Idea Path: ideas/open/624_prepared_outgoing_stack_argument_destination_offsets.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh outgoing-stack argument evidence

# Current Packet

## Just Finished

Activation created the runbook from
`ideas/open/624_prepared_outgoing_stack_argument_destination_offsets.md`; no
implementation work has started.

## Suggested Next

Execute Step 1 from `plan.md`: refresh current outgoing stack-slot, byval, and
aggregate stack-copy call argument diagnostics, record the command and proof
artifact, and classify first-owner buckets before any implementation work.

## Watchouts

- Do not infer destination offsets inside RV64 from ABI index, final assembly
  layout, source filename, or testcase shape.
- Keep `src/20000808-1.c`, the `931004-*` family, `src/931031-1.c`,
  `src/950607-2.c`, and `src/pr69447.c` as breadth or guard evidence, not
  named-case implementation targets.
- Do not broaden into variadic/library policy, runtime mismatch, local/global
  producers, stack-frame consumers, expectations, unsupported markers,
  allowlists, timeouts, or accounting changes.

## Proof

Lifecycle activation only; no build or test proof required.
