# Current Packet

Status: Active
Source Idea Path: ideas/open/716_prepared_call_plan_cursor_complete_production.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Revalidate cursor-complete producer and lookup authority

## Just Finished

- Plan Step 1 revalidated the cursor-complete prepared-call producer and exact
  position lookup without code or expectation changes. The direct-extern
  boundary reached and passed its adjacent cursor-0 fixed-arity/cursor-1
  variadic positive assertions, unique compatible refinement assertion, and
  duplicate, out-of-range, and operand-contradicting negative assertions.
- The fixed proof then exposed only later independent failures: the frame/stack
  contract aborts in
  `check_x86_module_emitter_reads_grouped_spill_reload_authority` because the
  x86 emitter cannot obtain prepared MIR core facts for a defined function;
  the handoff boundary reaches the joined-branch suite and fails x86 target
  materialization of register-source shared edge-publication moves. Neither
  failure is owned by idea 716's common call-plan producer or exact lookup.

## Suggested Next

- Execute Plan Step 2's independent idea-716 route-quality review, while
  keeping the two classified downstream x86 consumer failures outside this
  producer/lookup slice.

## Watchouts

- The focused CTest command is not globally green: test 320 aborts in x86
  prepared MIR core-view/emitter integration, and test 369 fails in joined-edge
  x86 target materialization after the direct-extern idea-716 assertions pass.
- Do not absorb either downstream owner into common call-plan production, and
  continue to reject callee-name, source-order, nearest-cursor, route, and
  target-local fallback authority.

## Proof

- Ran exactly
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_x86_handoff_boundary)$' > test_after.log 2>&1`.
  The build passed (`ninja: no work to do`); CTest ran 2 tests and reported 0
  passed, with the two independently owned later failures classified above.
  Canonical proof log: `test_after.log`.
