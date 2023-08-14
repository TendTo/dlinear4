#!/usr/bin/env bash
# This script generates a version header file for dlinear.
# It is called from the Bazel build system, and uses some information
# about the workspace provided by Bazel in the file bazel-out/stable-status.txt.
# Usage: ./tools/generate_version_header.sh [VERSION]

VERSION=${1:-unknown.unknown.unknown} # read version from command line argument or use default 'unknown.unknown.unknown'
IFS='.' read -r -a VERSION_ARRAY <<< "$VERSION" # split string into an array on '.' delimiter
MAJOR=${VERSION_ARRAY[0]} # get major version
MINOR=${VERSION_ARRAY[1]} # get minor version
REVISION=${VERSION_ARRAY[2]} # get revision version
REPOSITORY_STATUS="$(grep '^STABLE_REPOSITORY_STATUS ' bazel-out/stable-status.txt  | cut -d' ' -f2-)" # get repository status from bazel-out/stable-status.txt

# print version header
cat <<EOF
#ifndef DLINEAR_VERSION_H_
#define DLINEAR_VERSION_H_

#define DLINEAR_PROGRAM_NAME    "dlinear"
#define DLINEAR_VERSION_STRING  "${VERSION}"
#define DLINEAR_VERSION_FULL     ${VERSION}
#define DLINEAR_VERSION_MAJOR    ${MAJOR}
#define DLINEAR_VERSION_MINOR    ${MINOR}
#define DLINEAR_VERSION_REVISION ${REVISION}
#define DLINEAR_VERSION_REPOSTAT "${REPOSITORY_STATUS}"

#endif  // DLINEAR_VERSION_H_
EOF
