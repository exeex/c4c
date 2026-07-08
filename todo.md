Status: Active
Source Idea Path: ideas/open/612_rv64_instruction_fragment_consumers.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Next-Family Or Close-Readiness Classification

# Current Packet

## Just Finished

Step 4 from `plan.md` classified close readiness after the Step 3 narrow
bitfield/add-immediate work. The narrow integer route is exhausted for now:
the add-immediate positives still pass in
`build/agent_state/612_step3_add_immediate_direct_probes.log`, and the only
remaining scalar narrow integer `BinaryInst` row is `src/931110-1.c`, owner
`i16 %t13.bf.sext`, an `ashr` singleton.

Classification: idea 612 is not close-ready as a completed source idea because
current residuals still include instruction-fragment-shaped rows. It should
not continue with another Step 3-style same-family narrow integer packet,
because the only same-family candidate is the singleton `ashr` row. The route
should be split or rewritten before more implementation: the best named
follow-up family is pointer `BinaryInst`/address authority, but that needs an
explicit pointer/address-authority runbook rather than being folded into the
narrow integer consumer route.

Remaining residuals by first owner:

- Pointer `BinaryInst`/address authority: current residuals include the 23
  plain pointer `BinaryInst` rows recorded in Step 3, plus broader pointer
  arithmetic/address-authority rows in the stale failure-reason refresh. This
  is the only plausible next named family, but it needs plan-owner split or
  rewrite before implementation.
- Cast producer/consumer: 27 stale `CastInst` instruction-fragment rows remain
  in `build/agent_state/612_step3_unsupported_instruction_rows.tsv`; focused
  probes also showed cast rows such as `src/20030714-1.c` and
  `src/pr23467.c`. These are not narrow integer continuation work.
- Call/ABI/policy: 36 stale `CallInst` instruction-fragment rows and broader
  `unsupported_call_abi` rows remain; ownership is ABI/policy/call-boundary,
  not the current integer fragment family.
- Select publication: 7 stale `SelectInst` instruction-fragment rows and
  broader select-publication/move rows remain; ownership is publication or
  selected-value wiring.
- Inline asm/policy: inline asm rows such as `src/pr84524.c` remain
  policy-owned.
- Terminator: rows such as `src/pr17252.c` remain terminator-lowering-owned.
- Move-bundle: rows such as `src/pr58277-2.c` and the broad ambiguous
  move-bundle population remain move-bundle target/source-authority work.
- Branch freshness: rows such as `src/strcpy-1.c` and other branch
  stack-load authority/source-freshness rows remain branch-freshness-owned.
- Global data/runtime: global data, local storage/access, stack-frame,
  parameter-home, and runtime/library-policy rows remain outside this route.
- Singleton narrow `ashr` row: `src/931110-1.c` is a real scalar integer
  `BinaryInst`, but implementing it alone would be a testcase-shaped
  micro-slice unless plan-owner finds more same-family `ashr` breadth.

## Suggested Next

Recommended plan-owner action: split or rewrite the route. Retire the current
narrow integer fragment runbook as exhausted, keep idea 612 open unless the
supervisor decides these residuals belong to separate ideas, and create a new
bounded runbook only if it names pointer `BinaryInst`/address authority as the
next family with explicit positive and negative authority rows.

## Watchouts

- Do not take `src/931110-1.c` as a standalone `ashr` implementation packet
  without more same-family rows; that would be testcase-shaped.
- Do not mix pointer/address authority with narrow integer add/shift lowering.
- Cast, call/ABI, select publication, inline asm, terminator, move-bundle,
  branch freshness, and global/runtime rows need separate owner-specific plans
  or explicit plan-owner scope changes.

## Proof

Ran delegated proof command exactly and preserved the output in
`test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed; `test_after.log` reports 346/346 backend tests passed and
total real test time `2.09 sec`.

Focused direct probes were classification-only; no implementation files were
changed in this packet. The Step 3 direct probe log remains available at
`build/agent_state/612_step3_add_immediate_direct_probes.log`.
