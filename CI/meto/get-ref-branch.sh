#!/bin/bash
#
# (C) Copyright 2024 MetOffice Crown Copyright
#
# This script attempts to checkout a branch from a dependency project with
# the same name as the branch that triggers the GitHub Actions workflow.
#
set -euxo pipefail
repo="$1"  # expect path to a repo clone

branch_name="${GITHUB_HEAD_REF}"
if [[ "${branch_name}" == 'develop' ]]; then
    exit
fi

cd "${repo}"
if git fetch --progress --depth=1 origin \
    "+refs/heads/${branch_name}:refs/remotes/origin/${branch_name}"
then
    git checkout --progress --force -B "${branch_name}" \
        "refs/remotes/origin/${branch_name}"
fi

exit
