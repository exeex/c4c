Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Select The Next Focused Owner

# Current Packet

## Just Finished

Step 3 - Select The Next Focused Owner is complete. The selected owner is the
existing open parked idea:

`ideas/open/328_aarch64_byval_aggregate_call_argument_lane_publication.md`

Reason: the strongest ready bucket from Step 2 is `00204` byval aggregate call
lane publication. Its first bad fact is `fa_s1(s1)`: the callee expects the
1-byte struct payload in `w0`, but generated caller `arg` prepares `s1` in a
stack temp and branches with `add x0, sp, #928`, passing the temp address
instead of packing the payload byte into the AAPCS64 integer argument lane.

That falls directly inside idea 328's scope: caller-side publication for small
byval aggregate call arguments, mapping prepared aggregate source bytes or
frame slots into AAPCS64 integer argument register lanes before `bl`. Idea 328
is still under `ideas/open/` and its lifecycle handoff says to keep it parked
unless fresh generated-code evidence shows another caller-side byval aggregate
lane publication fault. Step 2 supplies exactly that fresh evidence.

The other strong Step 2 candidates are not selected for this handoff:

- Dynamic indexed aggregate/global access (`00195`/`00205`, adjacent `00176`)
  has concrete evidence but no matching open parked idea in the current
  activatable set; selecting it would require proposing a new focused idea.
- FP expression/comparison lowering (`00119`/`00123`/`00174`) also has concrete
  evidence, but no matching existing open parked owner among the listed
  variadic/HFA/byval/stdarg/MOVI/scalar-cast topics.
- `00204` byval is preferred because it has both a precise first bad fact and
  an existing open owner whose source scope already defines in-scope,
  out-of-scope, acceptance, and reviewer reject boundaries.

## Suggested Next

Delegate lifecycle authority for Step 4 to switch from the umbrella inventory
plan to `ideas/open/328_aarch64_byval_aggregate_call_argument_lane_publication.md`
before any implementation work starts.

## Watchouts

- Do not implement fixes under the umbrella inventory plan.
- The selection relies on fresh Step 2 generated-code evidence, not on failing
  counts alone.
- When idea 328 resumes, the first packet should re-localize the current
  `00204` caller/callee byval lane shape against the latest generated
  artifacts before editing code, because idea 328 previously repaired related
  byval subcases and the current evidence may represent a regression or a
  newly exposed subcase.
- `00005` is still an admission failure, so it should not be grouped with
  runtime codegen regressions until semantic `lir_to_bir` accepts the program.
- Dynamic indexed aggregate/global access and FP expression/comparison
  lowering remain viable future owners, but this packet intentionally does not
  create new ideas.
- `00173` and `00207` are timeout quarantines from the fresh broad run; avoid
  unbounded reruns or output-storm diagnosis inside the umbrella.

## Proof

No new test run was required or performed for Step 3. Selection used the Step 2
classification already recorded in `todo.md`, the active plan, and existing
open idea text. The preserved classification proof log remains
`test_after.log` from:

`cmake --build --preset default && ctest --test-dir build -j10 -R backend --output-on-failure | tee test_after.log`
