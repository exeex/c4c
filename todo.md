Status: Active
Source Idea Path: ideas/open/71_aarch64_scalar_control_flow_prepared_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Consume BIR Scalar And Cast Semantic Facts

# Current Packet

## Just Finished

Completed `plan.md` Step 2 first slice for AArch64 ALU/cast semantic fact
consumption. `make_prepared_scalar_alu_record` now consumes a local
`ScalarAluSemanticFacts` boundary derived from `bir::BinaryOpcode` and
`bir::TypeKind` before AArch64-local operand, immediate, register, and record
selection. `make_prepared_scalar_cast_record` now consumes a local
`ScalarCastSemanticFacts` boundary derived from `bir::CastOpcode` and
`bir::TypeKind` before AArch64-local storage, register spelling, and machine
record construction.

## Suggested Next

Next coherent packet: continue `plan.md` Step 2 by applying the same
semantic-fact-consumption pattern only where the supervisor scopes it, or move
to the next planned control-flow packet if Step 2 is accepted as complete.

## Watchouts

Immediate admissibility, fallback materialization, register spelling, scratch
selection, and machine-record construction remain AArch64-local. This packet
did not touch comparison/control-flow code, shared prealloc code, `plan.md`, or
the source idea.

## Proof

Ran the delegated proof exactly:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: passed. `test_after.log` contains a successful build and 169/169
matching backend tests passed.
