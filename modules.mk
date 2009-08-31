mod_auth_hard.la: mod_auth.slo mod_auth_aux.slo
	$(SH_LINK) -rpath $(libexecdir) -module -avoid-version  mod_auth.lo mod_auth_aux.lo -lsqlapi
DISTCLEAN_TARGETS = modules.mk
shared =  mod_auth_hard.la
