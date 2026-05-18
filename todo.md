Status: Active
Source Idea Path: ideas/open/293_aarch64_side_effect_control_value_publication_authority.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Validate Boundary and Broader Sampling

# Current Packet

## Just Finished

Step 4 recorded the supervisor-run broader sampling result without rerunning
tests or changing code. The four in-scope side-effect/control-value publication
representatives passed: `00164`, `00169`, `00183`, and `00202`.

The broader sample
`00159|00164|00168|00169|00172|00183|00193|00202|00217` was `5/9`: `00164`,
`00169`, `00172`, `00183`, and `00202` passed; `00159`, `00168`, `00193`, and
`00217` failed.

Failure bucket classification against the source idea:

- `00159`: explicitly separated closed-owner overlap, not this
  side-effect/control-value publication route.
- `00168`: explicitly separated closed-owner overlap, not this
  side-effect/control-value publication route.
- `00193`: explicitly separated closed-owner overlap, not this
  side-effect/control-value publication route.
- `00217`: pointer/address/string-heavy bucket explicitly out of scope for this
  route.

## Suggested Next

Supervisor should treat Step 4 sampling as recorded and decide whether to send
the route to lifecycle close review.

## Watchouts

- The repair remains tied to explicit scalar control/select publication and
  coalesced out-of-SSA join-transfer authority. Do not broaden this into
  enabling every `PreparedMovePhase::BeforeInstruction` bundle; that previously
  regressed the first-six-line `00164` contract.
- The new call-boundary ordering is intentionally narrow: it hoists only
  argument moves whose fresh emitted source register aliases a pending
  call-site address materialization result, and it excludes the address value
  itself. Avoid turning this into wholesale reordering of the prepared
  call-argument bundle.
- The implemented edge hook currently handles the bounded shape needed by
  `%t106`: an out-of-SSA predecessor block-entry bundle with a named scalar
  compare source defined in the successor join prefix, and only when source and
  destination prepared homes name the same register. It now also allows prepared
  load-source operands for the edge-computed compare source. If the next stale
  value needs non-coalesced edge publication, add an explicit prepared-register
  move after source lowering rather than assuming the source home is the
  destination.
- The new select publication path handles scalar same-block binary producers
  for selected values and relies on `csel` after a predicate `cmp`; inserted
  materialization must not clobber condition flags before `csel`.
- Prepared memory access instruction indexes can lag the compacted prepared BIR
  block indexes; the scalar fallback now reloads by prepared result value name
  when ordinary memory lowering misses an un-emitted register-home load.
- Do not touch expected outputs, allowlists, unsupported classifications,
  timeout policy, runner behavior, CTest registration, or build/test
  infrastructure.
- The broader-sample failures are not evidence to expand this route: `00159`,
  `00168`, and `00193` remain closed-owner overlap buckets, and `00217` remains
  pointer/address/string-heavy. Reopening them would require fresh
  generated-code proof that contradicts the source idea's separation.

## Proof

Did not rerun tests per the delegated packet. Reused the supervisor-run broader
sampling log:

```sh
/tmp/c4c_aarch64_side_effect_step4_broader_sample.log
```

Result: `56% tests passed, 4 tests failed out of 9` for the broader sample.
Passing tests: `00164`, `00169`, `00172`, `00183`, and `00202`. Failing tests:
`00159`, `00168`, `00193`, and `00217`.

The in-scope Step 4 representatives `00164`, `00169`, `00183`, and `00202` are
all passing in the reused log.

```sh
pgrep -af '^/workspaces/c4c/build-aarch64-scan/c_testsuite_aarch64_backend/' || true
```

Result already observed clean: no generated runtime process remained.

Log paths:

- `/tmp/c4c_aarch64_side_effect_step4_broader_sample.log`
