Status: Active
Source Idea Path: ideas/open/361_aarch64_materialized_pointer_storelocal_writeback.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize The Pointer-Addressed Store Boundary

# Current Packet

## Just Finished

Plan Step 1 localized the first materialized pointer-addressed
`StoreLocal`/writeback boundary for idea 361 without implementation edits.
The focused proof still runs 6/7: `00170`, `00189`, and the named backend
contracts pass, while `00181` keeps the corrected starting state and then
prints every subsequent state unchanged.

First store owner:

- Source operation: `tests/c/external/c-testsuite/src/00181.c:76`,
  `dest[j-1] = source[i]`, inside `Move`, immediately followed by
  `source[i] = 0` at line 77 and `PrintAll()` at line 78.
- Semantic/prepared BIR carrier: in `Move` block `block_19`, `%t47` is the
  destination pointer carrier formed as `%p.dest + ((%lv.j - 1) * 4)`;
  `%t43` is loaded from `%p.source + (%lv.i * 4)`. The prepared BIR still
  contains `bir.store_local %t47.addr, i32 %t43, addr %t47`.
- Expected first destination memory: on the first recursive move,
  `Move(A, C)` should write `1` to `C[3]`, then the adjacent same-shape
  store `bir.store_local %t50.addr, i32 0, addr %t50` should clear `A[0]`.
- First wrong boundary: final AArch64 computes/spills the `%t47` address
  (`add x9, x1, ...`; `str x9, [sp, #168]`) and computes/spills the `%t50`
  address (`add x9, x0, ...`; `str x9, [sp, #184]`), but emits no store
  through either computed pointer before `bl PrintAll`. There is no
  equivalent of `str <source-value>, [<%t47 address>]` for `dest[j-1]` and no
  equivalent of `str wzr, [<%t50 address>]` for `source[i] = 0`.
- Later consumer path: `PrintAll` reloads global arrays with
  `load_global @A/@B/@C` in semantic BIR and generated `adrp/add/ldr`
  sequences for `A`, `B`, and `C`, so the missing `Move` writeback leaves
  those global loads observing the unchanged starting memory. The return path
  also recomputes `%p.dest + ((%lv.j - 1) * 4)` and emits `ldr w0, [x13]`.
- Adjacent same-shape evidence: the second pointer-addressed store
  `source[i] = 0` has the same disappearance pattern. The materialized
  pointer loads in the preceding `while` loops do emit loads through computed
  addresses, but their final assembly shows a separate suspicious offset
  materialization (`mov x9, #4; mul x9, x9, x9`), so the next packet should
  keep load-side address scaling visible while repairing store writeback.

## Suggested Next

Execute plan Step 2 by repairing the general AArch64 lowering/writeback path
for `StoreLocal` with an explicit `addr` operand so materialized pointer
stores emit a real store to the addressed memory. Add focused backend coverage
for this BIR shape before relying on `00181`.

## Watchouts

- Keep the idea 360 starting state correct: `A: 1 2 3 4`, `B/C` zero.
- Do not reintroduce the reviewed unguarded `StoreLocal` fallback without a
  localized owner and focused backend coverage.
- Keep `00170`, `00189`, and the named backend contracts stable.
- The first `Move` store should mutate `C[3]` and clear `A[0]`, but adjacent
  load-side address scaling may be another same-neighborhood bug; avoid
  claiming full `00181` progress from store writeback alone unless execution
  proves the first move advances.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -j10 --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00170_c|c_testsuite_aarch64_backend_src_00189_c)$' | tee test_after.log`

Result: build up to date, 6/7 tests passed. The only failing test is
`c_testsuite_aarch64_backend_src_00181_c`; `test_after.log` contains the
full expected/actual mismatch and preserves the delegated proof output.
