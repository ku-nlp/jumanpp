#!/usr/bin/env bash

set -x

if [[ -z ${VERSION+x} ]]; then
    echo "VERSION variable must be defined"
    exit 1
fi

if [[ ! -f "${MODEL}" ]]; then
    echo "MODEL variable must be defined and point to file"
    exit 1
fi

OUT_ROOT=${OUT_ROOT:-}

OUT_DIR="${OUT_ROOT}/release-stage-dir/jumanpp-${VERSION}"

mkdir -p "${OUT_DIR}"

cp -r cmake docs libs src test CMakeLists.txt \
    CONTRIBUTORS README.md Dockerfile \
    "${OUT_DIR}"

mkdir -p "${OUT_DIR}/script"
cp script/*.sh "${OUT_DIR}/script"

echo "set(PROJECT_VERSION ${VERSION})" > "${OUT_DIR}/version.cmake"

cp -r model-template "${OUT_DIR}/model"

cp "${MODEL}" "${OUT_DIR}/model/jumandic.jppmdl"
