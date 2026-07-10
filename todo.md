Status: Active
Source Idea Path: ideas/open/663_prepared_object_data_static_storage_runtime.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Refresh Static-Storage Evidence

# Current Packet

## Just Finished

Step 1: refreshed the focused static local storage evidence for both target
rows.

The focused proof rebuilt the tree and ran the six delegated rows. The two CLI
object rows and two codegen-route assembly rows pass. Only the two
object-runtime rows fail:

- row 183,
  `backend_obj_runtime_rv64_prepared_object_data_static_local_storage`:
  QEMU exits `224`, expected `11`.
- row 184,
  `backend_obj_runtime_rv64_prepared_object_data_static_local_initialized_storage`:
  QEMU exits `224`, expected `24`.

Object-data evidence for both rows is coherent before runtime:

- row 183 publishes `.bss`, `.balign 4`,
  `__static_local_rv64_step3_static_counter_0`, and `.zero 4`; the object has
  a 4-byte `.bss` section, an object symbol for the static local, and
  `R_RISCV_PCREL_HI20`/`R_RISCV_PCREL_LO12_I` relocations from both static
  load and store sites to that symbol.
- row 184 publishes `.data`, `.balign 4`,
  `__static_local_rv64_step4_static_initialized_counter_0`, and `.word 13`;
  the object has a 4-byte `.data` section containing `0d000000`, an object
  symbol for the static local, and the same PC-relative relocation shape from
  both static load and store sites.
- linked binary disassembly resolves the static-local accesses to concrete
  data addresses, so the first refreshed evidence does not point at prepared
  object-data publication, static layout, initialization payload placement, or
  relocation/symbol binding.

The first mismatch is the RV64 object-route text/runtime consumer: in both
linked binaries `main` copies the call result from `a0` into `t0`, then
immediately overwrites `t0` from `s2` before storing the saved `first` and
`second` values. That stale `s2` value drives both runtime exits to `224`.
The generated assembly-route output does not contain those `mv t0,s2`
overwrites and its snippet checks pass.

Owner finding: the two target rows share one first observed owner, but it is
not a static-storage object-data owner. The best split/routing target is the
RV64 object-route call/live-value consumer, likely the same neighborhood as
callee-saved GPR preservation across calls, rather than this static-storage
publication/layout/initializer/relocation route.

## Suggested Next

Supervisor or plan-owner should route before Step 2 implementation. Suggested
next packet is a lifecycle decision: either move both rows out of this
static-storage plan into the existing RV64 callee-saved/live-value object-route
owner, or rewrite Step 2 to explicitly select that non-static-storage boundary
and stop this runbook from making object-data changes.

## Watchouts

- Keep packed local member offsets, byval payloads, pointer-local lowering,
  callee-saved GPR preservation, CLI dump formatting, AArch64 dispatch,
  generic RISC-V object emission, and LLVM torture work outside this plan
  unless focused evidence proves the same first owner.
- Do not change expectations, unsupported markers, allowlists, timeout policy,
  runtime policy, baseline accounting, or unrelated backend families.
- Do not claim final assembly or final object bytes as authority when prepared
  object-data facts are missing or ambiguous.
- The static-storage facts are present and coherent in object files and linked
  binaries; a patch to object-data publication, layout, initializer bytes, or
  static-symbol relocations would be testcase drift unless new evidence
  contradicts this packet.
- `riscv64-linux-gnu-gcc` is not available in this container, so I could not
  add a supplementary assembly-link runtime check for the passing `.s` output.

## Proof

Ran the delegated proof command exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_cli_riscv64_prepared_object_data_static_local_storage_obj|backend_cli_riscv64_prepared_object_data_static_local_initialized_storage_obj|backend_codegen_route_riscv64_prepared_object_data_static_local_storage|backend_codegen_route_riscv64_prepared_object_data_static_local_initialized_storage|backend_obj_runtime_rv64_prepared_object_data_static_local_storage|backend_obj_runtime_rv64_prepared_object_data_static_local_initialized_storage)$' > test_after.log 2>&1`

Result: build completed with no work; CTest reported 4/6 passing and the two
object-runtime rows failing with the QEMU return mismatches above.
`test_after.log` is the canonical proof log.
