mod_auth_hard.la: mod_auth_hard.slo mod_auth_hard_aux.slo
	$(SH_LINK) -rpath $(libexecdir) -module -avoid-version mod_auth_hard.lo mod_auth_hard_aux.lo -lsqlapi
DISTCLEAN_TARGETS = modules.mk
shared =  mod_auth_hard.la
