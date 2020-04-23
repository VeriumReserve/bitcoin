package=minizip
$(package)_version=1.1
$(package)_download_path=https://codeload.github.com/nmoinvaz/minizip/tar.gz
$(package)_file_name=1.1
$(package)_sha256_hash=78d87c463dd6bd446292fe0d3ee72878355b3b7fa52add5722671dfe9290f262
$(package)_dependencies=zlib

define $(package)_set_vars
$(package)_build_opts= CC="$($(package)_cc)"
$(package)_build_opts+=CFLAGS="$($(package)_cflags) $($(package)_cppflags) -fPIC"
$(package)_build_opts+=RANLIB="$($(package)_ranlib)"
$(package)_build_opts+=AR="$($(package)_ar)"
$(package)_build_opts_darwin+=AR="$($(package)_libtool)"
$(package)_build_opts_darwin+=ARFLAGS="-o"
endef

define $(package)_config_cmds
  autoreconf -i && \
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef
