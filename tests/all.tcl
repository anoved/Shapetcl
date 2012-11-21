package require Tcl 8.5
package require tcltest 2

# Use the tests directory as the working directory for all tests.
::tcltest::workingDirectory [file dirname [info script]]

# Stow any temporary test files in the tmp subdirectory.
# runAllTests should only look here, in the tests directory, for tests.
# It should not bother looking for tests in the tmp subdirectory.
# Files that end in .test.tcl (excepting any lock files) contain tests.
::tcltest::configure -tmpdir tmp -testdir [::tcltest::workingDirectory] -asidefromdir tmp -file {*.test.tcl} -notfile {l.*.test.tcl}

# Apply any additional configuration arguments.
eval ::tcltest::configure $argv

# Perform the tests.
::tcltest::runAllTests
