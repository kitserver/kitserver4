#include <stdio.h>

#define BUFLEN 4096

#include "imageutil.h"
#include "config.h"

FILE* log = NULL;

void main(int argc, char* argv[])
{

	KSERV_CONFIG config;
	ZeroMemory(config.exeName, sizeof(config.exeName));
	if (!ReadConfig(&config)) GetExeNameFromReg(&config);

	FILE* rlog = fopen("restoretool.log", "wt");
	if (!rlog) rlog = stdout;

	fprintf(rlog, "RestoreTool report\n");
	fprintf(rlog, "-----------------------\n\n");

	FILE* f = fopen(config.exeName,"r+b");
	if (!f) 
	{
		fprintf(rlog, "ERROR: Unable to open and check %s.\n", config.exeName);
		fprintf(rlog, "Make sure the filename is correct in kserv.cfg,\n");
		fprintf(rlog, "and that it is not read-only.\n");
		return;
	}
	if (SeekCodeSectionFlags(f))
	{
		DWORD flags;
		fread(&flags, sizeof(flags), 1, f);
		// toggle off the WRITEABLE flag
		if (flags & 0x80000000) 
		{
			flags &= 0x7fffffff;
			fseek(f, -sizeof(flags), SEEK_CUR);
			if (fwrite(&flags, sizeof(flags), 1, f) == 1)
			{
				fprintf(rlog, "Original data restored for file:\n");
				fprintf(rlog, "%s\n", config.exeName);
			}
		}
		else
		{
			fprintf(rlog, "Looks like original data was already restored before.\n");
			fprintf(rlog, "File %s was not modified.\n", config.exeName);
		}
	}
	else 
	{
		fprintf(rlog, "ERROR: Unable to find code section flags.\n");
		fprintf(rlog, "%s was NOT modified.\n", config.exeName);
	}
	fclose(f);
}
