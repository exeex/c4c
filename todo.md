Status: Active
Source Idea Path: ideas/open/612_rv64_instruction_fragment_consumers.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Split Or Close-Readiness Classification

# Current Packet

## Just Finished

Completed Step 4, "Residual Split Or Close-Readiness Classification", by
classifying the remaining pointer `BinaryInst` and residual
`unsupported_instruction_fragment` owners after pointer `BinaryInst` authority
exhaustion. No code, expectations, unsupported markers, source idea files, or
plan files were changed.

Close-readiness result: active idea 612 is close-ready for the pointer
`BinaryInst` / instruction-fragment consumer route. It should not continue with
another implementation packet under the current runbook, and it does not need a
runbook rewrite for pointer `BinaryInst` authority. The only lifecycle split
worth considering is a new durable cast-specific idea if the supervisor wants
to preserve cast producer/consumer rows as near-term work; otherwise the
remaining visible rows are already owned by existing routes or separate
non-612 owners.

Residual owner split from current Step 3 refresh artifacts and focused
close-readiness inspection:
- Move-bundle owner: `src/strct-pack-3.c` now stops at
  `prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination`
  with `diagnostic_owner=rv64_prepared_move_bundle_consumer`; the earlier
  pointer `BinaryInst` `%t9` row has moved to prepared move-bundle authority.
  `src/pr41395-2.c` has the same move-bundle classification in the earlier
  Step 3 probe. `src/20000422-1.c` remains
  `unsupported_move_bundle_target_shape`.
- Address-materialization/prepared facts for frame-slot/local-slot pointer
  bases: `src/930526-1.c` remains a pointer `BinaryInst` first stop at
  `owner=ptr %t10`, but the base `%lv.m.0` is a frame-slot/local-slot address
  base. This is prepared/address-materialization fact ownership, not generic
  pointer `BinaryInst` consumer authority.
- Pointer/local-memory idea 614 or adjacent pointer-authority policy:
  `src/20000722-1.c` and `src/ptr-arith-1.c` remain
  `unsupported_local_memory_access`; `src/20021120-1.c` and
  `src/990524-1.c` remain `unsupported_pointer_arithmetic` because loaded or
  otherwise incomplete pointer bases still lack the accepted immediate/static
  byte-offset authority required by this runbook.
- Branch freshness/authority: `src/930930-1.c` remains branch stack-load
  freshness, and the earlier `src/20000801-1.c` probe remains
  `unsupported_branch_stack_load_authority`.
- Casts: `src/20010604-1.c` remains `CastInst`
  `unsupported_instruction_fragment`. This is the only residual class that may
  deserve a new source idea if cast rows are selected for durable follow-up.
- Calls/ABI: `src/20000603-1.c` remains `unsupported_call_abi`; the earlier
  `src/loop-2f.c` pointer probe first stops at a `CallInst` owner, so it is
  call/ABI rather than pointer `BinaryInst` continuation work.
- Select publication: `src/20000815-1.c` remains `SelectInst`
  `unsupported_instruction_fragment` and belongs with select publication or
  selected-authority routing, not this pointer runbook.
- Inline asm: `src/pr40022.c` remains `unsupported_inline_asm_fragment`.
- Terminator: `src/20000801-2.c` remains
  `unsupported_terminator_fragment`.
- Global/runtime: `src/20020213-1.c` remains `unsupported_global_data`.
  Runtime-owned residuals, if selected from broader diagnostics, belong with
  the existing runtime/global ownership routes listed in `plan.md`, not with
  pointer `BinaryInst` consumers.
- Scalar narrow `ashr`: `src/931110-1.c` remains singleton scalar
  `BinaryInst owner=i16` `unsupported_instruction_fragment`; there is still no
  refreshed same-family breadth to justify reopening the narrow integer route.
- Remaining pointer `BinaryInst` consumer rows: none are currently visible as
  in-scope generic pointer `BinaryInst` consumer blockers with complete
  upstream pointer/address authority. The prior observable pointer rows have
  either moved to move-bundle authority, remain excluded frame-slot/local-slot
  prepared-address materialization, or are guarded by pointer-arithmetic/local
  memory policy.

Recommendation to plan owner: close or retire this active idea-612 runbook as
exhausted and close idea 612 if no broader instruction-fragment consumer scope
is intentionally being kept open. If cast rows should become durable work,
split them into a new cast-specific open idea before closing or replacing this
runbook. Do not continue by implementing move-bundle, idea 614 local-memory,
ABI/call, select, inline asm, terminator, global/runtime, branch, or singleton
scalar `ashr` work under idea 612.

## Suggested Next

Supervisor should call the plan owner for lifecycle handling: close/retire the
active idea-612 runbook, with an optional split into a new cast-specific idea
only if casts are selected as durable follow-up.

## Watchouts

- Do not start move-bundle implementation from this runbook. The visible
  `strct-pack-3.c` and `pr41395-2.c` blockers are prepared move-bundle
  authority, not pointer `BinaryInst` consumer authority.
- Do not treat `%lv.m.0` in `src/930526-1.c` as an ordinary pointer home. It
  remains a frame-slot/local-slot prepared-address fact gap.
- Do not relax loaded-base, selected-authority, or pointer-arithmetic policy to
  manufacture another pointer `BinaryInst` packet.
- Do not use the singleton `src/931110-1.c` scalar `ashr` row as standalone
  implementation breadth.
- This packet is lifecycle classification only. Any actual close, split,
  source idea edit, or plan rewrite belongs to the plan owner.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

Result: passed, 346/346 backend tests. Proof log: `test_after.log`.
