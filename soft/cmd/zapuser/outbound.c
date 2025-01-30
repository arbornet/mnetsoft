#include "zapuser.h"
#include <sys/stat.h>
#include <unistd.h>

char *outbound_file = NULL;

/*
 * REAP_OUTBOUND
 *
 * The /usr/local/etc/exim.outbound file contains a list of users allowed to
 * send outbound mail, with each line looking like:
 *
 * <username>@cyberspace.org
 *
 * This function deletes all users on the zap list from that file.
 */

void
reap_outbound()
{
	char *tmp_file;
	FILE *ofp, *tfp;
	char bf[BFSZ], *at;
	struct stat st;
	size_t len;

	/* If no outbound file has been configured, do nothing */
	if (outbound_file == NULL)
		return;

	/* Open the file */
	if ((ofp = fopen(outbound_file, "r")) == NULL) {
		error("cannot open outbound file: %s", outbound_file);
		return;
	}
	/* Create a temporary file */
	len = strlen(outbound_file) + 5;
	tmp_file = malloc(len);
	snprintf(tmp_file, len, "%s.tmp", outbound_file);
	if ((tfp = fopen(tmp_file, "w")) == NULL) {
		error("cannot create temporary outbound file: %s", tmp_file);
		return;
	}
	/*
	 * Set permissions on temporary file the same as original file This
	 * is vulnerable to races, but who cares?
	 */
	if (fstat(fileno(ofp), &st)) {
		error("cannot stat outbound file: %s", tmp_file);
		return;
	}
	fchmod(fileno(tfp), st.st_mode);
	fchown(fileno(tfp), st.st_uid, st.st_gid);

	/* Read through the old file, writing undeleted users to the new file */
	while (fgets(bf, BFSZ, ofp) != NULL) {
		if ((at = strchr(bf, '@')) != NULL) {
			*at = '\0';
			if (in_user_list(bf) >= 0)
				continue;
			*at = '@';
		}
		fputs(bf, tfp);
	}
	fclose(tfp);
	fclose(ofp);

	/* Remove old file, and mv new file to old file */
	if (unlink(outbound_file)) {
		error("cannot unlink old outbound file: %s", outbound_file);
		return;
	}
	if (link(tmp_file, outbound_file)) {
		error("cannot link new outbound file %s to %s", tmp_file, outbound_file);
		return;
	}
	unlink(tmp_file);
}
