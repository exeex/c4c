Status: Active
Source Idea Path: ideas/open/lir-hir-to-lir-index-tightening.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reclassify Top-Level LIR Headers

# Current Packet

## Just Finished

Lifecycle activation created the active runbook and initialized executor state
for Step 1.

## Suggested Next

Execute Step 1 from `plan.md`: inspect top-level LIR header users, classify
exported surfaces versus model subheaders, make the smallest structural/header
edits needed, and build `c4c_codegen`.

## Watchouts

- Keep the work structural and navigational.
- Do not change HIR-to-LIR semantics, LIR semantics, printer output, verifier
  behavior, or testcase expectations.
- Do not introduce one header per `.cpp`.
- Record separate semantic cleanup as a new idea instead of expanding this
  active plan.

## Proof

Not run; lifecycle activation only.
