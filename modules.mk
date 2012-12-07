mod_go.la: mod_go.slo
	$(SH_LINK) -rpath $(libexecdir) -module -avoid-version  mod_go.lo
DISTCLEAN_TARGETS = modules.mk
shared =  mod_go.la
