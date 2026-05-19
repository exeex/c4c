Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Choose the Next Focused Owner

# Current Packet

## Just Finished

Step 3 from `plan.md` decided that the next focused owner is ready to split:
`00140.c` exposes an AArch64 selected call-boundary move
preparation/printing gap.

Decision evidence:

- `test_after.log` has a singleton `FRONTEND_FAIL` for
  `c_testsuite_aarch64_backend_src_00140_c` at the AArch64 machine-node
  printer:
  `printer requires selected machine node, got deferred_unsupported:
  call-boundary move node is outside the selected register call-boundary move
  subset`.
- No `build/c_testsuite_aarch64_backend/src/00140.c.s` artifact is present;
  the route stops before assembly emission, so emitted instruction spelling is
  not the owner.
- `tests/c/external/c-testsuite/src/00140.c` exercises direct calls to
  `f1(struct foo f, struct foo *p, int n, ...)`, including a variadic call
  with by-value aggregate and pointer arguments. That source shape is evidence
  for call-boundary move preparation pressure only, not a license for
  filename, argument-index, or source-shape matching.
- The producing code path already has a semantic selection gate:
  `call_boundary_move_selection_status` rejects records whose prepared move
  phase, destination kind/storage, op kind, or prepared source/destination
  facts are outside the currently selected call-boundary move subset.
- Existing backend tests already cover selected call-boundary move records and
  fail-closed printer behavior, so a focused owner can be proven with unit
  coverage plus the single c-testsuite frontend-fail probe.

## Suggested Next

Step 4 should split a focused source idea for AArch64 selected call-boundary
move preparation/printing semantics.

Suggested semantic scope:

- Repair the handoff for prepared call-boundary move records that currently
  reach the machine printer with `DeferredUnsupported` selection status even
  though the compiler has call-boundary move provenance.
- Preserve and validate prepared move facts from value homes, call argument
  plans, ABI bindings, and value-move bundles before marking a call-boundary
  move selected.
- Add focused backend coverage for the specific selected move shape that
  `00140.c` needs before relying on the c-testsuite probe.

Suggested proof command after implementation:

`cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00140_c)$' > test_after.log 2>&1`

## Watchouts

- This umbrella packet is classification only; do not implement repairs here.
- Do not reopen closed owners 285 through 309 from counts alone. This candidate
  is not the closed scalar direct-call value owner, the closed call-argument
  wrong-register authority owner, or the closed indirect-call argument
  preservation owner; the observed failure is earlier, at selected
  call-boundary move machine-node admission.
- Do not change expectations, allowlists, unsupported classifications,
  timeout policy, runner behavior, CTest registration, or proof-log policy.
- Reject any split that names only `00140.c`, `struct foo`, one argument index,
  one source spelling, or one emitted mnemonic as the owner.
- Reject any implementation that merely changes the diagnostic, suppresses the
  printer gate, or marks the node selected without a printable prepared source
  and destination.
- Keep `00164.c`, `00214.c`, `00204.c`, `00216.c`, and the broader runtime
  buckets parked; they have distinct diagnostics or need generated-code probes
  before another owner split.

## Proof

No build or CTest was run by this executor, per the Step 3 decision packet.
Read-only evidence used: supervisor-captured `test_after.log`, `plan.md`,
`tests/c/external/c-testsuite/src/00140.c`, generated-artifact presence checks,
and small diagnostic-owner reads in the AArch64 call-boundary move selection
and printer paths. `test_after.log` was not written or modified.
