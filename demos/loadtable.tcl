# loadtable.tcl
#
# Ensures that the table library extension is loaded

set ::VERSION 2.8
if {[string compare unix $tcl_platform(platform)]} {
    set table(library) Tktable$::VERSION[info sharedlibextension]
} else {
    set table(library) libTktable$::VERSION[info sharedlibextension]
}
if {
    [string match {} [info commands table]]
    && [catch {package require Tktable $::VERSION} err]
    && [catch {load [file join [pwd] $table(library)]} err]
    && [catch {load [file join [pwd] .. unix $table(library)]} err]
    && [catch {load [file join [pwd] .. win $table(library)]} err]
} {
    error $err
} else {
    puts "Tktable v[package provide Tktable] loaded"
}
