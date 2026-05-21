# Current Packet

Status: Complete
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Residual Families

## Just Finished

Step 2: Classify Residual Families completed a read-only classification of the
fresh backend-regex residuals from `test_after.log`.

Candidate buckets from the current 19 non-passing tests:

- Local backend-route expectation/stale snippet: `backend_codegen_route_x86_64_pointer_param_loaded_char_deref_observe_semantic_bir` fails because the required snippet still expects `bir.store_local %lv.p, ptr %t0`, while current semantic BIR stores the loaded string-address value through `@.str0 = bir.load_local ptr %lv.p, addr .str0` followed by `bir.store_local %lv.p, ptr @.str0`. This is not evidence for reopening the completed `00173` pointer-derived string-load chain; it is a local contract/expectation drift to keep separate from semantic capability work.
- Semantic `lir_to_bir` indirect local-memory admission: `00005` stops before prepared-module handoff with latest failure in the store local-memory semantic family for `**pp = 1`; `00217` stops before prepared-module handoff with latest failure in the load local-memory semantic family for `*(unsigned*)(data + r) += a - b`. Both are compile-stage semantic lowering blockers, not AArch64 printer, runner, timeout, or runtime ABI failures.
- Pointer/null and scalar compare/select publication: `00112` returns nonzero for `"abc" == (void *)0`; `00183` prints `0..9` instead of the ternary-selected square-or-triple sequence. `00119` is an adjacent floating comparison/global double case with exit 176. These are viable later scalar/FP result-publication buckets but need generated-code first-bad-fact evidence before selection.
- Composite/byval/HFA/f128/call-boundary ABI: `00140` segfaults around struct-by-value plus variadic calls; `00174` prints zeros and corrupt integers for most FP expression/comparison/assignment output; `00204` reaches the AArch64 machine-node printer but fails on a deferred unsupported call-boundary move requiring prepared GPR, scalar FPR, or structured f128 q-register authority. Existing `00204` variadic/HFA ideas are stale for this exact first bad fact unless fresh generated-code evidence moves back into their scoped HFA, byval, stdarg, or fixed-formal boundaries.
- Indexed aggregate, pointer, and address/writeback residuals: `00163` mostly prints the global struct fields correctly but `bolshevic.b` through an address local reads `42` instead of `34`; `00176` quicksort mutates the global array into a wrong order; `00182` LED output collapses toward zeros; `00187` file I/O output is mostly correct but the initial `fread` return check prints `couldn't read fred.txt`; `00205` has expected static aggregate initializer output but actual output is empty. This is a broad address/aggregate/writeback surface, not one selectable owner without fresh focused probes.
- Control-flow, dynamic stack, timeout, and initializer stress residuals: `00143` exits 1 in Duff's-device switch/fallthrough copy; `00200` times out in integer promotion/left-shift type checks; `00207` times out in VLA/goto/short-circuit control flow; `00216` segfaults in complex initializer/compound-literal/flexible-array coverage; `00218` reports `unsigned enum bit-fields broken`. These remain parked as separate semantic buckets until probed.

Rejected or stale owners:

- `ideas/open/356_semantic_bir_pointer_derived_string_loads.md` is parked and
  its repaired owner was dynamic pointer-derived byte loads for `00173`; the
  current local backend-route failure is a stale snippet around string pointer
  initialization spelling, while `00173` passed in the fresh inventory.
- `ideas/open/327_aarch64_fixed_formal_entry_publication.md`,
  `ideas/open/329_aarch64_variadic_aggregate_va_arg_call_setup.md`,
  `ideas/open/331_aarch64_variadic_stdarg_cursor_format_residual.md`, and
  `ideas/open/326_aarch64_variadic_hfa_floating_residual.md` do not own the
  current `00204` diagnostic from `test_after.log`: the first bad fact is a
  machine-printer deferred unsupported call-boundary move with f128/q-register
  authority, not their older fixed-formal, post-`va_arg`, stdarg cursor, or
  HFA/floating output mismatches.
- Historical indexed aggregate owner references such as
  `ideas/open/348_aarch64_indexed_aggregate_address_writeback.md` are not
  selectable as existing open paths in the current tree. The current
  aggregate/address runtime bucket is real, but it should be split from fresh
  first-bad-fact probes rather than from stale historical counts.
- Do not reopen closed owners 365/366 or any 00173-focused route: the fresh
  inventory records `00173` as passing, and no current residual contradicts
  that closure boundary.

Strongest current semantic owner:

- New focused lifecycle target: semantic BIR indirect local-memory load/store
  admission for pointer-derived lvalues.
- Representatives: `c_testsuite_aarch64_backend_src_00005_c` and
  `c_testsuite_aarch64_backend_src_00217_c`.
- First bad facts: both fail before prepared AArch64 handoff in semantic
  `lir_to_bir`; `00005` reports store local-memory failure for a pointer to a
  pointer local (`**pp = 1`), while `00217` reports load local-memory failure
  for pointer arithmetic plus casted unsigned load/store update.
- Why it wins now: these are direct compiler diagnostics from the current log,
  not inferred runtime symptoms, and they identify a shared semantic lowering
  boundary that can be repaired before broader AArch64 runtime buckets are
  probed.

New-idea handoff summary for plan-owner:

- Proposed title: Semantic BIR Indirect Local-Memory Lvalue Admission.
- Goal: repair semantic `lir_to_bir` lowering for indirect local-memory
  lvalues whose address comes from loaded pointer values, pointer-to-pointer
  locals, pointer arithmetic, or casts, so backend assembly-route handoff can
  admit the cases before AArch64 preparation.
- In scope: localize the failing semantic local-memory load/store path for
  `**pp = 1` and `*(unsigned*)(data + r) += a - b`; add focused semantic or
  backend-route coverage that proves indirect load and store lvalues remain
  represented as real pointer-based memory operations; prove `00005` and
  `00217` advance or pass.
- Out of scope: AArch64 register allocation, call-boundary moves, byval/HFA or
  f128 ABI work, runtime mismatch buckets, timeout policy, runner behavior,
  CTest registration, proof-log policy, expectation changes, unsupported
  classification changes, and local backend-route snippet rewrites unless the
  supervisor delegates a separate test-contract cleanup.
- Acceptance: focused coverage fails before the repair and passes after it for
  both indirect local-memory load and store lvalues; the two c-testsuite
  representatives advance past semantic handoff without filename-specific
  matching; adjacent passing `00173` semantic pointer-derived load coverage
  stays passing.
- Reviewer reject signals: special-casing `00005`, `00217`, `pp`, `data`, one
  cast shape, one pointer depth, or one emitted BIR name; hiding the failures
  with allowlist/unsupported/runner/timeout changes; claiming progress through
  expectation rewrites while semantic `lir_to_bir` still rejects the same
  local-memory families; broadening into runtime AArch64 ABI or aggregate
  writeback without a fresh split.

## Suggested Next

Supervisor should call plan-owner to split or activate the new focused idea
`Semantic BIR Indirect Local-Memory Lvalue Admission`, then delegate an
implementation packet against focused semantic/backend-route coverage plus the
two external representatives `00005` and `00217`.

## Watchouts

- The local backend-route failure is intentionally classified as stale
  contract drift, not accepted progress through expectation editing. Do not
  fix it in the same implementation packet as semantic admission unless the
  supervisor explicitly approves a separate test-contract cleanup.
- Runtime buckets are not denied; they are parked because the current packet
  did not produce generated-code first-bad-fact evidence strong enough to pick
  among scalar compare/select, aggregate/writeback, ABI, FP, timeout, and
  initializer owners.
- Existing open `00204` variadic/HFA/stdarg ideas should not be reactivated
  from their headers; the current `00204` evidence is a different
  call-boundary/f128 machine-printer residual.

## Proof

No proof command was delegated for this read-only classification packet.
Evidence came from existing `test_after.log`, source reads under
`tests/c/external/c-testsuite/src/`, current backend-route test registration,
and current `ideas/open/` candidate scans. `test_after.log`,
`test_before.log`, implementation files, tests, `plan.md`, source ideas,
closed ideas, and review artifacts were not modified.
