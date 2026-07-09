Status: Active
Source Idea Path: ideas/open/630_string_constant_local_memory_policy.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Reclassify Representative Rows

# Current Packet

## Just Finished

Completed Step 5: re-ran the ten idea-630 representative rows after the Step 3
prepared-authority and Step 4 RV64 consumer changes, then classified each row
from the fresh result log plus focused prepared-access evidence.

Proof summary:

- Total: 10
- Passed: 1
- Failed: 9
- Fresh result log: `build/agent_state/630_step5_string_constant.log`
- Focused access evidence:
  `build/agent_state/630_step5_prepared_access_extract.txt`

Row classification after Step 4:

- `src/20000722-1.c`: fail; remains
  `unsupported_local_memory_access`. Current owner is still idea-630
  string-constant authority/consumer policy: prepared access
  `base=string_constant ... size=8 ... range_verdict=proven_out_of_bounds`
  at `build/agent_state/630_step1_20000722-1.prepared.txt:318`, so the
  authority-gated consumer correctly fails closed instead of treating the
  string as an ordinary frame/global/pointer base. Evidence:
  `build/rv64_gcc_c_torture_backend/src_20000722-1.c/case.log`.
- `src/20010123-1.c`: pass. This row moved past
  `unsupported_local_memory_access` and is the representative row completed by
  the Step 3/4 string-constant path. Evidence:
  `build/rv64_gcc_c_torture_backend/src_20010123-1.c/case.log`.
- `src/20011109-2.c`: fail; remains
  `unsupported_local_memory_access`. Current owner is still idea-630
  string-constant producer/authority policy: string stores have
  `source_producer=unknown` at
  `build/agent_state/630_step1_20011109-2.prepared.txt:343` and `:345`, and
  string accesses are `range_verdict=proven_out_of_bounds` at `:409` and
  `:412`. Later byte reads through pointer values at `:420-:429` are a
  separate pointer-value byte-access owner if the string authority issue is
  fixed. Evidence:
  `build/rv64_gcc_c_torture_backend/src_20011109-2.c/case.log`.
- `src/20021204-1.c`: fail; remains
  `unsupported_local_memory_access`. Current owner is still idea-630
  string-constant extent/range authority: the prepared string access is
  `range_verdict=proven_out_of_bounds` at
  `build/agent_state/630_step1_20021204-1.prepared.txt:371`. The row also has
  out-of-scope direct-global work (`base=global_symbol ... symbol=z`) at
  `:375` and aggregate/block-entry stack-home publication notes at `:183` and
  `:185`, but the fresh stop is still local-memory admission. Evidence:
  `build/rv64_gcc_c_torture_backend/src_20021204-1.c/case.log`.
- `src/20030920-1.c`: fail; remains
  `unsupported_local_memory_access`. Current owner is still idea-630
  string-constant extent/range authority: prepared string access is
  `range_verdict=proven_out_of_bounds` at
  `build/agent_state/630_step1_20030920-1.prepared.txt:229`. Later
  pointer-value byte reads at `:234` and `:238` are out-of-scope if exposed
  after string authority. Evidence:
  `build/rv64_gcc_c_torture_backend/src_20030920-1.c/case.log`.
- `src/920429-1.c`: fail; remains
  `unsupported_local_memory_access`. Current owner is still idea-630
  string-constant extent/range authority at
  `build/agent_state/630_step1_920429-1.prepared.txt:393`
  (`range_verdict=proven_out_of_bounds`). Out-of-scope direct-global accesses
  to `i`/`j` appear at `:387` and `:389`, and select-carrier publication gaps
  appear at `:336-:337`. Evidence:
  `build/rv64_gcc_c_torture_backend/src_920429-1.c/case.log`.
- `src/930429-1.c`: fail; remains
  `unsupported_local_memory_access`. Current owner is still idea-630
  string-constant extent/range authority at
  `build/agent_state/630_step1_930429-1.prepared.txt:298`
  (`range_verdict=proven_out_of_bounds`). The row also contains an out-of-scope
  pointer-value byte access at `:293`. Evidence:
  `build/rv64_gcc_c_torture_backend/src_930429-1.c/case.log`.
- `src/pr34415.c`: fail; remains
  `unsupported_local_memory_access`. Current owner is still idea-630
  string-constant extent/range authority at
  `build/agent_state/630_step1_pr34415.prepared.txt:606`
  (`range_verdict=proven_out_of_bounds`). Out-of-scope pointer-value byte
  accesses are present at `:577`, `:587`, and `:600`, with select-carrier
  publication gaps at `:501-:506`. Evidence:
  `build/rv64_gcc_c_torture_backend/src_pr34415.c/case.log`.
- `src/pr35800.c`: fail; remains
  `unsupported_local_memory_access`. Current owner is a mixed string-constant
  authority bucket: many prepared string rows now have in-bounds authority, but
  several still report `range_verdict=proven_out_of_bounds`, including
  `build/agent_state/630_step1_pr35800.prepared.txt:1880`, `:1883`, `:1886`,
  `:1889`, `:1910`, `:1913`, `:1931`, and `:1967`. The row also has
  out-of-scope stack-home publication at `:989-:990`. Evidence:
  `build/rv64_gcc_c_torture_backend/src_pr35800.c/case.log`.
- `src/ptr-arith-1.c`: fail, but moved past
  `unsupported_local_memory_access`. Current owner is out of scope for idea
  630: `rv64_prepared_move_bundle_consumer`, with
  `unsupported_prepared_move_bundle_classification: non-parallel` and
  `fragment_status=producer_authority_missing_for_register_fan_in_stack_destination`
  in `build/rv64_gcc_c_torture_backend/src_ptr-arith-1.c/case.log`. Its
  prepared string access was already in-bounds at
  `build/agent_state/630_step1_ptr-arith-1.prepared.txt:278`.

Rows moved past `unsupported_local_memory_access`: `src/20010123-1.c` and
`src/ptr-arith-1.c`.

Rows that remain string-constant authority/consumer gaps:
`src/20000722-1.c`, `src/20011109-2.c`, `src/20021204-1.c`,
`src/20030920-1.c`, `src/920429-1.c`, `src/930429-1.c`,
`src/pr34415.c`, and `src/pr35800.c`.

Out-of-scope owners observed but not counted as idea-630 completion:
direct-global (`src/20021204-1.c`, `src/920429-1.c`), aggregate/block-entry
stack-home publication (`src/20021204-1.c`, `src/pr35800.c`), pointer-value
byte access (`src/20011109-2.c`, `src/20030920-1.c`, `src/930429-1.c`,
`src/pr34415.c`), select-carrier publication (`src/920429-1.c`,
`src/pr34415.c`), and RV64 prepared move-bundle fan-in
(`src/ptr-arith-1.c`).

Recommendation for idea 630: continue with one more narrow string-constant
packet focused on why prepared string constants still get
`range_verdict=proven_out_of_bounds` for 8-byte pointer loads even when the
carrier is a string label and the access is base-plus-offset. Do not broaden
into direct-global, aggregate-home, pointer-value byte access, select-carrier,
or move-bundle owners in the next packet.

## Suggested Next

Trace the prepared string-constant extent/range proof for the eight remaining
`unsupported_local_memory_access` rows and identify the first producer boundary
that marks their 8-byte string-constant pointer materialization as
`proven_out_of_bounds`.

## Watchouts

`src/ptr-arith-1.c` should be split away from idea 630 if pursued further; it
now fails in `rv64_prepared_move_bundle_consumer`, not string-constant local
memory. `src/20010123-1.c` is the only row that fully passed. Several remaining
rows have secondary out-of-scope blockers, so do not count them as idea-630
complete unless they actually move past the string-constant local-memory stop.

## Proof

Ran the delegated proof command exactly:

`cmake --build --preset default && ALLOWLIST=build/agent_state/630_step1_string_constant.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/630_step5_string_constant.log 2>&1`

Result: build succeeded; allowlist check exited nonzero because 9 of 10 rows
still fail. Proof/result log:
`build/agent_state/630_step5_string_constant.log`. Focused diagnostic extract:
`build/agent_state/630_step5_prepared_access_extract.txt`.
