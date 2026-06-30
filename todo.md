Status: Active
Source Idea Path: ideas/open/427_rv64_scalar_select_join_materialization.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Nearby Same-Feature Coverage

# Current Packet

## Just Finished

Step 4 exercised nearby same-feature coverage without changing implementation.
The backend object-emission tests already prove multiple select/join shapes
through the same semantic route:

- ordinary I8 and I16 scalar select materialization
- normalized signed less/equal select predicates
- direct prepared join-transfer select materialization
- register-homed published join-transfer carriers using predecessor copies
- stack-homed published join-transfer carriers materialized in the join block
- edge-compare and dependent edge-compare published select carriers
- ambiguous, non-select, stack-source, and incomplete publication shapes still
  reject fail-closed

Representative gcc-torture rows were probed through the direct RV64 object
case runner and prepared-BIR dumps under
`build/agent_state/select_join_step4*`. All five still fail the full object
route with `unsupported_instruction_fragment`, but the remaining blockers are
not clean scalar select/join-only implementation gaps for this packet:

- `src/pr43236.c`: retains many I8 select-store rows, but the prepared data also
  reaches `memcmp` calls with frame-slot address arguments and missing
  frame-slot argument publication for `%lv.C.0`; this is call/address
  materialization rather than a select-only packet.
- `src/pr51933.c`: the select chain is rooted in global loads from `@v2` and
  the case starts with inline asm over globals; this is global memory plus
  call-adjacent inline asm before it is a clean select materialization row.
- `src/pr68328.c`: has a coherent I8 join-transfer select in `baz`, but the
  same prepared function contains inline asm, same-module calls, global loads,
  direct-global select chains, and unsupported stack-slot block-entry
  publications; those are out of scope for this select/join packet.
- `src/pr82954.c`: contains nested I32 join-transfer select carriers with
  available register block-entry publications, but the full row also has
  inline asm and a same-module `foo` call whose frame-slot address arguments
  require missing publication/address materialization; a reduced nested
  join-transfer carrier would be the right in-scope follow-up if more Step 4
  implementation proof is desired.
- `src/pr84524.c`: contains a large I16 select chain, but the full row hits
  inline asm returning I16 with a frame-slot-value call argument missing
  publication; that is call-adjacent scalar/inline-asm materialization.

## Suggested Next

Execute Step 5: run the supervisor-selected broader RV64/gcc-torture proof and
prepare the close-readiness summary. If the supervisor wants one more
implementation packet before Step 5, the coherent target is a reduced nested
register-homed join-transfer carrier based on the `src/pr82954.c` prepared
shape, not the full gcc-torture row.

## Watchouts

- The five representative full gcc-torture rows are intentionally not green
  yet; their current blockers cross into call-adjacent scalar/inline asm,
  frame-slot address/value argument publication, global memory, or stack-slot
  publication work.
- Do not claim full-row progress by editing expectations, allowlists,
  unsupported markers, runtime comparison, or scan accounting.
- Do not fold `pr51933.c` global select chains, `pr84524.c` inline asm, or
  `pr43236.c`/`pr82954.c` frame-slot address calls into this source idea.
- Keep F128, call-adjacent publication, aggregate ABI, pointer provenance, and global memory repair out of this route.
- Reject filename-shaped fixes for representative gcc_torture rows.

## Proof

Ran the delegated proof command:
`{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1`.

Result: passed. `test_after.log` is the canonical proof log for this Step 4
coverage packet. Scratch evidence is under `build/agent_state/select_join_step4/`,
`build/agent_state/select_join_step4_allowlist.txt`, and
`build/agent_state/select_join_step4_classification.txt`.
