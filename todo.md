Status: Active
Source Idea Path: ideas/open/630_string_constant_local_memory_policy.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh String-Constant Evidence

# Current Packet

## Just Finished

Completed Step 1: refreshed the ten representative string-constant local-memory
rows from the source idea with the delegated RV64 GCC torture backend scan.

Scan result: `total=10 passed=0 failed=10`. Every row currently fails during
RV64 object compilation at `unsupported_local_memory_access`, with the object
route requiring prepared frame-slot or pointer-value base-plus-offset local
memory addressing.

Row classification from the scan and saved focused prepared-BIR probes:

- `src/20000722-1.c`: failed at `unsupported_local_memory_access`; prepared
  addressing has one `base=string_constant` access for `.str0`, offset `0`,
  size/access width `8`, address space `default`, `layout_authority=unknown`,
  `range_verdict=proven_out_of_bounds`, and `policy=unspecified`; also has
  pointer-value byte accesses after the pointer is stored through a local.
- `src/20010123-1.c`: failed at `unsupported_local_memory_access`; prepared
  addressing has one `base=string_constant` access for `.str0`, offset `0`,
  size/access width `8`, address space `default`, `layout_authority=unknown`,
  `range_verdict=proven_in_bounds`, and `policy=unspecified`.
- `src/20011109-2.c`: failed at `unsupported_local_memory_access`; prepared
  addressing has two `base=string_constant` accesses for `.str0`, offset `0`,
  size/access width `8`, address space `default`, `layout_authority=unknown`,
  `range_verdict=proven_out_of_bounds`, and `policy=unspecified`; later byte
  loads appear as pointer-value accesses from local-held string pointers.
- `src/20021204-1.c`: failed at `unsupported_local_memory_access`; prepared
  addressing has one `base=string_constant` access for `.str0`, offset `0`,
  size/access width `8`, address space `default`, `layout_authority=unknown`,
  `range_verdict=proven_out_of_bounds`, and `policy=unspecified`; one separate
  global-symbol access is visible and should remain out of this owner bucket.
- `src/20030920-1.c`: failed at `unsupported_local_memory_access`; prepared
  addressing has one `base=string_constant` access for `.str0`, offset `0`,
  size/access width `8`, address space `default`, `layout_authority=unknown`,
  `range_verdict=proven_out_of_bounds`, and `policy=unspecified`; subsequent
  byte loads are pointer-value accesses through a local string pointer.
- `src/920429-1.c`: failed at `unsupported_local_memory_access`; prepared
  addressing has one `base=string_constant` access for `.str0`, offset `0`,
  size/access width `8`, address space `default`, `layout_authority=unknown`,
  `range_verdict=proven_out_of_bounds`, and `policy=unspecified`; separate
  global-symbol accesses remain outside this string-constant policy packet.
- `src/930429-1.c`: failed at `unsupported_local_memory_access`; prepared
  addressing has one `base=string_constant` access for `.str0`, offset `0`,
  size/access width `8`, address space `default`, `layout_authority=unknown`,
  `range_verdict=proven_out_of_bounds`, and `policy=unspecified`; also has one
  pointer-value byte access through the local-held pointer.
- `src/pr34415.c`: failed at `unsupported_local_memory_access`; prepared
  addressing has one `base=string_constant` access for `.str0`, offset `0`,
  size/access width `8`, address space `default`, `layout_authority=unknown`,
  `range_verdict=proven_out_of_bounds`, and `policy=unspecified`; also has
  pointer-value byte accesses through the local-held pointer.
- `src/pr35800.c`: failed at `unsupported_local_memory_access`; prepared
  addressing has many `base=string_constant` accesses (`.str0` and later string
  symbols), offset `0`, size/access width `8`, address space `default`,
  `layout_authority=unknown`, mixed in-bounds and out-of-bounds verdicts, and
  `policy=unspecified`.
- `src/ptr-arith-1.c`: failed at `unsupported_local_memory_access`; prepared
  addressing has one `base=string_constant` access for `.str0`, offset `0`,
  size/access width `8`, address space `default`, `layout_authority=unknown`,
  `range_verdict=proven_in_bounds`, and `policy=unspecified`.

Current owner bucket: in-scope string-constant local-memory authority gap. The
prepared representation exposes string symbols, offsets, access widths, and
default address space, but the string address materializations have
`policy=unspecified` and the accesses have `layout_authority=unknown`, so RV64
has no explicit string/data local-memory authority to consume.

## Suggested Next

Execute Step 2: trace the prepared string/data authority carriers for the
in-scope bucket, identify where `policy=unspecified` and
`layout_authority=unknown` should become explicit string-constant local-memory
authority, and name the smallest non-overfit producer or carrier packet before
RV64 consumer admission.

## Watchouts

Do not treat string constants as frame slots or ordinary globals. Do not infer
string identity, extent, offset, byte contents, or authority from source
filenames, literal spelling, final assembly, or testcase identity. Rows with
later pointer-value byte loads should keep pointer arithmetic beyond the
selected string-constant access contract outside this idea; direct global-symbol
accesses seen in `src/20021204-1.c` and `src/920429-1.c` also remain outside
this bucket.

## Proof

`cmake --build --preset default && ALLOWLIST=build/agent_state/630_step1_string_constant.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/630_step1_string_constant.log 2>&1`

Result: build passed; focused scan returned nonzero with `total=10 passed=0
failed=10`, matching the expected evidence state for this classification
packet. Proof log:
`build/agent_state/630_step1_string_constant.log`.

Detailed prepared-addressing facts above are supported by:

- Combined summary:
  `build/agent_state/630_step1_string_constant.prepared_summary.txt`
- Full per-case prepared-BIR probes:
  `build/agent_state/630_step1_20000722-1.prepared.txt`,
  `build/agent_state/630_step1_20010123-1.prepared.txt`,
  `build/agent_state/630_step1_20011109-2.prepared.txt`,
  `build/agent_state/630_step1_20021204-1.prepared.txt`,
  `build/agent_state/630_step1_20030920-1.prepared.txt`,
  `build/agent_state/630_step1_920429-1.prepared.txt`,
  `build/agent_state/630_step1_930429-1.prepared.txt`,
  `build/agent_state/630_step1_pr34415.prepared.txt`,
  `build/agent_state/630_step1_pr35800.prepared.txt`, and
  `build/agent_state/630_step1_ptr-arith-1.prepared.txt`.
