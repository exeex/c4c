Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed `plan.md` Step 2 on the adjacent local-memory store lane by extending
shared `lir_to_bir` byte-storage reinterpret handling through nested aggregate
GEP views and by routing reinterpreted scalar accesses through addressed local
byte-storage slots instead of mismatched direct scalar slots. That moved
`c_testsuite_x86_backend_src_00046_c` out of the `store local-memory semantic
family`; it now reaches the same x86 prepared-module control-flow boundary as
`c_testsuite_x86_backend_src_00042_c` (`minimal single-block i32 return
terminator or bounded compare-against-zero branch`). The proving pair now moves
together as one shared local-memory store capability lane.

## Suggested Next

Start the next bounded packet on `plan.md` Step 3 and keep it honest at the x86
prepared-module boundary: use the now-lowered `00042`/`00046` pair plus the
direct `backend_x86_handoff_boundary` probe to define the smallest shared
single-block control-flow/return lane needed for these cases, without widening
into unrelated backend families.

## Watchouts

- Do not weaken `x86_backend` expectations to accept fallback LLVM IR.
- Do not add testcase-named shortcuts or rendered-text recognizers.
- The local-memory store lane is now coherent; the next blocker is the x86
  prepared-module control-flow contract shared by both `00042` and `00046`.
- The direct handoff boundary test name is `backend_x86_handoff_boundary`; it
  still aborts on the canonical prepared-module support contract and should be
  the control-flow sentinel for the next packet.
- Keep the next packet on the minimal single-block control-flow/return lane
  needed by the proving pair; do not widen into scalar-cast, global-data, or
  broader multi-block routes.
- `00037` is still an adjacent scalar-cast-family failure and remains out of
  scope for this packet.

## Proof

Supervisor-chosen narrow proof command for this packet:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00042_c|c_testsuite_x86_backend_src_00046_c)$' >> test_after.log 2>&1`.
The build completed and `backend_lir_to_bir_notes` stayed green. Within that
exact subset, both `00042` and `00046` now fail at the same x86
prepared-module control-flow boundary instead of splitting across local-memory
store vs. control-flow families. A direct semantic-BIR probe of `00046`
confirms the nested anonymous-wrapper `v.c` path now lowers through addressed
local byte-storage accesses (`addr %lv.v.8`) before hitting the same prepared
module blocker.
Proof log:
`test_after.log`.
