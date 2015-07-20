#Copyright (c) 2013 Comcast. All rights reserved.

SUBDIRS = src src/main src/services test
DIST_SUBDIRS =src src/main src/services test

## IPK Generation Support
networkmanager_IPK_OUTDIR = $(abs_top_builddir)/ipk
networkmanager_IPK_STAGING_DIR=$(abs_top_builddir)/staging_dir
networkmanager_IPK_TMP_DIR = $(networkmanager_IPK_STAGING_DIR)/ipk
networkmanager_CONTROL_FILES = control postinst prerm
networkmanager_IPK_FILES  = debian-binary control.tar.gz data.tar.gz
networkmanager_IPK_OUTPUT = networkmanager.ipk


package_ipk:
        @echo "Generating IPK Package"
        ## Install files
        $(MAKE) prefix=$(networkmanager_IPK_STAGING_DIR) install
        ## Create temp paths
        $(mkdir_p) $(networkmanager_IPK_TMP_DIR)/usr/bin
        $(mkdir_p) $(networkmanager_IPK_TMP_DIR)/etc
        $(mkdir_p) $(networkmanager_IPK_TMP_DIR)/lib/systemd/system
        ## Install files to temp path
        install -m 0755 $(networkmanager_IPK_STAGING_DIR)/bin/networkmanager $(networkmanager_IPK_TMP_DIR)/usr/bin/
        install -m 0644 $(abs_top_builddir)/conf/networkmanager.conf $(networkmanager_IPK_TMP_DIR)/etc/
        install -m 0644 $(abs_top_builddir)/networkmanager.service $(networkmanager_IPK_TMP_DIR)/lib/systemd/system
        ## Create package files
        tar -czvf $(networkmanager_IPK_OUTDIR)/data.tar.gz -C $(networkmanager_IPK_TMP_DIR) usr etc lib
        tar -czvf $(networkmanager_IPK_OUTDIR)/control.tar.gz -C $(networkmanager_IPK_OUTDIR) $(networkmanager_CONTROL_FILES)
        pushd $(networkmanager_IPK_OUTDIR) && ar cr $(networkmanager_IPK_OUTPUT) $(networkmanager_IPK_FILES); popd
        rm -rf $(networkmanager_IPK_TMP_DIR)
        rm -f $(networkmanager_IPK_OUTDIR)/*.tar.gz
clean_package_ipk:
        rm -rf $(networkmanager_IPK_STAGING_DIR)
        rm -f $(networkmanager_IPK_OUTDIR)/*.tar.gz
        rm -f $(networkmanager_IPK_OUTDIR)/*.ipk

