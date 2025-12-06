# SL Test Framework Documentation

## Table of Contents

1. [Goals](#goals)
1. [Testing Guidelines](#testing-guidelines)
1. [Building](#building)
1. [Installation](#installation)
1. [Testing Guidelines](testing-guidelines)
1. [Getting Ready to Test](#getting-ready-to-test)
1. [Running Tests](#running-tests)
1. [Debugging](#debugging)
1. [Further Reading](#further-reading)
1. [SerDes Testing](#serdes-testing)
1. [FAQS](#faqs)

## Goals

Provide a test framework for sl-driver that ...

1. Calls sl-driver kernel API
1. Is easy to debug.
1. Provides good documentation and help.
1. Sequentially runs all tests.
1. Individually runs tests.

## Testing Guidelines

The best way to write a test is not to. Instead, copy an existing test and
modify it to suit your needs. However, before writing any test, keep in mind
the following guidelines. It is important to keep the sl-test framework as
standard as possible.

1. Every test verifies _one thing and one thing only_.
1. Every test must have a `test_verify` function. This determines the pass/fail result.
1. Every test must have a `test_cleanup` function. This returns the system to a usable
state for the next test.
1. Every test must provide help and a brief description.
1. Tests are sequential. There is a hierarchy and order to the when a test is run.

For more details see [Further Reading](further-reading).

## Building

### NIC

<!--- TODO: -->

### SWITCH

The `sl-driver-test` package is built as part of the step `43.1_build_sl_driver`
in the [hms-scimage](https://github.hpe.com/hpe/hpc-hms_ec-hms-scimage/tree/release/slingshot-2.9) repo.

```sh
./build-all --sequence-dir=tools_r2_arm64.d -S=43.1
```

If you haven't started a build yet you may need to first run.

```sh
./build-all --sequence-dir=tools_r2_arm64.d --end=29
```

The `sl-driver-test` package will then be located in.

`$WORKAREA/artifacts/packages/arm64/sl-driver-test_${VERSION}-0_arm64.deb`

Where `${VERSION}` is the version you are building for. The version listed in
the sl-driver repo under `common/package_version`.

## Installation

The `sl-driver-test` package includes everything needed for testing.

1. Set the test package version and build number appropriately. In this case
I am using version 1.19.3, with build number 0.

### Via Package Manager

> [!note] Future installation method (once package is published.)

1. After logging into a switch. Add the [artifactory](https://arti.hpc.amslabs.hpecorp.net/ui/native/slingshot-internal-rosetta_fw-debian-local/pool/cairngorms/arm64/)
sources for slingshot and install the `sl-driver-test` package.

```sh
echo 'deb [trusted=yes] https://arti.hpc.amslabs.hpecorp.net/artifactory/slingshot-internal-rosetta_fw-debian-local cairngorms main' > /etc/apt/sources.list.d/hpe.list
apt update
apt install sl-driver-test
depmod -a
```

You can view all the available versions of the package Using the following command.

```sh
apt-cache show sl-driver-test | grep Version
```

### Manual Installation

First copy your `sl-driver-test_${VERSION}-${BUILD}_arm64.deb`  package over
to a switch. You can then install the package using the `dpkg` tool.

> [!note] If you aren't sure where to get the package, see [Building Section](#building) above.

```sh
VERSION=1.19.3 # Replace with your version
BUILD=0        # Replace with your build number
dpkg -i ./sl-driver-test_${VERSION}-${BUILD}_arm64.deb
depmod -a
```

## Getting Ready to Test

This section assumes you have already [installed](#installation) the SL test
Framework or are using an image that has the package already
installed. If not please see the [Installation](#installation) section.

The intent of this section is to familiarize yourself with a few basic operations
for testing. This is not an exhaustive explanation of testing and focuses on
the process of bringing a link up, which is one of the most common testing
operations.

### Learning Outcomes

1. Understand [Test Environment Initialization](test-environment-initialization).
1. Understand [Link Group Configuration](link-group-configuration).
1. Understand [Link Up](#link-up).
1. Understand [Link Group Cleanup](#link-group-cleanup).

### Test Environment Initialization

The first step in testing is to source the test environment.

```sh
source /usr/bin/sl_test_scripts/sl_test_env.sh
```

This provides [common environment variables](#environment-variables) and sets
up your path to run tests and bash test functions. One
such function `sl_test_init` is the next step in
initialization.

```sh
sl_test_init
```

This test will prepare the device under test. This involves a few basic steps
depending on the type of device (either switch or NIC).

1. End processes using kernel modules
1. Unload kernel modules
1. Load kernel modules
1. (Switch Only) Initialize device. e.g `swtest init`.

### Link Group Configuration

Once the we have completed [test environment initialization](#test-environment-initialization)
you can continue to configure the link group. This is the first step in
bringing links up.

```sh
sl_test_lgrp_setup ldev_num lgrp_nums settings
```

For further details on this function please see `sl_test_lgrp_setup -h`.
The purpose of this function is the following.

1. Create sl objects; `sl_ldev`, `sl_lgrp`, `sl_link`, `sl_mac`, `sl_llr`.
1. Set configurations for all the objects in 1.
1. Set the policies for all the objects in 1.

#### Link Group Configuration Example

```sh
sl_test_lgrp_setup 0 "8 9 40 41" /usr/bin/sl_test_scripts/systems/settings/bs200_x1_lb_fec_calc.sh
```

### Link Up

Once we have completed both;

1. [Test Environment Initialization](#test-environment-initialization)
1. [Link Group Configuration](link-group-configuration)

We can continue to bring a link up. This will result in a command to the sl-driver
to bring the link up.

```sh
sl_test_link_up ldev_num lgrp_nums link_nums
```

#### Link Up Example

```sh
sl_test_link_up 0 "8 9 40 41" 0
```

### Link Group Cleanup

Cleanup is a common process when running a test. It ensures we are starting
from a clean slate. Cleanup involves the following steps.

1. Removing all outstanding notifications for the link group.
1. Unregistering for link group notifications.
1. Turning off test features e.g software FEC counters.
1. Deleting SL objects; `sl_ldev`, `sl_lgrp`, `sl_link`, `sl_mac`, `sl_llr`.

```sh
sl_test_lgrp_cleanup ldev_num lgrp_nums
```

#### Link Group Cleanup Example

```sh
sl_test_lgrp_cleanup 0 "8 9 40 41"
```

### Full Link Up Example

It is recommended that you begin the section [Test Environment Initialization](#test-environment-initialization)
to further explain the full example provided here.

```sh
lgrps=(8 9 40 41)

sl_test_init
sl_test_lgrp_setup 0 "${lgrps[*]}" /usr/bin/sl_test_scripts/systems/settings/bs200_x1_lb_fec_calc.sh
sl_test_link_up 0 "${lgrps[*]}" 0
sl_test_lgrp_cleanup 0 "${lgrps[*]}"
```

## Running Tests

### Running All Tests

```sh
/usr/bin/sl_test_scripts/sl_run_test.sh
```

#### Source Environment

You may want to the environment made available in your shell session.

1. Source the test environment.

```sh
source /usr/bin/sl_test_scripts/sl_test_env.sh
```

1. Run all the tests.

```sh
# Run all the tests using the default manifest
sl_run_test.sh --all
```

### Running Individual Tests

Individual tests can be run in a few different ways.

1. Run [by test id](#by-test-id)
2. Run [by test name](#by-test-name)
3. Run test script [directly](#directly).

Running a test by the ID or the name provides the advantage of supplying the test
arguments for you. However, you may wish to run a test directly. All tests are
added to the `PATH` variable and can be directly executed.

#### Get List of all Tests

Run all tests defined in the manifest.

- `--all` Select all tests.
- `--brief` Get brief test information (will not execute test).

```sh
sl_run_test.sh --all --brief
```

#### By Test ID

Run test by a unique ID, or by specifying a space delimited list of IDs.

##### Single ID

```sh
# Run the initialization test
sl_run_test.sh --id 0
```

##### Multiple IDs

```sh
# Run the initialization test
sl_run_test.sh --id "0 1"
```

#### By Test Name

Run the test using the test name. This can also be a space delimited list of names.

##### Single Name

```sh
# Run the initialization test
sl_run_test.sh --name "sl_run_init.sh"
```

##### Multiple Names

The test name must only be contained in the file name. So...

1. `sl_run_init` will match `sl_run_init.sh`.
1. `sl_run_link` will match all tests with string such as
`sl_run_link_policy_fec_on` or `sl_run_link_policy_fec_calc`

```sh
# Run the initialization test
sl_run_test.sh --name "sl_run_init.sh sl_run_link_policy_fec_on.sh"
```

#### Directly

Run the test directly by invoking the script and bypassing the `sl_run_test` runner.

```sh
sl_run_init.sh
```

### Running from a Diagnostic Container

The diagnostics container is run as part of continuous testing (CT). The
script `sl_run_test.sh` is executed as part of the container alongside
other tests.

```sh
git clone git@github.hpe.com:hpe/hpc-sshot-scimage-diag-container.git
```

Follow the README.md to build the diagnostic image. Once built tar the image to
send to the device under test (DUT). In this case I am testing rbt28. Replace
rbt28 with the system you are testing.

```sh
DUT="rbt28"
docker save -o diags.tar scimage-diagnostics
rsync -az --progress diags.tar ${DUT}:~
```

On the DUT, load the container you just copied over.

```sh
sudo docker load -i diags.tar
```

Run the `test-runner.py` script.

```sh
DIAGS_IMAGE=scimage-diagnostics:250207174948
sudo docker run --rm -it --network=host -v /root/${USER}:/root/${USER} \
        --env http_proxy="" --env https_proxy="" \
        ${DIAGS_IMAGE} opt/cray/testrunner/bin/test-runner.py \
        --no-reboot --ct-target-arch r2 --sc-hostname x0c0r0b0
```

## Debugging

### Debugging Test Scripts

You can turn on debug with the following command. See [Logging](logging) below
for further details.

```sh
sl_test_log_level_set debug
```

### Driver Debugging

Turn dynamic debug on for a function or list of functions. See help for further details.

```sh
sl_test_dynamic_debug func on sl_ctrl_link_up sl_ctrl_link_down
```

To see all functions that have been turned on

```sh
sl_test_dynamic_debug_on_show
```

## Further Reading

The purpose of this section is to explain how to use the sl-test framework to
write your own tests.

### Introduction

sl-test provides a rich framework for testing the sl-driver. It is written in
bash and backed up by a kernel module allowing for easy access to the sl-driver
kernel API. For example the C function to create a new link group can be called
in bash in the following manner.

#### C

```c
struct sl_lgrp *sl_lgrp_new(struct sl_ldev *ldev, u8 lgrp_num, struct kobject *sysfs_parent)
```

#### Bash

```sh
sl_test_lgrp_new ${ldev_num} ${lgrp_num}
```

> [!NOTE] The `sysfs_parent` is automatically determined for the device you run on.

You could also call this with a list of link groups.

```sh
sl_test_lgrp_new 0 "0 1"
```

First it is necessary to source the testing environment. This is installed as part
of the sl-driver-test package.

```sh
source /usr/bin/sl_test_scripts/sl_test_env.sh
```

### Getting Help

#### List of Functions

If you aren't sure where to start I would recommend getting a list of all the
functions available by either typing `sl_` and tab completing or running
`sl_test_help`

```sh
sl_test_help
```

Output shortened for brevity.

```text
In order to use sl_test you must first run "sl_test_init"

See sl_test_type_help for help with specific types of functions.

Functions:
1. sl_test_nic_exit
2. sl_test_nic_init
3. sl_test_cmd_check
4. sl_test_cmd_help
...
```

#### Function Help

All of the bash functions provide help. For example

```sh
sl_test_link_new -h
```

```text
Usage: sl_test_link_new [-h | --help] ldev_num lgrp_nums link_nums
Create new links for the link groups.

Mandatory:
ldev_num   Link Device Number the lgrp_nums belongs to.
lgrp_nums  Link Group Numbers the link_nums belongs to.
link_nums  Link Numbers to create.

Options:
-h, --help This message.
```

##### Examples

```sh
# Create a new link 0 for link group 0 on link device 0
sl_test_link_new 0 0 0

# Create 4 new links for link groups 0 and 1 on link device 0
sl_test_link_new 0 "0 1" "0 1 2 3"

# Create 4 new links for link groups 0 and 1 on link device 0
lgrps=(0 1)
links=({0..3})
sl_test_link_new 0 "${lgrps[*]}" "${links[*]}"
```

### Types of Test Functions

Test functions are divided into a few different categories (types) based on
the different objects that created within the sl-driver.

| Type | Definition |
|--- |---|
| ldev | Link device functions (currently are noops). |
| lgrp | Link group functions tests like creating, deleting, setting configurations/policies for the link group. |
| link | Link functions like creating, deleting, setting configurations/policies for the link. |
| llr | Link layer retry (LLR) functions like creating, deleting, starting and stopping the LLR.  |
| mac | Media Access Control (MAC) functions like creating, deleting, starting and stopping the MAC. |
| serdes | Serial Deserializer (SerDes) functions like configuring the SerDes settings |

You can get a help page listing all available functions for these different categories.

```sh
sl_test_type_help lgrp | less
```

### Logging

The test framework provides different log levels for severity. By default the
log level is set to info. This includes all error and warning logs. When tests
are run with the runner (`sl_run_test`) logs are printed both to stdout and
sent to the log

| Log Level | Definition                                                                                |
|-----------|-------------------------------------------------------------------------------------------|
| error     | Fatal test error resulting in a test failure.                                             |
| warn      | Unwanted behavior, that doens't necessarily result in test failure.                       |
| info      | Execution step a test is running. Any info needed for test log output other than failure. |
| debug     | Developer messages to assist in debugging test behavior.                                  |

### Adding Tests to the Manifest

The manifest file located in `${SL_TEST_DIR}/systems/manifests/` provides a
list of tests to run. Each test is defined by the following JSON snippet

```json
{
        "id": <num>,
        "file": <script_name.sh>,
        "parameters": {
                "arguments": ""
        }
}
```

| Name | Value | Type |
|--- |--- |--- |
| id | Unique test identification | integer |
| file | Name of test file including extension | string |
| parameters | Test parameters | json object |
| arguments | Arguments to pass to the test file | string |

### Environment Variables

The following environment variables can be sourced from the `sl_test_env.sh` file.
It is useful to use these environment variables to access common directories
and files.

```sh
source /usr/bin/sl_test_scripts/sl_test_env.sh
```

> [!note] Not all environment variables are listed. For a full list see the
> `sl_test_env.sh` file above.

| Name | Definition |
|--- |--- |
| SL_TEST_DIR | Directory for the SL test framework. |
| SL_TEST_LIBS_DIR | Directory for the SL test framework library. |
| SL_TEST_CONFIGS_DIR | Directory for the SL test configurations. |
| SL_TEST_POLICY_DIR | Directory for the SL test policies. |
| SL_TEST_SERDES_SETTINGS_DIR | Directory for the SL test SerDes Settings. |
| SL_TEST_SYSTEMS_SETTINGS_DIR | Directory for the SL test system settings. |
| SL_TEST_LOG_DIR | Directory logs are stored after running `sl_run_test`. |
| SL_TEST_LGRP_DEBUGFS_NOTIFS | File to read notifications registered for. |
| SL_TEST_DEVICE_TYPE | Type of device tests are running on. |
| SL_TEST_DEFAULT_MANIFEST | Default manifest file. |

### Internal Design

#### Debugfs

All interactions with the sl module take place through the debugfs interface
provided by sl_test module. Think of this as the translation layer from user
space to kernel space. Since debugfs is a filesystem, each of the parameters
passed are represented by a file. For example the kernel API `sl_link_up`

##### Kernel API `sl_link_up`

Here the call to `sl_link_up` takes the parameter `struct sl_link` containing
the `ldev_num`, `lgrp_num` and link `num`. Note, you can ignore
`magic`, `ver` and `size` for now.

```c
struct sl_link {
        u32 magic;
        u32 ver;
        u32 size;

        u8  num;
        u8  lgrp_num;
        u8  ldev_num;
};

struct sl_lgrp *sl_link_up(struct sl_link *link)
```

##### Test API `sl_test_link_up`

Here the test framework explicitly includes the parameters passed to the
`sl_link_up` function. All other members of `struct sl_link` are automatically
populated.

```sh
sl_test_link_up ${ldev_num} ${lgrp_num} ${link_num}
```

Ultimately these parameters are written into debugfs to signify which link
to command up. The above function could be more simply be written as

```sh
echo ${ldev_num} > /sys/kernel/debug/sl/ldev/num
echo ${lgrp_num} > /sys/kernel/debug/sl/lgrp/num
echo ${link_num} > /sys/kernel/debug/sl/link/num
echo "up" > /sys/kernel/debug/sl/link/cmd
```

The file `/sys/kernel/debug/sl/link/cmd` matches the string "up" and calls
`sl_link_up` using `ldev_num`, `lgrp_num` and `link_num` written to debugfs
in the previous steps.

##### Notifications

Notifications from the sl module can be captured in the test framework. After
registering for a link group with `sl_test_lgrp_notif_reg` notifications can
be read using the tool `sl_test_lgrp_notif_read`. It may also be useful to
wait for a notification from a link `sl_test_lgrp_links_notif_wait`.

> `sl_test_lgrp_notif_read` is event driven and uses `POLLIN` to signify
> reception of a notification. See `sl_test_lgrp_notif_read --help` for
> further details.

Notifications are held in a FIFO in the sl_test module. Each link group can hold
a total of `SL_TEST_LGRP_NOTIF_NUM_ENTRIES` notifications. This allows you to
register for notifications for multiple links at once.

It is important to remember to unregister from notifications using
`sl_test_lgrp_notif_unreg`.

## SerDes Testing

This section covers using sl-test to change the SerDes settings for a link.
We will cover;

1. [SerDes Settings File](#serdes-settings-file)
1. [Set SerDes Settings for a Link](#set-serdes-settings-for-a-link)
1. [Set SerDes Settings Full Example](#set-serdes-settings-full-example)

If you haven't already, you should read the section on [Getting Ready to Test](#getting-ready-to-test).
This section will cover how to bring a link up which is a requirement for
testing SerDes settings.

### SerDes Settings File

Below is an example of the default SerDes settings file. Taken from a link on oat-cf2.

```txt
# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Hewlett Packard Enterprise Development LP. All rights reserved.
#

clocking     = 85/170
cursor       = 100
dfe          = enabled
encoding     = PAM4_normal
media        = headshell
osr          = OSX2
post1        = 0
post2        = 0
pre1         = 0
pre2         = 0
pre3         = 0
scramble_dis = not_set
width        = 80
```

The options available for non-numeric values can be found in the help for
`sl_test_serdes_settings_set`. i.e `sl_test_serdes_settings_set -h`.

#### Example sl_test_serdes_settings_set Help Output

```txt
root@x0c0r4b0:~# sl_test_serdes_settings_set -h
Usage: sl_test_serdes_settings_set [-h | --help] ldev_num lgrp_nums link_nums settings
Set SerDes parameters for the SerDes in link groups.

Mandatory:
ldev_num  Link device number the lgrp_nums belongs to.
lgrp_nums Link group number the link_nums belongs to.
link_nums Link Numbers to set serdes settings for.
settings  Serdes parameters file. See /usr/bin/sl_test_scripts/settings/serdes/

Options:
-h, --help  This message.

Settings Options:
clocking     = 82.5/165 | 85/170
cursor       = -32,768 < cursor < 32,767
dfe          = enabled | disabled
encoding     = NRZ | PAM4_normal | PAM4_extended
media        = headshell | backplane | electrical | optical | passive | active | analog | digital | AOC | PEC | AEC | BKP
osr          = OSX1 | OSX2 | OSX4 | OSX42P5
post1        = -32,768 < post1 < 32,767
post2        = -32,768 < post2 < 32,767
pre1         = -32,768 < pre1 < 32,767
pre2         = -32,768 < pre2 < 32,767
pre3         = -32,768 < pre3 < 32,767
scramble_dis = set | not_set
width        = 40 | 80 | 160

Settings File:
/usr/bin/sl_test_scripts/settings/serdes/default.config
```

### Set SerDes Settings for a Link

SerDes settings are made per link, and later used in the SerDes lanes.
Settings can only be made on a link that is down (See command `sl_test_link_down`)

```sh
sl_test_serdes_settings_set ${ldev_num} ${lgrp_nums} ${link_nums} ${settings}
```

#### Example Set SerDes Settings

The following example will use the default SerDes settings for link 0 in link
group 22 for link device 0.

```sh
sl_test_serdes_settings_set 0 22 0 /usr/bin/sl_test_scripts/settings/serdes/default.sh
```

You can then print out the SerDes settings being used per lane. In our
example we print out lane 0.

```sh
lgrp=22
lane=0
for f in /sys/class/rossw/rossw0/pgrp/${lgrp}/serdes/lane/${lane}/settings/*; do echo "$(basename $f): $(cat $f)"; done
```

```txt
clocking: 85/170
cursor: 168
dfe: enabled
encoding: PAM4_normal
link_training: disabled
media: headshell
osr: OSX1
post1: 0
post2: 0
pre1: 0
pre2: 0
pre3: 0
scramble_dis: not_set
width: 160
```

##### Debugfs Settings

If you need to see the configuration held in debugfs you can do so with the
following command.

```sh
for f in $(find /sys/kernel/debug/sl/serdes/settings/ -type f -not -name "*_options"); do echo "$(basename $f): $(cat $f)"; done
```

```txt
scramble_dis: not_set
dfe: enabled
media: headshell
width: 160
osr: OSX1
clocking: 85/170
encoding: PAM4_normal
post2: 0
post1: 0
cursor: 168
pre3: 0
pre2: 0
pre1: 0
```

## Set SerDes Settings Full Example

The following example was run on oat-cf2 where link groups 8,9,40,41 are connected
together. The links will be brought up with the default SerDes config and if the
link is up after 10 seconds the SerDes settings will be printed.

```sh
source /usr/bin/sl_test_scripts/sl_test_env.sh
lgrp_nums=(8 9 40 41)

sl_test_init
sl_test_lgrp_setup 0 "${lgrp_nums[*]}" /usr/bin/sl_test_scripts/systems//settings/bs200_x1_lb_fec_calc.sh
sl_test_serdes_settings_set 0 "${lgrp_nums[*]}" 0 /usr/bin/sl_test_scripts/settings/serdes/default.config
sl_test_link_up 0 "${lgrp_nums[*]}" 0

sleep 10

for lgrp_num in ${lgrp_nums[@]}; do
        state=$(cat /sys/class/rossw/rossw0/pgrp/${lgrp_num}/test_port/0/link/state)
        if [[ "${state}" == "up" ]]; then
                echo "SerDes Settings for ${lgrp_num}"
                for f in /sys/class/rossw/rossw0/pgrp/${lgrp_num}/serdes/lane/0/settings/*; do
                        echo "$(basename $f): $(cat $f)";
                done
        fi
done
```

```txt
SerDes Settings for 8
clocking: 85/170
cursor: 100
dfe: enabled
encoding: PAM4_normal
link_training: disabled
media: headshell
osr: OSX2
post1: 0
post2: 0
pre1: 0
pre2: 0
pre3: 0
scramble_dis: not_set
width: 80
...
```

## FAQS

1. Why is my test failing from the error `sysfs_parent_set failed`.

Example failure.

```txt
2025-01-24 17:44:40.76249 (info)     sl_run_test.sh                                  : (Total Available Tests = 8)
2025-01-24 17:44:40.77367 (info)     sl_run_test.sh                                  : (Manifest = /usr/bin/sl_test_scripts/systems/manifests/quick.json)
2025-01-24 17:44:40.78695 (info)     sl_run_test.sh                                  : (Log File = .test_default.log)
2025-01-24 17:44:41.08397 (info)     exec_test                                       : running (test_id = 1, test_file = sl_run_link_policy_fec_on.sh)
2025-01-24 17:44:41.20747 (info)     sl_run_link_policy_fec_on.sh                    : Starting
2025-01-24 17:44:41.22029 (info)     main                                            : (settings = /usr/bin/sl_test_scripts/systems/settings/bs200_x1_fec_on_il.sh)
2025-01-24 17:44:41.23474 (error)    __sl_test_ldev_sysfs_parent_set                 : sysfs_parent_set failed
2025-01-24 17:44:41.25480 (error)    __sl_test_lgrp_sysfs_parent_set                 : ldev_sysfs_parent_set failed [1]
2025-01-24 17:44:41.27760 (error)    main                                            : lgrp_sysfs_parent_set failed [1]
2025-01-24 17:44:41.30002 (error)    sl_run_link_policy_fec_on.sh                    : failed [1]
2025-01-24 17:44:41.32017 (info)     sl_run_link_policy_fec_on.sh                    : exit (main_rtn = 1)
2025-01-24 17:44:41.33366 (info)     exec_test                                       : FAILED (rtn = 1)
2025-01-24 17:44:41.35547 (error)    exec_test_by_ids                                : exec_test failed [1]
2025-01-24 17:44:41.37917 (info)     sl_run_test.sh                                  : (passed=0, failed=1)
2025-01-24 17:44:41.38972 (info)     sl_run_test.sh                                  : exit (rtn = 1)
```

Likely the test is failing because you haven't initialized the test framework yet.
Before any testing, you must load the `sl_test` module.

- Run the initialization test.

```sh
sl_run_test.sh -i 0
```

- Run the initialization function.

```sh
sl_test_init
```

1. Why is my test failing from the error
`/sys/kernel/debug/sl/ldev//num: No such file or directory`

Example

```txt
2025-01-24 17:49:45.43747 (info)     sl_run_test.sh                                  : (Total Available Tests = 8)
2025-01-24 17:49:45.45500 (info)     sl_run_test.sh                                  : (Manifest = /usr/bin/sl_test_scripts/systems/manifests/quick.json)
2025-01-24 17:49:45.47169 (info)     sl_run_test.sh                                  : (Log File = .test_default.log)
2025-01-24 17:49:45.78507 (info)     exec_test                                       : running (test_id = 1, test_file = sl_run_link_policy_fec_on.sh)
2025-01-24 17:49:45.92045 (info)     sl_run_link_policy_fec_on.sh                    : Starting
2025-01-24 17:49:45.93007 (info)     main                                            : (settings = /usr/bin/sl_test_scripts/systems/settings/bs200_x1_fec_on_il.sh)
2025-01-24 17:49:45.94157 (info)     main                                            : lgrp_setup (ldev_num = 0, lgrp_num = 0, settings = /usr/bin/sl_test_scripts/systems/settings/bs200_x1_fec_on_il.sh)
/usr/bin/sl_test_scripts/libs//sl_test_lgrp.sh: line 73: /sys/kernel/debug/sl/ldev//num: No such file or directory
2025-01-24 17:49:46.00382 (error)    __sl_test_lgrp_cmd                              : ldev_set failed [1]
2025-01-24 17:49:46.02544 (error)    sl_test_lgrp_new                                : lgrp_new failed [1]
2025-01-24 17:49:46.04653 (error)    sl_test_lgrp_setup                              : lgrp_new failed [1]
2025-01-24 17:49:46.06932 (error)    main                                            : lgrp_setup failed [1]
2025-01-24 17:49:46.09427 (error)    sl_run_link_policy_fec_on.sh                    : failed [1]
2025-01-24 17:49:46.11371 (info)     sl_run_link_policy_fec_on.sh                    : exit (main_rtn = 1)
2025-01-24 17:49:46.13254 (info)     exec_test                                       : FAILED (rtn = 1)
2025-01-24 17:49:46.15341 (error)    exec_test_by_ids                                : exec_test failed [1]
2025-01-24 17:49:46.17699 (info)     sl_run_test.sh                                  : (passed=0, failed=1)
2025-01-24 17:49:46.19616 (info)     sl_run_test.sh                                  : exit (rtn = 1)
```

Likely the test is failing because you haven't initialized the test framework yet.
Before any testing, you must load the `sl_test` module.

- Run the initialization test.

```sh
sl_run_test.sh -i 0
```

- Run the initialization function.

```sh
sl_test_init
```
