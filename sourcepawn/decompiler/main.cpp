#include <stdio.h>
#include <stdlib.h>
#include "decompiler.h"

int main()
{
	int err;
	sp_decomp_t *dc;

	const char *file = "../../plugins/test.smx";

	if ((dc = Sp_InitDecomp(file, &err)) == NULL)
	{
		fprintf(stderr, "Could not parse plugin (error %d)\n", err);
		exit(1);
	}

	if ((err = Sp_DecompFunction(dc, dc->plugin->publics[0].code_offs, true)) != SP_ERROR_NONE)
	{
		fprintf(stderr, "Failed to decode function (error %d)\n", err);
	}

	Sp_FreeDecomp(dc);
}
