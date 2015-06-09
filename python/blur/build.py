
import os
import sys
import string
import re
import shutil
import traceback
import cPickle
import ConfigParser
import exceptions
import psutil
from defaultdict import *

All_Targets = []
Targets = []
Config_Replacement_File = None
Args = []
Generated_Installers = {}

try:
    import subprocess
except:
    import popen2
	
class BlurException(Exception):
    def __init__(self, value):
        self.value = value
    def __str__(self):
        return repr(self.value)

class TerminalControllerDummy:
    def render(self,text):
        return re.sub( r'\${\w+}', '', text )
try:
    from termctl import TerminalController
except:
    TerminalController = TerminalControllerDummy

term = TerminalControllerDummy()

def find_targets(name):
    ret = []
    for t in All_Targets:
        if t.name == name or re.match('^' + name + '$',t.name):
            ret.append(t)
    return ret

def find_target(name):
    return find_targets(name)[0]

def add_target(target):
    try:
        if find_target(target) == target:
            # Same target added twice
            pass
        else:
            raise ("Adding target with duplicate name " + target.name)
    except:
        All_Targets.append(target)


# Returns a tuple containing (returncode,stdout)
# Cmd can be a string or a list of args
def cmd_output(cmd,outputObject=None,shell=None):
    p = None
    outputFd = None
    pollRunVal = None
    if shell is None:
        shell = sys.platform != 'win32'
    try:
        if 'subprocess' in globals():
            p = subprocess.Popen(cmd,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,shell=shell)
            outputFd = p.stdout
        else:
            p = popen2.Popen4(cmd)
            pollRunVal = -1
            outputFd = p.fromchild
    except Exception, e:
        print "Error starting command: " + str(cmd)
        raise e

    output = ''
    ret = 0
    def processOutput(existing, new, outputProgress):
        existing += new
        if outputProgress:
            outputObject.output(new)
        return existing
    while True:
        ret = p.poll()
        if ret == pollRunVal:
            output = processOutput(output,outputFd.readline(),outputObject)
        else:
            break
    
    return (ret,processOutput(output,outputFd.read(),outputObject))

# Baseclass for all targets
class Target:
    def __init__(self,name,dir,pre_deps=[],post_deps=[]):
        self.name = name
        self.dir = dir
        self.pre_deps = pre_deps
        self.post_deps = post_deps
        self.built = False
        self.args = []
        
        # "Registers" this target
        add_target(self)
        
    def __repr__(self):
        return self.name
    
    def check_arg_sanity(self):
        if self.has_arg('install') and self.has_arg('clean') and not self.has_arg('build'):
            print term.render("${RED}Asked to clean then install without building. Bailing out${NORMAL}")
            sys.exit(1)

    # Returns true if this target has already been built
    # Some targets have different build steps, and can
    # return different values depending on the args
    def is_built(self):
        return self.built
    
    def apply_arg(self,arg):
        if arg is not None:
            try:
                for a in arg: self.args.append(a)
            except:
                self.args.append(arg)
        
    # Returns true if the argument list contains arg or
    # contains TARGET:arg
    def has_arg(self,arg):
        # Local args
        for a in self.args:
            if arg == a:
                return True
        # Global args
        sa = '-'+arg
        for a in Args:
            if arg == a or sa == a:
                return True
        return False
    
    # Returns a string to execute using os.system to complete the target
    # Simplified method for creating simpler targets that
    # dont need to run multiple commands or study any output
    #
    # The return value of the command indicates the whether
    # the target was completed. 0 for success
    def command(self):
        return ''
    
    def output(self,output):
        if self.has_arg('verbose'):
            print output,
        elif self.has_arg('progress'):
            for line in output.splitlines():
                match = re.match("^(g\+\+|gcc)",line)
                if match:
                    # Compile
                    if re.search(r"\s-c\s",line):
                        match = re.search(r'([\w\._-]+)\s*$',line)
                        if match:
                            print "Compiling", match.group(1)
                    # Link
                    else:
                        match = re.search(r'-o\s+([\w\._-]+)',line)
                        if match:
                            print "Linking", match.group(1)
                match = re.match(r"^\S+uic\s+(\S+)",line)
                if match:
                    print "Uic", match.group(1)
    
    def cmd_error(self,cmd,output):
        if not self.has_arg('verbose'):
            print output,
        print term.render("${RED}Error Building Target${NORMAL}: %s, cmd was: %s" % (self.name,cmd))
        raise Exception()
        
    def run_cmd(self,cmd,shell=None,noThrow=False):
        if self.has_arg('verbose') or self.has_arg('show-commands'):
            print term.render('${BLUE}Running Command${NORMAL}:'), str(cmd)
        try:
            (ret,output) = cmd_output(cmd,self,shell)
        except:
            print "Exception while running cmd:", cmd
            traceback.print_exc()
            raise
        if ret and not noThrow:
            self.cmd_error(cmd,output)
        return (ret,output)
        
    def run_make(self, arg_string=''):
        make_cmd = 'make'
        if 'QMAKESPEC' in os.environ and 'msvc' in os.environ['QMAKESPEC']:
            make_cmd = 'nmake'
        if arg_string and arg_string[0] != ' ':
            arg_string = ' ' + arg_string
        return self.run_cmd(make_cmd + arg_string)

    # Central function of a Target, responsible for completing the target.
    # Raises an exception if it cannot complete the target
    def build_run(self):
        cmd = self.command()
        if cmd and len(cmd):
            self.run_cmd(cmd)
    
    # This is used to check whether the given target is buildable.
    # A target may only be buildable on certain systems, or when
    # certain requirements are met
    def is_buildable(self):
        return True
    
    # This builds all the dependencies for this target
    # If the 'skip-ext-deps' option is passed, targets
    # that are specified as string are ignored, otherwise
    # a string target is looked up in the All_Targets list
    # with the find_target function.
    def build_deps(self,deps):
        skipext = self.has_arg('skip-ext-deps')
        for d in deps:
            if isinstance(d,str) and not skipext:
                if ':' in d:
                    parts = d.split(':')
                    #local_args.append(d)
                    d = parts[0]
                try:
                    d = find_target(d)
                except:
                    raise BlurException("Target.build_deps: couldn't find dependancy: %s for target: %s" % (d, self.name))
            if isinstance(d,Target):
                d.build()
    
    # Builds the target using by building the deps and callign build_run
    # Skips build if name:skip exists in the args list.  Changes directories
    # to the target directory if there is one
    def build(self):
        if self.has_arg('skip') or self.is_built():
            return
        if not self.is_buildable():
            return
        self.build_deps(self.pre_deps)
        cwd = os.getcwd()
        nwd = os.path.join(cwd,self.dir)
        print term.render("${YELLOW}Building${NORMAL}: %s\t\t%s" % (self.name, nwd))
        #print "Target.build: doing ", self.name
        #print "Target.build: chdir to ", nwd
        os.chdir( nwd )
        self.build_run()
        os.chdir(cwd)
        self.built = True
        self.build_deps(self.post_deps)

# Executes a static command inside dir
class StaticTarget(Target):
    def __init__(self,name,dir,cmd,pre_deps=[],post_deps=[],shell=False):
        Target.__init__(self,name,dir,pre_deps,post_deps)
        self.cmd = cmd
        self.shell = shell
        
    def command(self):
        return self.cmd

    def build_run(self):
        self.check_arg_sanity()

        if self.has_arg('build'):
            if not self.built:
                cmd = self.command()
                if cmd and len(cmd):
                    self.run_cmd(cmd,shell=self.shell)

# Copies a single file
class CopyTarget(Target):
    def __init__(self,name,dir,src,dest):
        Target.__init__(self,name,dir)
        self.Source = src
        self.Dest = dest
        
    def build_run(self):
        shutil.copyfile(self.Source,self.Dest)

# Run configure.py, make, and optionally make install
# for building sip targets, these are python bindings
# including pyqt, pystone, pyclasses, various application
# interfaces, and whatever else is added...
class SipTarget(Target):
    def __init__(self,name,dir,static=False,platform=None,pre_deps=[]):
        Target.__init__(self,name,dir,pre_deps)
        self.Static = static
        self.Platform = platform
        self.CleanDone = False
        self.InstallDone = False
        self.config = ""
        self.name=name

    def is_built(self):
        if self.has_arg('clean') and not self.CleanDone:
            return False
        if self.has_arg('install') and not self.InstallDone:
            return False
        return Target.is_built(self)

    def configure_command(self):
        pass

    def build_run(self):
        self.check_arg_sanity()

        if os.environ.has_key('PYTHON'):
            self.config = os.environ['PYTHON'] + " configure.py"
        else:
            self.config = "python configure.py"
        if self.Static:
            self.config += " -k"
        if self.Platform:
            self.config += " -p " + self.Platform
        if self.has_arg("debug"):
            self.config += " -u"
        if self.has_arg("trace"):
            self.config += " -r"

        if self.has_arg('build') or (not os.path.exists(os.getcwd() + 'Makefile') and not self.name.startswith('py') ):
            self.configure_command()
            self.run_cmd(self.config)
        if self.has_arg('clean') and not self.CleanDone:
            self.run_make('clean')
            self.CleanDone = True
            self.built = False
            self.InstallDone = False
            if os.name == 'nt':
                wantedName = self.name.replace("static","").replace("py","",1)
                print "Cleaning sip working dir"
                if os.path.isfile('sip' + wantedName + '/' + wantedName + '.lib'):
                    os.remove('sip' + wantedName + '/' + wantedName + '.lib')
                if os.path.isfile('sip' + wantedName + '/py' + wantedName + '.lib'):
                    os.remove('sip' + wantedName + '/py' + wantedName + '.lib')
        if self.has_arg('build'):
            self.run_make()
            self.built = True
            if os.name == 'nt':
                wantedName = self.name.replace("static","").replace("py","",1)
                if self.has_arg("debug"):
                    wantedName += "_d"
                print "Checking for the existance of (%s)" % ('sip' + wantedName + '/' + wantedName + '.lib')
                if os.path.isfile('sip' + wantedName + '/' + wantedName + '.lib'):
                    shutil.copyfile('sip' + wantedName + '/' + wantedName + '.lib', 'sip' + wantedName + '/py' + wantedName + '.lib')
                    os.remove('sip' + wantedName + '/' + wantedName + '.lib')
                #if os.path.isfile('sip' + wantedName + '/py' + wantedName + '.lib'):
                    #os.remove('sip' + wantedName + '/py' + wantedName + '.lib')
        if self.has_arg('install') and not self.InstallDone:
            cmd = 'install'
            try:
                pos = args.index('-install-root')
                cmd += ' INSTALL_ROOT=' + args[pos+1]
            except: pass
            self.run_make(cmd)
            self.InstallDone = True

# Builds a Qt Project(.pro) using qmake and make
# Runs qmake with optional debug and console
# Runs make clean(optional), make, and make install(optional)
#
# make clean is done if args contains 'clean' or 'TARGET:clean'
# make install is done if args contains 'install' or 'TARGET:install' 
class QMakeTarget(Target):
    def __init__(self,name,dir,target=None,pre_deps=[],post_deps=[],Static=False):
        Target.__init__(self,name,dir,pre_deps,post_deps)
        self.Static = Static
        self.Target = target
        self.UicOnly = False
        self.Defines = []
        self.ConfigDone = False
        self.CleanDone = False
        self.BuildDone = False
        self.InstallDone = False
        
    def is_built(self):
        if self.UicOnly and self.built:
            return True
        if self.has_arg('clean') and not self.CleanDone:
            return False
        if self.has_arg('install') and not self.InstallDone:
            return False
        return self.ConfigDone and self.BuildDone
    
    # Returns the arguments to qmake
    # Override if you need special args
    def qmakeargs(self):
        Args = []
        if self.Static:
            Args.append("CONFIG+=staticlib")
        if self.has_arg("debug")>0:
            Args.append("CONFIG+=debug")
        else:
            Args.append("CONFIG-=debug")
        if self.has_arg("console"):
            Args.append("CONFIG+=console")
        for d in self.Defines:
            Args.append("DEFINES+=\"" + d + "\"")
        if os.environ.has_key('PYTHON'):
            Args.append("PYTHON="+os.environ['PYTHON'])
        if self.Target:
            Args.append(self.Target)
        return string.join(Args,' ')
    
    # Runs qmake, make clean(option), make, make install(option)
    def build_run(self):
        self.check_arg_sanity()

        if not self.ConfigDone and self.has_arg("build"):
            cmd = "qmake " + self.qmakeargs()
            self.run_cmd(cmd)
            self.ConfigDone = True
        if self.has_arg("clean") and not self.CleanDone:
            self.run_make('clean')
            self.BuildDone = self.InstallDone = False
            self.CleanDone = True
        if self.UicOnly:
            if sys.platform=='win32':
                self.run_make("-f Makefile.release compiler_uic_make_all")
            else:
                self.run_make("compiler_uic_make_all")
            return
        if not self.BuildDone and self.has_arg("build"):
            self.run_make()
            self.BuildDone = True
            self.built = True
        if not self.InstallDone and self.has_arg('install'):
            self.run_make('install')
            self.InstallDone = True

# Finds the makensis command and runs it on the specified file
# Nullsoft install - used to generate windows installers
class NSISTarget(Target):
    if sys.platform=='win32':
        NSIS_PATHS = ["C:/Program Files (x86)/NSIS/","C:/Program Files/NSIS/","E:/Program Files (x86)/NSIS/","E:/Program Files/NSIS/"]
        NSIS = "makensis.exe"
        CanBuild = True
    else:
        CanBuild = False
    def __init__(self,name,dir,file,pre_deps=[],makensis_extra_options=[], revdir=None):
        Target.__init__(self,name,dir,pre_deps)
        self.File = file
        self.ExtraOptions = makensis_extra_options
        self.RevDir = revdir
        if not self.RevDir:
            self.RevDir = dir

    # Only buildable on win32
    def is_buildable(self):
        return NSISTarget.CanBuild

    def find_nsis(self):
        for p in self.NSIS_PATHS:
            if os.access(p,os.F_OK):
                return p + "makensis.exe"
		raise Exception("Couldn't find nsis cmd, searched " + str(self.NSIS_PATHS))
        return None
    
    def makensis_options(self):
        p = self.find_nsis()
        file = os.getcwd() + "/" + self.File
        cmd_parts = [p]
        plat = 'Unkown'
        if 'QMAKESPEC' in os.environ:
            plat = os.environ['QMAKESPEC']
        if self.has_arg('X86_64'):
            plat += '_64'
        cmd_parts.append( '/DPLATFORM=%s' % plat )
        rev = GetRevision(self.RevDir)
        cmd_parts.append( '/DREVISION=%s' % rev )
        if self.ExtraOptions:
            cmd_parts += self.ExtraOptions
        cmd_parts.append(file)
        return cmd_parts
        
    def build_run(self):
        cmd_parts = self.makensis_options()
        (ret,output) = self.run_cmd( cmd_parts )
        outputFile = None
        for line in output.splitlines():
            outputMatch = re.match( r'Output:\s+"(.*)"', line )
            if outputMatch:
                outputFile = outputMatch.group(1)
        if outputFile is None:
            raise ("Unable to parse output file from output\n"+output)
        Generated_Installers[self.name] = outputFile
        if self.has_arg('install'):
            if self.has_arg('progress') or self.has_arg('-verbose'):
                print term.render("${YELLOW}Installing${NORMAL}"), outputFile
            self.run_cmd( [outputFile,'/S'] )

class KillTarget(Target):
    def __init__(self, name, path, applications):
        Target.__init__(self,name, path)
        self.name = name
        self.apps = applications
    def is_buildable(self):
        return True
    def build_run(self):
        for proc in psutil.process_iter():
            try:
                if proc.name() in self.apps:
                    print "Terminating process (%s)" % (proc.name())
                    try:
                        proc.kill()
                    except AccessDenied, ad:
                        print "Unable to kill the process"
            except:
                # We can't read process names of system owned procs, so we ignore these errors
                pass
			
# Finds the subwcrev.exe program
# Part of toroisesvn used to get revision info for a file/dir
def find_wcrev():
    WCREV_PATHS = ["C:/Program Files/TortoiseSVN/bin/","C:/Program Files (x86)/TortoiseSVN/bin/","C:/blur/TortoiseSVN/bin/"]
    for p in WCREV_PATHS:
        if os.access(p,os.F_OK):
            return p + "/" + "subwcrev.exe"
    raise Exception("Couldn't find subwcrev.exe, searched " + WCREV_PATHS)
    return None

def isSubversion(dir):
    return os.path.exists(os.path.join(dir,'.svn'))

def GetRevision_nocache(dir):
    if not isSubversion(dir):
        return 0
    if sys.platform == 'win32':
        wcrev = find_wcrev()
        (ret,output) = cmd_output([wcrev,dir])
        m = re.search("Updated to revision (\d+)",output)
        if m != None:
            return m.group(1)
        m = re.search("Last committed at revision (\d+)",output)
        if m != None:
            return m.group(1)
        raise "Couldn't parse valid revision: " + output
    else:
        (ret,output) = cmd_output('svnversion ' + dir)
        m = re.search("(\d+)",output)
        if m != None:
            return m.group(1)
    return None

# Gets the svn revision from the current directory
rev_cache = {}
def GetRevision(dir):
    global rev_cache
    if dir in rev_cache:
        return rev_cache[dir]
    rev = GetRevision_nocache(dir)
    if rev:
        rev_cache[dir] = rev
    return rev

# Takes a template file, runs subwcrev.exe on it and outputs to output
class WCRevTarget(Target):
    def __init__(self,name,dir,revdir,input,output):
        Target.__init__(self,name,dir)
        self.Input = input
        self.Output = output
        self.Revdir = revdir
    
    def build_run(self):
        if not isSubversion(self.Revdir):
            shutil.copyfile(self.Input,self.Output)
            return
        if sys.platform == 'win32':
            p = find_wcrev()
            self.run_cmd( [p,self.Revdir,self.Input,self.Output,"-f"] )
        else:
            rev = GetRevision(self.Revdir)
            self.run_cmd( "cat " + self.Input + " | sed 's/\\$WCREV\\$/" + str(rev) + "/' > " + self.Output )

# Takes a template .ini file, and sets certain keys based the inputed IniConfig object
class IniConfigTarget(Target):
    def __init__(self,name,dir,template_ini,output_ini,install_dir = None, config = None):
        Target.__init__(self,name,dir)
        self.TemplateIni = template_ini
        self.OutputIni = output_ini
        self.Config = config
        self.InstallDir = install_dir
        
    def build_run(self):
        replacementsBySection = DefaultDict(dict)
        if not self.Config:
            self.Config = Config_Replacement_File
        section = "Default"
        replacementsFile = None
        try:
            replacementsFile = open(self.Config)
        except:
            print ("Failure Reading %s for config replacement" % str(self.Config))
        if replacementsFile:
            for line in replacementsFile:
                matchSection = re.match("^\[(.*)\]$",line)
                if matchSection:
                    section = matchSection.group(1)
                    continue
                matchKV = re.match("^([^=]*)=(.*)$",line)
                if matchKV:
                    replacementsBySection[section][matchKV.group(1)] = matchKV.group(2) 
        
        outLines = []
        section = "Default"
        for line in open(self.TemplateIni):
            matchSection = re.match("^\[(.*)\]$",line)
            if matchSection:
                section = matchSection.group(1)
                outLines.append(line)
                continue
            matchKV = re.match("^([^=]*)=(.*)$",line)
            if matchKV:
                key = matchKV.group(1)
                if key in replacementsBySection[section]:
                    outLines.append( "%s=%s\n" % (key, replacementsBySection[section][key] ) )
                    continue
            outLines.append(line)
        
        open(self.OutputIni,"w").write( ''.join(outLines) )
        if self.has_arg('install') and not self.InstallDir == None:
            if self.has_arg('verbose'):
                print "${GREEN}Installing config file${NORMAL} %s" % (self.InstallDir + "/" + self.OutputIni)

            if not os.path.exists(self.InstallDir):
                os.makedirs(self.InstallDir)
            shutil.copy2(self.OutputIni, self.InstallDir + "/" + self.OutputIni)

# Copies a file, replacing part of the name with the
# svn revisision number from revdir, which is relative
# to dir(or absolute)
class RevCopyTarget(Target):
    def __init__(self,name,dir,revdir,src,dest):
        Target.__init__(self,name,dir)
        self.RevDir = revdir
        self.Source = src
        self.Dest = dest
    
    def build_run(self):
        rev = GetRevision(self.RevDir)
        dest = re.sub("{REVSTR}",rev,self.Dest)
        shutil.copyfile(self.Source,dest)

class LibVersionTarget(Target):
    def __init__(self,name,dir,revdir,library):
        Target.__init__(self,name,dir)
        self.RevDir = revdir
        self.Library = library
        
    def build_run(self):
        if sys.platform == 'win32':
            rev = GetRevision(self.RevDir)
            shutil.copyfile(self.Library+".dll",self.Library+str(rev)+".dll")
            shutil.copyfile("lib"+self.Library+".a","lib" + self.Library+str(rev)+".a")
        else:
            return

class LibInstallTarget(Target):
    def __init__(self,name,dir,library,dest):
        Target.__init__(self,name,dir)
        self.Library = library
        self.Dest = dest

    def build_run(self):
        libname = self.Library
        dest = self.Dest
        if sys.platform == 'win32':
            libname = libname + '.dll'
        else:
            libname = 'lib' + libname + '.so'
        destpath = dest + libname
        shutil.copyfile(libname,destpath)

class RPMTarget(Target):
    CanBuildRpms = None
    """ # This class is used to build rpms
        # It currently has 5 steps
        # 1. Create a tarball named packageName-Version-rRevision.tgz in /usr/src/redhat/SOURCES/
        # 2. Create a spec file, uses a template and replaces $VERSION$ and $WCREV$ in /usr/src/redhat/SPECS/
        # 3. Run rpmbuild on the created spec file.
        # 4. Parse the output from rpmbuild to generate a list of created rpms
        # 5(optional, depeding on install arg). Install the rpms, except for debug-info and source rpm
    """
    def __init__(self,targetName,packageName,dir,specTemplate,version,pre_deps):
        """  for targetName and dir refer to Target docs
        "    packageName is the name of the .tgz package ie  packageName-Version-rRevision.tgz
        "    specTemplate is the path to the spec template file, relative to the file the RPMTarget is
        "       constructed in
        "    version is the Version for the rpm, the revision is taken using GetRevision on dir
        """
        Target.__init__(self,targetName,dir,pre_deps)
        self.SpecTemplate = specTemplate
        self.Version = version
        self.Revision = GetRevision(dir)
        self.PackageName = packageName
        self.BuiltRPMS = []
        self.InstallDone = False
        self.pre_deps = pre_deps
        self.BuildRoot = '/usr/src/redhat/'
        try:
            if os.path.exists('/etc/redhat-release'):
                match = re.search( 'release ([\d\.]+)', open('/etc/redhat-release','r').read())
                if match and float(match.group(1)) >= 6.0:
                    self.BuildRoot = '/root/rpmbuild/'
        except: pass
    
    # Only buildable on linux, this should probably check for the existance
    # of rpmbuild and other required commands.
    def is_buildable(self):
        if RPMTarget.CanBuildRpms == None:
            if sys.platform == 'win32':
                RPMTarget.CanBuildRpms = False
                return False
            RPMTarget.CanBuildRpms = True
        return RPMTarget.CanBuildRpms
    
    def is_built(self):
        if self.has_arg('install') and not self.InstallDone:
            return False
        return Target.is_built(self)
    
    def build_run(self):
        destDir = ""
        buildRoot = ""
        if "DESTDIR" in os.environ:
            destDir = os.environ['DESTDIR']
            buildRoot = "--buildroot %s" % destDir
        if not self.built:
            sourceDir = self.BuildRoot + 'SOURCES/'
            specDir = self.BuildRoot + 'SPECS/'
            tarball = destDir + sourceDir + '%s-%s-%s.tgz' % (self.PackageName,self.Version,self.Revision)
            specDest = destDir + specDir + '%s.spec' % self.PackageName
            dirName = os.path.split(self.dir)[1]
            
            if not os.path.exists(destDir + sourceDir):
                os.makedirs(destDir + sourceDir)
            if not os.path.exists(destDir + specDir):
                os.makedirs(destDir + specDir)
            if os.path.exists(tarball):
                os.remove(tarball)

            if self.run_cmd('tar -C .. -czf %s %s' % (tarball,dirName))[0]:
                raise "Unable to create rpm tarball"
            
            if self.run_cmd('cat %s | sed "s/\\$WCREV\\\\$/%s/" | sed "s/\\$VERSION\\\\$/%s/" > %s' % (self.SpecTemplate,self.Revision,self.Version,specDest) )[0]:
                raise "Unable to process spec file template."

            if self.run_cmd('sed -i "s/BuildRoot:.*$/BuildRoot: %s/" %s' % (destDir.replace('/', '\/'),specDest) )[0]:
                raise "Unable to process spec file template."

            pythonVersion = 'python%s' % sys.version[:3]
            if self.run_cmd('sed -i "s/python2.5/%s/" %s' % (pythonVersion,specDest))[0]:
                raise "Unable to process spec file template."
            
            ret,output = self.run_cmd('rpmbuild -bb %s %s' % (buildRoot, specDest))
            for line in output.splitlines():
                res = re.match('Wrote: (.+)$',line)
                if res:
                    self.BuiltRPMS.append(res.group(1))
                    print line

        if self.has_arg('install') and not self.InstallDone:
            for rpm in self.BuiltRPMS:
                if not '/SRPMS/' in rpm and not '-debuginfo-' in rpm:
                    cmd = 'rpm -Uv ' + rpm
                    (ret,output) = self.run_cmd(cmd,noThrow=True)
                    if ret:
                        if 'is already installed' in output:
                            print ("Skipping rpm installation, %s already installed" % rpm)
                        else:
                            self.cmd_error( cmd, output )
            self.InstallDone = True

class UploadTarget(Target):
    DefaultHost = 'hartigan'
    DefaultDest = ''
    def __init__(self,name,dir,files,host,dest):
        Target.__init__(self,name,dir)
        if files is None:
            self.files = []
        else:
            try:
                iter(files)
                self.files = files
            except:
                self.files = [files]
        self.host = host
        self.dest = dest
    
    def build_run(self):
        for file in self.files:
            self.run_cmd(['scp',file,"%s:%s" % (self.host,self.dest)])

class ClassGenTarget(Target):
    def __init__(self,name,dir,schema, pre_deps = [], post_deps = []):
        Target.__init__(self,name,dir,pre_deps,post_deps)
        self.Schema = schema
        
    def get_command(self):
        # Here is the static command for running classmaker to generate the classes
        classmakercmd = 'classmaker'
        if sys.platform == 'win32':
            classmakercmd = 'classmaker.exe'

        # Run using cmd in path, unless we are inside the tree
        if os.path.isfile(os.path.join(self.dir,'../../apps/classmaker/',classmakercmd)):
            if sys.platform != 'win32':
                classmakercmd = './' + classmakercmd
            classmakercmd = 'cd ../../apps/classmaker && ' + classmakercmd
        return classmakercmd
        
    def build_run(self):
        self.run_cmd( self.get_command() + " -s " + self.Schema + " -o " + self.dir, shell=True )

argv = sys.argv

def build():
    global All_Targets
    global Targets
    global Args
    global Config_Replacement_File
    # Targets
    Args = argv[1:]
    build_resume_path = os.path.join(os.path.abspath(os.getcwd()),'build_resume.cache')
    if Args.count('-resume'):
        try:
            pf = open( build_resume_path, 'rb' )
            All_Targets = cPickle.load(pf)
            Targets = cPickle.load(pf)
            Args = cPickle.load(pf)
        except:
            traceback.print_exc()
            print "Unable to load build resume cache."
            return
        
    if Args.count('-config-replacement-file'):
        try:
            idx = args.index('-config-replacement-file')
            Config_Replacement_File = Args[idx+1]
            del Args[idx+1]
            del Args[idx]
        except:
            print "Unable to load config replacement file"
    
    # Gather the targets
    for a in argv[1:]:
        targetName = a
        args = []
        
        # Parse args if there are any
        try:
            col_idx = targetName.index(':')
            targetName = a[:col_idx]
            args = a[col_idx+1:].split(',')
        except: pass
        
        # Find the targets, continue to next arg if none found
        try:
            targets = find_targets(targetName)
        except: continue
        
        # Remove the arg from the global arg list if it applies to specified targets
        if len(targets) > 1 and a in Args:
            Args.remove(a)
        
        for target in targets:
            target.apply_arg(args)
            Targets.append(target)
            # Has an argument
            try:
                Args.remove(target.name)
            except: pass

    if argv.count('all'):
        Targets = All_Targets
        Args.remove('all')

    if sys.platform == 'win32':
        if 'QMAKESPEC' in os.environ and 'win32-msvc' in os.environ['QMAKESPEC']:
            if 'FrameworkDir' in os.environ and os.environ['FrameworkDir'].endswith('Framework64'):
                Args.append('X86_64')
    
    if '-color' in Args:
        global term
        term = TerminalController()
    
    # These Options are passed to the module build scripts
    print term.render('${YELLOW}Starting Build${NORMAL}')
    print "Targets: ", Targets
    print "Args: ", Args

    for t in Targets:
        #print "Entering Target.build(args) on target ", t.name
        #print "Target pre_deps are: ", t.pre_deps
        #print "Target post_deps are: ", t.post_deps
        #print "\n [build process starts here] \n"
        try:
            t.build()
        except Exception,e:
            if not e.__class__ == exceptions.KeyboardInterrupt:
                traceback.print_exc()
            print "Writing build resume file to",build_resume_path
            pf = open(build_resume_path,'wb')
            cPickle.dump(All_Targets,pf,-1)
            cPickle.dump(Targets,pf,-1)
            cPickle.dump(Args,pf,-1)
            pf.close()
            print "Exiting"
            sys.exit(1)

    if '--write-installers-file' in Args:
        f = open('new_installers.pickle','wb')
        cPickle.dump(Generated_Installers,f)
        f.close()

