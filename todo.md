Status: Active
Source Idea Path: ideas/open/310_rv64_c_testsuite_qemu_nonzero_runtime_triage.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Capture Representative Backend Evidence

# Current Packet

## Just Finished

Step 2: Split Runtime Failures By Exit Class inspected
`build/rv64_c_testsuite_probe_latest/summary.tsv` and the corresponding
runtime-output artifacts without regenerating the probe. Counting only real
source rows whose first column starts with `src/`, all 122 `QEMU_NONZERO`
rows and the single `QEMU_TIMEOUT` row are covered by these observable
classes:

| Runtime class | Exit status | Count | Representative |
| --- | ---: | ---: | --- |
| QEMU_NONZERO | 132 | 90 | `src/00005.c` |
| QEMU_NONZERO | 139 | 31 | `src/00008.c` |
| QEMU_NONZERO | 112 | 1 | `src/00112.c` |
| QEMU_TIMEOUT | 124 | 1 | `src/00025.c` |
| TOTAL | - | 123 | - |

The representative QEMU output files
`src_00005_c.qemu.out`, `src_00008_c.qemu.out`,
`src_00112_c.qemu.out`, and `src_00025_c.qemu.out` are all zero bytes. A
size sweep over every bucketed `*.qemu.out` selected from these TSV rows found
123 files of size 0, so there is no runtime text payload to subdivide further
from these artifacts.

Exit status 132 cases:

`src/00005.c`, `src/00006.c`, `src/00007.c`, `src/00009.c`, `src/00018.c`,
`src/00026.c`, `src/00027.c`, `src/00028.c`, `src/00029.c`, `src/00030.c`,
`src/00031.c`, `src/00035.c`, `src/00036.c`, `src/00037.c`, `src/00039.c`,
`src/00041.c`, `src/00042.c`, `src/00043.c`, `src/00044.c`, `src/00046.c`,
`src/00051.c`, `src/00056.c`, `src/00058.c`, `src/00064.c`, `src/00066.c`,
`src/00084.c`, `src/00087.c`, `src/00090.c`, `src/00095.c`, `src/00096.c`,
`src/00102.c`, `src/00104.c`, `src/00109.c`, `src/00113.c`, `src/00124.c`,
`src/00125.c`, `src/00126.c`, `src/00131.c`, `src/00132.c`, `src/00133.c`,
`src/00137.c`, `src/00138.c`, `src/00140.c`, `src/00143.c`, `src/00144.c`,
`src/00147.c`, `src/00151.c`, `src/00154.c`, `src/00156.c`, `src/00157.c`,
`src/00158.c`, `src/00159.c`, `src/00160.c`, `src/00161.c`, `src/00163.c`,
`src/00164.c`, `src/00165.c`, `src/00166.c`, `src/00167.c`, `src/00168.c`,
`src/00169.c`, `src/00172.c`, `src/00173.c`, `src/00174.c`, `src/00176.c`,
`src/00177.c`, `src/00178.c`, `src/00179.c`, `src/00180.c`, `src/00181.c`,
`src/00183.c`, `src/00184.c`, `src/00186.c`, `src/00187.c`, `src/00188.c`,
`src/00191.c`, `src/00194.c`, `src/00197.c`, `src/00198.c`, `src/00201.c`,
`src/00202.c`, `src/00203.c`, `src/00206.c`, `src/00210.c`, `src/00211.c`,
`src/00212.c`, `src/00215.c`, `src/00218.c`, `src/00219.c`, `src/00220.c`

Exit status 139 cases:

`src/00008.c`, `src/00019.c`, `src/00032.c`, `src/00033.c`, `src/00053.c`,
`src/00072.c`, `src/00073.c`, `src/00077.c`, `src/00078.c`, `src/00079.c`,
`src/00081.c`, `src/00082.c`, `src/00083.c`, `src/00086.c`, `src/00092.c`,
`src/00105.c`, `src/00111.c`, `src/00130.c`, `src/00134.c`, `src/00135.c`,
`src/00170.c`, `src/00171.c`, `src/00175.c`, `src/00182.c`, `src/00185.c`,
`src/00190.c`, `src/00192.c`, `src/00193.c`, `src/00196.c`, `src/00199.c`,
`src/00207.c`

Exit status 112 cases:

`src/00112.c`

Timeout cases:

`src/00025.c` (`QEMU_TIMEOUT`, recorded exit/status column `124`)

## Suggested Next

Start Step 3 from `plan.md`: capture representative backend evidence for the
dominant exit classes, beginning with one exit-132 case and one exit-139 case,
then checking whether the singleton exit-112 case needs its own backend trace.

## Watchouts

This active idea is triage-only. Do not implement runtime fixes, weaken test
contracts, or turn the full 220-case sweep into mandatory CTest coverage. The
bucket evidence is TSV-driven because all selected QEMU output files are empty;
Step 3 should use backend/HIR/BIR or emitted assembly evidence for root-cause
shape rather than expecting runtime diagnostics in `*.qemu.out`.

## Proof

No build or CTest proof required for this triage-only todo update. Read-only
artifact inspection was run against
`build/rv64_c_testsuite_probe_latest/summary.tsv` and the selected
`build/rv64_c_testsuite_probe_latest/*.qemu.out` files. No `test_after.log`
was produced because the delegated proof explicitly did not require
build/ctest proof.
