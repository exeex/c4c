Status: Active
Source Idea Path: ideas/open/216_aarch64_machine_node_asm_printer_external_smoke.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Reconcile Failure Boundaries And Documentation

# Current Packet

## Just Finished

Step 5 reconciled the public AArch64 `.s` route wording in CLI help and nearby
AArch64 backend documentation.

Completed work:
- Updated `c4cll --help` wording to show the concrete command shape
  `c4cll --codegen asm --target aarch64-linux-gnu test.c -o out.s`.
- Documented that AArch64 asm output is `.s` printer output from selected
  machine nodes.
- Replaced stale docs that described future-only printer work or the old
  fallback text-emitter path with the current selected-machine-node printer
  route.
- Preserved the boundaries that `c4cll` does not parse printed assembly back
  into backend semantics and does not provide a built-in assembler, encoder,
  object writer, or linker for this route.

## Suggested Next

Supervisor should review and commit the Step 5 wording/docs slice, then hand
the active plan to plan-owner for close/deactivate/split decision if all todo
items are complete.

## Watchouts

- The `.s` printer is output-only; do not parse printed assembly back into
  backend semantics.
- Do not bypass BIR/prepared BIR, AArch64 target MIR, or machine-node selection
  when serving the public CLI route.
- Do not implement an internal assembler, encoder, object writer, linker, or
  `.s` parser in this plan.
- The selected return printer remains intentionally narrow: it prints void
  returns and small non-negative integer immediates to `w0`/`x0` followed by
  `ret`.

## Proof

Delegated docs/help proof was written to `test_after.log`:

`{ rg ... stale-claim searches; git diff --check; } > test_after.log 2>&1`

Result: old fallback/text-emitter route searches, parser-roundtrip-as-implemented
route searches, and implemented encoder/object/linker support searches produced
no stale claims. The only parser-roundtrip hit is the explicit rejected route in
`MACHINE_INSTRUCTION_NODE_CONTRACT.md`.

No focused help-text assertion test was found, so this packet used the
delegated docs-only proof path rather than adding a broader build/test subset.
