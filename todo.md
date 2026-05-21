Status: Active
Source Idea Path: ideas/open/376_aarch64_sizeof_loop_bound_stack_home_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Focused Coverage

# Current Packet

## Just Finished

Step 3 added focused backend coverage in
`tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp` for a
fused compare whose RHS is a named same-block constant-binary `%sizeof.bound`
value with a prepared stack-slot home. The new direct-dispatch case requires
the branch to compare against the folded immediate `#64` and rejects any `ldr`
from the RHS stack home `[sp, #32]`, guarding the unpublished stack-home RHS
pattern independently of external `00205`.

## Suggested Next

Supervisor should review the Step 2 implementation plus Step 3 focused
coverage as one coherent slice and decide whether to commit or request any
additional route-quality review.

## Watchouts

The new test allows the constant producer to publish its own stack home, but it
fails if the fused compare consumes that stack home by reloading `[sp, #32]`
instead of comparing against the folded constant. Existing cast-owned and
stack-publication route tests stayed in the delegated proof subset.

## Proof

Ran exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_branch_control_lowering|backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00205_c)$'; } > test_after.log 2>&1`

Result: passed. The build rebuilt and linked
`backend_aarch64_branch_control_lowering_test`; CTest ran
`backend_aarch64_branch_control_lowering`,
`backend_aarch64_instruction_dispatch`, and
`c_testsuite_aarch64_backend_src_00205_c`; all three passed. The canonical
proof log is `test_after.log`.
