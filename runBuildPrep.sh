#!/bin/bash

# Add OS-appropriate repos for build dependencies


if [[ -v SHS_NEW_BUILD_SYSTEM ]]; then  # new build system
  . ${CE_INCLUDE_PATH}/load.sh
  
  target_branch=$(get_effective_target)
  quality_stream=$(get_artifactory_quality_label)

  add_repository "${ARTI_URL}/${PRODUCT}-rpm-${quality_stream}-local/${target_branch}/${TARGET_OS}" "${PRODUCT}-${quality_stream}"

  generate_local_rpmmacros
  install_dependencies "sl-driver.spec"
else  # old build system  
  :  # nothing
fi
