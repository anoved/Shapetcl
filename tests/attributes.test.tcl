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
# [attributes] command
#

test attr-1.0 {
# invoke [attr] command with no arguments
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp attr
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result "wrong # args: *"

test attr-1.1 {
# invoke [attr] command with invalid argument
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp attr foo
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result "bad action *"

#
# [attr read] action
#

test attr-2.0 {
# invoke [attr read] with too many arguments
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp attr read foo bar soom
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result "wrong # args: *"

test attr-2.1 {
# invoke [attr read record field] with non-numeric field index
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp attr read 0 foo
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result "expected integer but got *"

test attr-2.2 {
# invoke [attr read record field] with non-numeric record index
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp attr read foo 0
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result "expected integer but got *"

test attr-2.3 {
# invoke [attr read record field] with invalid record index (too high)
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp attr read 300 0
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result "invalid record index *"

test attr-2.4 {
# invoke [attr read record field] with invalid field index (too high)
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp attr read 0 40
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result "invalid field index *"

test attr-2.5 {
# read a NULL value field
} -setup {
	set shp [shapefile tmp/foo point {integer id 10 0}]
	# create a new feature with null attribute record
	$shp coord write {{1 1}}
} -body {
	$shp attr read 0 0
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {}

test attr-2.6 {
# read an integer
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp attr read 0 [$shp fields index "pop_max"]
} -cleanup {
	$shp close
} -result {832}

test attr-2.7 {
# read a double
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp attr read 0 [$shp fields index "latitude"]
} -cleanup {
	$shp close
} -result {41.9000122264}

test attr-2.8 {
# read a string
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp attr read 0 [$shp fields index "ls_name"]
} -cleanup {
	$shp close
} -result {Vatican City}

# note that many numeric values will look the same when read as strings.
# difference include: all decimal places returned, even if zero, and instances
# of exponential notation (or other notation transparently supported by atof()?)
# In 2.9 and 2.10, the integer appears the same but the double adds a 0 digit.

test attr-2.9 {
# re-read that integer as a string
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp config readRawStrings 1
	$shp attr read 0 [$shp fields index "pop_max"]
} -cleanup {
	$shp close
} -result {832}

test attr-2.10 {
# re-read that double as a string
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	$shp config readRawStrings 1
	$shp attr read 0 [$shp fields index "latitude"]
} -cleanup {
	$shp close
} -result {41.90001222640}

test attr-2.11 {
# read a large integer (max signed 32 bit integer value)
} -setup {
	set shp [shapefile sample/misc/attr readonly]
} -body {
	$shp attr read 0 [$shp fields index "IntVal"]
} -cleanup {
	$shp close
} -result {2147483647}

test attr-2.11 {
# read a double (with no significant decimal digits)
} -setup {
	set shp [shapefile sample/misc/attr readonly]
} -body {
	$shp attr read 0 [$shp fields index "DoubleVal"]
} -cleanup {
	$shp close
} -result {100.0}

test attr-2.12 {
# read a string containing Unicode characters (app/encoding-dependent!)
} -setup {
	set shp [shapefile sample/misc/attr readonly]
} -body {
	$shp attr read 0 [$shp fields index "StringVal"]
} -cleanup {
	$shp close
} -result {Unicode test: €и✈❄◐}

test attr-2.12 {
# read a double with no sig decimal digits as a string (and see trailing zeroes)
} -setup {
	set shp [shapefile sample/misc/attr readonly]
} -body {
	$shp config readRawStrings 1
	$shp attr read 0 [$shp fields index "DoubleVal"]
} -cleanup {
	$shp close
} -result {100.000000000}

test attr-2.13 {
# read an attribute record
} -setup {
	set shp [shapefile sample/misc/attr readonly]
} -body {
	$shp attr read 0
} -cleanup {
	$shp close
} -result {2147483647 100.0 {Unicode test: €и✈❄◐}}

test attr-2.14 {
# read a longer attribute record
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	llength [$shp attr read 0]
} -cleanup {
	$shp close
} -result {36}

test attr-2.15 {
# read a negative integer (-1 is the signed interpretation of 2^32-1)
} -setup {
	set shp [shapefile sample/misc/attr readonly]
} -body {
	# IntVal field index is 0
	$shp attr read 1 0
} -cleanup {
	$shp close
} -result {-1}

test attr-2.16 {
# read an empty string field (resembles a null value)
} -setup {
	set shp [shapefile sample/misc/attr readonly]
} -body {
	# StringVal field index is 2
	$shp attr read 1 2
} -cleanup {
	$shp close
} -result {}

test attr-2.17 {
# read all attribute records of a small test file
} -setup {
	set shp [shapefile sample/misc/attr readonly]
} -body {
	$shp attr read
} -cleanup {
	$shp close
} -result {{2147483647 100.0 {Unicode test: €и✈❄◐}} {-1 0.0 {}}}

test attr-2.18 {
# read all attribute records of a shapefile with no records
} -setup {
	set shp [shapefile tmp/foo point {integer id 10 0 double value 19 9}]
} -body {
	$shp attr read
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {}

test attr-2.17 {
# read all attribute records (longer & more records - but hardly huge)
} -setup {
	set shp [shapefile sample/xy/point readonly]
} -body {
	llength [$shp attr read]
} -cleanup {
	$shp close
} -result {243}

test attr-2.18 {
# attempt to read a record that doesn't exist
} -setup {
	set shp [shapefile sample/misc/attr readonly]
} -body {
	$shp attr read 10
} -cleanup {
	$shp close
} -returnCodes {
	error
} -match glob -result "invalid record index *"

#
# [attr write] action
#

test attr-3.0 {
# attempt to write attributes to readonly shapefile
} -setup {
	set shp [shapefile sample/misc/attr readonly]
} -body {
	$shp attr write {{0 0 {}}}
} -cleanup {
	$shp close
} -returnCodes {
	error
} -result "cannot write attributes to readonly shapefile"

test attr-3.1 {
# invoke [attr write] with too many arguments
} -setup {
	set shp [shapefile tmp/foo point {integer id 10 0 double value 19 9 string label 32 0}]
} -body {
	$shp attr write 0 0 {} foo
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -match glob -result "wrong # args: *"

test attr-3.2 {
# invoke [attr write] with too few arguments
} -setup {
	set shp [shapefile tmp/foo point {integer id 10 0 double value 19 9 string label 32 0}]
} -body {
	$shp attr write
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -match glob -result "wrong # args: *"

test attr-3.3 {
# write an explicit null attribute record (with implied null shape)
} -setup {
	set shp [shapefile tmp/foo point {integer id 10 0 double value 19 9 string label 32 0}]
} -body {
	$shp attr write {}
	$shp attr read 0
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {{} {} {}}

test attr-3.4 {
# attempt to write a string value that is too long
} -setup {
	set shp [shapefile tmp/foo point {string text 5 0}]
} -body {
	$shp attr write {abcdef}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -match glob -result "string value * would be truncated *"

test attr-3.4 {
# attempt to write a unicode string value that is too long
} -setup {
	set shp [shapefile tmp/foo point {string text 5 0}]
} -body {
	# ≥ is a 3-byte character
	# → is a 3-byte character
	$shp attr write {≥→}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -match glob -result "string value * would be truncated *"

test attr-3.5 {
# write a string of multi-byte characters exactly the field width
} -setup {
	set shp [shapefile tmp/foo point {string text 5 0}]
} -body {
	# → is a 3-byte character
	# Ю is a 2-byte character
	$shp attr write {→Ю}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {0}

# interpretation of multi-byte characters is application-dependent; for best
# results, create a .cpg file to accompany the shapefile containing the name
# of the encoding (eg, "utf-8"). Scripts using Shapetcl may choose to sanitize
# string output for compatibility with a particular encoding (such as ISO8859-1,
# the default indicated in DBF files created by Shapelib), but data loss may
# occur. The alternative is that multi-byte characters may be displayed mangled,
# instead of simply substituted with question marks as when sanitized as here:

test attr-3.6 {
# "sanitize" multi-byte characters for compatibility w/a particular encoding
} -setup {
	set shp [shapefile tmp/foo point {string text 7 0}]
} -body {
	# the ñ character can be represented in iso8859-1, but the others cannot.
	$shp attr write [encoding convertto iso8859-1 {ñ→Ю}]
	# so, the ñ appears when read, but ? has been substituted for other chars.
	$shp attr read 0
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {ñ??}

test attr-3.7 {
# write a record with a bunch of fields
} -setup {
	for {set i 0} {$i < 500} {incr i} {
		lappend fields integer [format f%d $i] 10 0
		lappend values $i
	}
	set shp [shapefile tmp/foo point $fields]
} -body {
	$shp attr write $values
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {0}

test attr-3.8 {
# write a record already
} -setup {
	set shp [shapefile tmp/foo point {integer id 10 0 double value 19 9 string label 32 0}]
} -body {
	$shp attr write {0 34.2 "Hello, world."}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {0}

test attr-3.9 {
# check that creating a new entity with [attr write] creates a null shape
} -setup {
	set shp [shapefile tmp/foo polygon {integer id 10 0 double value 19 9 string label 32 0}]
	set index [$shp attr write {0 34.2 "Hello, world."}]
} -body {
	$shp coord read $index
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {}

test attr-3.10 {
# attempt to overwrite an as-yet nonexistant record
} -setup {
	set shp [shapefile tmp/foo polygon {integer id 10 0 double value 19 9 string label 32 0}]
} -body {
	$shp attr write 0 {0 34.2 "Hello, world."}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -match glob -result "invalid record index *"

test attr-3.10 {
# overwrite an existing record
} -setup {
	set shp [shapefile tmp/foo polygon {integer id 10 0 double value 19 9 string label 32 0}]
	set index [$shp attr write {0 34.2 "Hello, world."}]
} -body {
	$shp attr write $index {0 543.2109 "Goodnight, moon."}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {0}

test attr-3.11 {
# attempt to write a new record by "overwriting record -1"
} -setup {
	set shp [shapefile tmp/foo polygon {integer id 10 0 double value 19 9 string label 32 0}]
} -body {
	$shp attr write -1 {0 34.2 "Hello, world."}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -match glob -result "invalid record index *"

test attr-3.12 {
# attempt to overwrite a field that doesnt exist
} -setup {
	set shp [shapefile tmp/foo polygon {integer id 10 0 double value 19 9 string label 32 0}]
	$shp attr write {0 34.2 "Hello, world."}
} -body {
	$shp attr write 0 10 "foo"
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -match glob -result "invalid field index *"

test attr-3.13 {
# attempt to overwrite a valid field of a record that doesn't exist
} -setup {
	set shp [shapefile tmp/foo polygon {integer id 10 0 double value 19 9 string label 32 0}]
	$shp attr write {0 34.2 "Hello, world."}
} -body {
	$shp attr write 10 2 "foo"
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -match glob -result "invalid record index *"

test attr-3.14 {
# overwrite a field value
} -setup {
	set shp [shapefile tmp/foo polygon {integer id 10 0 double value 19 9 string label 32 0}]
	set index [$shp attr write {0 34.2 "Hello, world."}]
} -body {
	$shp attr write $index 1 543.2109
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {0}

test attr-3.15 {
# overwrite a field with a null value
} -setup {
	set shp [shapefile tmp/foo polygon {integer id 10 0 double value 19 9 string label 32 0}]
	set index [$shp attr write {0 34.2 "Hello, world."}]
} -body {
	$shp attr write $index 1 {}
	$shp attr read $index
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {0 {} {Hello, world.}}

test attr-3.16 {
# overwrite a record with a null record
} -setup {
	set shp [shapefile tmp/foo polygon {integer id 10 0 double value 19 9 string label 32 0}]
	set index [$shp attr write {0 34.2 "Hello, world."}]
} -body {
	$shp attr write $index {}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -result {0}

test attr-3.17 {
# attempt to write a valid integer that is too big for alloted field width
} -setup {
	set shp [shapefile tmp/foo point {integer id 4 0}]
} -body {
	$shp attr write {12345}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -match glob -result "integer value * would be truncated *"

test attr-3.18 {
# attempt to write # that's too big for Tcl to parse as a 32-bit int
} -setup {
	set shp [shapefile tmp/foo point {integer id 10 0}]
} -body {
	# ten digits, but too big for int
	$shp attr write {4294967296}
} -cleanup {
	$shp close
	file delete {*}[glob tmp/foo.*]
} -returnCodes {
	error
} -match glob -result "integer value too large to represent*"
	
# additional related tests in exponent.test.tcl

::tcltest::cleanupTests
