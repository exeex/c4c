Status: Active
Source Idea Path: ideas/open/57_aarch64_hfa_aggregate_return_result_consumption_repair.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Consume ABI Result Lanes In Caller-Side Aggregate Uses

# Current Packet

## Just Finished

Step 3 - Consume ABI Result Lanes In Caller-Side Aggregate Uses: taught the
AArch64 call-result scalar-state publication path to consume prepared
`AfterCall` `CallResultAbi` lane bindings through
`module::find_prepared_after_call_result_lane_binding`. After a call, dispatch
now records the scalar call result and authoritative ABI result lane registers,
then reasserts only explicit `result_lanes` after prepared after-call moves.
That keeps immediate post-call aggregate consumers on the live `s0`/`s1` ABI
lanes while preserving scalar before-return publication such as the required
`x21 -> x0` move. This covers the traced `%t95`/`%t95.hfa.ret.lane.1` and
`%t100`/`%t100.hfa.ret.lane.1` lane paths without testcase names, raw BIR
scans, or expectation changes.

## Suggested Next

Have the supervisor review whether Step 3 is sufficient to close or whether the
runbook needs a final cleanup/review packet around call-result lane publication
shape and broader validation.

## Watchouts

- Explicit ABI lane records are intentionally reasserted after prepared
  after-call moves because those moves can publish copied FPR homes; post-call
  aggregate lane consumers should prefer the live call-result ABI lanes until a
  later call clobbers them.
- The post-move reassertion must stay lane-only. Reasserting scalar
  `call->result` values there masks required `BeforeReturn`
  `FunctionReturnAbi` publications when the scalar result home has already been
  copied to a callee-saved register.
- The implementation consumed existing prepared lookup authority only. It did
  not change tests, expectations, prepared lookup construction, variadic
  handling, by-value argument publication, or callee-side return publication.
- If broader review wants a smaller shape, the likely refactor is extracting the
  scalar `PreparedCallPlan::result` publication and prepared lane publication
  into separate helpers inside `calls.cpp`; behavior should remain the same.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_return_lowering|backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`
was run and captured in `test_after.log`. Build succeeded; all 9 delegated
tests passed, including `backend_aarch64_return_lowering` and
`c_testsuite_aarch64_backend_src_00204_c`.
