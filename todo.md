Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Select the Next Semantic Owner

# Current Packet

## Just Finished

Step 3 - Select the Next Semantic Owner: selected the next focused owner
candidate from the classified backend-regex inventory without changing
implementation, tests, expectations, runner behavior, timeout policy, proof-log
policy, `plan.md`, or `ideas/open/*`.

Selected owner candidate: residual AArch64 semantic `lir_to_bir`
local-memory admission for prepared-module handoff.

Semantic scope:

- Admit and preserve structured semantic lowering facts for local-memory
  GEP/load forms that currently fail before AArch64 prepared-module handoff.
- Representative diagnostics from `test_after.log`:
  - `c_testsuite_aarch64_backend_src_00204_c`: `semantic lir_to_bir function
    'myprintf' failed in gep local-memory semantic family`.
  - `c_testsuite_aarch64_backend_src_00216_c`: `semantic lir_to_bir function
    'foo' failed in load local-memory semantic family`.
- Artifact boundary: neither `00204.c` nor `00216.c` has generated
  `build/c_testsuite_aarch64_backend/src/*.s` or `*.bin` output in the current
  evidence, so this is a semantic admission/prepared-handoff owner, not an
  emitted AArch64 assembly, machine-printer, assembler, linker, runtime, or
  timeout owner.

Representative tests:

- `c_testsuite_aarch64_backend_src_00204_c`
- `c_testsuite_aarch64_backend_src_00216_c`
- Existing semantic dump helpers for `00204.c`:
  `backend_cli_dump_bir_00204_stdarg_semantic_handoff`,
  `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff`,
  `backend_cli_dump_bir_focus_function_filters_00204`,
  `backend_cli_dump_prepared_bir_focus_function_filters_00204`, and
  `backend_cli_dump_prepared_bir_focus_block_entry_00204`
- Existing broad semantic guard: `backend_lir_to_bir_notes`

Closed-owner boundary reasoning:

- Do not reopen idea 298 by residual count alone. The current evidence does
  not show a return of the closed global/pointer/aggregate projection owner;
  it shows precise residual admission failures reported as `gep local-memory
  semantic family` and `load local-memory semantic family` before
  prepared-module handoff.
- Do not select `00164.c` yet. It is a crisp selected scalar `mul` printable
  RHS diagnostic, but it sits near closed idea 302's scalar machine-node
  operand boundary and needs a focused boundary probe before a new owner is
  safe.
- Do not select `00214.c` yet. It is a crisp prepared frame-slot source
  diagnostic, but it sits near closed memory-store and scalar operand
  boundaries; it should be probed separately from the semantic `lir_to_bir`
  admission pair.
- Runtime nonzero, runtime mismatch/crash, output-storm, and timeout buckets
  remain parked because their current evidence does not identify a first bad
  backend fact suitable for a focused owner without narrower generated-artifact
  probes.

Proposed focused proof command for the owner split:

```bash
cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_cli_dump_bir_00204_stdarg_semantic_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff|backend_cli_dump_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_block_entry_00204|c_testsuite_aarch64_backend_src_00204_c|c_testsuite_aarch64_backend_src_00216_c)$' > test_after.log 2>&1
```

Acceptance for the focused owner should require the two c-testsuite cases to
advance past the current semantic `lir_to_bir` admission diagnostics without
expectation changes, unsupported downgrades, runner edits, timeout-policy
changes, or filename-shaped special cases. If either case advances to a later
printer/runtime residual, that later residual belongs to a separate owner
unless generated-code evidence proves the same semantic repair owns it.

## Suggested Next

Ask the plan owner to perform Step 4 by splitting and activating a focused
source idea for residual AArch64 semantic `lir_to_bir` local-memory admission,
using `00204.c` and `00216.c` as representatives and the proof scope above as
the initial focused validation contract.

## Watchouts

- This packet selected an owner candidate only; no implementation files,
  tests, expectations, allowlists, unsupported classifications, runner
  behavior, timeout policy, proof-log policy, `plan.md`, or `ideas/open/*`
  were changed.
- The selected owner should stay at semantic admission/prepared-handoff scope.
  Do not fold in the `00164.c` scalar `mul` printable-RHS diagnostic, the
  `00214.c` frame-slot source diagnostic, runtime buckets, or timeout buckets
  without new generated-code or diagnostic evidence.
- `00204.c` and `00216.c` currently have no generated `.s`/`.bin` artifacts;
  any later assembly or runtime residual after admission repair should be
  reclassified before expanding the owner.
- The proposed proof command includes existing `00204.c` semantic dump helpers
  because no comparable `00216.c` dump helper is currently registered by name.
  A focused owner may add narrowly scoped semantic/dispatch tests as needed,
  but should not satisfy progress through test expectation downgrades.

## Proof

No new CTest run was required or performed for Step 3. Used the existing fresh
evidence log and generated artifact inventory:

```bash
cd /workspaces/c4c/build && ctest -j10 -R backend --output-on-failure
```

Captured at repo root as `test_after.log` by Step 1. Result in that log:
CTest exited nonzero with 57 failures out of 352 selected tests. The log is
suitable owner-selection evidence, not a green acceptance proof.
