Status: Active
Source Idea Path: ideas/open/71_aarch64_scalar_control_flow_prepared_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Consume Prepared Branch And Short-Circuit Facts

# Current Packet

## Just Finished

Completed `plan.md` Step 4 for AArch64 comparison/branch lowering. Conditional
branch helper paths now consume prepared branch-condition facts, prepared
control-flow labels, and available short-circuit branch plans through a local
prepared-fact boundary before AArch64-local condition spelling, operand
selection, branch record updates, and assembler-line construction.

## Suggested Next

Next coherent packet: implement `plan.md` Step 5 by consuming prepared
materialized-compare join and fused-operand producer facts before local compare
operand materialization and compare record construction in `comparison.cpp`.

## Watchouts

The new local branch-fact boundary keeps materialized-bool branches on prepared
control-flow labels and only resolves compare/short-circuit branch plans when
prepared compare operands and predicates exist. Do not widen the next packet
into shared prealloc or expectation rewrites.

## Proof

Ran the delegated proof exactly:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: passed. `test_after.log` contains a successful build and 169/169
matching backend tests passed.
