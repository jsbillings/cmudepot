\begindata{text, 17095340}
\textdsversion{12}
\template{roff}




\begindata{bp,17238420}
\enddata{bp,17238420}
\view{bpv,17238420,0,0,0}








\majorheading{25 September 1991        CUSTOM.DEPOT(5)

}

\indent1{
\tempindentneg1{
NAME 

}custom.depot - depot customization instructions 

\tempindentneg1{
SYNOPSIS 

}\bold{[targetdir]/depot/custom.depot }

\tempindentneg1{
DESCRIPTION 

}The file \italic{custom.depot }contains user-specified values for resources 
for \bold{depot}(1) which may be used to tailor the behavior of 
\bold{depot}(1) as desired. Resources are specified as strings of the form 


\italic{collection-name.preference: value} or \italic{commandlabel.preference: 
value} or \italic{preference: value} 


one on each line in the \italic{custom.depot} file. 


The first form is used to specify resources which are to be applied on a per 
collection basis. An asterisk (*) may be used in place of the collection-name 
to instruct \bold{depot}(1) to apply the preference to all collections. 


The second form is used to specify resources which are to be applied to 
command labels specified in the configuration files of collections (see 
\bold{depot.conf(5)}). 


The third form is used for resources which apply to the \bold{depot}(1) 
program in general. 


Lines in \bold{custom.depot} starting with a # are treated as comments and 
ignored by \bold{depot}(1). 


\bold{Depot}(1) recognizes the following resources: 


}\tempindentneg1{\indent2{label.command: commandname argumentlist 

}}\indent2{specifies the actual command and list of arguments to be associated 
with the given label. The magic sequence %t may be used in the commandname and 
argumentlist to refer to the \italic{targetdir}. If the commandname does not 
start with either a %t or a /, it is assumed to be an executable to be 
searched for using the PATH environment variable. 


}\tempindentneg1{\indent2{copytarget: filelist 

}}\indent2{specifies a list of files (or directories!) under \italic{targetdir} 
which \bold{depot}(1) is to install by making copies of the appropriate 
sources. This preference supersedes any collection-specific mapcommand 
preference for these files. This preference also supersedes any linktarget 
preference for these files. If a directory is specified then all the files and 
subdirectories will be copied. The list of files must be separated by commas 
with no intervening whitespaces. 


}\tempindentneg1{\indent2{deletetarget: filelist 

}}\indent2{specifies a list of files (or directories!) under \italic{targetdir} 
which \bold{depot}(1) is to delete if any of them exist. This preference 
supersedes any attempt at installation from any collection for these files. 
This preference also supersedes any copytarget or linktarget preference for 
these files. If a directory is specified then all the files and subdirectories 
will be deleted. The list of files must be separated by commas with no 
intervening whitespaces. 


}\tempindentneg1{\indent2{deleteunreferenced: true/false 

}}\indent2{specifies whether \bold{depot}(1) is to delete or maintain files 
under \italic{targetdir} which are not referenced by any collection in the 
current database. Defaults to true. 


}\tempindentneg1{\indent2{ignore: collectionlist 

}}\indent2{specifies a list of possible collections which will not be 
installed by \bold{depot}(1). The collection names must be separated by commas 
with no intervening whitespaces. Multiple instances of ignore will be 
concatenated together. 


}\tempindentneg1{\indent2{linktarget: filelist 

}}\indent2{specifies a list of files (or directories!) under \italic{targetdir} 
which \bold{depot}(1) is to install by making links to the appropriate 
sources. This preference supersedes any collection-specific mapcommand 
preference for these files. If a directory is specified then all the files and 
subdirectories will be linked. The list of files must be separated by commas 
with no intervening whitespaces. 


}\tempindentneg1{\indent2{collection.mapcommand: copy/link 

}}\indent2{specifies whether \bold{depot}(1) installs software from the named 
collection by making copies at the appropriate location under the 
\italic{targetdir} or by making symbolic links instead. Defaults to link. 

\bold{CAVEAT:} \bold{depot}(1) will make EXACT copies of links if mapcommand 
is set to copy. Any manipulation with the \bold{depot.conf} will not change 
what the link points to. Thus, if a relative symlink is moved into a different 
directory with the \bold{depot.conf} the link could end up pointing at the 
wrong item. 


}\tempindentneg1{\indent2{nooptarget: filelist 

}}\indent2{specifies a list of files (or directories!) under \italic{targetdir} 
which \bold{depot}(1) is to not install, update or modify in any way during 
its operation. This preference supersedes any attempt at installation from any 
collection for these files. This preference also supersedes any copytarget, 
deletetarget or linktarget preference for these files. If a directory is 
specified then all the files and subdirectories will be ignored. The list of 
files must be separated by commas with no intervening whitespaces. 


}\tempindentneg1{\indent2{collection.override: collectionlist 

}}\indent2{specifies a list of collections whose software contributions may be 
overwritten by the named collection in case of a conflict during installation. 
Multiple overriders will be concatenated together. 


}\tempindentneg1{\indent2{collection.path: path 

}}\indent2{specifies the directory where software for the named collection 
resides. The path must be either an absolute pathname or a path relative to 
the \italic{targetdir}. 


}\tempindentneg1{\indent2{collection.searchpath: pathlist 

}}\indent2{specifies a list of directories under which \bold{depot}(1) is to 
search for the named collection if no specific path has been specified for the 
collection. The pathnames must be either absolute pathnames or paths relative 
to the \italic{targetdir} and must be separated by commas with no intervening 
whitespaces. A collection for which no searchpath is specified is searched for 
under [targetdir]/depot. 


}\tempindentneg1{\indent2{specialfile: filelist 

}}\indent2{specifies a list of files (or directories!) under \italic{targetdir} 
which \bold{depot}(1) should not attempt to update or modify in any way during 
its operation. Any attempt by any collection to modify any file or directory 
tree specified in the filelist will cause \bold{depot} to exit with an error 
message. The list of files must be separated by commas with no intervening 
whitespaces. Multiple instances of specialfile will be concatenated together. 


}\tempindentneg1{\indent2{usemodtimes: true/false 

}}\indent2{specifies whether \bold{depot}(1) is to use modification time 
information for updating maps by copy. Defaults to false. May be overridden by 
the -t flag while running \bold{depot}(1). 


}\tempindentneg1{\indent2{collection.version: versionnumber 

}}\indent2{specifies the version of the named collection which \bold{depot}(1) 
is to use during installation. If no version is specified, the version found 
with the highest version number is used by \bold{depot}(1). Versions for 
collections are specified by appending a version number following a delimiter 
to the collection name. For example, 


With the default version delimiter of ~, Version 11 of the collection named 
foo would be specified as 


foo~11 


}\tempindentneg1{\indent2{versiondelimiter: character 

}}\indent2{specifies the character used to delimit version numbers. The 
default delimiter is a tilde(~). 

}\indent1{\tempindentneg1{
AUTHOR 

}Sohan C. Ramakrishna-Pillai 

\tempindentneg1{
SEE ALSO 

}depot(1) 

}\enddata{text,17095340}
