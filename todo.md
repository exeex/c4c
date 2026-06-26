Status: Active
Source Idea Path: ideas/open/395_rv64_object_route_instruction_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Classify Instruction Fragments

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from
`ideas/open/395_rv64_object_route_instruction_fragment_lowering.md`.

## Suggested Next

Execute Step 1: classify the `unsupported_instruction_fragment` bucket, starting
from `tests/c/external/gcc_torture/src/20000223-1.c`, and identify the first
reusable fragment family for repair.

## Watchouts

- Do not match on testcase filenames or source-specific constants.
- Do not weaken gcc_torture expectations, supported-path contracts, or
  allowlists.
- Do not reconstruct BIR control/data-flow ownership inside RV64 object
  emission.
- Split producer-side missing facts into a separate idea instead of hiding them
  in target lowering.

## Proof

No code proof yet; this is lifecycle-only activation.
