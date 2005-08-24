#ifdef _MSC_VER
#pragma warning ( disable : 4786 )
#endif//_MSC_VER

#include <string>
#include <vector>

#include <stdio.h>

#include "msvc.h"

using std::string;
using std::vector;

#if 0
void
MSVCBackend::_generate_dsp ( FILE* OUT, const std::string& moduleName )
{
	size_t i;

	Module& module = *ProjectNode.LocateModule ( moduleName );

	std::string dsp_file = DspFileName(module);
	my @imports = @{module->{imports}};

	bool lib = (module->{type} eq "lib");
	my $dll = (module->{type} eq "dll");
	my $exe = (module->{type} eq "exe");

	my $console = $exe; # FIXME: Not always correct

	// TODO FIXME - not sure if the count here is right...
	int parts = 1;
	const char* p = strchr ( dsp_file.c_str(), '/' );
	while ( p )
	{
		++parts;
		p = strchr ( p+1, '/' );
	}
	std::string msvc_wine_dir = "..";
	while ( parts-- )
		msvc_wine_dir += "\\..";

	std::string wine_include_dir = msvc_wine_dir + "\\include";

	//$progress_current++;
	//$output->progress("$dsp_file (file $progress_current of $progress_max)");

	my @c_srcs = @{module->{c_srcs}};
	my @source_files = @{module->{source_files}};
	// TODO FIXME - we don't include header files in our build system
	//my @header_files = @{module->{header_files}};
	vector<string> header_files;
	my @resource_files = @{module->{resource_files}};

	if (module.name !~ /^wine(?:_unicode|build|runtests|test)?$/ &&
		module.name !~ /^(?:gdi32)_.+?$/ &&
		module.name !~ /_test$/)
	{
		push @source_files, "$project.spec" );
		@source_files = sort(@source_files);
	}

	bool no_cpp = true;
	bool no_msvc_headers = true;
	if (module.name =~ /^wine(?:runtests|test)$/
		|| module.name =~ /_test$/)
	{
		no_msvc_headers = false;
	}

	std::vector<std::string> cfgs;

	cfgs.push_back ( module.name + " - Win32" );

	if (!no_cpp)
	{
		std::vector<std::string> _cfgs;
		for ( i = 0; i < cfgs.size(); i++ )
		{
			_cfgs.push_back ( cfgs[i] + " C" );
			_cfgs.push_back ( cfgs[i] + " C++" );
		}
		cfgs.resize(0);
		cfgs = _cfgs;
	}

	if (!no_release)
	{
		std::vector<std::string> _cfgs;
		for ( i = 0; i < cfgs.size(); i++ )
		{
			_cfgs.push_back ( cfgs[i] + " Debug" );
			_cfgs.push_back ( cfgs[i] + " Release" );
		}
		cfgs.resize(0);
		cfgs = _cfgs;
	}

	if (!no_msvc_headers)
	{
		std::vector<std::string> _cfgs;
		for ( i = 0; i < cfgs.size(); i++ )
		{
			_cfgs.push_back ( cfgs[i] + " MSVC Headers" );
			_cfgs.push_back ( cfgs[i] + " Wine Headers" );
		}
		cfgs.resize(0);
		cfgs = _cfgs;
	}

	string default_cfg = cfgs.back();

	fprintf ( OUT, "# Microsoft Developer Studio Project File - Name=\"%s\" - Package Owner=<4>\n", module.name.c_str() );
	fprintf ( OUT, "# Microsoft Developer Studio Generated Build File, Format Version 6.00\n" );
	fprintf ( OUT, "# ** DO NOT EDIT **\n" );
	fprintf ( OUT, "\n" );

	if ( lib )
	{
		fprintf ( OUT, "# TARGTYPE \"Win32 (x86) Static Library\" 0x0104\n" );
	}
	else if ( dll )
	{
		fprintf ( OUT, "# TARGTYPE \"Win32 (x86) Dynamic-Link Library\" 0x0102\n" );
	}
	else
	{
		fprintf ( OUT, "# TARGTYPE \"Win32 (x86) Console Application\" 0x0103\n" );
	}
	fprintf ( OUT, "\n" );

	fprintf ( OUT, "CFG=$default_cfg\n" );
	fprintf ( OUT, "!MESSAGE This is not a valid makefile. To build this project using NMAKE,\n" );
	fprintf ( OUT, "!MESSAGE use the Export Makefile command and run\n" );
	fprintf ( OUT, "!MESSAGE \n" );
	fprintf ( OUT, "!MESSAGE NMAKE /f \"%s.mak\".\n", module.name.c_str() );
	fprintf ( OUT, "!MESSAGE \n" );
	fprintf ( OUT, "!MESSAGE You can specify a configuration when running NMAKE\n" );
	fprintf ( OUT, "!MESSAGE by defining the macro CFG on the command line. For example:\n" );
	fprintf ( OUT, "!MESSAGE \n" );
	fprintf ( OUT, "!MESSAGE NMAKE /f \"%s.mak\" CFG=\"%s\"\n", module.name.c_str(), default_cfg.c_str() );
	fprintf ( OUT, "!MESSAGE \n" );
	fprintf ( OUT, "!MESSAGE Possible choices for configuration are:\n" );
	fprintf ( OUT, "!MESSAGE \n" );
	for ( i = 0; i < cfgs.size(); i++ )
	{
		string& cfg = cfgs[i];
		if ( lib )
		{
			fprintf ( OUT, "!MESSAGE \"%s\" (based on \"Win32 (x86) Static Library\")\n", cfg.c_str() );
		}
		else if ( dll )
		{
			fprintf ( OUT, "!MESSAGE \"%s\" (based on \"Win32 (x86) Dynamic-Link Library\")\n", cfg.c_str() );
		}
		else
		{
			fprintf ( OUT, "!MESSAGE \"%s\" (based on \"Win32 (x86) Console Application\")\n", cfg.c_str() );
		}
	}
	fprintf ( OUT, "!MESSAGE \n" );
	fprintf ( OUT, "\n" );

	fprintf ( OUT, "# Begin Project\n" );
	fprintf ( OUT, "# PROP AllowPerConfigDependencies 0\n" );
	fprintf ( OUT, "# PROP Scc_ProjName \"\"\n" );
	fprintf ( OUT, "# PROP Scc_LocalPath \"\"\n" );
	fprintf ( OUT, "CPP=cl.exe\n" );
	if ( !lib && !exe ) fprintf ( OUT, "MTL=midl.exe\n" );
	fprintf ( OUT, "RSC=rc.exe\n" );

	int n = 0;

	std::string output_dir;
	for ( size_t icfg = 0; icfg < cfgs.size(); icfg++ )
	{
		std::string& cfg = cfgs[icfg];
		if ( icfg )
		{
			if ( n == 0 )
			{
				fprintf ( OUT, "!IF  \"\$(CFG)\" == \"%s\"\n", cfg.c_str() );
				fprintf ( OUT, "\n" );
			}
			else
			{
				fprintf ( OUT, "\n" );
				fprintf ( OUT, "!ELSEIF  \"\$(CFG)\" == \"%s\"\n", cfg.c_str() );
				fprintf ( OUT, "\n" );
			}
		}

		bool debug = !strstr ( cfg.c_str(), "Release" );
		bool msvc_headers = ( 0 != strstr ( cfg.c_str(), "MSVC Headers" ) );

		fprintf ( OUT, "# PROP BASE Use_MFC 0\n" );

		if ( debug )
		{
			fprintf ( OUT, "# PROP BASE Use_Debug_Libraries 1\n" );
		}
		else
		{
			fprintf ( OUT, "# PROP BASE Use_Debug_Libraries 0\n" );
		}

		output_dir = Replace(cfg,module.name + " - ","");
		output_dir = Replace(output_dir," ","_");
		output_dir = Replace(output_dir,"C++","Cxx");
		if ( output_prefix_dir.size() )
			output_dir = output_prefix_dir + "\\" + output_dir;

		fprintf ( OUT, "# PROP BASE Output_Dir \"%s\"\n", output_dir.c_str() );
		fprintf ( OUT, "# PROP BASE Intermediate_Dir \"%s\"\n", output_dir.c_str() );

		fprintf ( OUT, "# PROP BASE Target_Dir \"\"\n" );

		fprintf ( OUT, "# PROP Use_MFC 0\n" );
		if ( debug )
		{
			fprintf ( OUT, "# PROP Use_Debug_Libraries 1\n" );
		}
		else
		{
			fprintf ( OUT, "# PROP Use_Debug_Libraries 0\n" );
		}
		fprintf ( OUT, "# PROP Output_Dir \"%s\"\n", output_dir.c_str() );
		fprintf ( OUT, "# PROP Intermediate_Dir \"%s\"\n", output_dir.c_str() );

		fprintf ( OUT, "# PROP Ignore_Export_Lib 0\n" if $dll;
		fprintf ( OUT, "# PROP Target_Dir \"\"\n" );

		vector<string> defines;
		defines.push_back ( "WINVER=0x0501" );
		defines.push_back ( "_WIN32_WINNT=0x0501" );
		defines.push_back ( "_WIN32_IE=0x0600" );
		defines.push_back ( "WIN32" );
		defines.push_back ( "_WINDOWS" );
		defines.push_back ( "WIN32" );
		defines.push_back ( "_MBCS" );
		if ( debug )
		{
			defines.push_back ( "_DEBUG" );
			if ( lib || exe )
			{
				fprintf ( OUT, "# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od" );
				defines.push_back ( "_LIB" );
			}
			else
			{
				fprintf ( OUT, "# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od" );
				defines.push_back ( "_WINDOWS" );
				defines.push_back ( "_USRDLL" );
				defines.push_back ( string("\U") + module.name + "\E_EXPORTS" );
			}
		}
		else
		{
			defines.push_back ( "NDEBUG" );
			if ( lib || exe )
			{
				fprintf ( OUT, "# ADD BASE CPP /nologo /W3 /GX /O2" );
				defines.push_back ( "_LIB" );
			}
			else
			{
				fprintf ( OUT, "# ADD BASE CPP /nologo /MT /W3 /GX /O2" );
				defines.push_back ( "_WINDOWS" );
				defines.push_back ( "_USRDLL" );
				defines.push_back ( string("\U") + module.name + "\E_EXPORTS" );
			}
		}

		for ( i = 0; i < defines.size(); i++ )
		{
			fprintf ( OUT, " /D \"%s\"", defines[i].c_str() );
		}
		if ( lib || exe ) fprintf ( OUT, " /YX" );
		fprintf ( OUT, " /FD" );
		if ( debug )
		{
			fprintf ( OUT, " /GZ" );
			if ( lib || exe ) fprintf ( OUT, " " );
		}
		fprintf ( OUT, " /c" );
		fprintf ( OUT, "\n" );

		vector<string> defines2;
		defines2.push_back ( "WINVER=0x0501" );
		defines2.push_back ( "_WIN32_WINNT=0x0501" );
		defines2.push_back ( "_WIN32_IE=0x0600" );
		defines2.push_back ( "WIN32" );
		defines2.push_back ( "_WINDOWS" );
		defines2.push_back ( "_MBCS" );
		if(debug)
		{
			defines2.push_back ( "_DEBUG" );
			if(lib)
			{
				fprintf ( OUT, "# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od" );
				defines2.push_back ( "_LIB" );
			}
			else
			{
				fprintf ( OUT, "# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od" );
				defines2.push_back ( "_USRDLL" );
			}
		}
		else
		{
			defines2.push_back ( "NDEBUG" );
			if(lib)
			{
				fprintf ( OUT, "# ADD CPP /nologo /MT /W3 /GX /O2" );
				defines2.push_back ( "_LIB" );
			}
			else
			{
				fprintf ( OUT, "# ADD CPP /nologo /MT /W3 /GX /O2" );
				defines2.push_back ( "_USRDLL" );
			}
		}

		std::vector<std::string> includes;
		if ( wine )
		{
			defines2.push_back ( string("_\U") + module.name + "\E_" );
			if ( module.name !~ /^(?:wine(?:build|test)|.*?_test)$/ )
				defines2.push_back ( "__WINESRC__" );
			if ( msvc_headers )
				defines2.push_back ( "__WINE_USE_NATIVE_HEADERS" );
			string output_dir2 = Replace(output_dir,"\\","\\\\");
			defines2.push_back ( ssprintf("__WINETEST_OUTPUT_DIR=\\\"%s\\\"",output_dir.c_str()) );
			defines2.push_back ( "__i386__" );
			defines2.push_back ( "_X86_" );

			// TODO FIXME - wine hacks?
			/*if($project =~ /^gdi32_(?:enhmfdrv|mfdrv)$/) {
				push @includes, ".." );
			}

			if ($project =~ /_test$/) {
				push @includes, "$msvc_wine_dir\\$output_dir" );
			}

			if (!$msvc_headers || $project eq "winetest") {
				push @includes, $wine_include_dir;
			}*/
		}

		if ( wine )
		{
			for ( i = 0; i < includes.size(); i++ );
			{
				string& include = includes[i];
				if ( strpbrk ( include.c_str(), "[\\\"]" ) )
				{
					fprintf ( OUT, " /I \"%s\"", include.c_str() );
				}
				else
				{
					fprintf ( OUT, " /I %s", include.c_str() );
				}
			}
		}

		for ( i = 0; i < defines2.size(); i++ )
		{
			string& define = defines2[i];
			if ( strpbrk ( define.c_str(), "[\\\"]" ) )
			{
				fprintf ( OUT, " /D \"%s\"", define.c_str() );
			}
			else
			{
				fprintf ( OUT, " /D %s", define.c_str() );
			}
		}
		if ( wine ) fprintf ( OUT, " /D inline=__inline" );
		//fprintf ( OUT, " /D \"__STDC__\"" if 0 && $wine;

		fprintf ( OUT, lib ? " /YX" : " /FR" );
		fprintf ( OUT, " /FD" );
		if ( debug ) fprintf ( OUT, " /GZ" );
		if ( debug && lib ) fprintf ( OUT, " " );
		fprintf ( OUT, " /c" );
		if ( !no_cpp ) fprintf ( OUT, " /TP" );
		fprintf ( OUT, "\n" );

		if ( debug )
		{
			fprintf ( OUT, "# SUBTRACT CPP /X /YX\n" if $dll;
			fprintf ( OUT, "# ADD BASE MTL /nologo /D \"_DEBUG\" /mktyplib203 /win32\n" if $dll;
			fprintf ( OUT, "# ADD MTL /nologo /D \"_DEBUG\" /mktyplib203 /win32\n" if $dll;
			fprintf ( OUT, "# ADD BASE RSC /l 0x41d /d \"_DEBUG\"\n" );
			fprintf ( OUT, "# ADD RSC /l 0x41d" );
			if ( wine )
			{
				for ( i = 0; i < includes.size(); i++ )
				{
					fprintf ( OUT, " /i \"%s\"", includes[i].c_str() );
				}
			}
			fprintf ( OUT, " /d \"_DEBUG\"\n" );
		}
		else
		{
			if ( dll )
			{
				fprintf ( OUT, "# SUBTRACT CPP /YX\n" );
				fprintf ( OUT, "# ADD BASE MTL /nologo /D \"NDEBUG\" /mktyplib203 /win32\n" );
				fprintf ( OUT, "# ADD MTL /nologo /D \"NDEBUG\" /mktyplib203 /win32\n" );
			}
			fprintf ( OUT, "# ADD BASE RSC /l 0x41d /d \"NDEBUG\"\n" );
			fprintf ( OUT, "# ADD RSC /l 0x41d" );
			if ( wine )
			{
				for ( i = 0; i < includes.size(); i++ )
					fprintf ( OUT, " /i \"%s\"", includes[i].c_str() );
				}
			}
			fprintf ( OUT, "/d \"NDEBUG\"\n" );
		}
		fprintf ( OUT, "BSC32=bscmake.exe\n" );
		fprintf ( OUT, "# ADD BASE BSC32 /nologo\n" );
		fprintf ( OUT, "# ADD BSC32 /nologo\n" );

		if ( exe || dll )
		{
			fprintf ( OUT, "LINK32=link.exe\n" );
			fprintf ( OUT, "# ADD BASE LINK32 " );
			my @libraries = qw(kernel32.lib user32.lib gdi32.lib winspool.lib
						   comdlg32.lib advapi32.lib shell32.lib ole32.lib
						   oleaut32.lib uuid.lib odbc32.lib odbccp32.lib);
			for ( i = 0; i < libraries.size(); i++ )
			{
				fprintf ( OUT, "%s ", libraries[i].c_str() );
			}
			fprintf ( OUT, " /nologo" );
			if ( dll ) fprintf ( OUT, " /dll" );
			if ( console ) fprintf ( OUT, " /subsystem:console" );
			if ( debug ) fprintf ( OUT, " /debug" );
			fprintf ( OUT, " /machine:I386" );
			if ( debug ) fprintf ( OUT, " /pdbtype:sept" );
			fprintf ( OUT, "\n" );

			fprintf ( OUT, "# ADD LINK32" );
			fprintf ( OUT, " /nologo" );
			// TODO FIXME - do we need their kludge?
			//fprintf ( OUT, " libcmt.lib" if $project =~ /^ntdll$/; # FIXME: Kludge
			for ( i = 0; i < imports.size(); i++ )
			{
				const string& import = imports[i];
				if ( import != "msvcrt" )
					fprintf ( OUT, " %s.lib", import.c_str() );
			}
			if ( dll ) fprintf ( OUT, " /dll" );
			if ( console ) fprintf ( OUT, " /subsystem:console" );
			if ( debug ) fprintf ( OUT, " /debug" );
			fprintf ( OUT, " /machine:I386" );
			// TODO FIXME - do we need their kludge?
			//fprintf ( OUT, " /nodefaultlib" if $project =~ /^ntdll$/; # FIXME: Kludge
			if ( dll ) fprintf ( OUT, " /def:\"%s.def\"", module.name.c_str() )
			if ( debug ) fprintf ( OUT, " /pdbtype:sept" );
			fprintf ( OUT, "\n" );
		}
		else
		{
			fprintf ( OUT, "LIB32=link.exe -lib\n" );
			fprintf ( OUT, "# ADD BASE LIB32 /nologo\n" );
			fprintf ( OUT, "# ADD LIB32 /nologo\n" );
		}

		n++;
	}

	if ( cfgs.size() != 0 )
	{
		fprintf ( OUT, "\n" );
		fprintf ( OUT, "!ENDIF \n" );
		fprintf ( OUT, "\n" );
	}

	if ( module.name == "winebuild" )
	{
		fprintf ( OUT, "# Begin Special Build Tool\n" );
		fprintf ( OUT, "SOURCE=\"\$(InputPath)\"\n" );
		fprintf ( OUT, "PostBuild_Desc=Copying wine.dll and wine_unicode.dll ...\n" );
		fprintf ( OUT, "PostBuild_Cmds=" );
		fprintf ( OUT, "copy ..\\..\\library\\$output_dir\\wine.dll \$(OutDir)\t" );
		fprintf ( OUT, "copy ..\\..\\unicode\\$output_dir\\wine_unicode.dll \$(OutDir)\n" );
		fprintf ( OUT, "# End Special Build Tool\n" );
	}
	fprintf ( OUT, "# Begin Target\n" );
	fprintf ( OUT, "\n" );
	for ( i = 0; i < cfgs.size(); i++ )
	{
		fprintf ( OUT, "# Name \"%s\"\n", cfgs[i].c_str() );
	}

	fprintf ( OUT, "# Begin Group \"Source Files\"\n" );
	fprintf ( OUT, "\n" );
	fprintf ( OUT, "# PROP Default_Filter \"cpp;c;cxx;rc;def;r;odl;idl;hpj;bat\"\n" );

	for ( i = 0; i < source_files.size(); i++ )
	{
		string source_file = DosSeparators(source_files[i]);

		if ( strncmp ( source_file.c_str(), ".\\", 2 ) )
		{
			source_file = string(".\\") + source_file );
		}

		if ( !strcmp ( &source_file[source_file.size()-5], ".spec" ) )
		{
			string basename = string ( source_file, source_file.size() - 5 );

			if ( $basename !~ /\..{1,3}$/; ) basename += string(".dll");
			string dbg_c_file = basename + ".dbg.c";

			fprintf ( OUT, "# Begin Source File\n" );
			fprintf ( OUT, "\n" );
			fprintf ( OUT, "SOURCE=%s\n", dbg_c_file.c_str() );
			fprintf ( OUT, "# End Source File\n" );
		}

		fprintf ( OUT, "# Begin Source File\n" );
		fprintf ( OUT, "\n" );

		fprintf ( OUT, "SOURCE=%s\n", source_file.c_str() );

		if ( !strcmp ( &source_file[source_file.size()-5], ".spec" ) )
		{
			string basename = string ( source_file, source_file.size() - 5 );

			string spec_file = source_file;
			string def_file = basename + ".def";

			if ( $basename !~ /\..{1,3}$/; ) basename += ".dll";
			string dbg_file = basename + ".dbg";
			string dbg_c_file = basename + ".dbg.c";

			string srcdir = "."; // FIXME: Is this really always correct?

			fprintf ( OUT, "# Begin Custom Build\n" );
			fprintf ( OUT, "InputPath=%s\n", spec_file.c_str() );
			fprintf ( OUT, "\n" );
			fprintf ( OUT, "BuildCmds= \\\n" );
			fprintf ( OUT, "\t..\\..\\tools\\winebuild\\%s\\winebuild.exe --def %s > %s \\\n",
				output_dir.c_str(),
				spec_file.c_str(),
				def_file.c_str() );
			
			if ( module.name =~ /^ntdll$/ )
			{
				int n = 0;
				for ( i = 0; i < c_srcs.size(); i++ )
				{
					const string& c_src = c_srcs[i];
					if(n++ > 0)
					{
						fprintf ( OUT, "\techo %s >> %s \\\n", c_src.c_str(), dbg_file.c_str() );
					}
					else
					{
						fprintf ( OUT, "\techo %s > %s \\\n", c_src.c_str(), dbg_file.c_str() );
					}
				}
				fprintf ( OUT, "\t..\\..\\tools\\winebuild\\%s\\winebuild.exe",
					output_dir.c_str() );
				fprintf ( OUT, " -o %s --debug -C%s %s \\\n",
					dbg_c_file.c_str(),
					srcdir.c_str(),
					dbg_file.c_str() );
			}
			else
			{
				string sc_srcs;
				for ( i = 0; i < c_srcs.size(); i++ )
				{
					const string& c_src = c_srcs[i];
					if ( !strcmp ( &c_src[c_src.size()-2], ".c" ) )
					{
						if ( sc_srcs.size() )
							sc_srcs += " ";
						sc_srcs += c_src;
					}
				}

				fprintf ( OUT, "\t..\\..\\tools\\winebuild\\%s\\winebuild.exe",
					output_dir.c_str() );
				fprintf ( OUT, " -o %s --debug -C%s %s \\\n",
					dbg_c_file.c_str(),
					srcdir.c_str(),
					sc_srcs.c_str() );
			}

			fprintf ( OUT, "\t\n" );
			fprintf ( OUT, "\n" );
			fprintf ( OUT, "\"%s\" : \$(SOURCE) \"\$(INTDIR)\" \"\$(OUTDIR)\"\n", def_file.c_str() );
			fprintf ( OUT, "   \$(BuildCmds)\n" );
			fprintf ( OUT, "\n" );
			fprintf ( OUT, "\"%s\" : \$(SOURCE) \"\$(INTDIR)\" \"\$(OUTDIR)\"\n", dbg_c_file.c_str() );
			fprintf ( OUT, "   \$(BuildCmds)\n" );
			fprintf ( OUT, "# End Custom Build\n" );
		}
		/*else if ( source_file =~ /([^\\]*?\.h)$/ )
		{
			my $h_file = $1;

			foreach my $cfg (@cfgs) {
				if($#cfgs == 0) {
					# Nothing
				} elsif($n == 0) {
					fprintf ( OUT, "!IF  \"\$(CFG)\" == \"$cfg\"\n" );
					fprintf ( OUT, "\n" );
				} else {
					fprintf ( OUT, "\n" );
					fprintf ( OUT, "!ELSEIF  \"\$(CFG)\" == \"$cfg\"\n" );
					fprintf ( OUT, "\n" );
				}

				$output_dir = $cfg;
				$output_dir =~ s/^$project - //;
				$output_dir =~ s/ /_/g;
				$output_dir =~ s/C\+\+/Cxx/g;
				if($output_prefix_dir) {
					$output_dir = "$output_prefix_dir\\$output_dir" );
				}

				fprintf ( OUT, "# Begin Custom Build\n" );
				fprintf ( OUT, "OutDir=%s\n", output_dir.c_str() );
				fprintf ( OUT, "InputPath=%s\n", source_file.c_str() );
				fprintf ( OUT, "\n" );
				fprintf ( OUT, "\"\$(OutDir)\\wine\\%s\" : \$(SOURCE) \"\$(INTDIR)\" \"\$(OUTDIR)\"\n", h_file.c_str() );
				fprintf ( OUT, "\tcopy \"\$(InputPath)\" \"\$(OutDir)\\wine\"\n" );
				fprintf ( OUT, "\n" );
				fprintf ( OUT, "# End Custom Build\n" );
			}

			if ( cfgs.size() != 0)
			{
				fprintf ( OUT, "\n" );
				fprintf ( OUT, "!ENDIF \n" );
				fprintf ( OUT, "\n" );
			}
		}*/

		fprintf ( OUT, "# End Source File\n" );
	}
	fprintf ( OUT, "# End Group\n" );

	fprintf ( OUT, "# Begin Group \"Header Files\"\n" );
	fprintf ( OUT, "\n" );
	fprintf ( OUT, "# PROP Default_Filter \"h;hpp;hxx;hm;inl\"\n" );
	for ( i = 0; i < header_files.size(); i++ )
	{
		const string& header_file = header_files[i];
		fprintf ( OUT, "# Begin Source File\n" );
		fprintf ( OUT, "\n" );
		fprintf ( OUT, "SOURCE=.\\%s\n", header_file.c_str() );
		fprintf ( OUT, "# End Source File\n" );
	}
	fprintf ( OUT, "# End Group\n" );



	fprintf ( OUT, "# Begin Group \"Resource Files\"\n" );
	fprintf ( OUT, "\n" );
	fprintf ( OUT, "# PROP Default_Filter \"ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe\"\n" );
	for ( i = 0; i < resource_files.size(); i++ )
	{
		const string& resource_file = resource_files[i];
		fprintf ( OUT, "# Begin Source File\n" );
		fprintf ( OUT, "\n" );
		fprintf ( OUT, "SOURCE=.\\%s\n", resource_file.c_str() );
		fprintf ( OUT, "# End Source File\n" );
	}
	fprintf ( OUT, "# End Group\n" );

	fprintf ( OUT, "# End Target\n" );
	fprintf ( OUT, "# End Project\n" );

	fclose(OUT);
}
#endif
void
MSVCBackend::_generate_dsw_header ( FILE* OUT )
{
    fprintf ( OUT, "Microsoft Developer Studio Workspace File, Format Version 6.00\n" );
    fprintf ( OUT, "# WARNING: DO NOT EDIT OR DELETE THIS WORKSPACE FILE!\n" );
    fprintf ( OUT, "\n" );
}

void
MSVCBackend::_generate_dsw_project (
	FILE* OUT,
	const Module& module,
	std::string dsp_file,
	const std::vector<Dependency*>& dependencies )
{
    dsp_file = std::string(".\\") + dsp_file;
	// TODO FIXME - what does next line do?
    //$dsp_file =~ y%/%\\%;
    
	// TODO FIXME - must they be sorted?
    //@dependencies = sort(@dependencies);

    fprintf ( OUT, "###############################################################################\n" );
    fprintf ( OUT, "\n" );
    fprintf ( OUT, "Project: \"%s\"=%s - Package Owner=<4>\n", module.name.c_str(), dsp_file.c_str() );
    fprintf ( OUT, "\n" );
    fprintf ( OUT, "Package=<5>\n" );
    fprintf ( OUT, "{{{\n" );
    fprintf ( OUT, "}}}\n" );
    fprintf ( OUT, "\n" );
    fprintf ( OUT, "Package=<4>\n" );
    fprintf ( OUT, "{{{\n" );
	for ( size_t i = 0; i < dependencies.size(); i++ )
	{
		Dependency& dependency = *dependencies[i];
		fprintf ( OUT, "    Begin Project Dependency\n" );
		fprintf ( OUT, "    Project_Dep_Name %s\n", dependency.module.name.c_str() );
		fprintf ( OUT, "    End Project Dependency\n" );
	}
	fprintf ( OUT, "}}}\n" );
	fprintf ( OUT, "\n" );
}

void
MSVCBackend::_generate_dsw_footer ( FILE* OUT )
{
	fprintf ( OUT, "###############################################################################\n" );
	fprintf ( OUT, "\n" );
	fprintf ( OUT, "Global:\n" );
	fprintf ( OUT, "\n" );
	fprintf ( OUT, "Package=<5>\n" );
	fprintf ( OUT, "{{{\n" );
	fprintf ( OUT, "}}}\n" );
	fprintf ( OUT, "\n" );
	fprintf ( OUT, "Package=<3>\n" );
	fprintf ( OUT, "{{{\n" );
	fprintf ( OUT, "}}}\n" );
	fprintf ( OUT, "\n" );
	fprintf ( OUT, "###############################################################################\n" );
	fprintf ( OUT, "\n" );
}

void
MSVCBackend::_generate_wine_dsw ( FILE* OUT )
{
	_generate_dsw_header(OUT);
	// TODO FIXME - is it necessary to sort them?
	for ( size_t i = 0; i < ProjectNode.modules.size(); i++ )
	//foreach my $module (sort(keys(%modules)))
	{
		Module& module = *ProjectNode.modules[i];

		//my $project = module->{project};
		std::string dsp_file = DspFileName ( module );

		// TODO FIXME - more wine hacks?
        /*if($project =~ /^gdi32$/) {
			foreach my $dir (@gdi32_dirs) {
				my $dir2 = $dir;
				$dir2 =~ s%^.*?/([^/]+)$%$1%;

				my $module = "gdi32_$dir2";
				$module =~ s%/%_%g;
				push @dependencies, $module;
			}
        }*/

		_generate_dsw_project ( OUT, module, dsp_file, module.dependencies );
    }
    _generate_dsw_footer ( OUT );
}
