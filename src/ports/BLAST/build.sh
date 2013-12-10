#!/bin/bash
source pkg_info
source ../../build_tools/common.sh


ConfigureStep() {
  #hack
  pwd
  cp ../../../build_tools/config.sub ./c++/src/build-system/config.sub   
    
  local EXTRA_CONFIGURE_OPTS=("${@:-}")
  Banner "Configuring ${PACKAGE_NAME}"
  # export the nacl tools
  export CC=${NACLCC}
  export CXX=${NACLCXX}
  export AR=${NACLAR}
  export RANLIB=${NACLRANLIB}
  export READELF=${NACLREADELF}
  export PKG_CONFIG_PATH=${NACLPORTS_LIBDIR}/pkgconfig
  export PKG_CONFIG_LIBDIR=${NACLPORTS_LIBDIR}
  export FREETYPE_CONFIG=${NACLPORTS_PREFIX_BIN}/freetype-config
  export CFLAGS=${NACLPORTS_CFLAGS}
  export CXXFLAGS=${NACLPORTS_CXXFLAGS}
  export LDFLAGS=${NACLPORTS_LDFLAGS}
  export PATH=${NACL_BIN_PATH}:${PATH};
  local SRC_DIR=${NACL_PACKAGES_REPOSITORY}/${PACKAGE_DIR}
  local CONFIGURE=${NACL_CONFIGURE_PATH:-${SRC_DIR}/c++/configure}
  if [ ! -f "${CONFIGURE}" ]; then
    echo "No configure script found at ${CONFIGURE}"
    return
  fi
  local DEFAULT_BUILD_DIR=${SRC_DIR}/${NACL_BUILD_SUBDIR}
  NACL_BUILD_DIR=${NACL_BUILD_DIR:-${DEFAULT_BUILD_DIR}}
  MakeDir ${NACL_BUILD_DIR}
  ChangeDir ${NACL_BUILD_DIR}

  local conf_host=${NACL_CROSS_PREFIX}
  if [ "${NACL_ARCH}" = "pnacl" -o "${NACL_ARCH}" = "emscripten" ]; then
    # The PNaCl tools use "pnacl-" as the prefix, but config.sub
    # does not know about "pnacl".  It only knows about "le32-nacl".
    # Unfortunately, most of the config.subs here are so old that
    # it doesn't know about that "le32" either.  So we just say "nacl".
    conf_host="nacl"
  fi

  if [ -n "${CONFIGURE_SENTINEL:-}" -a -f "${CONFIGURE_SENTINEL:-}" ]; then
    return
  fi

  LogExecute ${CONFIGURE} \
    --host=${conf_host} \
    --prefix=${NACLPORTS_PREFIX} \
    --exec-prefix=${NACLPORTS_PREFIX} \
    --libdir=${NACLPORTS_LIBDIR} \
    "${EXTRA_CONFIGURE_OPTS[@]}" ${EXTRA_CONFIGURE_ARGS:-}
}



BuildStep() {
  Banner "Build ${PACKAGE_NAME}"
  echo "Directory: $(pwd)"
  # Build ${MAKE_TARGETS} or default target if it is not defined
  cd ../c++/GCC443-Debug/build
  MAKE_TARGETS="all_r"
  if [ -n "${MAKEFLAGS:-}" ]; then
    echo "MAKEFLAGS=${MAKEFLAGS}"
    export MAKEFLAGS
  fi
  GLIBC_COMPAT_INCLUDE_DIR="../../../../glibc-compat-0.1/include"
  export PATH=${NACL_BIN_PATH}:${PATH};
  LogExecute make -j${OS_JOBS} -I${GLIBC_COMPAT_INCLUDE_DIR}/syslog.h  ${MAKE_TARGETS:-}
}





PackageInstall
exit 0
