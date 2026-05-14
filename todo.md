Status: Active
Source Idea Path: ideas/open/219_aarch64_natural_operator_naming_and_printer_spelling.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Current AArch64 Naming And Spelling Surfaces

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from idea 219 and aligned this
executor scratchpad to Step 1.

## Suggested Next

Execute Step 1: audit the current AArch64 machine-node, structured
asm/encoding, printer, and diagnostic naming surfaces. Update this file with
the audited paths, supported naming surfaces, and concrete drift or duplication
targets before implementation changes begin.

## Watchouts

- Keep idea 218's boundary intact: printed `.s` is terminal renderer output,
  not semantic backend input.
- Do not touch `ideas/open/220_aarch64_markdown_driven_backend_case_bringup.md`.
- Do not expand instruction selection coverage just to add more operator names.
- Do not downgrade expectations or introduce testcase-shaped shortcuts.

## Proof

Activation is lifecycle-only. No build proof is required until an executor
packet changes implementation files.
