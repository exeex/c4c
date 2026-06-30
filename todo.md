Status: Active
Source Idea Path: ideas/open/427_rv64_scalar_select_join_materialization.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Broader Validation And Close Readiness

# Current Packet

## Just Finished

Step 5 ran the supervisor-delegated backend close-readiness proof for the RV64
scalar select/join materialization route and prepared the close-review summary.

Changed behavior from Steps 2 and 3:

- ordinary I8 and I16 scalar `bir.select` results now lower through RV64 object
  emission instead of being rejected by the old I32/I64-only select fragment
  gate.
- stack-slot select destinations are validated with the actual selected result
  size instead of the old 4-byte default.
- coherent stack-homed published join-transfer carrier rows now skip
  predecessor publication moves and reuse the join-block scalar select
  materialization path.
- register-homed published carriers keep the existing predecessor-copy route.
- pointer, F128, non-scalar, missing-home, ambiguous, mixed, cycle, non-select,
  and incomplete prepared-publication shapes still reject fail-closed.

Step 4 representative-row findings remain unchanged: focused backend tests now
prove multiple same-feature select/join shapes through semantic prepared facts,
while the full gcc-torture representatives still expose out-of-scope blockers:

- `src/pr43236.c`: I8 select-store rows are now in scope for the repaired
  select path, but the full row reaches `memcmp` frame-slot address/value
  argument publication; owner: `ideas/open/428_rv64_call_adjacent_scalar_inline_asm_materialization.md`
  and `ideas/open/429_rv64_pointer_address_materialization.md`.
- `src/pr51933.c`: the select chain is rooted in global loads and starts with
  inline asm over globals; owner: call-adjacent inline asm plus later global
  memory work, not this select/join idea.
- `src/pr68328.c`: has a coherent I8 join-transfer select in `baz`, but the
  full row also contains inline asm, same-module calls, global loads,
  direct-global select chains, and unsupported stack-slot block-entry
  publications; owner: call-adjacent inline asm plus stack/publication or
  global-memory follow-up.
- `src/pr82954.c`: contains nested I32 join-transfer select carriers with
  available register block-entry publications; the full row is blocked by
  inline asm and same-module `foo` frame-slot address arguments. A reduced
  nested register-homed join-transfer carrier is the only scoped residual risk
  if the supervisor wants one more in-family implementation proof before close.
- `src/pr84524.c`: contains a large I16 select chain, but the full row hits
  inline asm returning I16 with missing frame-slot-value call argument
  publication; owner: `ideas/open/428_rv64_call_adjacent_scalar_inline_asm_materialization.md`.

No expectations, allowlists, unsupported markers, runtime comparison files, or
pass/fail accounting files were changed or used as proof of progress. Based on
the focused semantic coverage, preserved fail-closed behavior, representative
row classification, and fresh backend proof, this source idea appears ready for
supervisor lifecycle close review. The only scoped close-review risk is whether
to require a reduced nested register-homed join-transfer carrier test based on
the `src/pr82954.c` shape before closing.

## Suggested Next

Ask the supervisor to run lifecycle close review for
`ideas/open/427_rv64_scalar_select_join_materialization.md`. If close review
requires one more implementation packet, keep it limited to a reduced nested
register-homed join-transfer carrier based on the `src/pr82954.c` prepared
shape, not the full gcc-torture row.

## Watchouts

- The representative full gcc-torture rows are not expected to go green from
  this source idea alone; their current blockers cross into call-adjacent
  scalar/inline asm, frame-slot address/value argument publication, global
  memory, or stack/publication work.
- Do not fold `pr51933.c` global select chains, `pr84524.c` inline asm, or
  `pr43236.c`/`pr82954.c` frame-slot address calls into this source idea.
- Keep F128, call-adjacent publication, aggregate ABI, pointer provenance, and global memory repair out of this route.
- Reject filename-shaped fixes for representative gcc_torture rows.

## Proof

Ran the delegated proof command:
`{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1`.

Result: passed. `test_after.log` is the canonical proof log for this Step 5
close-readiness packet. The backend subset reported `100% tests passed, 0 tests
failed out of 327`.
