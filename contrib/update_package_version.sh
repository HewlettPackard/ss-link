#!/bin/bash

set -e

exec < /dev/tty

SCRIPT_PATH=$(dirname $0)

export EMAIL="hpcdev_ss_plat@hpe.com"
export NAME="Slingshot Platform Team"

PACKAGE_VERSION_FILE="${SCRIPT_PATH}/../common/package_version"
DEB_CHANGELOG="${SCRIPT_PATH}/../debian/changelog.template"
RPM_SPEC_FILE="${SCRIPT_PATH}/../sl-driver.spec"

source ${PACKAGE_VERSION_FILE}

M=${MAJOR}
m=${MINOR}
p=${PATCH}

echo "Current Package Version: (${M}.${m}.${p})"
echo "Version Format: (Major.Minor.Patch)"
echo "M - Increment Major version."
echo "m - Increment Minor version."
echo "p - Increment Patch version."
echo "i - Ignore version update. Continue without updating package version."
echo "n - No Version increment but write package version directly."

new_version=$(mktemp)
cat ${PACKAGE_VERSION_FILE} > ${new_version}

while read -p "Update Package Version [M,m,p,n,i]? " user_input; do

        if [ "${user_input}" == "M" ]; then
                M=$((${MAJOR} + 1))
                m=0
                p=0
                echo "Major: (${MAJOR} -> ${M})"
                break
        elif [ "${user_input}" == "m" ]; then
                m=$((${MINOR} + 1))
                p=0
                echo "Minor: (${MINOR} -> ${m})"
                break
        elif [ "${user_input}" == "p" ]; then
                p=$((${PATCH} + 1))
                echo "Patch: (${PATCH} -> ${p})"
                break
        elif [ "${user_input}" == "n" ]; then
                echo "Write Current Package Version: (${M}.${m}.${p})"
                break
        elif [ "${user_input}" == "i" ]; then
                echo "Ignore Package Version Update"
                exit 0
        else
                continue
        fi
done

echo -e "MAJOR=${M}\nMINOR=${m}\nPATCH=${p}" > ${new_version}

##########################################################################################
# Update DEB Package
##########################################################################################
new_changelog=$(mktemp)

cat << EOF > ${new_changelog}
sl-driver (${M}.${m}.${p}-1) unstable; urgency=medium
    _GIT_HASH_
 -- ${NAME} <${EMAIL}>  $(date +"%a, %d %b %Y %T %z")
EOF


##########################################################################################
# Update RPM Package
##########################################################################################
new_specfile=$(mktemp)

CHANGELOG="* $(date +"%a %b %d %Y") ${NAME} <${EMAIL}> ${M}.${m}.${p}\n\n%{_git_hash}"

# The last 3 lines are the changelog entry and are removed for the new entry
head -n -3 ${RPM_SPEC_FILE} > ${new_specfile}
echo -e "${CHANGELOG}" >> ${new_specfile}
sed -i "s/^Version:\s*[0-9]\+.[0-9]\+.[0-9]\+/Version:        ${M}.${m}.${p}/g" ${new_specfile}

while read -p "New Version: (${M}.${m}.${p}) Add changes (y/n)? " user_input; do

        if [ "${user_input,,}" == "y" ]; then
                break
        elif [ "${user_input,,}" == "n" ]; then
                exit 0
        else
                continue
        fi
done

mv ${new_version} ${PACKAGE_VERSION_FILE}
mv ${new_changelog} ${DEB_CHANGELOG}
mv ${new_specfile} ${RPM_SPEC_FILE}

git add ${DEB_CHANGELOG}
git add ${RPM_SPEC_FILE}
git add ${PACKAGE_VERSION_FILE}

echo "Version update complete."
