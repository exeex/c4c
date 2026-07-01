Status: Active
Source Idea Path: ideas/open/482_semantic_frame_slot_materialization_probe_decomposition.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Bind Each Focused Case To One Owned Backend Seam

# Current Packet

## Just Finished

Completed Step 3 for idea 482 by binding each accepted focused probe candidate
to exactly one backend seam, one owner, and one fail-closed boundary. No tests
or implementation files were added.

Binding matrix:

| Candidate | Proposed future path | Backend seam | Owner | Fail-closed boundary |
| --- | --- | --- | --- | --- |
| Scalar compare result forced to frame-slot destination | `tests/backend/case/riscv64_scalar_compare_frame_slot_destination.c` | Semantic instruction-result identity plus frame-slot destination/layout facts. | Prepared semantic identity/destination fact producer and prepared dump/fixture coverage. | Must not claim materialization authority, source facts, branch-stack-load availability, or RV64 consumption from final home/layout alone. |
| Storage-only move rejection | `tests/backend/case/riscv64_storage_only_move_not_semantic_materialization.c` | Materialization-authority rejection for `authority=none` value/storage movement. | Prepared semantic materialization authority checker/rejection status. | Must reject raw value-to-value or value-to-slot copies unless an explicit semantic result materialization event authority exists. |
| Select-result stack-destination boundary | `tests/backend/case/riscv64_select_result_stack_destination_boundary.c` | Select-result publication boundary. | Prepared select-result/source-producer publication layer. | Must not treat select-result stack destination as scalar compare-result materialization for a different value. |
| Explicit synthetic materialization-point positive | `tests/backend/case/riscv64_explicit_compare_frame_slot_materialization_point.c` | Materialization-authority positive path from explicit event inputs. | Prepared semantic result frame-slot materialization-point producer/checker. | Must fail closed when event source/result, destination slot/object/layout, or authority is missing or inferred from raw shape. |

Proposed future proof command for any test/probe packet:

```sh
cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_' && git diff --check
```

First narrowest generic seam for Step 4:

- Target: `tests/backend/case/riscv64_scalar_compare_frame_slot_destination.c`
  as a proposed focused test/probe addition.
- Seam: semantic scalar result identity plus frame-slot destination/layout.
- Reason: it is the smallest non-duplicative seam that can establish the
  destination baseline while explicitly failing closed on materialization
  authority and all downstream consumers.

Artifact:

- `build/agent_state/482_step3_probe_seam_bindings/bindings.md`

## Suggested Next

Execute Step 4 by selecting the scalar compare result forced to frame-slot
destination as the first focused implementation/probe packet, or record why
even that probe cannot be represented without duplicating the monolithic
`930930-1` route. The packet should remain bounded to semantic identity plus
destination/layout facts and should not implement materialization authority.

## Watchouts

- Do not add a test that passes by using raw BIR adjacency, final home, stack
  object, offset, value name, function name, testcase shape, or dump order as
  materialization authority.
- Do not let the scalar destination probe publish source facts,
  `PreparedBranchStackLoadAuthority`, or RV64 branch-load behavior.
- Keep storage-only move rejection, select-result stack destination, and
  explicit synthetic materialization-point positive probes as separate later
  packets unless Step 4 deliberately records no executable scalar destination
  seam.

## Proof

Delegated proof:

```sh
git diff --check
```

Result: passed.
