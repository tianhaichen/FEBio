#include "stdafx.h"
#include "FEBioApp.h"
#include "console.h"
#include "CommandManager.h"
#include <FECore/log.h>
#include "console.h"
#include "breakpoint.h"
#include <FEBioLib/febio.h>
#include <FEBioLib/version.h>
#include "febio_cb.h"
#include "Interrupt.h"

FEBioApp* FEBioApp::m_This = nullptr;

FEBioApp::FEBioApp()
{
	assert(m_This == nullptr);
	m_This = this;

	m_fem = nullptr;
}

FEBioApp* FEBioApp::GetInstance()
{
	assert(m_This);
	return m_This;
}

CMDOPTIONS& FEBioApp::CommandOptions()
{
	return m_ops;
}

int FEBioApp::Init(int argc, char* argv[])
{
	// Initialize kernel
	FECoreKernel::SetInstance(febio::GetFECoreKernel());

	// divert the log output to the console
	felog.SetLogStream(new ConsoleStream);

	// parse the command line
	if (ParseCmdLine(argc, argv) == false) return 0;

	// say hello
	if (m_ops.bsplash && (!m_ops.bsilent)) febio::Hello();

	// Initialize FEBio library
	febio::InitLibrary();

	// read the configration file if specified
	if (m_ops.szcnf[0])
		if (febio::Configure(m_ops.szcnf) == false)
		{
			fprintf(stderr, "FATAL ERROR: An error occurred reading the configuration file.\n");
			return 1;
		}

	// read command line plugin if specified
	if (m_ops.szimp[0] != 0)
	{
		febio::ImportPlugin(m_ops.szimp);
	}

	return 0;
}

//-----------------------------------------------------------------------------
int FEBioApp::Run()
{
	// activate interruption handler
	Interruption I;

	// run FEBio either interactively or directly
	if (m_ops.binteractive)
		return prompt();
	else
		return RunModel();
}

//-----------------------------------------------------------------------------
void FEBioApp::Finish()
{
	febio::FinishLibrary();

	Console::GetHandle()->CleanUp();
}

//-----------------------------------------------------------------------------
// get the current model
FEBioModel* FEBioApp::GetCurrentModel()
{
	return m_fem;
}

//-----------------------------------------------------------------------------
// set the currently active model
void FEBioApp::SetCurrentModel(FEBioModel* fem)
{
	m_fem = fem;
}

//-----------------------------------------------------------------------------
// Run an FEBio input file. 
int FEBioApp::RunModel()
{
	// if silent mode only output to file
	if (m_ops.bsilent)
	{
		felog.SetMode(Logfile::LOG_FILE);
		Console::GetHandle()->Deactivate();
	}

	// create the one and only FEBioModel object
	FEBioModel fem;
	SetCurrentModel(&fem);

	// register callbacks
	fem.AddCallback(update_console_cb, CB_MAJOR_ITERS | CB_INIT | CB_SOLVED, 0);
	fem.AddCallback(interrupt_cb, CB_ALWAYS, 0);
	fem.AddCallback(break_point_cb, CB_ALWAYS, 0);

	// set options that were passed on the command line
	fem.SetDebugFlag(m_ops.bdebug);

	// set the output filenames
	fem.SetLogFilename(m_ops.szlog);
	fem.SetPlotFilename(m_ops.szplt);
	fem.SetDumpFilename(m_ops.szdmp);

	// read the input file if specified
	int nret = 0;
	if (m_ops.szfile[0])
	{
		// read the input file
		if (fem.Input(m_ops.szfile) == false) nret = 1;
	}

	// solve the model with the task and control file
	if (nret == 0)
	{
		bool bret = febio::SolveModel(fem, m_ops.sztask, m_ops.szctrl);

		nret = (bret ? 1 : 0);
	}

	// reset the current model pointer
	SetCurrentModel(nullptr);

	return nret;
}

//-----------------------------------------------------------------------------
//! Prints the FEBio prompt. If the user did not enter anything on the command
//! line when running FEBio then commands can be entered at the FEBio prompt.
//! This function returns the command arguments as a CMDOPTIONS structure.
int FEBioApp::prompt()
{
	// get a pointer to the console window
	Console* pShell = Console::GetHandle();

	// set the title
	pShell->SetTitle("FEBio3");

	// process commands
	ProcessCommands();

	return 0;
}

//-----------------------------------------------------------------------------
//!  Parses the command line and returns a CMDOPTIONS structure
//
bool FEBioApp::ParseCmdLine(int nargs, char* argv[])
{
	CMDOPTIONS& ops = m_ops;

	// set default options
	ops.bdebug = false;
	ops.bsplash = true;
	ops.bsilent = false;
	ops.binteractive = true;

	bool blog = false;
	bool bplt = false;
	bool bdmp = false;
	bool brun = true;

	// initialize file names
	ops.szfile[0] = 0;
	ops.szplt[0] = 0;
	ops.szlog[0] = 0;
	ops.szdmp[0] = 0;
	ops.sztask[0] = 0;
	ops.szctrl[0] = 0;
	ops.szimp[0] = 0;

	// set initial configuration file name
	if (ops.szcnf[0] == 0)
	{
		char szpath[1024] = { 0 };
		febio::get_app_path(szpath, 1023);
		sprintf(ops.szcnf, "%sfebio.xml", szpath);
	}

	// loop over the arguments
	char* sz;
	for (int i=1; i<nargs; ++i)
	{
		sz = argv[i];

		if (strcmp(sz,"-r") == 0)
		{
			if (ops.sztask[0] != 0) { fprintf(stderr, "-r is incompatible with other command line option.\n"); return false; }
			strcpy(ops.sztask, "restart");
			strcpy(ops.szctrl, argv[++i]);
			ops.binteractive = false;
		}
		else if (strcmp(sz, "-d") == 0)
		{
			if (ops.sztask[0] != 0) { fprintf(stderr, "-d is incompatible with other command line option.\n"); return false; }
			strcpy(ops.sztask, "diagnose");
			strcpy(ops.szctrl, argv[++i]);
			ops.binteractive = false;
		}
		else if (strcmp(sz, "-p") == 0)
		{
			bplt = true;
			strcpy(ops.szplt, argv[++i]);
		}
		else if (strcmp(sz, "-a") == 0)
		{
			bdmp = true;
			strcpy(ops.szdmp, argv[++i]);
		}
		else if (strcmp(sz, "-o") == 0)
		{
			blog = true;
			strcpy(ops.szlog, argv[++i]);
		}
		else if (strcmp(sz, "-i") == 0)
		{
			++i;
			const char* szext = strrchr(argv[i], '.');
			if (szext == 0)
			{
				// we assume a default extension of .feb if none is provided
				sprintf(ops.szfile, "%s.feb", argv[i]);
			}
			else strcpy(ops.szfile, argv[i]);
			ops.binteractive = false;
		}
		else if (strcmp(sz, "-s") == 0)
		{
			if (ops.sztask[0] != 0) { fprintf(stderr, "-s is incompatible with other command line option.\n"); return false; }
			strcpy(ops.sztask, "optimize");
			strcpy(ops.szctrl, argv[++i]);
		}
		else if (strcmp(sz, "-g") == 0)
		{
			ops.bdebug = true;
		}
		else if (strcmp(sz, "-nosplash") == 0)
		{
			// don't show the welcome message
			ops.bsplash = false;
		}
		else if (strcmp(sz, "-silent") == 0)
		{
			// no output to screen
			ops.bsilent = true;
		}
		else if (strcmp(sz, "-cnf") == 0)	// obsolete: use -config instead
		{
			strcpy(ops.szcnf, argv[++i]);
		}
		else if (strcmp(sz, "-config") == 0)
		{
			strcpy(ops.szcnf, argv[++i]);
		}
		else if (strcmp(sz, "-noconfig") == 0)
		{
			ops.szcnf[0] = 0;
		}
		else if (strncmp(sz, "-task", 5) == 0)
		{
			if (sz[5] != '=') { fprintf(stderr, "command line error when parsing task\n"); return false; }
			strcpy(ops.sztask, sz+6);

			if (i<nargs-1)
			{
				char* szi = argv[i+1];
				if (szi[0] != '-')
				{
					// assume this is a control file for the specified task
					strcpy(ops.szctrl, argv[++i]);
				}
			}
		}
		else if (strcmp(sz, "-break") == 0)
		{
			char szbuf[32]={0};
			strcpy(szbuf, argv[++i]);

			add_break_point(szbuf);
		}
		else if (strcmp(sz, "-info")==0)
		{
			FILE* fp = stdout;
			if ((i<nargs-1) && (argv[i+1][0] != '-'))
			{
				fp = fopen(argv[++i], "wt");
				if (fp == 0) fp = stdout;
			}
			fprintf(fp, "compiled on " __DATE__ "\n");
#ifdef _DEBUG
			fprintf(fp, "FEBio version  = %d.%d.%d (DEBUG)\n", VERSION, SUBVERSION, SUBSUBVERSION);
#else
			fprintf(fp, "FEBio version  = %d.%d.%d\n", VERSION, SUBVERSION, SUBSUBVERSION);
#endif
			if (fp != stdout) fclose(fp);
		}
		else if (strcmp(sz, "-norun") == 0)
		{
			brun = false;
		}

		else if (strcmp(sz, "-import") == 0)
		{
			strcpy(ops.szimp, argv[++i]);
		}
		else if (sz[0] == '-')
		{
			fprintf(stderr, "FATAL ERROR: Invalid command line option.\n");
			return false;
		}
		else
		{
			// we allow FEBio to run without a -i option if no option is specified on the command line
			// so that we can run an .feb file by right-clicking on it in windows
			if (nargs == 2)
			{
				const char* szext = strrchr(argv[1], '.');
				if (szext == 0)
				{
					// we assume a default extension of .feb if none is provided
					sprintf(ops.szfile, "%s.feb", argv[1]);
				}
				else
				{
					strcpy(ops.szfile, argv[1]);
				}
				ops.binteractive = false;
			}
			else
			{
				fprintf(stderr, "FATAL ERROR: Invalid command line option\n");
				return false;
			}
		}
	}

	// do some sanity checks
	if (strcmp(ops.sztask, "optimize") == 0)
	{
		// make sure we have an input file
		if (ops.szfile[0]==0)
		{
			fprintf(stderr, "FATAL ERROR: no model input file was defined (use -i to define the model input file)\n\n");
			return false;
		}
	}

	// if no task is defined, we assume a std solve is wanted
	if (ops.sztask[0] == 0) strcpy(ops.sztask, "solve");

	// derive the other filenames
	if (ops.szfile[0])
	{
		char szbase[256]; strcpy(szbase, ops.szfile);
		char* ch = strrchr(szbase, '.');
		if (ch) *ch = 0;

		char szlogbase[256];
		if (ops.szctrl[0])
		{
			strcpy(szlogbase, ops.szctrl);
			ch = strrchr(szlogbase, '.');
			if (ch) *ch = 0;
		}
		else strcpy(szlogbase, szbase);

		if (!blog) sprintf(ops.szlog, "%s.log", szlogbase);
		if (!bplt) sprintf(ops.szplt, "%s.xplt", szbase);
		if (!bdmp) sprintf(ops.szdmp, "%s.dmp", szbase);
	}
	else if (ops.szctrl[0])
	{
		char szbase[256]; strcpy(szbase, ops.szfile);
		strcpy(szbase, ops.szctrl);
		char* ch = strrchr(szbase, '.');
		if (ch) *ch = 0;

		if (!blog) sprintf(ops.szlog, "%s.log", szbase);
		if (!bplt) sprintf(ops.szplt, "%s.xplt", szbase);
		if (!bdmp) sprintf(ops.szdmp, "%s.dmp", szbase);
	}

	return brun;
}

void FEBioApp::ProcessCommands()
{
	// get a pointer to the console window
	Console* pShell = Console::GetHandle();

	// get the command manager
	CommandManager* CM = CommandManager::GetInstance();

	// enter command loop
	int nargs;
	char* argv[32];
	while (1)
	{
		// get a command from the shell
		pShell->GetCommand(nargs, argv);
		if (nargs > 0)
		{
			// find the command that has this name
			Command* pcmd = CM->Find(argv[0]);
			if (pcmd)
			{
				int nret = pcmd->run(nargs, argv);
				if (nret == 1) break;
			}
			else
			{
				printf("Unknown command: %s\n", argv[0]);
			}
		}

		// make sure to clear the progress on the console
		FEBioModel* fem = GetCurrentModel();
		if ((fem == nullptr) || fem->IsSolved()) pShell->SetProgress(0);
	}
}