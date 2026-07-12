# Current Packet

Status: Active
Source Idea Path: ideas/open/708_x86_named_handoff_materializer_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate direct-call and scalar handoff materialization

## Just Finished

- Plan Step 2 migrated x86 direct-call and scalar call-argument materialization to cursor-exact common prepared call/argument plans. The consumer now requires the prepared argument selector and source identity, checks call-plan/move-bundle destination agreement, and rejects missing or inconsistent authority without consulting Route 6 or falling back to a semantic operand name.

## Suggested Next

- Review and accept the Plan Step 2 slice, then select the next plan packet.

## Watchouts

- Route 6 lookup/debug compatibility remains present, but direct-call/scalar materialization no longer consumes it.
- The delegated boundary subset remains red only at the pre-existing unrelated `x86 module route did not emit register-source shared-publication edge moves` assertion; the owned direct-call/scalar assertions add no failure.

## Proof

- Ran `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_x86_handoff_boundary$' >> test_after.log 2>&1`.
- Build passed. The selected test reached the same single unrelated baseline failure recorded in `test_before.log`; `test_after.log` is the canonical proof log.
