Status: Active
Source Idea Path: ideas/open/63_aarch64_shared_select_chain_dependency_authority.md
Source Plan Path: plan.md
Current Step ID: Step 6
Current Step Title: Prove Select-Chain Coverage And Ownership Split

# Current Packet

## Just Finished

Completed the Step 6 blocker fix for repeated select materialization packages.
Replaced emitted `.Lselect_mat_*` branch labels with deterministic assembler
numeric local labels (`1f`/`1:`, `2f`/`2:`, and so on) in the target-local
select materialization and edge-publication packaging paths. Repeated packages
can now reuse the same local label numbers without global assembler symbol
collisions, while each branch still resolves inside its own package. The repair
did not change prepared dependency authority, add semantic discovery, or weaken
the failing c-testsuite cases.

Updated focused AArch64 instruction-dispatch assertions to recognize reusable
numeric local labels and to keep checking the materialized selected value feeds
the fused compare/branch path.

## Suggested Next

Supervisor should review and commit the Step 6 blocker fix with the updated
proof log, then decide whether the active runbook is ready for lifecycle
review/closure.

## Watchouts

The fix intentionally uses assembler numeric local labels rather than a
process-local counter or address-derived discriminator, so output remains
stable across runs. `select_chain_label` remains available for deterministic
named-label construction, but emitted repeated packages now use reusable local
label references/definitions.

## Proof

Focused proof command run first:
`(cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|backend_aarch64_instruction_dispatch)$') > test_after.log 2>&1`

Result: passed.

Full Step 6 proof command run exactly:
`(cmake --build build -j && ctest --test-dir build -j --output-on-failure) > test_after.log 2>&1`

Result: passed; `test_after.log` is the canonical proof log from the full
proof. `git diff --check` passed.
