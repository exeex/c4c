# Backend Regex Failure Family Inventory Todo

Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture Current Backend Regex Evidence

# Current Packet

## Just Finished

Step 1 captured current backend regex evidence from a fresh delegated run:
`ctest -j10 -R backend --output-on-failure` executed from
`/workspaces/c4c/build`, with complete output written to `test_after.log`.

Result counts from the fresh run:
- Selected: 352
- Passed: 291
- Failed: 61

Exact failing test list:
- `1485 - c_testsuite_aarch64_backend_src_00024_c (Failed)  aarch64_backend c_testsuite`
- `1497 - c_testsuite_aarch64_backend_src_00031_c (Failed)  aarch64_backend c_testsuite`
- `1503 - c_testsuite_aarch64_backend_src_00035_c (Failed)  aarch64_backend c_testsuite`
- `1531 - c_testsuite_aarch64_backend_src_00050_c (Failed)  aarch64_backend c_testsuite`
- `1559 - c_testsuite_aarch64_backend_src_00064_c (Failed)  aarch64_backend c_testsuite`
- `1563 - c_testsuite_aarch64_backend_src_00066_c (Failed)  aarch64_backend c_testsuite`
- `1595 - c_testsuite_aarch64_backend_src_00086_c (Failed)  aarch64_backend c_testsuite`
- `1601 - c_testsuite_aarch64_backend_src_00089_c (Failed)  aarch64_backend c_testsuite`
- `1627 - c_testsuite_aarch64_backend_src_00102_c (Failed)  aarch64_backend c_testsuite`
- `1631 - c_testsuite_aarch64_backend_src_00104_c (Failed)  aarch64_backend c_testsuite`
- `1633 - c_testsuite_aarch64_backend_src_00105_c (Failed)  aarch64_backend c_testsuite`
- `1645 - c_testsuite_aarch64_backend_src_00111_c (Failed)  aarch64_backend c_testsuite`
- `1647 - c_testsuite_aarch64_backend_src_00112_c (Failed)  aarch64_backend c_testsuite`
- `1649 - c_testsuite_aarch64_backend_src_00113_c (Failed)  aarch64_backend c_testsuite`
- `1661 - c_testsuite_aarch64_backend_src_00119_c (Failed)  aarch64_backend c_testsuite`
- `1665 - c_testsuite_aarch64_backend_src_00121_c (Failed)  aarch64_backend c_testsuite`
- `1669 - c_testsuite_aarch64_backend_src_00123_c (Failed)  aarch64_backend c_testsuite`
- `1675 - c_testsuite_aarch64_backend_src_00126_c (Failed)  aarch64_backend c_testsuite`
- `1691 - c_testsuite_aarch64_backend_src_00134_c (Failed)  aarch64_backend c_testsuite`
- `1693 - c_testsuite_aarch64_backend_src_00135_c (Failed)  aarch64_backend c_testsuite`
- `1697 - c_testsuite_aarch64_backend_src_00137_c (Failed)  aarch64_backend c_testsuite`
- `1699 - c_testsuite_aarch64_backend_src_00138_c (Failed)  aarch64_backend c_testsuite`
- `1701 - c_testsuite_aarch64_backend_src_00139_c (Failed)  aarch64_backend c_testsuite`
- `1703 - c_testsuite_aarch64_backend_src_00140_c (Failed)  aarch64_backend c_testsuite`
- `1709 - c_testsuite_aarch64_backend_src_00143_c (Failed)  aarch64_backend c_testsuite`
- `1711 - c_testsuite_aarch64_backend_src_00144_c (Failed)  aarch64_backend c_testsuite`
- `1725 - c_testsuite_aarch64_backend_src_00151_c (Failed)  aarch64_backend c_testsuite`
- `1737 - c_testsuite_aarch64_backend_src_00157_c (Failed)  aarch64_backend c_testsuite`
- `1741 - c_testsuite_aarch64_backend_src_00159_c (Failed)  aarch64_backend c_testsuite`
- `1751 - c_testsuite_aarch64_backend_src_00164_c (Failed)  aarch64_backend c_testsuite`
- `1759 - c_testsuite_aarch64_backend_src_00168_c (Failed)  aarch64_backend c_testsuite`
- `1761 - c_testsuite_aarch64_backend_src_00169_c (Failed)  aarch64_backend c_testsuite`
- `1763 - c_testsuite_aarch64_backend_src_00170_c (Failed)  aarch64_backend c_testsuite`
- `1767 - c_testsuite_aarch64_backend_src_00172_c (Failed)  aarch64_backend c_testsuite`
- `1769 - c_testsuite_aarch64_backend_src_00173_c (Failed)  aarch64_backend c_testsuite`
- `1771 - c_testsuite_aarch64_backend_src_00174_c (Failed)  aarch64_backend c_testsuite`
- `1773 - c_testsuite_aarch64_backend_src_00175_c (Failed)  aarch64_backend c_testsuite`
- `1775 - c_testsuite_aarch64_backend_src_00176_c (Failed)  aarch64_backend c_testsuite`
- `1781 - c_testsuite_aarch64_backend_src_00179_c (Failed)  aarch64_backend c_testsuite`
- `1785 - c_testsuite_aarch64_backend_src_00181_c (Failed)  aarch64_backend c_testsuite`
- `1787 - c_testsuite_aarch64_backend_src_00182_c (Failed)  aarch64_backend c_testsuite`
- `1793 - c_testsuite_aarch64_backend_src_00185_c (Failed)  aarch64_backend c_testsuite`
- `1795 - c_testsuite_aarch64_backend_src_00186_c (Failed)  aarch64_backend c_testsuite`
- `1797 - c_testsuite_aarch64_backend_src_00187_c (Failed)  aarch64_backend c_testsuite`
- `1801 - c_testsuite_aarch64_backend_src_00189_c (Failed)  aarch64_backend c_testsuite`
- `1809 - c_testsuite_aarch64_backend_src_00193_c (Failed)  aarch64_backend c_testsuite`
- `1811 - c_testsuite_aarch64_backend_src_00194_c (Failed)  aarch64_backend c_testsuite`
- `1813 - c_testsuite_aarch64_backend_src_00195_c (Failed)  aarch64_backend c_testsuite`
- `1815 - c_testsuite_aarch64_backend_src_00196_c (Failed)  aarch64_backend c_testsuite`
- `1823 - c_testsuite_aarch64_backend_src_00200_c (Failed)  aarch64_backend c_testsuite`
- `1831 - c_testsuite_aarch64_backend_src_00205_c (Failed)  aarch64_backend c_testsuite`
- `1835 - c_testsuite_aarch64_backend_src_00207_c (Failed)  aarch64_backend c_testsuite`
- `1837 - c_testsuite_aarch64_backend_src_00208_c (Failed)  aarch64_backend c_testsuite`
- `1839 - c_testsuite_aarch64_backend_src_00209_c (Failed)  aarch64_backend c_testsuite`
- `1847 - c_testsuite_aarch64_backend_src_00213_c (Failed)  aarch64_backend c_testsuite`
- `1849 - c_testsuite_aarch64_backend_src_00214_c (Failed)  aarch64_backend c_testsuite`
- `1851 - c_testsuite_aarch64_backend_src_00215_c (Failed)  aarch64_backend c_testsuite`
- `1853 - c_testsuite_aarch64_backend_src_00216_c (Failed)  aarch64_backend c_testsuite`
- `1857 - c_testsuite_aarch64_backend_src_00218_c (Failed)  aarch64_backend c_testsuite`
- `1861 - c_testsuite_aarch64_backend_src_00220_c (Timeout) aarch64_backend c_testsuite`
- `1863 - c_testsuite_aarch64_backend_src_00204_c (Failed)  aarch64_backend c_testsuite`

## Suggested Next

Proceed to Step 2 classification using the fresh `test_after.log` evidence.

## Watchouts

- This is an umbrella inventory. Do not implement fixes before a focused owner
  is split and activated.
- Preserve closed-owner boundaries for ideas 285 through 297 unless current
  generated-code, proof, or diagnostic evidence contradicts them.
- The fresh failing list is entirely `aarch64_backend c_testsuite`; no local
  backend/unit/CLI failures appear in the CTest failure summary.
- `c_testsuite_aarch64_backend_src_00220_c` timed out in the fresh run and
  should be treated as a timeout/hang classification input, not folded into a
  generic failure bucket without evidence.
- Treat residual global/pointer/aggregate GEP work, `00216`
  pointer-parameter/flexible-array aggregate projection, and printer residuals
  as classification inputs, not automatic new owners.
- Do not change expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, CTest registration, implementation files, or
  canonical logs during lifecycle activation.
- `test_before.log` is the accepted focused baseline. `test_after.log` is
  absent at activation, and `test_baseline.log` is a pre-existing ignored root
  log.

## Proof

Fresh delegated capture run from `/workspaces/c4c/build`:
`ctest -j10 -R backend --output-on-failure > /workspaces/c4c/test_after.log 2>&1`.
CTest exited `8` because 61 of 352 selected backend-regex tests failed.
`test_after.log` is the complete proof/capture log for this packet.
