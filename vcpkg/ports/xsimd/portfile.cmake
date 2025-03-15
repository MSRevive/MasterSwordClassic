vcpkg_download_distfile(
    WIN_PATCHES
    URLS "https://github.com/xtensor-stack/xsimd/pull/1040/commits/e8cb862e434eb1e367afb83e1a3685bccff3e566.diff?full_index=1"
    FILENAME "xsimd-e8cb862e434eb1e367afb83e1a3685bccff3e566.patch"
    SHA512 e584033fb79c602a19222c177d5db28f9887dd17e741844d57f2236a5749ac4c02cc0740f8011ca990602887a6ee3dd21ae0b695455c447686b1a6c8bda2e092
)
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO xtensor-stack/xsimd
    REF "${VERSION}"
    SHA512 cdc42ddad3353297cf25ea2b6b3f09967f5f388efc26241f2997979fdbbac072819ff771145bc5bfa86cb326cca84b4119e8e6e3f658407961cf203a40603a7f
    HEAD_REF master
    PATCHES
        "${WIN_PATCHES}"
)

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES
        xcomplex ENABLE_XTL_COMPLEX
)

set(VCPKG_BUILD_TYPE release) # header-only port

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DBUILD_TESTS=OFF
        ${FEATURE_OPTIONS}
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/${PORT})

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/lib")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
