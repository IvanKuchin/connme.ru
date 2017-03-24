#!/usr/bin/perl

use POSIX;
use File::Copy;
use File::Copy::Recursive qw(fcopy rcopy dircopy fmove rmove dirmove);
use File::Path;
use Cwd;
use strict;
my	($str, %config, $key, $DEBUG, $outi, $timestamp, $command, $archive_filename);
my	($cgidir, $htmldir, $srcdir, $mysqlhost, $mysqllogin, $mysqlpass, $mysqldb);
my	($logFileName, $chatLogFileName, $adminLogFileName);
my	($action);
my	%dataDirs;
my	$num_args = $#ARGV + 1;

#
# unbuffer output
#
$|++;

$DEBUG = 0;

#
# usage archive.pl [OPT]
# --backup - full backup
# --backup_structure - structure backup
# --restore - restore from archive
# --restore_structure - restore structure only
# 
$action = "";
if($num_args eq 1)
{
	if($ARGV[0] eq "--backup_structure") 
	{ 
		$action = "--backup_structure"; 
	}
	if($ARGV[0] eq "--restore_structure") 
	{ 
		$action = "--restore_structure"; 
	}
	if($ARGV[0] eq "--restore") 
	{ 
		$action = "--restore"; 
	}
	if($ARGV[0] eq "--backup") 
	{ 
		$action = "--backup"; 
	}
}

if($action eq "")
{
	usage();
	die;
}

print "action = $action\n" if($DEBUG);

if($action =~ /^--restore/ and not isFilesReadyToRestore())
{
	print "ERROR: not all files are exists in the restore folder\n";
	die;
}

if($action =~ /^--restore/ and not isRoot())
{
	print "ERROR: recover can be run with root privilege only\n";
	die;
}

if($action =~ /^--restore/)
{
	print "Changing from dev.connme.ru to www.connme.ru in src files\n";
	ChangeDevToWWW("include/localy.h");
	ChangeDevToWWW("include/csession.h");
	ChangeDevToWWW("Makefile");
	ChangeDevToWWW("cgi-bin/admin/templates/admin_domain.htmlt");
	ChangeDevToWWW("cgi-bin/admin/.htaccess");
	ChangeIPDevToIPWWW("cgi-bin/admin/templates/admin_chat_get_cnx_list.htmlt");
	ChangeIPDevToIPWWW("cgi-bin/admin/templates/admin_chat_get_presence_cache.htmlt");
	ChangeIPDevToIPWWW("html/js/pages/chat.js");
	ChangeIPDevToIPWWW("html/quality_assurance/chat_checking.html");
	ChangeIPDevToIPWWW("html/quality_assurance/chat_image_uploading_ddos.html");

}

#
# init the variables
#
$dataDirs{"IMAGEAVATARDIR"} = "";
$dataDirs{"IMAGECHATDIR"} = "";
$dataDirs{"IMAGECOMPANIESDIR"} = "";
$dataDirs{"IMAGECAPTCHADIR"} = "";
$dataDirs{"IMAGEFEEDDIR"} = "";

$timestamp = strftime "%Y%m%d%H%M%S", localtime;

#
# parse Makefile on "key = value" hash
#
print "find Makefile";
unless(open(F, "Makefile"))
{
	print " (can't open Makefile)\n";
	die;
}
print "....\n";


while (<F>)
{
	$str = $_;
	if($str =~ /(\w+)\s*=\s*(.*)/)
	{
		my	$configKey = $1;
		my	$configValue = $2;

		$config{$configKey} = $configValue;

		foreach $key (keys %dataDirs) {
			if($key eq $configKey)
			{
				my	$tmp;

				$tmp = $configValue;
				$tmp =~ s/\$\(.*\)([\w\d]+)\/?/$1/;
				$dataDirs{$configKey} = $tmp;
			}
		}

	}
}
close(F);

if($DEBUG)
{
	foreach my $key (keys %config)
	{
		print $key." = ".$config{$key}."\n";
	}
}

#
# Rander a key = value
# $(CGIDIR) -> /real/path
#

$srcdir = "./";
$cgidir = $config{"CGIDIR"};
$htmldir = $config{"HTMLDIR"};

{
	my	$needToRender = 1;
	while($needToRender)
	{
		$needToRender = 0;

		foreach $key (keys %config)
		{
			if($cgidir =~ /$key/)
			{
				$cgidir =~ s/$key/$config{$key}/e;
				$needToRender = 1;
			}
			if($htmldir =~ /$key/)
			{
				$htmldir =~ s/$key/$config{$key}/e;
				$needToRender = 1;
			}

			foreach my $key2 (keys %dataDirs) {
				if($dataDirs{$key2} =~ /\$\($key\)/)
				{
					$dataDirs{$key2} =~ s/\$\($key\)/$config{$key}/e;
					$needToRender = 1;
				}
			}
			
		}	
	}
	
}

if($DEBUG)
{
	foreach my $key2 (keys %dataDirs) {
			print "exclude_dirs: $key2 = ".$dataDirs{$key2}."\n" if($DEBUG);
	}
}

$cgidir =~ s/\$//;
$cgidir =~ s/\(//;
$cgidir =~ s/\)//;
$htmldir =~ s/\$//;
$htmldir =~ s/\(//;
$htmldir =~ s/\)//;

print "CGIDIR = $cgidir\n";
print "HTMLDIR = $htmldir\n";

#
# ASSERT all dirs are found
#
if($cgidir eq "") { die "CGIDIR is not found in Makefile\n"; }
if($htmldir eq "") { die "HTMLDIR is not found in Makefile\n"; }
foreach my $key2 (keys %dataDirs) 
{
	if($dataDirs{$key2} eq "") { die $dataDirs{$key2}." is not found in Makefile\n"; }
}


#
# Look for DB access credentials
#
print "find include/localy.h";
unless(open(F, "include/localy.h"))
{
	print "\n(can't open include/localy.h)\n";
	die;
}
print "....\n";


while (<F>)
{
	$str = $_;
	if($str =~ /#define\s+DB_NAME\s*\"(.*)\"/)
	{
		$mysqldb = $1;
	}
	if($str =~ /#define\s+DB_LOGIN\s*\"(.*)\"/)
	{
		$mysqllogin = $1;
	}
	if($str =~ /#define\s+DB_PASSWORD\s*\"(.*)\"/)
	{
		$mysqlpass = $1;
	}
	if($str =~ /#define\s+DB_HOST\s*\"(.*)\"/)
	{
		$mysqlhost = $1;
	}
	if($str =~ /#define\s+LOG_FILE_NAME\s*\"(.*)\"/)
	{
		$logFileName = $1;
	}
	if($str =~ /#define\s+CHAT_LOG_FILE_NAME\s*\"(.*)\"/)
	{
		$chatLogFileName = $1;
	}
	if($str =~ /#define\s+ADMIN_LOG_FILE_NAME\s*\"(.*)\"/)
	{
		$adminLogFileName = $1;
	}
}
close(F);

unless(CheckValidityLoadedVariables())
{
	die "ERROR: reading variables any of following variable from include/localy.h: mysqldb, mysqllogin, mysqlhost, logFileName, chatLogFileName, adminLogFileName\n";
}

print "MYSQL db: $mysqldb\n";
print "MYSQL login: $mysqllogin\n";
print "MYSQL pass: $mysqlpass\n";
print "MYSQL host: $mysqlhost\n";

#
# Build archive filename
#
$archive_filename  = $mysqldb."_";
if($action eq "--backup")
{
	$archive_filename .= "full_";
}
if($action eq "--backup_structure")
{
	$archive_filename .= "struct_";
}
$archive_filename .= $timestamp.".tar.gz";


#
# Start main action
#

if(($action eq "--backup") or ($action eq "--backup_structure"))
{
	print "\n\nPerforming ".($action eq "--backup" ? "full" : "structure")." backup\n";
	print "make clean  ....\n";
	system("make clean");

	print "removing old files  ....\n";
	unlink 'sql', '*~', '$mysqldb.tar.gz';

	print "removing old dir  ....\n";
	remove_dir_recursively("./html");
	remove_dir_recursively("./cgi-bin");

	print "copying new dir  ....\n";
	system("cp -R $cgidir ./cgi-bin");
	system("cp -R $htmldir ./html");
	system("find ./cgi-bin -name *.cgi -exec rm -f {} \\;");

	if($action eq "--backup_structure")
	{
		foreach my $key (keys %dataDirs)
		{
			remove_data_dirs("./cgi-bin", $dataDirs{$key});
			remove_data_dirs("./html", $dataDirs{$key});
		}
	}

	print "mysqldump ....\n";

	if($action eq "--backup")
	{
		system("mysqldump  -Q -u $mysqllogin -p$mysqlpass -h $mysqlhost $mysqldb > sql");
	}
	if($action eq "--backup_structure")
	{
		system("mysqldump --no-data -Q -u $mysqllogin -p$mysqlpass -h $mysqlhost $mysqldb > sql");
	}

	print "archiving ....\n";
	system("tar -czf ".$archive_filename." *");
	if(isDevServer())
	{
		if(isMDev())
		{
			print "copying mob development environment ....\n";

			system("cp ".$archive_filename." /storage/ikuchin/backup/mdev.connme.ru/");
		}
		else
		{
			print "copying web development environment ....\n";

			system("cp ".$archive_filename." /storage/ikuchin/backup/dev.connme.ru/");
		}

	}
	else
	{
		if(isMobileVersion())
		{
			print "preparing to copy mob production environment ....\n";

		    system("scp ".$archive_filename." ikuchin\@home:/storage/ikuchin/backup/m.connme.ru/");
		}
		else
		{
			print "preparing to copy web production environment ....\n";

		    system("scp ".$archive_filename." ikuchin\@home:/storage/ikuchin/backup/www.connme.ru/");
		}
	}

	###
	### system("smbclient \\\\\\\\bigdisk\\\\ikuchin sdlkashhs -U ikuchin -c 'cd connme_backup; put ".$mysqldb."_".$timestamp.".tar.gz'");
	### system("smbclient \\\\\\\\192.168.168.1\\\\volume1 \"\" -c 'cd connme_backup; put ".$mysqldb."_".$timestamp.".tar.gz'");
	###

	unlink $archive_filename;
	unlink 'sql';
	remove_dir_recursively("./cgi-bin");
	remove_dir_recursively("./html");
	remove_dir_recursively("/home/ikuchin/home/");


	if(isDevServer() and ($action eq "--backup_structure"))
	{
		if(isMDev())
		{
		}
		else
		{
			print "\n ---recover cli: \nsudo date;cd ~ikuchin/src/www.connme.ru/;rm -r ./*;scp home:/storage/ikuchin/backup/dev.connme.ru/".$archive_filename." ~ikuchin/src/www.connme.ru/;tar -zxf ".$archive_filename.";time sudo ./archive.pl --restore_structure;sudo /etc/init.d/chatd stop;sleep 2;sudo /etc/init.d/chatd start;sleep 1;ps -aux | grep chat\n\n";
		}

	}
}
if(($action eq "--restore") or ($action eq "--restore_structure"))
{
	print "\n\nPerforming ".($action eq "--restore" ? "full" : "structure")." restore ($action)\n";

	print "remove old _tmp dirs ...\n";
	remove_dir_recursively("./cgi-bin_tmp");
	remove_dir_recursively("./html_tmp");
	remove_dir_recursively("./2delete");
	system("find ./cgi-bin -name *.cgi -exec rm -f {} \\;");

	if($action eq "--restore_structure")
	{
		print "copying existing dirs with _tmp suffix ....\n";

		#
		# create temporary dirs
		#
		mkdir "./cgi-bin_tmp";
		mkdir "./html_tmp";
		dircopy($cgidir, "./cgi-bin_tmp");
		dircopy($htmldir, "./html_tmp");
		system("find ./cgi-bin_tmp -name *.cgi -exec rm -f {} \\;");

		foreach my $key (keys %dataDirs)
		{
			copy_data_dirs("./cgi-bin_tmp", "./cgi-bin", $dataDirs{$key});
			copy_data_dirs("./html_tmp", "./html", $dataDirs{$key});
		}
		
		system("mysqldump --no-create-info --skip-add-drop-table -Q -u $mysqllogin -p$mysqlpass -h $mysqlhost $mysqldb >> sql");
	}

	
	#
	# remove production Web-server folders
	#
	remove_dir_recursively($cgidir);
	remove_dir_recursively($htmldir);

	print "restore DB ...\n";
	system("mysql -u $mysqllogin -p$mysqlpass -h $mysqlhost $mysqldb < sql");
	# system("mysql -u root 2delete < sql");

	dircopy("./cgi-bin", $cgidir);
	dircopy("./html", $htmldir);
	
	#
	# remove temporary dirs
	#
	remove_dir_recursively("./cgi-bin_tmp");
	remove_dir_recursively("./html_tmp");

	system("make clean");
	system("make -j 2");
	system("make install");
	system("make clean");

	print "create 'recover point' @ folder 2delete ...\n";
	mkdir "./2delete";
	mkdir "./2delete/cgi-bin";
	mkdir "./2delete/html";
	fmove("./sql", "./2delete/");
	dirmove("./cgi-bin", "./2delete/cgi-bin/");
	dirmove("./html", "./2delete/html/");

	#
	# change ownership of html and cgi-bin folders
	#
	print "fix the owner and access rights ...\n";
	system("chown -R ikuchin:ikuchin ./*");
	system("chown -R www-data:www-data $htmldir");
	system("chown -R www-data:www-data $cgidir");
	system("chmod -R g+w $htmldir");
	system("chmod -R g+w $cgidir");
	RemoveLogFiles();
	CreateLogFiles();
	FixAccessRightsToLogFiles();
}

sub	RemoveLogFiles
{
    unlink $logFileName;
    unlink $chatLogFileName;
    unlink $adminLogFileName;
}

sub	CreateLogFiles
{
    system("touch ".$logFileName);
    system("touch ".$chatLogFileName);
    system("touch ".$adminLogFileName);
}

sub FixAccessRightsToLogFiles
{
    system("chown ikuchin:www-data ".$logFileName);
    system("chown ikuchin:www-data ".$chatLogFileName);
    system("chown ikuchin:www-data ".$adminLogFileName);
    system("chmod g+w ".$logFileName);
    system("chmod g+w ".$chatLogFileName);
    system("chmod g+w ".$adminLogFileName);
}

sub copy_data_dirs
{
    my ($from_dir, $to_dir, $regex_dir) = @_;
    # print "copy_data_dirs: start ($from_dir, $regex_dir)\n";
    opendir my($dh), $from_dir or die "copy_data_dirs: Could not open dir '$from_dir': $!";
    for my $entry (readdir $dh) {
    	# if(-d $from_dir."/".$entry)
    	# {
	    # 	print "--- $entry\n";
    	# }
        if(($entry =~ /^$regex_dir$/) and (-d $from_dir."/".$entry))
        {
	        print "copy_data_dirs: cp $from_dir/$entry $to_dir/$entry\n" if($DEBUG);
    		if(-e $to_dir."/".$entry and -d $to_dir."/".$entry)
    		{
		        remove_dir_recursively($to_dir."/".$entry);
    		}
			mkdir $to_dir."/".$entry; 

	        my($num_of_files_and_dirs,$num_of_dirs,$depth_traversed) = dircopy($from_dir."/".$entry, $to_dir."/".$entry);

	        next;
        }
    	if(!($entry eq "..") and !($entry eq ".") and (-d $from_dir."/".$entry))
    	{
    		copy_data_dirs($from_dir."/".$entry, $to_dir."/".$entry, $regex_dir);
    	}
    }
    closedir $dh;
    return;
}

sub CheckValidityLoadedVariables
{
	my $result;

	$result = 1;
	$result = 0 unless(length $mysqldb);
	$result = 0 unless(length $mysqllogin);
	$result = 0 unless(length $mysqlhost);
	$result = 0 unless(length $logFileName);
	$result = 0 unless(length $chatLogFileName);
	$result = 0 unless(length $adminLogFileName);

	return $result;
}

sub remove_data_dirs
{
    my ($from_dir, $regex_dir) = @_;
    # print "remove_data_dirs: start ($from_dir, $regex_dir)\n";
    opendir my($dh), $from_dir or die "remove_data_dirs: Could not open dir '$from_dir': $!";
    for my $entry (readdir $dh) {
    	# if(-d $from_dir."/".$entry)
    	# {
	    # 	print "--- $entry\n";
    	# }
        if(($entry =~ /^$regex_dir$/) and (-d $from_dir."/".$entry))
        {
	        print "remove_data_dirs: removing directory with mask ($entry) \n" if($DEBUG);
	        remove_dir_recursively($from_dir."/".$entry);
	        next;
        }
    	if(!($entry eq "..") and !($entry eq ".") and (-d $from_dir."/".$entry))
    	{
    		remove_data_dirs($from_dir."/".$entry, $regex_dir);
    	}
    }
    closedir $dh;
    return;
}

sub remove_dir_recursively
{
	my	($dir_to_remove) = @_;

	rmtree($dir_to_remove, {error => \my $err} );
	if (@$err) {
	  for my $diag (@$err) {
	      my ($file, $message) = %$diag;
	      if ($file eq '') {
	          print "general error: $message\n";
	      }
	      else {
	          print "problem unlinking $file: $message\n";
	      }
	  }
	}
	else {
	  print "removing [".$dir_to_remove."] success\n";
	}
}

sub isRoot()
{
	my		$login;

	$login = getpwuid($>);
	return ($login eq "root" ? 1 : 0);
}

sub isDevServer()
{
	my 		$HOSTNAME = `hostname -s`;

	chomp($HOSTNAME);
	return ($HOSTNAME eq "dev" ? 1 : 0);
}

sub isMDev()
{
	my	$currentPath = getcwd;

	return ($currentPath =~ /\/mdev\./) && 1 || 0;
}

sub isMobileVersion()
{
	my	$currentPath = getcwd;

	return ($currentPath =~ /\/m\./) && 1 || 0;
}

sub isDev()
{
	my	$currentPath = getcwd;

	return ($currentPath =~ /\/dev\./) && 1 || 0;
}

sub isDesktopVersion()
{
	my	$currentPath = getcwd;

	return ($currentPath =~ /\/www\./) && 1 || 0;
}

sub isFilesReadyToRestore()
{
	unless(-f "Makefile")
	{
		print "ERROR: Makefile is missed\n";
		return 0;
	}
	unless(-f "sql")
	{
		print "ERROR: sql is missed\n";
		return 0;
	}
	unless(-f "index.cpp")
	{
		print "ERROR: index.cpp is missed\n";
		return 0;
	}
	unless(-f "include/localy.h")
	{
		print "ERROR: index.cpp is missed\n";
		return 0;
	}
	unless(-d "html")
	{
		print "ERROR: html dir is missed\n";
		return 0;
	}
	unless(-d "cgi-bin")
	{
		print "ERROR: cgi-bin is missed\n";
		return 0;
	}
	unless(-d "include")
	{
		print "ERROR: include is missed\n";
		return 0;
	}
	return 1;
}

sub ChangeDevToWWW
{
    my	($filenameFrom) = @_;
    my	($filenameTo) = $filenameFrom.".tmp";

    print "sed -e \"s/dev./www./\" $filenameFrom\n";

    open my $inF, "<", $filenameFrom or die "ChangeDevToWWW: ERROR: $filenameFrom is not exists\n";
    open my $outF, ">", $filenameTo or die "ChangeDevToWWW: ERROR: can't open $filenameTo\n";

    while(<$inF>)
    {
	s/\/dev\./\/www\./g;
	print $outF $_;
    }

    close $inF;
    close $outF;

    unlink $filenameFrom;
    rename $filenameTo, $filenameFrom;
}

sub ChangeIPDevToIPWWW
{
    my	($filenameFrom) = @_;
    my	($filenameTo) = $filenameFrom.".tmp";

    print "sed -e \"s/192\.168\.168\.12/90\.156\.142\.18/\" $filenameFrom\n";

    open my $inF, "<", $filenameFrom or die "ChangeDevToWWW: ERROR: $filenameFrom is not exists\n";
    open my $outF, ">", $filenameTo or die "ChangeDevToWWW: ERROR: can't open $filenameTo\n";

    while(<$inF>)
    {
	s/192\.168\.168\.12/90\.156\.142\.18/g;
	print $outF $_;
    }

    close $inF;
    close $outF;

    unlink $filenameFrom;
    rename $filenameTo, $filenameFrom;
}

sub usage
{
	print "ERROR: ".$ARGV[0]." is not correct parameter\n\n";
	print "usage: archive.pl [OPT]\n";
	print " --backup - full backup\n";
	print " --backup_structure - structure backup\n";
	print " --restore - restore from archive\n";
	print " --restore_structure - restore structure only\n";
}