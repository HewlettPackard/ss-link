## swtest

## Testing
### CTL Test Suite

Tests must be run on `r2x4_2con3_pcs` model. It is recommended to use the same model, otherwise the link group and link map used in the test must be modified.

Once you have transferred the test scripts from `knl/ctl/test/scripts` onto the emulator. You can begin testing. The *full suite* of test runs rather quickly and should complete in roughly 40 seconds. Any longer, it is likely a test has failed.

> You must not initialize the switch from swtest. These tests run in isolation in the sl driver. Only the drivers can be loaded.

```sh
# Example of transferring scripts to z1build to the sshotplat users home directory in `ctl_tests`
rsync -az --progress sl-driver/knl/ctl/test/scripts sshotplat@z1build:~/ctl_tests
```

```sh
./load_modules.sh --no-init
./ctl_test_all.sh
```

#### Check results

Each test will display a pass fail result

```
Running: test_fec_rate_calc.swtest... PASSED
```

or

```
Running: test_fec_rate_calc.swtest... FAILED
```

If a test fails then the test suite run ends and future tests do not continue.

#### Check Log Results

Sometimes it is useful to see which step failed. I recommend grepping for the "result" string.

```sh
# Using the directory from the example transfer above.
export CTL_TEST_DIR="/root/sshotplat/ctl_tests/scripts"
grep -n result $CTL_TEST_DIR/ctl_tests.log
```

Example results file.

```
---[ New Run ]---
Running: no_device/test_fec_rate_calc.swtest...
read: # Run on r2x4_2con3_pcs model
read: #
read: # Used for debugging the test
read: # ddebug file sl_ctl_test.c on
read: # ddebug file sl_ctl_link_fec.c on
read: # Verify FEC limit calculations
read: # Verify BER calculations
read: test 1 11 0 0 0
test result 0 (0x0)
read: # Verify FEC rate check (reached)
read: test 1 13 0 0 0
test result 0 (0x0)
no_device/test_fec_rate_calc.swtest: PASSED
```

The results of the test follow the filename, in this case "test_fec_rate_calc.swtest." The line with "test result 0 (0x0)" indicates the test step passed. Any non-zero result means the test failed. Note, to help facilitate debugging a test that may error, the lines like `ddebug file sl_ctl_test.c on` can be uncommented to produce more output in dmesg.

## Causing CCW/UCW error during runtime

The following procedure can be used to cause a UCW error for a link.
```sh
# Bring the links up
noshutdown 0,1 0 ether

# Update the policy to turn the FEC monitor on.
test 1 27 3 1 1

# Cause the fec counter to increment UCW (increments at 60 UCW/s) continuously
test 2 41 3 1 0

# Don't forget to stop the counters from incrementing
test 2 41 3 1 2
```

Example output from dmesg.

```sh
[10602.168105] sl [0:00:0]^ lgrp00         ctl-l-fec     : UCW rate reached down limit (UCW = 60 CCW = 0)
[10602.175931] sl [0:01:0]^ lgrp01         ctl-l-fec     : UCW rate reached down limit (UCW = 60 CCW = 0)
```

The following procedure can be used to cause a CCW error for a link.
```sh
# Bring the links up
noshutdown 0,1 0 ether

# Update the policy to turn the FEC monitor on.
test 1 27 3 1 1

# Cause the fec counter to increment UCW (increments at 60 UCW/s)
test 2 41 3 1 1

# Don't forget to stop the counters from incrementing
test 2 41 3 1 2
```

Example output from dmesg.
```sh
[10821.624081] sl [0:01:0]^ lgrp01         ctl-l-fec     : CCW rate reached down limit (UCW = 0 CCW = 20000000)
[10821.630561] sl [0:00:0]^ lgrp00         ctl-l-fec     : CCW rate reached down limit (UCW = 0 CCW = 20000000)
```

## Cause UCW/CCW during Up Check

Currently this cannot be tested without modifications to `swtest` see https://github.hpe.com/hpe/hpc-sshot-rosetta2-drivers/blob/integration/usr/swtest/tags.c#L302 for details on updating the startup limits.

## Setting Burst FEC Errors in HW

This only works in the `r2x4_2con3_pcs` model. It is also worth noting that the sl-driver must be build with testing disabled. Otherwise, the hardware reads are mocked in software.

> You will need to install the `tcl` package in linux.

Located in `/var/users/emu/tools/qemu-tools/rosetta/z1bin` typically. Should be part of your path. Make sure the `rosnic` is loaded.

```sh
init-links -f
```

```sh
err_period_adj -l 0 -t 100000 -D a_to_b -p 2
```

Example Output

```
-> Configuring port group 2 lane 0 to lane_error_period 100000
xeclient cflcadence02.us.cray.com:43170 stop
xeclient cflcadence02.us.cray.com:43170 memory -deposit ztop.enet_link.port_group[2].ports.link_port.link.a_to_b.lane_err_period[0] 100000
xeclient cflcadence02.us.cray.com:43170 run 10us
xeclient cflcadence02.us.cray.com:43170 run -nowait
```

```sh
burst_err_adj -l 0 -0 20 -1 100 -D a_to_b -p 2
```

```
[-> Configuring port group 0 lane 1 to minimum burst length 20$, and maximum burst
 length 100
xeclient cflcadence02.us.cray.com:35135 stop
xeclient cflcadence02.us.cray.com:35135 scope ztop.enet_link.port_group[0].ports.
link_port.link.a2b
xeclient cflcadence02.us.cray.com:35135 deposit burst_error_min_length[1] 20
xeclient cflcadence02.us.cray.com:35135 deposit burst_error_max_length[1] 100
xeclient cflcadence02.us.cray.com:35135 run 10us
xeclient cflcadence02.us.cray.com:35135 run -nowait](<-%3E Configuring port group 2 lane 0 to minimum burst length 20$, and maximum burst length 100
xeclient cflcadence02.us.cray.com:43170 stop
xeclient cflcadence02.us.cray.com:43170 scope ztop.enet_link.port_group[2].ports.link_port.link.a_to_b
xeclient cflcadence02.us.cray.com:43170 deposit burst_error_min_length[0] 20
xeclient cflcadence02.us.cray.com:43170 deposit burst_error_max_length[0] 100
xeclient cflcadence02.us.cray.com:43170 run 10us
xeclient cflcadence02.us.cray.com:43170 run -nowait>)
```

> Counters can be read using `dgrcounters` tool. This tool is part of the `rosetta2-diags_1.0.0_amd64.deb` package.

```
dgrcounters -p 3 | grep cw
```

Example

```
[p03]                    pcs_corrected_cw_00: 219
[p03]                    pcs_corrected_cw_01: 0
[p03]                    pcs_corrected_cw_02: 0
[p03]                    pcs_corrected_cw_03: 0
[p03]                  pcs_uncorrected_cw_00: 0
[p03]                  pcs_uncorrected_cw_01: 0
[p03]                  pcs_uncorrected_cw_02: 0
[p03]                  pcs_uncorrected_cw_03: 0
[p03]                         pcs_good_cw_00: 1495801
[p03]                         pcs_good_cw_01: 0
[p03]                         pcs_good_cw_02: 0
[p03]                         pcs_good_cw_03: 0
[p03]                pcs_corrected_cw_bin_00: 5
[p03]                pcs_corrected_cw_bin_01: 1
[p03]                pcs_corrected_cw_bin_02: 7
[p03]                pcs_corrected_cw_bin_03: 105
[p03]                pcs_corrected_cw_bin_04: 5
[p03]                pcs_corrected_cw_bin_05: 47
[p03]                pcs_corrected_cw_bin_06: 1
[p03]                pcs_corrected_cw_bin_07: 47
[p03]                pcs_corrected_cw_bin_08: 0
[p03]                pcs_corrected_cw_bin_09: 1
[p03]                pcs_corrected_cw_bin_10: 0
[p03]                pcs_corrected_cw_bin_11: 0
[p03]                pcs_corrected_cw_bin_12: 0
[p03]                pcs_corrected_cw_bin_13: 0
[p03]                pcs_corrected_cw_bin_14: 0
[p03]                pcs_corrected_cw_bin_15: 0
[p03]                pcs_corrected_cw_bin_16: 0
[p03]                pcs_corrected_cw_bin_17: 0
[p03]                pcs_corrected_cw_bin_18: 0
[p03]                pcs_corrected_cw_bin_19: 0
[p03]                pcs_corrected_cw_bin_20: 0
[p03]                pcs_corrected_cw_bin_21: 0
[p03]                pcs_corrected_cw_bin_22: 0
[p03]                pcs_corrected_cw_bin_23: 0
[p03]                pcs_corrected_cw_bin_24: 0
[p03]                pcs_corrected_cw_bin_25: 0
[p03]                pcs_corrected_cw_bin_26: 0
[p03]                pcs_corrected_cw_bin_27: 0
[p03]                pcs_corrected_cw_bin_28: 0
[p03]                pcs_corrected_cw_bin_29: 0
[p03]                pcs_corrected_cw_bin_30: 0
[p03]                pcs_corrected_cw_bin_31: 0
[p03]                pcs_corrected_cw_bin_32: 0
[p03]                pcs_corrected_cw_bin_33: 0
[p03]                pcs_corrected_cw_bin_34: 0
[p03]                pcs_corrected_cw_bin_35: 0
[p03]                pcs_corrected_cw_bin_36: 0
[p03]                pcs_corrected_cw_bin_37: 0
[p03]                pcs_corrected_cw_bin_38: 0
[p03]                pcs_corrected_cw_bin_39: 0
[p03]                pcs_corrected_cw_bin_40: 0
[p03]                pcs_corrected_cw_bin_41: 0
[p03]                pcs_corrected_cw_bin_42: 0
[p03]                pcs_corrected_cw_bin_43: 0
[p03]                pcs_corrected_cw_bin_44: 0
[p03]                pcs_corrected_cw_bin_45: 0
[p03]                pcs_corrected_cw_bin_46: 0
[p03]                pcs_corrected_cw_bin_47: 0
[p03]                pcs_corrected_cw_bin_48: 0
[p03]                pcs_corrected_cw_bin_49: 0
[p03]                pcs_corrected_cw_bin_50: 0
[p03]                pcs_corrected_cw_bin_51: 0
[p03]                pcs_corrected_cw_bin_52: 0
[p03]                pcs_corrected_cw_bin_53: 0
[p03]                pcs_corrected_cw_bin_54: 0
[p03]                pcs_corrected_cw_bin_55: 0
[p03]                pcs_corrected_cw_bin_56: 0
[p03]                pcs_corrected_cw_bin_57: 0
[p03]                pcs_corrected_cw_bin_58: 0
[p03]                pcs_corrected_cw_bin_59: 0
```
