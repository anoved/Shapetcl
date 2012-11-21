package require Tcl 8.5
package require tcltest 2

# Use the tests directory as the working directory for all tests.
::tcltest::workingDirectory [file dirname [info script]]

# Stow any temporary test files in the tmp subdirectory.
::tcltest::configure -testdir [::tcltest::workingDirectory] -tmpdir tmp -asidefromdir tmp

# Apply any additional configuration arguments.
eval ::tcltest::configure $argv

# Perform the tests.
::tcltest::runAllTests
