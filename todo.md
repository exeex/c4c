Status: Active
Source Idea Path: ideas/open/612_rv64_instruction_fragment_consumers.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement First Pointer Address-Authority Consumer

# Current Packet

## Just Finished

Completed Step 2, "Implement First Pointer Address-Authority Consumer", by
adding an RV64 prepared pointer `BinaryInst` `add` consumer for the shared
shape with explicit prepared homes for pointer base, named byte-offset value,
and result. The consumer requires explicit target-register identity for any
register home and refuses immediate-only pointer adds, so loaded-base plus
immediate/scaled producer rows stay under `unsupported_pointer_arithmetic`.
The stack-home path now selects only unoccupied temporary GPRs for result,
base, and offset materialization and fails closed when the required scratches
cannot be proven safe. If this first consumer declines a pointer-add candidate,
the dispatcher now continues to later precise owner diagnostics instead of
turning downstream local-memory or pointer-arithmetic owners into generic
instruction-fragment stops.

Focused probes before coding confirmed the sampled positive family still
started at pointer `BinaryInst` instruction-fragment stops:
- `src/20000801-1.c`: `unsupported_instruction_fragment`, `foo`, `entry`,
  `instruction_index=3`, `BinaryInst`, `owner=ptr %t2`.
- `src/930526-1.c`: `unsupported_instruction_fragment`, `f`, `block_1`,
  `instruction_index=4`, `BinaryInst`, `owner=ptr %t10`.
- `src/loop-2f.c`: `unsupported_instruction_fragment`, `f`,
  `logic.end.7`, `instruction_index=1`, `BinaryInst`, `owner=ptr %t10`.
- `src/pr41395-2.c`: `unsupported_instruction_fragment`, `foo`, `entry`,
  `instruction_index=2`, `BinaryInst`, `owner=ptr %t3`.
- `src/strct-pack-3.c`: `unsupported_instruction_fragment`, `f`, `entry`,
  `instruction_index=8`, `BinaryInst`, `owner=ptr %t9`.

Focused prepared-BIR probes identified the implemented subfamily as pointer
`add` with a prepared pointer base home, named prepared integer byte-offset
home, and prepared result home. Examples included `src/20000801-1.c`
(`%t2 = bir.add ptr %t0, %t1`, result stack home and store-source publication),
`src/loop-2f.c` (`%t10 = bir.add ptr %p.p, %t9`, result stack home), and
`src/pr41395-2.c` (`%t3 = bir.add ptr %p.p, %t3.byte_offset.static`, result
stack home and prepared pointer-value local access). `src/930526-1.c` and
`src/strct-pack-3.c` remain residuals because their current prepared facts do
not satisfy this first consumer's explicit target-register/home boundary.

Post-fix focused direct-probe movement:
- `src/20000801-1.c` moved past the sampled pointer `BinaryInst` stop to
  `unsupported_branch_stack_load_authority` for branch lhs `%t3`.
- `src/loop-2f.c` moved past the sampled `f`/`logic.end.7` pointer
  `BinaryInst` stop; the current first object stop is now `main` entry
  `CallInst` `unsupported_instruction_fragment`.
- `src/pr41395-2.c` moved past the sampled pointer `BinaryInst` stop to
  `ambiguous_non_parallel_multi_source_stack_destination` move-bundle
  classification at `foo` entry instruction `9`.
- `src/930526-1.c` remains `unsupported_instruction_fragment` at the sampled
  pointer `BinaryInst` `%t10`.
- `src/strct-pack-3.c` remains `unsupported_instruction_fragment` at the
  sampled pointer `BinaryInst` `%t9`.

Post-fix negative guard checks stayed under their existing owners:
- Producer/authority rows `src/20021120-1.c` and `src/990524-1.c` still stop at
  `unsupported_pointer_arithmetic`.
- Idea-614/local-memory rows `src/20000722-1.c` and `src/ptr-arith-1.c` still
  stop at `unsupported_local_memory_access`.
- Branch/select/cast/ABI/global/move-bundle guards stayed under their owners:
  `src/930930-1.c` under branch stack-load freshness,
  `src/20000815-1.c` under `SelectInst` instruction-fragment,
  `src/20010604-1.c` under `CastInst` instruction-fragment,
  `src/20000603-1.c` under `unsupported_call_abi`,
  `src/20020213-1.c` under `unsupported_global_data`, and
  `src/20000422-1.c` under `unsupported_move_bundle_target_shape`.
- Policy/scalar guards stayed under their owners: `src/pr40022.c` under
  `unsupported_inline_asm_fragment`, `src/20000801-2.c` under
  `unsupported_terminator_fragment`, and `src/931110-1.c` still as scalar
  `BinaryInst owner=i16` `unsupported_instruction_fragment`.

## Suggested Next

Step 3 should classify the remaining pointer `BinaryInst` residuals after this
first explicit-home pointer-add consumer. Start with `src/930526-1.c` and
`src/strct-pack-3.c`: determine whether they need a small adjacent prepared
home/target-identity extension, an existing stricter address-materialization
handoff, or should remain blocked until prepared facts are strengthened.

## Watchouts

- Do not relax the new consumer to accept immediate-only pointer adds; that
  reclassifies `src/20021120-1.c` and `src/990524-1.c` away from
  `unsupported_pointer_arithmetic`.
- Do not accept legacy `register_name` alone as register authority in this
  generic path; an existing frame-slot address-materialization fail-closed test
  showed that would bypass missing prepared materialization facts.
- Stack result and stack offset homes are covered by object-emission tests; an
  occupied scratch-register shape is rejected instead of reusing a live
  prepared home.
- Do not reintroduce an early generic pointer-add rejection before existing
  precise diagnostics; that regresses `src/ptr-arith-1.c` from local-memory
  ownership back to `unsupported_instruction_fragment`.
- `src/20000801-1.c`, `src/loop-2f.c`, and `src/pr41395-2.c` moved only to
  downstream owners, not to full object success.
- No expectations, unsupported markers, allowlists, runtime/accounting files,
  `plan.md`, or idea files were changed.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

Result: passed. Proof log: `test_after.log`.
