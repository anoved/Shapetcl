package require Tcl 8.5
package require tcltest 2
namespace import ::tcltest::test ::tcltest::makeFile ::tcltest::removeFile

# Use the tests directory as the working directory for all tests.
::tcltest::workingDirectory [file dirname [info script]]

# Stow any temporary test files in the tmp subdirectory.
::tcltest::configure -tmpdir tmp

# Apply any additional configuration arguments.
eval ::tcltest::configure $argv

# Load the extension to test.
lappend auto_path ..
package require Shapetcl

#
# [fields] command
#

test fields-1.0 {
# invoke [fields] cmd with too few arguments
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp fields
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result "wrong # args: *"

test fields-1.1 {
# invoke [fields] cmd with invalid action
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp fields foo
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result "bad action *"

#
# [fields add] action
#

test fields-2.0 {
# invoke [fields add] with too many arguments
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp fields add foo bar
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result "wrong # args: *"

test fields-2.1 {
# invoke [fields add] on a readonly shapefile
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp fields add {}
} -cleanup {
	$shp close
} -returnCodes {
	error
} -result "cannot add field to readonly shapefile"

# reminder that [fields add] instructs cmd_fields_add to validate=1
# so a number of these tests are exercising cmd_fields_validate and
# cmd_fields_validateField as well as cmd_fields_add itself.

test fields-2.2 {
# attempt to [fields add] with a malformed field definition
} -setup {
	file copy {*}[glob sample/xy/point.*] tmp
	set shp [shapefile tmp/point]
} -body {
	$shp fields add {foo bar}
} -cleanup {
	$shp close
	file delete {*}[glob -nocomplain tmp/point.*]
} -returnCodes {
	error
} -match glob -result "malformed field definition: *"

test fields-2.3 {
# attempt to [fields add] an undefined field
} -setup {
	file copy {*}[glob sample/xy/point.*] tmp
	set shp [shapefile tmp/point]
} -body {
	$shp fields add {}
} -cleanup {
	$shp close
	file delete {*}[glob -nocomplain tmp/point.*]
} -returnCodes {
	error
} -match glob -result "missing field definition: *"

test fields-2.4 {
# attempt to [fields add] an invalid/unsupported field type
} -setup {
	file copy {*}[glob sample/xy/point.*] tmp
	set shp [shapefile tmp/point]
} -body {
	$shp fields add {foo Title 12 0}
} -cleanup {
	$shp close
	file delete {*}[glob -nocomplain tmp/point.*]
} -returnCodes {
	error
} -match glob -result "invalid field type *"

test fields-2.5 {
# attempt to [fields add] with an invalid field title (too long)
} -setup {
	file copy {*}[glob sample/xy/point.*] tmp
	set shp [shapefile tmp/point]
} -body {
	$shp fields add {integer ThisTitleIsTooLong 10 0}
} -cleanup {
	$shp close
	file delete {*}[glob -nocomplain tmp/point.*]
} -returnCodes {
	error
} -match glob -result "invalid field name *"

test fields-2.5.1 {
# attempt to [fields add] with an invalid field title (too short)
} -setup {
	file copy {*}[glob sample/xy/point.*] tmp
	set shp [shapefile tmp/point]
} -body {
	$shp fields add {integer {} 10 0}
} -cleanup {
	$shp close
	file delete {*}[glob -nocomplain tmp/point.*]
} -returnCodes {
	error
} -match glob -result "invalid field name: *"

test fields-2.5.2 {
# attempt to [fields add] with an invalid field title (contains space)
} -setup {
	file copy {*}[glob sample/xy/point.*] tmp
	set shp [shapefile tmp/point]
} -body {
	$shp fields add {integer {foo bar} 10 0}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/point.*]
} -returnCodes {
	error
} -match glob -result "invalid field name: *"

test fields-2.5.3 {
# attempt to [fields add] with an valid field title containing numbers and _
} -setup {
	set shp [shapefile tmp/foo point {integer id 10 0}]
} -body {
	$shp fields add {integer Field_33 10 0}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {1}

test fields-2.5.4 {
# attempt to [fields add] an invalid field title (doesn't start w/alphabetic)
} -setup {
	file copy {*}[glob sample/xy/point.*] tmp
	set shp [shapefile tmp/point]
} -body {
	$shp fields add {integer 1foo 10 0}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/point.*]
} -returnCodes {
	error
} -match glob -result "invalid field name: *"

test fields-2.5.5 {
# attempt to create a shapefile with duplicate field names
} -body {
	shapefile tmp/foo point {integer info 10 0 string info 32 0}
} -returnCodes {
	error
} -match glob -result "invalid field name: duplicate names disallowed *"

test fields-2.5.6 {
# attempt to add a field with a name that is a duplicate of an existing field
} -setup {
	set shp [shapefile tmp/foo point {integer info 10 0}]
} -body {
	$shp fields add {string info 32 0}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -match glob -result "invalid field name: duplicate names disallowed *"

test fields-2.5.7 {
# confirm that duplicate checking is case insensitive (DUPE == dupe)
} -body {
	shapefile tmp/foo point {integer ID 10 0 integer id 10 0}
} -returnCodes {
	error
} -match glob -result "invalid field name: duplicate names disallowed *"

# "invalid" numeric field definitions are those whose width/precision
# parameters imply values that would be returned as another type (int/double)

# for example, integers with >=11 digits won't fit in 32-bit int type.
# shapelib would actually let us write them, but they won't necessarily be
# readable as such (may be readable as doubles, or not). We warn instead.
test fields-2.6 {
# attempt to [fields add] with an invalid (too many digits) integer definition
} -setup {
	file copy {*}[glob sample/xy/point.*] tmp
	set shp [shapefile tmp/point]
} -body {
	$shp fields add {integer Title 20 0}
} -cleanup {
	$shp close
	file delete {*}[glob -nocomplain tmp/point.*]
} -returnCodes {
	error
} -match glob -result "invalid integer field definition: *"

# a floating-point double type that has 0 precision (no digits right of decimal)
# and width <= 10 is essentially an integer. If the field is exactly 10 wide,
# it could store values that cannot be stored in a 32-bit integer. So maybe <10?
test fields-2.7 {
# attempt to [fields add] an invalid (integer mimic) double definition
} -setup {
	file copy {*}[glob sample/xy/point.*] tmp
	set shp [shapefile tmp/point]
} -body {
	$shp fields add {double Title 10 0}
} -cleanup {
	$shp close
	file delete {*}[glob -nocomplain tmp/point.*]
} -returnCodes {
	error
} -match glob -result "invalid double field definition: *"

test fields-2.8 {
# use [fields add] to successively add 3 fields (integer, double, & string)
} -setup {
	file copy {*}[glob sample/xy/point.*] tmp
	set shp [shapefile tmp/point]
} -body {
	set initialCount [$shp fields count]
	$shp fields add {integer TestInt 10 0}
	$shp fields add {double TestDub 19 6}
	$shp fields add {string TestStr 32 0}
	expr {[$shp fields count] - $initialCount}
} -cleanup {
	$shp close
	file delete {*}[glob -nocomplain tmp/point.*]
} -result {3}

test fields-2.8b {
# use [fields add] to add 3 fields at once
} -setup {
	file copy {*}[glob sample/xy/point.*] tmp
	set shp [shapefile tmp/point]
} -body {
	set initialCount [$shp fields count]
	$shp fields add {integer TestInt 10 0 double TestDub 19 6 string TestStr 32 0}
	expr {[$shp fields count] - $initialCount}
} -cleanup {
	$shp close
	file delete {*}[glob -nocomplain tmp/point.*]
} -result {3}

test fields-2.9 {
# check that value of new [fields add] field is null for an existing record
} -setup {
	file copy {*}[glob sample/xy/point.*] tmp
	set shp [shapefile tmp/point]
} -body {
	set fieldIndex [$shp fields add {integer TestInt 10 0}]
	# read value of the new field (fieldIndex) from an existing record (0)
	$shp attributes read 0 $fieldIndex
} -cleanup {
	$shp close
	file delete {*}[glob -nocomplain tmp/point.*]
} -result {}

#
# [fields count] action
#

test fields-3.0 {
# invoke [fields count] with too many arguments
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp fields count foo
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result "wrong # args: *"

#
# [fields list] action
#

test fields-4.0 {
# invoke [fields list] action with too many arguments
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp fields list foo bar
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result "wrong # args: *"

test fields-4.1 {
# invoke [fields list index] with non-integer index argument
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp fields list foo
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result "expected integer but got *"

test fields-4.2 {
# invoke [fields list index] with invalid (too high) index argument
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp fields list 50
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result "invalid field index *"

# getting field definitions with [fields list] exercises cmd_fields_description

test fields-4.3 {
# confirm field definition for a specific field using [fields list index]
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp fields list 4
} -cleanup {
	$shp close
} -result {string name 100 0}

test fields-4.4 {
# check that [fields list] returns expected number of field definitions (36)
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	set definitions [$shp fields list]
	# four parameters per field definition (type, name, width, precision)
	expr {[llength $definitions] / 4}
} -cleanup {
	$shp close
} -result {36}

test fields-4.5 {
# check that [fields list index] outputs input given to [fields add]
} -setup {
	file copy {*}[glob sample/xy/point.*] tmp
	set shp [shapefile tmp/point]
} -body {
	set index [$shp fields add {integer StrTheory 10 0}]
	$shp fields list $index
} -cleanup {
	$shp close
	file delete {*}[glob -nocomplain tmp/point.*]
} -result {integer StrTheory 10 0}

#
# [fields index] action
#

test fields-5.0 {
# invoke [fields index] with too few arguments
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp fields index
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result "wrong # args: *"

test fields-5.1 {
# invoke [fields index] with too many arguments
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp fields index foo bar
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result "wrong # args: *"

test fields-5.2 {
# invoke [fields index name] with field name that cannot be found
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp fields index foo
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result "field named * not found"

test fields-5.3 {
# confirm that [fields index name] gets expected field index
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp fields index "latitude"
} -cleanup {
	$shp close
} -result {21}

test fields-5.4 {
# invoke [fields index name] with empty name {}
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp fields index {}
} -cleanup {
	$shp close
} -returnCodes {
	error
} -result "missing field name"

test fields-5.5 {
# confirm that [fields index] returns the first field if multiple match
} -setup {
	set shp [shapefile sample/misc/fieldnames readonly]
} -body {
	# Both fields 0 and 1 are named {# Field}.
	# We disallow creation of duplicate field names, but support reading them.
	# We also disallow creation of field names w/non-alphanumeric_ characters,
	# but support reading them. The strict output rules are a concession to
	# compatibility with other shapefile software that may be less flexible. 
	$shp fields index {# Field}
} -cleanup {
	$shp close
} -result {0}

::tcltest::cleanupTests
