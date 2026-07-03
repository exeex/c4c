Status: Active
Source Idea Path: ideas/open/562_bir_direct_call_semantic_boundary_evidence.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Focused Call-Publication Coverage

# Current Packet

## Just Finished

Step 3: identified adjacent tracked semantic-BIR printer coverage and captured
the missing focused direct-call publication surface without leaving a failing
tracked test.

Existing adjacent coverage:
- `backend_codegen_route_x86_64_byval_member_array_params_observe_semantic_bir`
  covers direct byval aggregate calls from local aggregate values in
  `tests/backend/case/param_member_array.c` and
  `tests/backend/case/nested_param_member_array.c`.
- `backend_codegen_route_x86_64_aggregate_param_return_pair_fn_param_observe_semantic_bir`
  covers a byval aggregate formal forwarded through an indirect aggregate call
  in `tests/backend/case/aggregate_param_return_pair_fn_param.c`, with the
  expected semantic BIR snippet
  `bir.call void %p.fn(ptr sret(size=8, align=4) %t0, ptr byval(size=8, align=4) %lv.param.p.p)`.

Focused missing surface:
- No tracked test found for the exact Step 2 boundary: a direct callee where a
  by-value aggregate parameter is forwarded as the aggregate argument.
- Captured the smallest red case under ignored evidence:
  `build/agent_state/562_step3_byval_call_publication/byval_param_direct_call_publication.c`.
- `./build/c4cll --dump-bir --target x86_64-unknown-linux-gnu build/agent_state/562_step3_byval_call_publication/byval_param_direct_call_publication.c`
  exits `2`; stderr in
  `build/agent_state/562_step3_byval_call_publication/byval_param_direct_call_publication.dump-bir.stderr`
  reports `semantic lir_to_bir function 'forward_trio' failed in semantic call
  family 'direct-call semantic family'`.
- LLVM call-shape evidence in
  `build/agent_state/562_step3_byval_call_publication/byval_param_direct_call_publication.llvm-call-sites.txt`
  shows `%t0 = call i32 (i32, %struct.Trio) @consume_trio(i32 %p.seed, %struct.Trio %p.value)`.

Smallest future tracked surface after repair:
- Add `tests/backend/case/byval_param_direct_call_publication.c` using the
  captured evidence case, then add a semantic BIR route test requiring
  `bir.func @forward_trio(ptr byval(size=12, align=4) %p.value, i32 %p.seed) -> i32`
  and a direct byval call resembling
  `bir.call i32 consume_trio(i32 %p.seed, ptr byval(size=12, align=4) %lv.param.value.value)`.
- The test should forbid the LLVM aggregate-value call spelling
  `call i32 (i32, %struct.Trio) @consume_trio`.

## Suggested Next

Continue with Step 4: repair only the BIR byval aggregate direct-call argument
publication boundary so the Step 3 red case can become a tracked semantic BIR
route/printer test.

## Watchouts

- Do not add the focused tracked ctest before the repair unless the supervisor
  explicitly wants a red tracked test; the captured Step 3 case currently fails
  before BIR publication.
- The existing `aggregate_param_return_pair_fn_param` test proves the byval
  formal-to-call path for an indirect aggregate call with sret, not the direct
  callee scalar-return boundary classified in Step 2.
- Keep the Step 4 repair semantic and independent of `src/20000717-1.c`; no
  expectation rewrite, unsupported downgrade, diagnostic rename, or named-case
  shortcut should count as progress.

## Proof

Ran delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log && git diff --check -- todo.md`.

Result: passed. `test_after.log` contains the backend CTest subset output and
reports `Total Test time (real) =   2.33 sec`.
Supervisor regression comparison also passed with:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Focused red evidence command:
`./build/c4cll --dump-bir --target x86_64-unknown-linux-gnu build/agent_state/562_step3_byval_call_publication/byval_param_direct_call_publication.c`
failed as expected before repair and is recorded under
`build/agent_state/562_step3_byval_call_publication/`.

Local plan-review state was aligned with `Current Step ID: 3` and
`Current Step Title: Add Focused Call-Publication Coverage` using
`scripts/plan_review_state.py set-step`.
