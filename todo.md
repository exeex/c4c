Status: Active
Source Idea Path: ideas/open/638_rv64_string_label_pointer_runtime_object_correctness.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh `src/20000722-1.c` object/link/runtime evidence

# Current Packet

## Just Finished

Step 1 refreshed `src/20000722-1.c` object/link/runtime evidence from a fresh
build. The one-row backend object scan still fails after c4c object emission
and clang link succeed:
`[RV64_BACKEND_RUNTIME_MISMATCH] clang_exit=0 c4c_exit=Segmentation fault`.
Direct qemu reruns agree: `clang_qemu.rc=0`, `c4c_qemu.rc=139`, with empty
stdout/stderr for both binaries.

Artifacts are under `build/agent_state/638_step1_runtime_object/`:
`case.log`, `test_after.log`, `summary.tsv`, `failed.txt`, `prepared_bir.txt`,
`prepared_authority_excerpt.txt`, `c4c_o_objdump_dr.txt`,
`c4c_bin_objdump_dr.txt`, `clang_bin_objdump_dr.txt`,
`c4c_o_readelf_sections_symbols_relocs.txt`,
`c4c_bin_readelf_sections_symbols_relocs.txt`,
`clang_bin_readelf_sections_symbols_relocs.txt`, `*_qemu.rc`,
`*_qemu.out`, `*_qemu.err`, and `step2_owner_evidence_extract.txt`.

The string-label pointer authority is present before object emission:
prepared BIR records `base=string_constant result=@.str0 symbol=.str0`
with `layout_authority=string_constant_label_pointer`, followed by
`address_materialization ... kind=string_constant result=@.str0`. The c4c
object then contains `.str0` PC-relative relocations, and the linked c4c binary
materializes `.str0` at `bar+0x48`.

The strongest Step 2 lead is not missing string-label pointer authority. The
prepared BIR also records `address_materialization ... kind=frame_slot
result=%lv._clit_` for the compound literal, but the c4c object/linked binary
stores the local struct on `sp` and calls `foo` with `a0=s2`. The clang binary
instead passes the compound-literal address (`addi a0,s0,-32`) to `foo`.

## Suggested Next

Run Step 2 classification against
`build/agent_state/638_step1_runtime_object/step2_owner_evidence_extract.txt`,
starting with call-argument materialization for the local compound literal /
frame-slot address path, then reject or confirm ABI, layout, local/global
memory, and branch/control-flow lanes from the saved object/runtime artifacts.

## Watchouts

- This is a research route; do not make implementation edits inside this idea.
- Do not reopen string-constant local-memory authority unless fresh evidence
  shows a missing string-label pointer authority fact before object emission.
- Do not claim progress from expectation, unsupported-marker, allowlist,
  timeout/accounting, runtime-comparison, or pass/fail accounting changes.
- The object/link route is live; the current first observed runtime symptom is
  a c4c segfault from passing `s2` as `foo`'s pointer argument, while the saved
  evidence shows `.str0` address materialization itself reaches the linked
  binary.

## Proof

Supervisor-selected proof command:
`cmake --build --preset default && ALLOWLIST=build/agent_state/638_step1_runtime_object.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log 2>&1`

Result: build succeeded; one-row backend object scan failed as expected with
`src/20000722-1.c` runtime mismatch. Canonical proof log: `test_after.log`.
Focused artifact copy: `build/agent_state/638_step1_runtime_object/test_after.log`.
