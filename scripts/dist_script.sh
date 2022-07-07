#!/usr/bin/env bash
set -euo pipefail

MESON_SOURCE_ROOT=$1

if test $MESON_SOURCE_ROOT = ""; then
	echo "Source root missing" >&2
	exit 1
fi

# strip irrelevant README sections if exist
sed -i'' -e '/## Status/,+2d' ${MESON_DIST_ROOT}/README.md
sed -i'' -e '/By default, Meson is downloading Dolby project/,+6d' ${MESON_DIST_ROOT}/README.md

SUBPROJECT_ACTION=$2
SUBPROJECTS=${MESON_SOURCE_ROOT}/subprojects

actions=("none" "export" "strip")
if [[ ! " ${actions[@]} ~= ${SUBPROJECT_ACTION} " ]]; then
	echo "Invalid subproject action" >&2
	exit 1
fi

# remove CI files
rm -f ${MESON_DIST_ROOT}/.gitlab-ci.yml

if [ ! -d ${SUBPROJECTS} ] || test ${SUBPROJECT_ACTION} = "none"; then
	echo "Nothing to do"
	exit 0
fi

if test ${SUBPROJECT_ACTION} = "strip"; then
	echo "Removing subprojects from dist package"
    rm -rf ${MESON_DIST_ROOT}/subprojects
	exit 0
fi

# export: remove wrappers
rm -f ${MESON_DIST_ROOT}/subprojects/*.wrap
