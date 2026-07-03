Status: Active
Source Idea Path: ideas/open/562_bir_direct_call_semantic_boundary_evidence.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Repair Only The Named Direct-Call Boundary

# Current Packet

## Just Finished

Step 4: repaired the BIR byval aggregate direct-call argument publication
boundary and added the focused tracked Step 3 surface.

Changed files:
- `src/backend/bir/lir_to_bir/calling.cpp`
- `tests/backend/bir/CMakeLists.txt`
- `tests/backend/case/byval_param_direct_call_publication.c`

Completed work:
- Direct-call typed argument parsing now accepts a callee-expected byval pointer
  ABI parameter when the source call still spells the argument as a non-scalar
  aggregate value, leaving layout selection to the structured argument/type-ref
  metadata.
- Direct-call scalar argument type lookup now falls back from empty structured
  argument type refs to populated `arg_type_refs`, which keeps adjacent scalar
  arguments from failing before the byval aggregate argument is lowered.
- Byval aggregate call argument lowering now publishes an existing aggregate
  parameter slot as the call argument source when the operand names a by-value
  aggregate formal.
- Added the tracked semantic BIR route test
  `backend_codegen_route_x86_64_byval_param_direct_call_publication_observe_semantic_bir`,
  requiring the direct call
  `bir.call i32 consume_trio(i32 %p.seed, ptr byval(size=12, align=4) %lv.param.p.value)`
  and forbidding the LLVM aggregate-value call spelling
  `call i32 (i32, %struct.Trio) @consume_trio`.

## Suggested Next

Supervisor should review the Step 4 diff for route quality and decide whether
the active runbook is ready for lifecycle review or another bounded packet.

## Watchouts

- The actual canonical byval parameter slot for `%p.value` is
  `%lv.param.p.value`, not the Step 3 forecast spelling
  `%lv.param.value.value`.
- The repair does not touch RV64 call lowering or reconstruct target-shaped
  argument chunks; it only publishes the semantic BIR direct-call argument fact.
- Diagnostics remain fail-closed through the existing direct-call semantic
  family guards when structured metadata is absent or aggregate layout cannot
  be selected.

## Proof

Ran delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log && git diff --check -- todo.md src/backend/bir tests/backend`.

Result: passed. `test_after.log` contains the backend CTest subset output and
reports `Total Test time (real) =   2.21 sec`.
Supervisor regression comparison also passed with:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log
```

The strict monotonic guard passed because the focused backend route test
increased the selected CTest count from 345 to 346.

Focused pre-proof check also passed:
`ctest --test-dir build --output-on-failure -R '^backend_codegen_route_x86_64_byval_param_direct_call_publication_observe_semantic_bir$'`.
