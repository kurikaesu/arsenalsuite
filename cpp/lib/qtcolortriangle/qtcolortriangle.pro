win32 {
    !system(util\licensecheck.bat):error( "You are not licensed to use this software." )
} else {
    !system(util/licensecheck.sh):error( "You are not licensed to use this software." )
}

TEMPLATE = subdirs
SUBDIRS = examples

