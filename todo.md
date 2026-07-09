Status: Active
Source Idea Path: ideas/open/633_aggregate_stack_home_local_memory_policy.md
Source Plan Path: plan.md
Current Step ID: 9
Current Step Title: Trace Remaining Stack-Home Local-Memory Bucket

# Current Packet

## Just Finished

Step 9 traced the remaining stack-home local-memory bucket after the Step 7
RV64 consumer repair and recorded focused notes at
`build/agent_state/633_step9_remaining_stack_home_trace.md`.

Primary target results:
- `src/pr30185.c`: byval loads such as function `foo`, block `entry`,
  instruction `36`, `%lv.param.x.byval.copy.0 = load_local i8 addr %p.x`
  have pointer-value base, offset `0`, size `1`, align `8`,
  `base_plus_offset=yes`, proven range, byval value home `%p.x`, frame slot
  `#1`, and byval stack object size/alignment `16/8`. The first remaining
  failure is not that byval lane. The matching sret stores such as
  instruction `77`, `store_local ... addr %ret.sret`, have the same
  pointer-value/proven-range shape, but `prepared_stack_home_local_memory_has_authority(...,
  SretParam)` still rejects because the current predicate compares the
  requested pointee extent for `sret(size=16, align=8)` against the 8-byte
  `%ret.sret` pointer-home stack object.
- `src/20020215-1.c`: byval loads such as function `foo`, block `entry`,
  instruction `30`, `%lv.param.s.byval.copy.0 = load_local i16 addr %p.s`
  have offset `0`, size `2`, align `8`, byval stack object size/alignment
  `24/8`, and are expected to pass the Step 7 stronger-than-lane alignment
  route. Sret stores such as instruction `64`, `store_local ... addr
  %ret.sret`, reject at the same sret authority boundary because the ABI
  pointee extent is `sret(size=24, align=8)` while `%ret.sret` is an 8-byte
  pointer home.
- `src/950628-1.c`: representative stores in `g` instruction `5` and `f`
  instruction `12` write 1-byte lanes through `%ret.sret` with
  `base_plus_offset=yes`, proven range, and stack-home metadata, but reject at
  the same sret authority boundary because the ABI pointee extent is
  `sret(size=6, align=2)` while `%ret.sret` is an 8-byte pointer home.

Comparison against moved row:
- `src/pr38969.c` moved because its sret/byval pointee extent is 8 bytes.
  That happens to match the `%ret.sret` pointer-home object size, so the
  current sret authority predicate accepts the F32 route after Step 7. The
  remaining rows expose the same route with non-8-byte sret pointee extents.

Other remaining local-memory rows:
- In scope for the same Step 10 repair from current evidence:
  `src/941110-1.c`, `src/950628-1.c`, `src/pr30185.c`,
  `src/20020215-1.c`.
- Likely related but should be reclassified after the sret repair:
  `src/ipa-sra-2.c`, `src/pr49073.c`, `src/pr57861.c`,
  `src/pr58431.c`, `src/pr60017.c`, `src/pr60822.c`,
  `src/pr66556.c`, `src/pr68185.c`, `src/pr68321.c`,
  `src/pr70005.c`, `src/pr88739.c`.
- Out of scope for Step 10 from current evidence: `src/pr46309.c`
  pointer-loaded-from-global, F128/16-byte width rows, move-bundle rows, later
  call ABI/call instruction rows, runtime mismatch rows, and aggregate
  global-data rows.

## Suggested Next

Delegate Step 10 as a narrow prepared-authority/RV64 consumer repair for
non-8-byte sret stack-home stores. The repair should make sret stack-home
local-memory authority validate the ABI memory-return pointee extent, selected
offset, and requested range separately from the 8-byte `%ret.sret` pointer
home, then prove RV64 emits the integer byte/halfword/word/dword sret stores
from explicit authority. Keep byval behavior unchanged unless the focused test
shows the same extent conflation there.

## Watchouts

The smallest shared repair is not source-file matching and not an expectations
rewrite. It is the authority shape difference between an sret pointer home and
the memory-return pointee extent. Preserve the `pr38969` moved route and keep
later call ABI, call-instruction, branch stack-load, move-bundle, aggregate
global-data, F128/16-byte, runtime, pointer-loaded-from-global, and mixed
local/global rows out of the Step 10 packet.

The non-floating local-memory diagnostic is asymmetric: store handling has an
sret pre-check, while the generic non-floating diagnostic checks byval but not
sret. If the sret authority fix is correct, the store pre-check should prevent
the generic diagnostic for valid stores, but focused negative tests should
still keep malformed sret routes fail-closed with the right owner.

## Proof

Trace-only packet; no build proof required and no root-level `.log` file was
created.

Evidence inputs:
- `build/agent_state/633_step8_current_diagnostics.tsv`
- `build/agent_state/633_step8_pr30185.prepared.txt`
- `build/agent_state/633_step8_950628-1.prepared.txt`
- `build/agent_state/633_step8_20020215-1.prepared.txt`
- `build/agent_state/633_step8_pr38969.prepared.txt`

Evidence output:
- `build/agent_state/633_step9_remaining_stack_home_trace.md`

Extra check: `git diff --check` passed.
