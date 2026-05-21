Status: Active
Source Idea Path: ideas/open/372_aarch64_pointer_valued_subobject_address_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Pointer-Local Address Publication Gap

# Current Packet

## Just Finished

Executed Step 1 localization for active idea 372. The focused representative
proof still fails on `00163` with the final line printing `bolshevic.b = 42`
instead of `bolshevic.b = 34`.

First bad fact:

- Source/HIR has the intended assignment at `00163.c:34`: `b =
  &(bolshevic.b)`.
- Semantic BIR reaches the backend with `bir.store_local %lv.b, ptr %t34`,
  followed by `%t36 = bir.load_local ptr %lv.b` and dereference through
  `%t36`.
- Prepared BIR classifies `%t34` as a pointer-valued computed address:
  `storage %t34 ... encoding=computed_address` and `home %t34 ... kind=
  pointer_base_plus_offset`, with the store-local memory access at inst 24
  writing `%t34` into `%lv.b`'s frame slot.
- Generated AArch64 never materializes `bolshevic+4` for that store and never
  writes the new pointer value into `%lv.b`'s slot. After the `tsar->c`
  `printf`, emission falls straight to `ldr x13, [sp, #8]`, reloading the old
  `b = &a` pointer home; `ldr w9, [x13]` therefore reloads local `a == 42`.

Owning boundary: AArch64 publication/lowering of `StoreLocal` when the stored
source is a computed address value for a global object/subobject. The semantic
and prepared records have enough identity to say `%lv.b` should receive a new
pointer value, but the AArch64 store-local path does not materialize the
computed address source before publishing it to the scalar pointer local home.

## Suggested Next

Execute Step 2 with focused backend coverage for pointer-local address
publication:

- A scalar pointer local is first assigned one address, then reassigned from a
  distinct global or global-subobject address.
- The later dereference or pointer-valued consumer must observe the reassigned
  address, proving the store-local publication materialized and stored the
  computed address rather than retaining the old pointer home.
- Coverage should assert semantic route facts around computed-address
  `StoreLocal` publication or generated AArch64 shape generally; it must not
  depend on `00163`, `bolshevic`, `b`, one stack offset, one register, or one
  exact instruction sequence.

## Watchouts

Do not reopen or edit closed ideas 294 or 355. Do not special-case `00163`,
`bolshevic`, `b`, `a`, one stack offset, one register, or one emitted
instruction sequence. Keep `00187`, `00182`, `00205`, `00216`, `00174`,
`00218`, `00200`, and `00207` parked under idea 295 unless fresh localization
proves a handoff.

Closed-scope comparison:

- Closed idea 294 repaired pointer-derived lvalue/address authority for loads,
  stores, call arguments, and compound updates. Current evidence is narrower:
  the address-of global subobject is the value being assigned to a scalar
  pointer local, not the direct lvalue address consumed by the final load.
- Closed idea 355 repaired address-valued memory/call argument publication.
  Current evidence is not a call argument or address/pointee ambiguity at an
  external boundary; it is local pointer-home publication for a computed
  global/subobject address. Keep both closed unless Step 2/3 localization finds
  the exact archived helper is the shared implementation boundary.

## Proof

Delegated Step 1 command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00163_c$' > test_after.log 2>&1
```

Result: build completed; CTest selected 1 test and `00163` failed with the
known runtime mismatch. Proof log is `test_after.log`.

Inspection commands read `tests/c/external/c-testsuite/src/00163.c`,
generated `build/c_testsuite_aarch64_backend/src/00163.c.s`, `build/c4cll
--dump-bir`, `build/c4cll --dump-prepared-bir`, HIR/canonical dumps for
`00163`, `plan.md`, source idea 372, and closed idea notes 294/355. No
implementation files were edited.
