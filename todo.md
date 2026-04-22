# Execution State

Status: Active
Source Idea Path: ideas/open/76_prepared_byval_home_publication_and_layout_correctness_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Isolate The Authoritative Home Publisher
Plan Review Counter: 0 / 5
# Current Packet

## Just Finished

Completed plan step `2` for idea 76 by tracing the owned publication/layout
producers with AST-backed queries plus a focused prepared-BIR dump of
`arg()`. The authoritative stack-layout publisher is still
`stack_layout::assign_frame_slots(...)` in
`src/backend/prealloc/stack_layout/slot_assignment.cpp:147-210`, reached from
`BirPreAlloc::run_stack_layout()` in
`src/backend/prealloc/stack_layout.cpp:623-666`. For `arg()`, that publisher
assigns the local byte home `%t37.0` to stack slot `#990` at offset `364`,
while the local float homes `%t38.0/.4/.8/.12` live on stack slots
`#718-#721` at offsets `80/84/88/92`; those owned stack-layout facts are not
overlapping. The authoritative regalloc home publisher is
`allocate_stack_slot(...)` plus `classify_prepared_value_home(...)` in
`src/backend/prealloc/regalloc.cpp:411-429` and `:561-564`, seeded from the
module-max stack base at `src/backend/prealloc/regalloc.cpp:1773-1779`. That
publisher gives the `fa4(...)` byval copies distinct late stack homes:
`%t37` at slot `#2440` offset `6264`, `%t38` at slot `#2441` offset `6272`,
and `%t38.global.aggregate.load.{0,4,8,12}` at slots `#2461-#2464` offsets
`6376/6380/6384/6388`. The prepared move bundle for the `fa4(...)` call uses
only register ABI destinations for args `0..5`, so the owned prealloc/layout
surfaces are publishing separate facts; the `[rsp + 364]` overwrite seen in
`build/c_testsuite_x86_backend/src/00204.c.s` is being introduced after those
publications, not by the current prealloc or stack-layout producers.

## Suggested Next

Take the next packet downstream of the owned prealloc producers:

- inspect the consumer that lowers `PreparedAddressingFunction` frame-slot
  accesses and regalloc `PreparedValueHome` stack offsets into final x86
  addresses for `arg()`'s `fa4(...)` setup
- prove where the truthful owned offsets above collapse into the emitted
  `[rsp + 352..364]` overlap
- keep the proving set narrow on `c_testsuite_x86_backend_src_00204_c` plus
  targeted prepared-BIR / assembly inspection around `arg()` instruction index
  `623`

## Watchouts

- The owned prealloc/layout files do still own the authoritative publication
  facts, but the current overlap is not caused by duplicate offsets inside
  those facts; a repair in these files would be speculative unless it explains
  how the downstream consumer collapses `364`, `6264`, `6272`, and
  `6376..6388`.
- `stack_layout::assign_frame_slots(...)` only publishes the small local homes
  from `PreparedStackObject`s. The large `#2440+` and `#2461+` offsets come
  from regalloc-only stack homes allocated after `prepared_.stack_layout`.
- Idea 75 and idea 77 still do not own the first bad fact; the corruption is
  still visible before `fa4` executes, but the next seam to repair is now
  downstream of the current owned files rather than another generic search
  inside `src/backend/prealloc/**`.

## Proof

Fresh step-2 proof on 2026-04-22 used the delegated command:

- `cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^c_testsuite_x86_backend_src_00204_c$' | tee test_after.log`
- result: still fails, with the first mismatch still inside `Arguments:` in
  `arg()` before `Return values:`
- proof log: `test_after.log`
- supporting inspection:
  `build/c_testsuite_x86_backend/src/00204.c.s` around the `arg()`
  `fa4(...)` setup and a focused prepared-BIR dump showing:
  `prepared.func @arg frame_size=376`, stack-layout slots `#718-#721` and
  `#990`, regalloc homes `#2440-#2441` and `#2461-#2464`, and the
  `move_bundle phase=before_call block_index=0 instruction_index=623`
  register-only ABI destinations for `fa4(...)`
