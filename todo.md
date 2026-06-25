Status: Active
Source Idea Path: ideas/open/356_rv64_object_route_assembler_backed_prepared_text.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Audit RV64 Fixup, Relocation, and Label Ownership

# Current Packet

## Just Finished

Step 4 cleanup retired the duplicate staged RV64 assembler minimal JAL relocation
object branch from `src/backend/mir/riscv/assembler/elf_writer.cpp`.
Removed `PendingReloc`, `MinimalJalRelocationSlice`,
`parse_minimal_jal_relocation_slice`, `build_minimal_jal_relocation_object`,
`pending_branch_relocs_`, and `minimal_jal_relocation_slice_`, leaving
`write_elf_object` with only the existing minimal prepared-LIR return-add object
slice. Updated `src/backend/mir/riscv/assembler/mod.cpp` so its unsupported
object-emission error no longer advertises the removed bounded `jal-helper`
handoff.

## Suggested Next

Supervisor should decide whether Step 4 has enough ownership cleanup to hand to
review or whether another narrow audit cleanup remains before moving plan state.

## Watchouts

- Do not claim additional representative allowlist progress from this audit;
  current accepted state remains 20000205 and 20010119 passing, 20000113 and
  20030216 failing.
- `c4c-as.cpp::resolve_local_control_flow_labels` is assembler-text local-label
  lowering before shared object-module construction and was intentionally left
  untouched.
- The staged assembler surface no longer accepts the old shape-specific
  `main: jal helper; ret; helper: ret` object handoff through
  `assembler/elf_writer.cpp`; textual assembler object output should continue
  through the `c4c-as.cpp` shared `RiscvObjectFunction`/fragment builder path.
- Keep rejecting testcase-name shortcuts, expectation weakening, rendered-text
  probes, prepared/BIR semantic reconstruction in assembler cleanup, and silent
  block/edge dropping.

## Proof

Ran the delegated proof command:

`cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^backend_c4c_as_parse_suite$|^backend_rv64_roundtrip_contract$|^backend_cli_riscv64_return_zero_writes_elf_obj$|^backend_riscv_object_emission$' > test_after.log 2>&1`

Result: build completed with no work; CTest passed 4/4 tests. Proof log:
`test_after.log`.
