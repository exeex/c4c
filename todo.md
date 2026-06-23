Status: Active
Source Idea Path: ideas/open/312_rv64_local_stack_slot_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Normalize Local Address Evidence

# Current Packet

## Just Finished

Step 1 evidence packet completed for the primary local stack/address
representatives: `src/00005.c`, `src/00032.c`, `src/00072.c`,
`src/00130.c`, and residual overlap probe `src/00143.c`.

Fresh probe artifacts live under
`build/rv64_c_testsuite_probe_latest/triage_312_step1/`, including per-case
`.s`, `.bir.txt`, `.prepared-bir.txt`, emit/clang/qemu stdout/stderr files,
`probe_results.tsv`, and `summary.md`.

Probe result summary:

| Case | Emit | Clang | QEMU | Classification |
| --- | ---: | ---: | ---: | --- |
| `src/00005.c` | `0` | `0` | `132` | `QEMU_NONZERO` |
| `src/00032.c` | `0` | `0` | `139` | `QEMU_NONZERO` |
| `src/00072.c` | `0` | `0` | `139` | `QEMU_NONZERO` |
| `src/00130.c` | `0` | `0` | `139` | `QEMU_NONZERO` |
| `src/00143.c` | `0` | `0` | `132` | `QEMU_NONZERO` |

Classification:

| Case | First bad point | Classification |
| --- | --- | --- |
| `src/00005.c` | Assembly emits `&x`/`&p` stack address stores and loads `p`/`x`, then stops at the first compare result whose prepared home is stack slot `%t2` before branch emission. | Address-taking is present; defer as compare-result stack-slot/fused-branch materialization, not the first local-address repair. |
| `src/00032.c` | After storing `arr[0]`/`arr[1]`, BIR stores pointer local `p` from `%t5`, but prepared addressing has no address materialization for `%t5`; assembly stops before publishing `&arr[0]` into `p`. | True local array base address publication failure. |
| `src/00072.c` | First store of pointer local `p` uses an unmaterialized pointer value (`sd t0, 8(sp)`); later direct `addr %lv.arr.0+4` store/load exists, but pointer value flow is stale. | True local array base/pointer-value materialization failure. |
| `src/00130.c` | `p = arr` materializes a base address, but `q = &arr[1][3]` stores unmaterialized `%t4` (`sd s1, 16(sp)`); later work also has an `i8` sext tail. | True local subobject address publication failure, with later scalar-width tail. |
| `src/00143.c` | Prepared BIR expands indexed local writes into large `i16` select/update chains beginning at `%t9 = add ptr %lv.a.0, %t9.byte_offset`; current asm stops before the indexed local-array body. | In-scope overlap for local array indexed effective-address/select-update work, but too broad for the first repair packet. |

## Suggested Next

Start the first implementation packet with local frame-slot address
publication for address-taken local array/subobject values stored into pointer
locals. Focused backend tests should cover materializing `&arr[0]`, array
decay/base address, and a constant subobject address such as `&arr[1][3]`
without relying on candidate filenames or exact stack offsets.

## Watchouts

- Do not special-case candidate filenames, variable names, or fixed stack
  offsets.
- `src/00005.c` should not drive the first local-address patch: its simple
  address-taking path is already emitted, and the current first stop is compare
  result materialization when the condition home is a stack slot.
- `src/00143.c` overlaps this plan through local array indexed
  effective-address/select-update chains, but it should follow the smaller
  local array/subobject address-publication repair rather than drive a broad
  first patch.
- `src/00130.c` has a later `i8` sext/check tail after the subobject address
  issue; keep scalar-width cleanup separate unless the local address repair
  reaches that point.

## Proof

Evidence-only packet. No build/CTest was required and `test_after.log` was not
produced or modified. Probe command shape was emit -> clang -> qemu with a 5s
timeout per phase for the five delegated c-testsuite targets.
