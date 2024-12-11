# Build for Cassini hardware by default; build for emulator or netsim by
# defining the 'platform' macro.
# e.g. rpmbuild --define 'platform PLATFORM_CASSINI_SIM' ...
# If 'platform' is not defined, default to PLATFORM_CASSINI_HW
%define platform_arg %{!?platform:PLATFORM_CASSINI_HW}%{?platform:%platform}=1

%{!?dkms_source_tree:%define dkms_source_tree /usr/src}

# Exclude -preempt kernel flavor, this seems to get built alongside the -default
# flavor for stock SLES. It doesn't get used, and its presence can cause issues
# (see NETCASSINI-4032)
%define kmp_args_common -x preempt -p %{_sourcedir}/%name.rpm_preamble

%if 0%{?rhel}
# On RHEL, override the kmod RPM name to include the kernel version it was built
# for; this allows us to package the same driver for multiple kernel versions.
%define kmp_args -n %name-k%%kernel_version %kmp_args_common
%else
%if 0%{?with_shasta_premium:1}
%define kmp_args -x default %kmp_args_common
%else
%define kmp_args %kmp_args_common
%endif
%endif

# Used only for OBS builds; gets overwritten by actual Slingshot product version
%define release_extra 0

Name:           sl-driver
Version:        1.19.8
Release:        %(echo ${BUILD_METADATA})
Summary:        HPE Slingshot Link driver
License:        GPL-2.0
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cray-cassini-headers-user
BuildRequires:  %kernel_module_package_buildreqs

Prefix:         /usr

Obsoletes: link_media
Obsoletes: sslink

# Generate a preamble that gets attached to the kmod RPM(s). Kernel module
# dependencies can be declared here. The 'Obsoletes' and 'Provides' lines for
# RHEL allow the package to be referred to by its base name without having to
# explicitly specify a kernel version.
%(/bin/echo -e "\
%if 0%{?rhel} \n\
Obsoletes:      kmod-%{name} \n\
Provides:       kmod-%{name} = %version-%release \n\
%endif" > %{_sourcedir}/%{name}.rpm_preamble)

%if 0%{with shasta_premium}
# The nvidia-gpu-build-obs package (necessary for building against CUDA
# drivers) causes a bogus default kernel flavor to be added. This causes
# builds to fail, as upstream dependencies (i.e. SBL) are not built for
# default on shasta-premium. Work around this by explicitly excluding the
# default flavor on shasta-premium
%kernel_module_package -x default %kmp_args
%else
%kernel_module_package %kmp_args
%endif

%description
Slingshot Link driver

%package devel
Summary:  Development files for Slingshot Link driver

%description devel
Development files for Slingshot Link driver

%package dkms
Summary:    DKMS package for Slingshot Link driver
BuildArch:  noarch
Requires:   dkms
Requires:   cray-cassini-headers-user
Conflicts:  kmod-%name
Conflicts:  %name-kmp

%description dkms
DKMS support for Slingshot Link driver

%prep
%setup

set -- *
mkdir source
mv "$@" source/
mkdir obj

%build
for flavor in %flavors_to_build; do
    rm -rf obj/$flavor
    mkdir -p obj/$flavor
    cp -r source/* obj/$flavor/
    cp obj/$flavor/common/configs/config.mak.cassini obj/$flavor/config.mak
    pushd obj/$flavor
    make all-cass STAGING_DIR=$PWD/staging-dir KDIR=%{kernel_source $flavor} %platform_arg %{?_smp_mflags}
    popd
done

%install
export INSTALL_MOD_PATH=$RPM_BUILD_ROOT
export INSTALL_MOD_DIR=extra/%{name}

for flavor in %flavors_to_build; do
    pushd obj/$flavor
    make INSTALL_DIR=$RPM_BUILD_ROOT STAGING_DIR=$PWD/staging-dir KDIR=%{kernel_source $flavor} %platform_arg %{?_smp_mflags} install
    popd
    install -D $PWD/obj/$flavor/knl/Module.symvers  $RPM_BUILD_ROOT/%{prefix}/src/sl/$flavor/Module.symvers
done

%if 0%{?rhel}
# Centos/Rocky/RHEL does not exclude the depmod-generated modules.* files from
# the RPM, causing file conflicts when updating
find $RPM_BUILD_ROOT -iname 'modules.*' -exec rm {} \;
%endif

# DKMS bits
dkms_source_dir=%{dkms_source_tree}/%{name}-%{version}-%{release}
mkdir -p %{buildroot}/${dkms_source_dir}
cp -r source/* %{buildroot}/${dkms_source_dir}

rm -rf %{buildroot}/${dkms_source_dir}/common/buildsys/common_exe.mak
rm -rf %{buildroot}/${dkms_source_dir}/common/buildsys/common_lib.mak
rm -rf %{buildroot}/${dkms_source_dir}/common/buildsys/common.mak
rm -rf %{buildroot}/${dkms_source_dir}/common/configs
rm -rf %{buildroot}/${dkms_source_dir}/config.mak
rm -rf %{buildroot}/${dkms_source_dir}/contrib/checkpatch.pl
rm -rf %{buildroot}/${dkms_source_dir}/contrib/const_structs.checkpatch
rm -rf %{buildroot}/${dkms_source_dir}/contrib/install_git_hooks.sh
rm -rf %{buildroot}/${dkms_source_dir}/contrib/spelling.txt
rm -rf %{buildroot}/${dkms_source_dir}/contrib/style.sh
rm -rf %{buildroot}/${dkms_source_dir}/debian/changelog.template
rm -rf %{buildroot}/${dkms_source_dir}/debian/compat
rm -rf %{buildroot}/${dkms_source_dir}/debian/control
rm -rf %{buildroot}/${dkms_source_dir}/debian/copyright
rm -rf %{buildroot}/${dkms_source_dir}/debian/README.Debian
rm -rf %{buildroot}/${dkms_source_dir}/debian/rules
rm -rf %{buildroot}/${dkms_source_dir}/debian/sl-driver-dev.install
rm -rf %{buildroot}/${dkms_source_dir}/debian/sl-driver.install
rm -rf %{buildroot}/${dkms_source_dir}/Jenkinsfile.debian
rm -rf %{buildroot}/${dkms_source_dir}/Jenkinsfile.rpmbuild
rm -rf %{buildroot}/${dkms_source_dir}/Jenkinsfile.rpmbuild.rhel
# TODO: Evaluate whether these files should be shipped with DKMS
#rm -rf %{buildroot}/${dkms_source_dir}/knl/sl_asic.h
rm -rf %{buildroot}/${dkms_source_dir}/knl/test/Kbuild
rm -rf %{buildroot}/${dkms_source_dir}/knl/test/Makefile
rm -rf %{buildroot}/${dkms_source_dir}/knl/test/sl_test_debugfs.c
rm -rf %{buildroot}/${dkms_source_dir}/knl/test/sl_test_debugfs.h
rm -rf %{buildroot}/${dkms_source_dir}/knl/test/sl_test.h
rm -rf %{buildroot}/${dkms_source_dir}/knl/test/sl_test_kconfig.h
rm -rf %{buildroot}/${dkms_source_dir}/knl/test/sl_test_module.c
rm -rf %{buildroot}/${dkms_source_dir}/knl/udev/98-sl.rules
rm -rf %{buildroot}/${dkms_source_dir}/pkgconfig
rm -rf %{buildroot}/${dkms_source_dir}/pkgconfig/sl-driver-dev.pc
rm -rf %{buildroot}/${dkms_source_dir}/rpm_build_multikernel.sh
rm -rf %{buildroot}/${dkms_source_dir}/runBuildPrep.rhel.sh
rm -rf %{buildroot}/${dkms_source_dir}/set_slingshot_version.sh
rm -rf %{buildroot}/${dkms_source_dir}/sl-driver.spec
rm -rf %{buildroot}/${dkms_source_dir}/usr/Makefile
rm -rf %{buildroot}/${dkms_source_dir}/usr/linktool
rm -rf %{buildroot}/${dkms_source_dir}/usr/linktool/linktool_cmds.c
rm -rf %{buildroot}/${dkms_source_dir}/usr/linktool/linktool_sbus.h
rm -rf %{buildroot}/${dkms_source_dir}/usr/linktool/linktool.c
rm -rf %{buildroot}/${dkms_source_dir}/usr/linktool/linktool_sbus.c
rm -rf %{buildroot}/${dkms_source_dir}/usr/linktool/Makefile
rm -rf %{buildroot}/${dkms_source_dir}/usr/linktool/linktool_iface.c
rm -rf %{buildroot}/${dkms_source_dir}/usr/linktool/linktool_pmi.c
rm -rf %{buildroot}/${dkms_source_dir}/usr/linktool/linktool_cmds.h
rm -rf %{buildroot}/${dkms_source_dir}/usr/linktool/linktool_iface.h
rm -rf %{buildroot}/${dkms_source_dir}/usr/linktool/linktool_pmi.h
rm -rf %{buildroot}/${dkms_source_dir}/usr/linktool/linktool_addr.c
rm -rf %{buildroot}/${dkms_source_dir}/usr/linktool/linktool_addr.h
rm -rf %{buildroot}/${dkms_source_dir}/usr/linktool/linktool.h

# QUIRK: copy Cassini make configuration into place
cp source/common/configs/config.mak.cassini %{buildroot}/${dkms_source_dir}/config.mak

sed \
    -e '/^$/d' \
    -e '/^#/d' \
    -e 's/@PACKAGE_NAME@/%{name}/g' \
    -e 's/@PACKAGE_VERSION@/%{version}-%{release}/g' \
    %{buildroot}/${dkms_source_dir}/dkms.conf.in \
    > %{buildroot}/${dkms_source_dir}/dkms.conf

rm %{buildroot}/${dkms_source_dir}/dkms.conf.in

echo "%dir ${dkms_source_dir}" > dkms-files
echo "${dkms_source_dir}" >> dkms-files

%pre dkms

%post dkms
if [ -f /usr/libexec/dkms/common.postinst ] && [ -x /usr/libexec/dkms/common.postinst ]
then
    postinst=/usr/libexec/dkms/common.postinst
elif [ -f /usr/lib/dkms/common.postinst ] && [ -x /usr/lib/dkms/common.postinst ]
then
    postinst=/usr/lib/dkms/common.postinst
else
    echo "ERROR: did not find DKMS common.postinst" >&2
    exit 1
fi

${postinst} %{name} %{version}-%{release}

%preun dkms
# 'dkms remove' may fail in some cases (e.g. if the user has already run 'dkms
# remove'). Allow uninstallation to proceed even if it fails.
/usr/sbin/dkms remove -m %{name} -v %{version}-%{release} --all --rpm_safe_upgrade || true

%files
/lib/firmware/sl_fw_quad_3.04.bin
/lib/firmware/sl_fw_octet_3.08.bin

%files devel
%{_includedir}/linux/sl_kconfig.h
%{_includedir}/linux/sl.h
%{_includedir}/linux/sl_ldev.h
%{_includedir}/linux/sl_lgrp.h
%{_includedir}/linux/sl_link.h
%{_includedir}/linux/sl_llr.h
%{_includedir}/linux/sl_mac.h
%{_includedir}/linux/sl_media.h
%{_includedir}/linux/sl_test.h
%{_includedir}/uapi/sl_ldev.h
%{_includedir}/uapi/sl_lgrp.h
%{_includedir}/uapi/sl_link.h
%{_includedir}/linux/sl_fec.h
%{_includedir}/uapi/sl_llr.h
%{_includedir}/uapi/sl_mac.h
%{_includedir}/uapi/sl_media.h
%{prefix}/src/sl/*/Module.symvers
/usr/bin/sl_sysfs_dump.sh

%files dkms -f dkms-files

%exclude /lib/modules/module.symvers.sl
%exclude /lib/modules/modules.builtin
%exclude /lib/modules/modules.order

%changelog
* Wed Dec 11 2024 Slingshot Platform Team <hpcdev_ss_plat@hpe.com> 1.19.8

%{_git_hash}
