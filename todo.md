Status: Active
Source Idea Path: ideas/open/241_inline_asm_clobber_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Clobber Ingress

# Current Packet

## Just Finished

Lifecycle split from `ideas/closed/240_aarch64_inline_asm_machine_nodes.md`.
The closed idea completed backend-local inline-asm machine-node support for
currently structured operand, name, output, tie, immediate, modifier, and
side-effect surfaces. Remaining durable clobber, memory/address, and
allocator-policy work now lives in separate open ideas.

## Suggested Next

Execute Plan Step 1: inventory clobber ingress across source/LIR/BIR, identify
the first structured carrier packet, and record the delegated proof subset
before implementation begins.

## Watchouts

- Do not parse clobbers from rendered template text, final assembly, or
  diagnostic strings.
- Preserve the completed inline-asm behavior from closed idea 240.
- Keep memory/address constraints in
  `ideas/open/242_inline_asm_memory_address_constraints.md`.
- Keep alias-aware tied-home allocation policy in
  `ideas/open/243_inline_asm_tied_home_allocation_policy.md`.

## Proof

Lifecycle-only transition. Existing close gate for idea 240 used the matching
backend logs already produced at Step 6:

`test_before.log`: 139/139 backend tests passing.

`test_after.log`: 139/139 backend tests passing.

Regression guard command:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: passed with no new failures.
