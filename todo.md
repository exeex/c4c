Status: Active
Source Idea Path: ideas/open/347_aarch64_local_conversion_store_load_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair the General Local Conversion Publication Rule

# Current Packet

## Just Finished

Step 2 repaired the local conversion result publication owner in
`src/backend/mir/aarch64/codegen/dispatch.cpp`. The general
`lower_store_local_value_publication` rule now recognizes local stores whose
stored value is produced by a same-block scalar cast and emits publication
into the selected store source register before the store is printed. The
repair covers integer-to-float, float-to-integer, and float-width conversions
across GPR/FPR homes instead of keying on `00175`, source locations, concrete
registers, offsets, or one instruction sequence.

The former stale `char c = 97.0` store now materializes the floating immediate
and converts it before the local home write:
`fmov d16, x9`, `fcvtzs w13, d16`, `strb w13, [sp, #17]`.
The adjacent local conversion facts are also repaired by the same path:
`int e = 97.0` emits `fcvtzs w13, d16` before `str w13, [sp, #4]`, and
`float f = 'a'; float g = 97;` emit `scvtf s13, w9` before the `str s13`
stores.

## Suggested Next

Execute Step 3: add focused backend coverage for local scalar/FP conversion
store publication if an existing AArch64 backend test surface can pin the
semantic rule without asserting incidental register numbers or stack offsets.

## Watchouts

- The fix is intentionally in the AArch64 dispatch-side publication rule
  because the selected store already names the authoritative destination
  register; the missing piece was materializing the conversion into that
  register before the store.
- Step 3 should avoid pinning the current scratch choices (`w13`, `s13`,
  `d16`) or stack offsets; those remain allocation details.
- Direct-call guard stability still belongs to a later supervisor packet.

## Proof

Ran the delegated proof command exactly:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00175_c$' | tee test_after.log`.
The build was up to date after the earlier local compile, and
`c_testsuite_aarch64_backend_src_00175_c` passed. Proof log:
`test_after.log`.
